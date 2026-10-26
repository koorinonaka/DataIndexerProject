---
status: ready
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
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
