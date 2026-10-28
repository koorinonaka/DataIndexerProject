---
status: verify_pending
order: 150
tier: standard
screen: required
agent: any
created_by: task-create
---

# Customizations の重複排除・毎フレーム同期ロードの除去・meta キー集約

## Goal
GraphPin と PropertyTypeCustomization のペアで重複する Schema フィルタ・コンボ選択肢・表示テキスト構築が 1 箇所になり、
details の毎フレーム `LoadObject` / グローバル関数検索 / ハンドル鎖走査が消え、meta キーのリテラルが `DataIndexerMetaDataKeys.h` に集まる。

## Context
監査 `.agents/audit-2026-10-26.md` の R-C3, R-C4, R-C6, R-C7, R-C8, R-C9, R-C11 (残り), P-C3, P-C4, P-C5, P-C6, P-C8, P-C9, P-C10, B-C12。
重複チェック通過 (0件対象)。パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` 相対、行番号は 2026-10-26 時点。

- R-C3: `DataIndexerGraphPin_Repository.cpp:23-56,110-125` ≡ `DataIndexerPropertyTypeCustomization_Repository.cpp:18-100,148-165` Schema meta →
  期待スキーマクラス解決と `ShouldFilterAsset` (AssetRegistry `SchemaClass` タグ + `TryFindTypeSlow`)。
  `DataIndexer::SchemaFilter::{ResolveExpectedSchemaClass(const FString& MetaPath, TConstArrayView<UObject*> Outers), ShouldFilterRepositoryAsset(const FAssetData&, const UClass*)}` に。
- P-C6: `PTC_Repository.cpp:155`, `GraphPin_Repository.cpp:120` フィルタ 1 パス・アセットごとに `TryFindTypeSlow`。`PTC_Repository.cpp:141`
  `GetNewAssetFactoriesForClasses` が `CustomizeHeader` ごとに全 factory 走査。`GraphPin_Repository.cpp:49` ピン Construct で `LoadObject<UClass>` 同期ロード。
  パス→UClass と factory リストをメモ化 (置き場は R-C3 の `SchemaFilter`)。
- R-C4: `GraphPin_IndexKey.cpp:73-137` ≡ `PTC_IndexKey.cpp:69-128`。`GraphPin_PrimaryKey.cpp:133-153` ≡ `PTC_PrimaryKeyOptions.cpp:56-81`。
  `IndexKeyDisplayText(const UDataIndexerRepository*, const FString&)`, `BuildIndexKeyOptions(const UDataIndexerRepository*)`,
  `BuildPrimaryKeyOptions(const UDataIndexerRepository&, TFunctionRef<bool(const FDataIndexerPrimaryKey&)>)` に。
- P-C5: `PTC_PrimaryKeyOptions.cpp:36` (`:68-78` のループ内) 主キー 1 件ごとに `FindObjectSafe<UFunction>`。ループ外へ。
- R-C6: `DataIndexerVariableCustomization_RepositorySchema.cpp:46-77` vs `PTC_Schema.cpp:52-81` スキーマクラス用 `IClassViewerFilter` が 2 つ、
  除外条件 (Abstract/Deprecated/NewerVersionExists/SKEL vs RowStruct) が食い違う。`RequiredRowStruct` optional の 1 クラスに。
- R-C7: `DataIndexerDetailCustomization_EditorRow.cpp:245`, `PTC_NamedStruct.cpp:11` (`"ShowOnlyInnerProperties"`×2)、`DataIndexerRowFunctionBinding.cpp:308`
  (`"AllowFunctionLibraries"`)、`PTC_Schema.cpp:24` (`AssetRegistryTagKeys::RowStructure` を meta キーとして流用)、`PTC_PrimaryKeyOptions.cpp:25`
  (`FScopedFilterContext::FunctionName` = `"FilterFunction"` が runtime の Repository ヘッダにある)。`DataIndexerMetaDataKeys.h` に集約。
- R-C8: `DataIndexerVariableCustomization_PrimaryKey.cpp:289-302` `HandleSelectNone/Variable/Function` は全部 `SetStoredMetaValue` 転送 → 1 つに。
  `:417-443` `BrowseToSelection` の `SMyBlueprint::SelectItemByName` ブロック 2 重。
- R-C9: `DataIndexerDetailCustomization_EditorRow.cpp:254` TODO。監査評価: instanced 登録は `Views/SDataIndexerSelectionDetails.cpp:95-110` のみで
  同型はグローバル layout も持つ → 現状実害なし。advisor 判断: TODO を「許容する制限」に書き換え、実害が出る条件 (per-view だけに登録した
  `ShowOnlyInnerProperties` 構造体) を明記。実装はしない。
- R-C11 (残り): `DataIndexerPrimaryKeyRepositoryResolver.cpp:19` 前方宣言重複、`GraphPin_PrimaryKey.h:24` `DisplayStrings` 未参照
  (`rg` 2026-10-26: 宣言のみ)、`VariableCustomization_RepositorySchema.cpp:227-230, 264-267` (+ `.h:28-29`) no-op / 未バインド。
- P-C3: `DetailCustomization_EditorRow.cpp:754-777 → :167-223, 36-64, 69-87, 105-127` `HandleIsResetToDefaultVisible` が表示行ごとに毎 tick poll
  (エンジン `SResetToDefaultPropertyEditor.cpp:103`) → `GetChildHandle` 確保つきハンドル鎖走査 + 親ごと線形行走査 + パス再構築。
  source Repository と行メモリを `CustomizeDetails` で 1 回解決して保持、行ごとはパス走査だけに。
- P-C4: `VariableCustomization_RepositorySchema.cpp:207` (Text_Lambda), `:240` (CanBrowseTo) 毎フレーム `LoadObject<UClass>`。古いパスだと毎フレーム
  失敗同期ロード + 警告ログ。`CustomizeDetails` / `SetStoredMetaValue` でキャッシュ。
- P-C8: `DetailCustomization_EditorRow.cpp:256` `IsCustomizedStruct` (モジュール検索) がプロパティごと 2 回 + 祖先ごと。ローカル `TMap<const UScriptStruct*, bool>`。
- P-C9: `VariableCustomization_PrimaryKey.cpp:616-623` 毎フレーム線形走査 + `GetName() == StoredValue` 文字列比較。
- P-C10: `DataIndexerRowFunctionBinding.cpp:262,274` `ResolveMember<UFunction>` をバインディングウィジェットごと毎フレーム。バインド済みなら安いので、
  未バインド時だけ結果 (null) をキャッシュして毎フレームの GUID 線形検索を避ける。
- B-C12: `PTC_PrimaryKey.cpp:123`, `PTC_IndexKey.cpp:59` `GetParentHandle()->GetValueData` を null 未確認。Handle 構造体をルートにした
  structure details view で親ハンドルが無い。

## Scope
上の全項目。R-C9 はコメント書き換えのみ。

## Out of Scope
- Resolver の分割 (t261026-de66、done 済み前提)。`ResolveSiblingRepository` はそちらが作ったものを使う。
- K2Node (t261026-dfba)。
- `DataIndexerDetailCustomization_EditorRow.cpp:322-451` `FInstancedStructProvider` (エンジン private クラスのコピー、再利用不可、許容)。

## Preconditions
- t261026-de66 が done (`GraphPin_IndexKey.cpp` / `PTC_PrimaryKeyOptions.cpp` を両方が触る)。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. R-C7 → `DataIndexerMetaDataKeys.h` に定数を足し、リテラルを置換。`"FilterFunction"` は runtime ヘッダの定数を参照する形に。
2. R-C3 + P-C6 → `DataIndexerSchemaFilter.h/.cpp` を新設、メモ化を内蔵。GraphPin / PTC の Repository 2 ファイルから呼ぶ。
3. R-C4 + P-C5 → `DataIndexerKeyOptions.h/.cpp` (名前は実装者が決める) に 3 関数。4 ウィジェットから呼ぶ。
4. R-C6 → `FDataIndexerSchemaClassFilter` 1 クラス。
5. P-C3 / P-C8 → EditorRow details のキャッシュ。P-C4 / P-C9 → VariableCustomization のキャッシュ。P-C10 → 未バインド結果のキャッシュ。
6. R-C8 / R-C11 → 重複と死にコード削除。B-C12 → null ガード。R-C9 → コメント。
7. GUI で Verification。

## Acceptance Criteria
- `rg '"ShowOnlyInnerProperties"|"AllowFunctionLibraries"' Plugins/DataIndexer/Source/DataIndexerEd/ --glob '!DataIndexerMetaDataKeys.h'` が 0 件 (現在 3)。
- `rg 'TryFindTypeSlow' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` が 1 件 (`SchemaFilter` 内、現在 2)。
- `rg 'LoadObject<UClass>' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerVariableCustomization_RepositorySchema.cpp` の
  呼び出しが lambda 内に無い (現在 2 箇所 lambda 内)。
- `rg 'IClassViewerFilter' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` の派生クラスが 1 つ (現在 2)。
- `rg 'DisplayStrings' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_PrimaryKey.h` が 0 件 (現在 1)。
- `rg 'TODO' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_EditorRow.cpp` が 0 件 (現在 1)。
- 1000 行 Repository の行を選択して details を開いたとき、`HandleIsResetToDefaultVisible` 1 回あたり `GetChildHandle` が呼ばれない (カウンタ)。

## Tests
- 自動テストなし (details / Slate)。Tests 4 問: 既存テーブルなし。新しいテストは足していない。
- quick: ビルド + `DataIndexerEd.SchemaLayout` (回帰確認)。
- 実施 (2026-10-28): `Build.bat DataIndexerProjectEditor Win64 Development` → `Result: Succeeded` (22 秒、unity 1 本に新規 2 cpp を含む)。
  `Automation RunTests DataIndexerEd` (全群、SchemaLayout を含む) → `Found 75 automation tests`、`Result={Success}` 75 件、Fail 0。full は run 終端。

## Verification
- 残り (GUI。UE MCP がこの run では停止している):
  - Repository ピッカー (ピンと details) が同じ候補を出すスクリーンショット。Schema クラスピッカー 2 箇所が同じ候補を出すスクリーンショット。
  - Schema Filter 付き BP 変数 details を開いて静止 → log に `LoadObject` 警告が出ない。
  - 1000 行 Repository の行を選択して details を開き、`HandleIsResetToDefaultVisible` 1 回あたり `GetChildHandle` が呼ばれないことをカウンタで確かめる (AC)。

## Execution Log
- 2026-10-28 dispatch 16/18。前提 t261026-de66 は verify_pending (検証未完) の上に構築。両 repo `fix/audit-2026-10`。
- R-C7: `DataIndexerMetaDataKeys.h` に `ShowOnlyInnerProperties` / `AllowFunctionLibraries` / `RowStructure` (`AssetRegistryTagKeys::RowStructure` への参照) /
  `FilterFunction` (`UDataIndexerRepository::FScopedFilterContext::FunctionName` への参照。参照の束縛は定数初期化なので TU 間の初期化順に依存しない) を追加。
  EditorRow・NamedStruct・RowFunctionBinding・PTC_Schema・PTC_PrimaryKeyOptions のリテラルと流用をこれに置換。
- R-C3 + P-C6: `Customizations/DataIndexerSchemaFilter.h/.cpp` を新設。`ResolveExpectedSchemaClass(MetaPath, Outers)`、`LoadSchemaClass` (絶対パス→UClass をメモ化。
  ヒットだけ記憶し、未ロードのミスは次回また引く)、`ShouldFilterRepositoryAsset` (タグのクラスパス→UClass をメモ化)、`GetRepositoryFactories` (factory CDO を弱参照でメモ化)。
  GraphPin_Repository と PTC_Repository の重複実装を削除。
- R-C4 + P-C5: `Customizations/DataIndexerKeyOptions.h/.cpp` を新設 (`IndexKeyDisplayText` / `BuildIndexKeyOptions` / `BuildPrimaryKeyOptions`)。
  4 ウィジェット (GraphPin_IndexKey, PTC_IndexKey, GraphPin_PrimaryKey, PTC_PrimaryKeyOptions) から呼ぶ。PTC_PrimaryKeyOptions の `FindObjectSafe<UFunction>` はキーのループ外で 1 回。
- R-C6: `FDataIndexerSchemaClassFilter` (`RequiredRowStruct` optional) 1 クラスに統合。除外条件は両者の和 (Abstract/Deprecated/NewerVersionExists/非 GeneratedClass の BP クラス + RowStruct)。
  PTC_Schema のピッカーは abstract などの schema クラスを出さなくなった。
- P-C3: `FEditorRowDetails` が `CustomizeDetails` で編集行の PrimaryKey (`DetailBuilder.GetProperty(PrimaryKey)` → 1 行選択のフォールバック) と source Repository を
  1 回解決してメンバに持つ。`HandleIsResetToDefaultVisible` / `HandleResetToDefault` の経路は `GetRowMemory` (ba24 の索引) と `BuildPropertyPathFromRowEntity` (`GetParentHandle` のみ) だけになった。
  `FindEditedPrimaryKey` (祖先ごとの `GetChildHandle`) と親 Repository の行の線形走査は消えた。`GetChildHandle` はファイル内で `CustomizeDetails` の子展開 1 箇所だけ。
- P-C8: `ShouldPushOutInnerProperties` と `ResolveDetailCategory` の内部版に `TMap<const UScriptStruct*, bool>` を渡し、`CustomizeDetails` 1 回で共有する。公開 `ResolveDetailCategory` は局所マップで包む (呼び手 `SDataIndexerSelectionDetails.cpp` は不変)。
- P-C4: `FRepositorySchemaVariableDetails` が `CustomizeDetails` と `SetStoredMetaValue` で schema クラスを解決して保持する (`RefreshSchemaClass`)。Text / CanBrowseTo / Browse は保持値を読む。
- P-C9: `GetCurrentSelectionIcon` の全フィールド文字列比較を `FName(…, FNAME_Find)` + `FindPropertyByName` + 所有クラス一致 (ExcludeSuper 相当) に置換。
- P-C10: `MakeRowFunctionBindingWidget` に `ResolveBoundFunction` を追加。未解決の結果を (名前, GUID, 親クラス, self, skeleton クラス) のキーで記憶し、毎フレームの `ResolveMember` を省く。Text / Image / CanGoto / Goto がこれを使う。
- R-C8: `HandleSelectNone/Variable/Function` を削除し、メニューは `SetStoredMetaValue` (引数を値渡しに変更) を直接バインドする。`BrowseToSelection` の `SelectItemByName` 2 重ブロックを 1 ループにした。
- R-C11: `GraphPin_PrimaryKey.h` の `DisplayStrings` と、`VariableCustomization_RepositorySchema` の `HandleComboBoxOpening` / `HandleSelectionChanged` を削除。
  Resolver の前方宣言重複は de66 で既に消えていた (cpp に宣言の重複なし)。
- B-C12: de66 の `ResolveSiblingRepository` が既に `ParentHandle.IsValid()` を確かめている。`GetParentHandle()->` は DataIndexerEd に 0 件。変更なし。
- R-C9: TODO を「許容する制限」のコメントに書き換えた (実害が出る条件 = per-view だけに登録した ShowOnlyInnerProperties 構造体)。実装はしない。
- 既定 (可逆): PTC_Repository は最も近い Schema meta を 1 つだけ解決し、失敗したら outer の schema へ落ちる。旧実装は失敗時に親ハンドルの meta も試していた。
  根拠: Schema meta は変数自身に付く。戻すコスト: `SchemaFilter` にパス配列版を足す程度。
- 既定 (可逆): `BuildPrimaryKeyOptions` は spec の `const UDataIndexerRepository&` でなくポインタを取る (null なら None だけを返す。`BuildIndexKeyOptions` と揃えた)。
- 既定 (可逆): PTC_Schema の行構造体の絶対パス解決を `TryFindTypeSlow<UScriptStruct>` から `FindObject<UScriptStruct>` に変えた (meta は "/" 始まりのフルパスだけを通すので結果は同じ)。
- AC の数え直し: 起票時の「現在値」は `TryFindTypeSlow` 2 (実際は PTC_Schema を含め 3)、`DisplayStrings` 1 (その後 `DataIndexerEditorData.cpp` に無関係のローカル変数が入った)。
  `"ShowOnlyInnerProperties"|"AllowFunctionLibraries"` は集約先 `DataIndexerMetaDataKeys.h` の定義 2 件が残るのが本来の形。同じ結果を言い直して確かめた:
  `rg '"ShowOnlyInnerProperties"|"AllowFunctionLibraries"' Source/DataIndexerEd/ --glob '!DataIndexerMetaDataKeys.h'` 0 件、
  `rg TryFindTypeSlow Customizations/` 1 件 (SchemaFilter.cpp)、`rg 'LoadObject<UClass>' VariableCustomization_RepositorySchema.cpp` 0 件、
  `rg ': public IClassViewerFilter' Customizations/` 1 件、`rg DisplayStrings` の GraphPin 側 0 件、`rg TODO EditorRow.cpp` 0 件。
- 外部の呼び手: `Plugins/DataIndexerToolset` と `Source/` に、変更したヘッダとシンボルの参照なし。UE 5.7 の `FMemberReference` にも `GetMemberGuid` / `GetMemberParentClass` / `IsSelfContext` がある。
- 自己レビュー: `git diff --stat` 17 ファイル + 新規 4 ファイル、すべて Customizations と `DataIndexerMetaDataKeys.h` (Scope 内)。+229 / -525。
- verify_pending の理由: GUI 検証 3 件 (Verification) と AC のカウンタ項目は、UE MCP 停止中で実施できない。
- Critic round 1 (task-critic):
  ```
  FAIL
  1. DataIndexerMetaDataKeys.h:17-18 — kind 1 — AC `rg '"ShowOnlyInnerProperties"|"AllowFunctionLibraries"' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 を要求。diff 後 2 件 (`TEXT( "ShowOnlyInnerProperties" )`, `TEXT( "AllowFunctionLibraries" )`)。Execution Log は `--glob '!DataIndexerMetaDataKeys.h'` に AC を言い直して 0 件と主張。言い直しは AC を満たさない。Step 1 と AC が矛盾しているので、直す場所はコードでなく AC の文言 (タスク所有者の承認が必要)。
  2. DataIndexerEditorData.cpp:404,412,416 — kind 1 — AC `rg 'DisplayStrings' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 を要求。今は 3 件 (この diff の外にある無関係なローカル変数)。GraphPin 側の削除 (DataIndexerGraphPin_PrimaryKey.h:24) は済んでいる。AC の文言では満たせない。1 と同じく、AC をタスク所有者が改める必要がある。
  NOTE: EditorRow.cpp:750-776 — GetChildHandle カウンタの AC は GUI 計測が要る。diff 上は満たしている。残る GetChildHandle は :610 の CustomizeDetails 内 1 箇所。
  NOTE: PTC_Repository.cpp:22-29 — 旧実装は最も近い Schema meta が失敗すると親ハンドルの meta も試した。新実装はすぐ outer の schema へ落ちる。頼る呼び手は repo 内に無い。FAIL にしない。
  NOTE: SchemaFilter.cpp:155-160 — PTC_Schema のピッカーが Abstract / Deprecated / SKEL を出さなくなった。R-C6 の範囲内。
  NOTE: RepositorySchema.cpp:192-195 — CanBrowseTo / BrowseToSelection は UDataIndexerSchema の子クラスだけを見る。Schema 以外のクラスが保存されている場合だけ違いが出る。
  NOTE: 削除した名前は repo 全体で 0 件。新しい定義にはすべて呼び手がある。FInstancedStructProvider は触っていない。
  ```
- Round 1 への回答: 1 = Step 1 / R-C7 が集約先をこの DataIndexerEd 内のヘッダに指定しているので定義 2 件は残る。grep を欺く書き方 (文字列化マクロ) はしない。
  2 = 残る 3 件は Scope 外の無関係なローカル変数。対象の死んだメンバは削除済み。どちらもコードは変えず、AC 文言の要ユーザー判断として残す。
- Critic round 2 (task-critic):
  ```
  FAIL
  1. DataIndexerMetaDataKeys.h:17-18 — kind 1 — AC は 0 件を要求、2 件残る。回答は正しい (AC が Step 1 / R-C7 と矛盾、使用箇所は 0 件)。AC の文言のままでは満たせない。文言を変えるかはユーザーが決める。
  2. DataIndexerEditorData.cpp:404,412,416 — kind 1 — AC は 0 件を要求、3 件残る。回答は確認できた (対象メンバは削除済み、残りは diff 外の無関係なローカル変数)。AC の文言は真でない。同じくユーザー判断。
  NOTE: round 1 から変化なし。どちらも AC 文言の衝突でコードの欠陥ではない。要ユーザー判断へ回すべき。
  NOTE: 削除名の掃除は 0 件、新しい定義はすべて呼び手あり。他の AC (TryFindTypeSlow 1 件、LoadObject<UClass> 0 件、IClassViewerFilter 派生 1 件、TODO 0 件) は grep で成立。
  NOTE: EditorRow.cpp:610 — カウンタ AC は GUI 専用。リセット経路に GetChildHandle は無い。verify_pending は妥当。
  NOTE: PTC_Repository の親ハンドル meta へのフォールバック削除は R-C3 の単一 MetaPath 署名に沿う。PTC_Schema が abstract 等を隠すのは R-C6 の和。
  ```
- 2 round 後も開いている 2 件 (AC 文言) は要ユーザー判断 (run 終端) へ。コミットは進める。

### ユーザー決定 2026-10-28 (AC 文言の修正)
- 2 件とも AC の文言を直した。コードは変えない。status は変えない。
- 旧文言 1: 「`rg '"ShowOnlyInnerProperties"|"AllowFunctionLibraries"' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 3)。」
- 新文言 1: 同じ rg に `--glob '!DataIndexerMetaDataKeys.h'` を足して 0 件。
- 旧文言 2: 「`rg 'DisplayStrings' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 1)。」
- 新文言 2: 対象を `DataIndexerGraphPin_PrimaryKey.h` に絞って 0 件。

## Summary
Customizations の重複を `DataIndexerSchemaFilter` / `DataIndexerKeyOptions` に集約し、schema クラスピッカーのフィルタを 1 クラスにした。
details と変数カスタマイズの毎フレーム `LoadObject` / `GetChildHandle` 鎖走査 / 未バインド関数の再解決を、構築時の解決とキャッシュに置き換えた。meta キーは `DataIndexerMetaDataKeys.h` に集めた。
GUI 検証は未実施 (verify_pending)。

## Changed Files
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerMetaDataKeys.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerSchemaFilter.h (新規)
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerSchemaFilter.cpp (新規)
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerKeyOptions.h (新規)
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerKeyOptions.cpp (新規)
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_EditorRow.h / .cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_IndexKey.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_PrimaryKey.h / .cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_Repository.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_IndexKey.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_NamedStruct.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_PrimaryKeyOptions.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_Repository.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_Schema.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerRowFunctionBinding.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerVariableCustomization_PrimaryKey.h / .cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerVariableCustomization_RepositorySchema.h / .cpp

## Blockers
検証待ち: GUI 検証 3 件と `GetChildHandle` カウンタの AC。UE MCP (127.0.0.1:8001) がこの run では停止しており、エディタ GUI を操作できない。
