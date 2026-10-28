---
status: done
order: 30
tier: standard
screen: none
agent: any
created_by: task-create
---

# Runtime module の bug 修正とホットパスの確保ゼロ化

## Goal
`DataIndexer` runtime module で、typo 1 つで CDO 構築中に落ちる経路・public ヘッダの依存漏れ・無バージョンの生シリアライズを直し、
`FindRow` 系ホットパスのヒープ確保と 2 重ルックアップを消す。

## Context
監査 `.agents/audit-2026-10-26.md` の B-R1, B-R3, B-R4, B-R5, P-R1, P-R2, P-R3, P-R5, P-R6, P-R7。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexer/` 相対、行番号は 2026-10-26 時点。

- B-R1: `Private/DataIndexerSchema.cpp:260` `BuildIndexFunctions.Emplace(Index).SetFromField<UFunction>(GetClass()->FindFunctionByName(FunctionName), false)`。
  null を渡すとエンジン `MemberReference.h:113` が `InField->GetOwnerClass()` で即 deref。`DI_REGISTER_BUILD_INDEX` に
  UFUNCTION でない名前を渡すとエディタ起動時に落ちる。
- B-R3: `Private/DataIndexerFunctionLibrary.cpp:51-60` `GetRowHandleName` と `Private/DataIndexerTypes.cpp:244-252`
  `FDataIndexerRowHandle::GetRowDisplayName` は、`Repository::GetDisplayName` (`DataIndexerRepository.cpp:248`) が常に GUID 文字列へ
  フォールバックするので Repository があれば絶対に false/NullOpt を返さない。`Public/DataIndexerSchema.h:49` の「NullOpt when no binding」契約と矛盾。
  `:261` のフォールバック分岐は Repository null 時しか通らず、そのとき `FSoftObjectPath(nullptr).GetAssetName()` は空。
- B-R4: `Public/DataIndexerEditorInterface.h:8` が `Widgets/SWidget.h` を無条件 include。`DataIndexer.Build.cs:17-18` では Slate が
  Private 依存。利用側 (`Source/DataIndexerProject/DataIndexerProject.Build.cs:14`) が SlateCore を自前で足している。
  `TSharedRef<SWidget>` 戻り値は前方宣言で足りる (`DataIndexerSchema.h:14` に既にある)。
- B-R5: `Private/DataIndexerRepository.cpp:84-87` `ReverseLookups` を `TMapPrivateFriend::Serialize` で生シリアライズ、custom version なし。
  advisor 判断: 1.0.x で on-disk レイアウトは変えない。`FCustomVersion` を登録して `Ar.UsingCustomVersion` を呼ぶだけ (分岐点を作る)。
  `TMapPrivateFriend` 直叩きは `Ar << ReverseLookups` で足りる (レイアウト同一)。
- P-R1: `Private/DataIndexerRepository.cpp:253` `FindRowEntity` が毎回 `TSet<const UDataIndexerRepository*>` をヒープ確保。
  `FindRow` / `RowHandle.IsValid()` / `GetRowHandleValue` 全部が通る。
- P-R2: `Public/DataIndexerSchemaInterface.h:27-29` `FindRow(RowHandle)` が `IsValid()` → `FindRow()` で `FindRowEntity` 2 回。
- P-R3: `Private/DataIndexerSchema.cpp:244` `CallBuildIndex` と `Private/DataIndexerSchema_RowFunction.cpp:54` `GetRowDisplayName` が毎回
  `FMemberReference::ResolveMember<UFunction>` → alloca → ProcessEvent。
- P-R5: `Private/DataIndexerDrivenCollection.cpp:14-19` インスタンスごとにコンストラクタで `OnAssetLoaded` を bind。
- P-R6: `Public/DataIndexerDrivenCollection.h:128-146` `Keys.Contains` を entry ごと、`IndexOfByKey` をソート比較ごと。
- P-R7: `Private/DataIndexerSchema_ColumnLayout.cpp:196-207, 88-93` `GetCheckedPropertyNames` が `TSet<FName>` 値返し、
  `VisitedStructs` を struct ごとにコピー。

## Scope
- B-R1: `ensureMsgf` + early return。
- B-R3: 追加のみで直す (advisor 判断: `UDataIndexerRepository::GetDisplayName` は Fab 配布の public API なので戻り型を変えない)。
  `TOptional<FText> TryGetDisplayName(const FDataIndexerPrimaryKey&) const` を新設し、既存 `GetDisplayName` はその GUID フォールバック wrapper に。
  `FDataIndexerRowHandle::GetRowDisplayName` と BP `GetRowHandleName` は `TryGetDisplayName` を使い、binding なしで NullOpt / false。`:261` の空文字パスを消す。
- B-R4: `SWidget.h` include を前方宣言に置換。ゲームモジュール Build.cs の SlateCore 追加が不要になることを確かめる (消すのは任意)。
- B-R5: `FDataIndexerCustomVersion` (enum + `FGuid` + `FCustomVersionRegistration`) を新設、`Serialize` 冒頭で `Ar.UsingCustomVersion`。
  `TMapPrivateFriend` を `Ar << ReverseLookups` に置換。レイアウトは変えない。
- P-R1: `TSet<..., DefaultKeyFuncs, TInlineSetAllocator<4>>`。
- P-R2: `return RowHandle.Repository ? FindRow(*RowHandle.Repository, RowHandle.PrimaryKey) : nullptr;` の 1 行。
- P-R3: 解決済み `UFunction` を `TMap<FDataIndexerIndexKey, TWeakObjectPtr<UFunction>>` (mutable、CDO) にキャッシュ。
  `PostLoad` / `PostCDOCompiled` で空にする。
- P-R5: コンストラクタの bind を消し `PostLoad()` で `Rebuild()`。
- P-R6: `TMap<Key,int32>` を 1 回作って比較。
- P-R7: ポインタ返し + `Visited` を add/remove で使い回す。

## Out of Scope
- `OwnerRepository` HACK (t261026-91cd)。`DataIndexerTypes.cpp:166-174` の `PostSerialize` に触らない。
- Runtime の refactor 群 R-R1〜R-R13 (t261026-a134)。重複 helper の抽出はそちら。
- `FLookupIndex` のレイアウト変更。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. B-R1 → `DataIndexerSchema.cpp` `BindBuildIndexFunction` にガード。
2. B-R4 → `DataIndexerEditorInterface.h` の include を前方宣言に。Editor + Game 両ターゲットをビルド。
3. B-R5 → `Private/DataIndexerCustomVersion.h` を新設、`UDataIndexerRepository::Serialize` に登録、`TMapPrivateFriend` を置換。
4. P-R1 / P-R2 → `FindRowEntity` の TSet と `FindRow(RowHandle)`。
5. B-R3 → `TryGetDisplayName` を新設、`GetDisplayName` を wrapper 化、`FDataIndexerRowHandle::GetRowDisplayName` / `GetRowHandleName` を切り替える。
   他の呼び手 (Views / Customizations) は `GetDisplayName` のまま (挙動不変)。
6. P-R3 → CDO キャッシュ。`DataIndexerEd/Private/DataIndexerCompiler.cpp` 経由の `CallBuildIndex` が同じ結果を返すことをテストで確認。
7. P-R5 / P-R6 / P-R7。
8. `DataIndexerEd.Schema` + `DataIndexerEd.Repository` + `DataIndexerEd.Compiler` を回す。

## Acceptance Criteria
- `DI_REGISTER_BUILD_INDEX` に存在しない関数名を渡す fixture を CDO 構築しても落ちず、ensure が 1 回出る (現在: クラッシュ)。
- `rg 'Widgets/SWidget.h' Plugins/DataIndexer/Source/DataIndexer/Public/` が 0 件 (現在 1)。
- `rg 'UsingCustomVersion' Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerRepository.cpp` が 1 件以上 (現在 0)。
  v1.0.2 で保存した `.uasset` がそのままロードできる (`Content/` の既存 Repository で確認)。
- `rg 'TMapPrivateFriend' Plugins/DataIndexer/Source/DataIndexer/` が 0 件 (現在 1)。
- `FindRowEntity` の TSet が inline allocator (`rg 'TInlineSetAllocator' DataIndexerRepository.cpp` 1 件以上、現在 0)。
- binding の無い Schema で `GetRowHandleName` が `false` を返す (現在: GUID 文字列で `true`)。`GetDisplayName` のシグネチャは不変
  (`rg 'FText GetDisplayName\(' DataIndexerRepository.h` 1 件のまま)。
- `CallBuildIndex` を同じ IndexKey で 2 回呼ぶと 2 回目は `ResolveMember` を通らない (テストで `UFunction*` 同一性確認)。

## Tests
- `DataIndexerSchemaTests.cpp` に行: 不正関数名 fixture で ensure 1 回、`BuildIndexFunctions` に entry が無い。修正前コードで落ちる (クラッシュ) ことを確かめる。
- `DataIndexerRepositoryReferenceTests.cpp` に行: `GetRowHandleName` が binding なしで `false`。
- `DataIndexerCompilerTests.cpp` に行: 2 回目の `CallBuildIndex` がキャッシュを使う。
- quick: `DataIndexerEd.Schema` + `DataIndexerEd.Repository` + `DataIndexerEd.Compiler`。Game ターゲットもビルド。

## Verification
- quick の log。既存 `.uasset` のロードは `UnrealEditor-Cmd -run=ResavePackages -OnlyUnversioned`
  ではなく、通常起動 log に `LoadErrors` が無いことで確認。GUI 不要。

## Execution Log
2026-10-28 (executor, dispatch 4/18, branch `fix/audit-2026-10` 両 repo)。前提: `release/v1.0.2` は submodule main (4fc12d0) にマージ済み。

実装 (submodule `Plugins/DataIndexer`):
- B-R1: `BindBuildIndexFunction` が `FindFunctionByName` の null を `ensureAlwaysMsgf` + early return で止める。
- B-R4: `DataIndexerEditorInterface.h` の `Widgets/SWidget.h` include を `class SWidget;` 前方宣言に置換。
- B-R5: `Private/DataIndexerCustomVersion.h` を新設 (enum + GUID)。GUID 定義と `FCustomVersionRegistration` は `DataIndexerRepository.cpp` に置く。`Serialize` は `UsingCustomVersion` の後に `BaseArchive << ReverseLookups`。エンジンの `operator<<(FArchive&, TMap&)` は `TMapPrivateFriend::Serialize` を呼ぶだけなのでレイアウトは同一 (5.8 / 5.7 の `Map.h.inl` で確認)。
- P-R1: `FindRowEntity` の visited set を `TSet<..., DefaultKeyFuncs<...>, TInlineSetAllocator<4>>` に変更 (private ネスト型 `FVisitedRepositories`。ヘッダは前方宣言、定義は `.cpp`)。
- P-R2: `TNativeSchemaInterface::FindRow(RowHandle)` を Scope の 1 行に変更。`FDataIndexerRowHandle::IsValid()` は `FindRowEntity().IsValid()` なので、2 重 lookup は実在した。
- B-R3: `UDataIndexerRepository::TryGetDisplayName` を新設 (schema binding のみ。無ければ NullOpt)。`GetDisplayName` は editor 分岐をそのまま残し、runtime 末尾を `TryGetDisplayName` + GUID フォールバックにした。`FDataIndexerRowHandle::GetRowDisplayName` は `TryGetDisplayName` を使う。`ToString` は Repository null のとき `Describe(PK)` だけを返す (空の asset 名 `": GUID"` を消した)。`GetRowHandleName` は RowHandle 経由なので本体は変更なし。Views / Customizations は `GetDisplayName` のまま。
- P-R3: `CallBuildIndex` が解決済み `UFunction` を `mutable TMap<FDataIndexerIndexKey, TWeakObjectPtr<UFunction>>` + `FRWLock` にキャッシュする。`ResolveRowFunction` / `CallResolvedRowFunction` を `DataIndexerSchemaRowFunction.h` に出し、`CallRowFunction` はその 2 つの合成にした。リセット箇所: `PostLoad` / `PostCDOCompiled` / `PostEditChangeProperty(BuildIndexFunctions)` / `BindBuildIndexFunction`。
- P-R5: コンストラクタの `OnAssetLoaded` bind と `HandleAssetLoaded` を削除。`PostLoad` で `SourceRepository->ConditionalPostLoad()` → `Rebuild()`。サンプル `UCharacterClassIconCollection` は `PostLoad` を override していない。
- P-R6: `Rebuild` が `TMap<Key,int32> KeyOrder` を 1 回作り、削除判定とソート比較に使う。
- P-R7: `GetCheckedPropertyNames` を `const TSet<FName>*` 返しに変更。`CollectColumnNameOptionsRecursive` の `TempVisited` コピーを Add → 判定 → 再帰 → Remove に変更。`HasAnyCheckedPropertyRecursive` 自身が Add/Remove 対称なので、兄弟の探索は汚れない。呼び手 `DataIndexerEd/Private/Views/DataIndexerViewTypes.cpp:196` もポインタ受けに合わせた (シグネチャ変更の必然)。

テスト (Tests 4 問は事前に回答済み):
- `DataIndexerEd.Repository.RowHandleNameRequiresBinding`: binding なしの schema (`UDataIndexerBuildIndexSchemaTests`、RowStruct 一致・row 実在) で `GetRowHandleName` false、`GetRowDisplayName` unset、`GetDisplayName` は GUID。対照: `UDataIndexerRowNameSchemaTests` で true + 名前。
  - 修正前 (HEAD d8fd277 + テストだけ) で実行 → Fail。落ちたのは狙った 2 assert (`GetRowHandleName without a binding` / `GetRowDisplayName without a binding`) だけ。GUID と対照は通過。
- `DataIndexerEd.Schema.BindUnknownBuildIndexFunctionEnsures`: `BindBuildIndexFunction(UnknownIndex, "DoesNotExist")` で ensure banner `=== Handled ensure: ===` がちょうど 1 回。`UnknownIndex` は未登録、有効な `MatchingIndex` は登録される。
  - AC の「`DI_REGISTER_BUILD_INDEX` に存在しない関数名」は `GET_FUNCTION_NAME_CHECKED` がコンパイル時に弾くので書けない。生 helper 経由で同じ `BindBuildIndexFunction` を通す。fixture のコンストラクタではなくテストから呼ぶ (CDO 構築は起動時なので、毎起動 ensure が出るのを避ける)。
  - 修正前の実行はしていない。エンジン `MemberReference.h:113` の `InField->GetOwnerClass()` が null deref してプロセスごと落ち、assert に届かない。
  - ensure のメッセージ本文は 1 ensure につき 2 回ログに出る (log 行 + ensure record)。回数は banner で数える。
- `DataIndexerEd.Compiler.BuildIndexUsesResolvedFunctionCache`:
  1. CDO に MatchingIndex を登録した fixture `UDataIndexerIndexedSchemaTests` で Repository を compile → `ReverseLookups` の Value 7 の bucket が KeyA のみ。CDO キャッシュに `BuildIndex_Matching` が入る。`ForEachPrimaryKeys(Value 8)` が KeyB を返す。
  2. 新規 instance: キャッシュ空 → 1 回目 GUID(0,0,0,9)、キャッシュ = `FindFunctionByName("BuildIndex_Matching")` → binding を `BuildIndex_Mismatched` (解決時に戻り型で拒否される) へキャッシュを触らずに差し替え → 2 回目も GUID(0,0,0,9) (= 再解決していない) → 対照: キャッシュ reset 後は NullOpt。
- fixture 追加: `UnknownIndex` / `RegisterUnknownBuildIndex` / `HasBuildIndexFunction` / `RepointBuildIndexWithoutCacheReset` / `GetCachedBuildIndexFunction` / `using ResetBuildIndexFunctionCache`、サブクラス `UDataIndexerIndexedSchemaTests`。既存 `UDataIndexerBuildIndexSchemaTests` の CDO は ViewModel テストが使うので変えない。

ゲート:
- Editor ビルド `Build.bat DataIndexerProjectEditor Win64 Development` ×7 (修正前 1、修正後 6)。うち 2 回はテスト文字列のエスケープ (C4129) で失敗、2 回は期待エラーの調整。最終 Result: Succeeded。
- `Automation RunTests DataIndexerEd` (全群。quick 指定の Schema / Repository / Compiler と、触った DrivenCollection / SchemaLayout / View を包含) → 64 passed / 0 failed、`TEST COMPLETE. EXIT CODE: 0`。full は run 終端。
- Game ビルド `Build.bat DataIndexerProject Win64 Development`: 1 回目はエンジン込みフルビルド (1128 actions、2060 s) で、エンジンのソース `Matrix.inl(385)` / `ContainerAllocationPolicies.h(1182)` に C1001。DataIndexer モジュールはコンパイル済み。run 注記どおり再実行 → Result: Succeeded (28 s)。
- 既存 `.uasset` のロード: 通常起動では Repository を読まないので、`UnrealEditor-Cmd DataIndexerProject.uproject -run=LoadPackage "DI_*.uasset" "DA_CharacterClassIconCollection.uasset" -PROJECTONLY -unattended -nullrhi` で強制ロードした。v1.0.2 保存の 10 パッケージ (DI_Ability / DI_Ability_Mage / DI_Ability_Warrior / DI_CharacterClass / DI_Characters / DI_CharactersFromDataTable / DI_ItemRarity / DI_Items / DI_ItemType / DA_CharacterClassIconCollection) がすべて Loading され、シリアライズ系のエラー・警告は 0。commandlet の `1 error` は `GameSingletonClassName '/Script/DataIndexerProject.GameDataSettings'` で、修正前のテスト起動 log にも同じ行がある (既存、本タスク無関係)。`Content/` に差分なし。
- 1 回目の LoadPackage は Game フルビルドと並走して `TurnkeySupport` で 10 分止まったので kill し、ビルド後に再実行した。

AC 照合:
- ensure 1 回 / 落ちない → 上記テスト。
- `rg 'Widgets/SWidget.h' .../DataIndexer/Public/` → 0 件。
- `rg UsingCustomVersion DataIndexerRepository.cpp` → 1 件。既存 asset のロード OK (上記)。
- `rg TMapPrivateFriend Plugins/DataIndexer/Source/DataIndexer/` → 0 件。
- `rg TInlineSetAllocator DataIndexerRepository.cpp` → 1 件。ヘッダは private のネスト型 `struct FVisitedRepositories;` を前方宣言するだけで、定義 (inline allocator の `TSet` 派生) は `.cpp` に置いた。
- `GetRowHandleName` が binding なしで false → テスト。`rg 'FText GetDisplayName\(' DataIndexerRepository.h` → 1 件のまま。
- 2 回目の `CallBuildIndex` が `ResolveMember` を通らない → 「解決時に拒否される binding に差し替えても結果が変わらない」で振る舞いとして確認。呼び出しカウンタは本番に口を足すので置かない。

既定 (取り消し可):
- B-R1 は Scope の `ensureMsgf` ではなく `ensureAlwaysMsgf`。`ensureMsgf` は callsite ごとにプロセスで 1 回しか出ず、2 クラス目の typo やテスト再実行で黙るため。戻すなら 1 語。
- P-R3 のキャッシュに `FRWLock` を付けた。`ForEachPrimaryKeys(IndexKey, ...)` を worker thread から呼ぶ利用者がいると、TMap の同時書き込みで落ちるため。戻すなら lock 3 箇所を消す。
- P-R3 のリセットを Scope の PostLoad / PostCDOCompiled に加え、`BindBuildIndexFunction` と `PostEditChangeProperty(BuildIndexFunctions)` でも行う。再 bind 後に古い UFunction を返さないため。
- P-R3 は BuildIndex のみ。`RowDisplayNameFunction` (Context の `GetRowDisplayName` 側) は Scope の設計 (IndexKey キーの TMap) に無いので未キャッシュ。
- B-R4 の任意項目: サンプル `DataIndexerProject.Build.cs` の `SlateCore` は消さない。サンプルのスキーマ (`CharacterSchema` / `ItemSchema` / `DataIndexerMVVMHelper.h`) が自分で `SWidget` を使うので、プラグイン側の修正と無関係に必要。
- `FDataIndexerRowHandle::ToString` / `GetRowDisplayName` は、エディタでも binding が無ければ `NewRow{N}` ではなく `<Asset>: <GUID>` になる。エディタ UI でこの 2 つを表示する箇所は無い (grep 0 件)。

critic round 1 の FAIL (下記) を受けて `FVisitedRepositories` の定義を `.cpp` へ移した。その後 Editor ビルド Succeeded、Game ビルド Succeeded、`Automation RunTests DataIndexerEd` 64 passed / 0 failed (EXIT CODE 0)。Editor ビルドは合計 8 回、Game は 3 回。

critic round 1:
```
FAIL
1. DataIndexerRepository.cpp:269 — kind 1 — AC は `rg 'TInlineSetAllocator' DataIndexerRepository.cpp` が 1 件以上を求める。diff 後は 0 件。TInlineSetAllocator は DataIndexerRepository.h:92 の private alias FVisitedRepositories にしか無い。中身は満たすが、AC 指定の計測を満たさない。
NOTE: DataIndexerSchemaTests.cpp — DI_REGISTER_BUILD_INDEX に存在しない名前は書けない (コンパイル時に弾く)。NewObject から BindBuildIndexFunction を直接呼ぶ。同じ関数のガードは確認している。判断は妥当。
NOTE: DataIndexerSchema.cpp:245 — Scope は ensureMsgf、実装は ensureAlwaysMsgf。理由は Execution Log にある。挙動は Scope の意図より強い。
NOTE: DataIndexerTypes.cpp:246 — エディタでの RowHandle 表示名の変化は Scope B-R3 どおり。呼び手は GetRowHandleName とテストだけ。
NOTE: DataIndexerCustomVersion.h:20 — InitialVersion の参照は 0 件。task が要求した分岐点なので kind 5 にしない。
NOTE: 削除名の sweep: HandleAssetLoaded は BugReport.md:269 の説明文のみ。TMapPrivateFriend 0 件。GetCheckedPropertyNames の呼び手 3 箇所は追従済み。PostSerialize には触れていない。
```
answer: 1 は直した (定義を .cpp へ)。

critic round 2:
```
PASS
NOTE: DataIndexerRepository.cpp:269 — round-1 の FAIL 1 は解消。TInlineSetAllocator は .cpp に 1 件。ヘッダは FVisitedRepositories の前方宣言だけ。
NOTE: 削除名の sweep をやり直した。HandleAssetLoaded は BugReport.md:269 の説明文だけ。TMapPrivateFriend 0 件。public ヘッダの Widgets/SWidget.h 0 件 (残る 1 件は DataIndexerEd の private ヘッダで AC 対象外)。GetCheckedPropertyNames の呼び手 3 箇所は追従済み。
NOTE: DataIndexerTypes.cpp:249 — ToString は Repository->GetName() を使う。トップレベルのアセットなら同じ文字列。出力文字列のテストは無い。
NOTE: Source/DataIndexerProject/Tests/ItemInterfaceMockTests.cpp:147 — GetDisplayName の挙動は不変なので影響なし。
NOTE: GetRowHandleName の BP 呼び出しが Content/ にあるかは grep で確かめられない。binding なしで false は Scope B-R3 の要求。
NOTE: AC 1 は生の BindBuildIndexFunction 経由でテスト。マクロはコンパイル時に弾くので判断は妥当。
```

自己レビュー: submodule の `git diff --stat` は 17 files + 新規 1。すべて Scope の項目、その呼び手 (`DataIndexerViewTypes.cpp`)、テスト / fixture。親 repo は submodule pointer とこのタスクファイルのみ。

## Summary
Runtime の B-R1 / B-R3 / B-R4 / B-R5 と P-R1 / P-R2 / P-R3 / P-R5 / P-R6 / P-R7 を修正した。typo の build index 名は ensure で止まる。`GetRowHandleName` は binding なしで false。`ReverseLookups` は custom version 付き (レイアウト不変)。`FindRow` 系はヒープ確保と 2 重 lookup が無い。`CallBuildIndex` は解決済み UFunction を再利用する。新規テスト 3 本を含む `DataIndexerEd` 64/64 通過、Editor / Game ビルド成功、v1.0.2 の既存 Repository 10 件がロードできる。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerCustomVersion.h` (新規)
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerDrivenCollection.cpp`
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerRepository.cpp`
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerSchema.cpp`
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerSchemaRowFunction.h`
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerSchema_ColumnLayout.cpp`
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerSchema_RowFunction.cpp`
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerTypes.cpp`
- `Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerDrivenCollection.h`
- `Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerEditorInterface.h`
- `Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerRepository.h`
- `Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerSchema.h`
- `Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerSchemaInterface.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewTypes.cpp` (`GetCheckedPropertyNames` の呼び手)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerCompilerTests.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerRepositoryReferenceTests.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerSchemaTests.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Fixtures/DataIndexerTestTypes.h`
- `.agents/tasks/done/t261026-2e18-runtime-bugs-perf.md`

## Blockers
None
