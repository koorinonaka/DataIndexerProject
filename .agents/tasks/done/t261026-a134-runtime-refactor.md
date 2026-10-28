---
status: done
order: 90
tier: standard
screen: none
agent: any
created_by: task-create
---

# Runtime module の重複排除・deprecated API 削除・ガード規則統一

## Goal
`DataIndexer` runtime module から、呼び手ゼロの deprecated API、3 写しの property 検索、二重定義の戻り値バリデータ、未使用 include と
モジュール依存、`WITH_EDITOR` / `WITH_EDITORONLY_DATA` の混在を消す。挙動は変えない。

## Context
監査 `.agents/audit-2026-10-26.md` の R-R1〜R-R13。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexer/` 相対、行番号は 2026-10-26 時点。

- R-R1: `Public/DataIndexerSchema.h:62-66` `UE_DEPRECATED RegisterFunction_BuildIndex(Index, FName)` の呼び手はプラグイン内ゼロ
  (`rg` 2026-10-26: 定義 `:63`、テンプレート版 `:71`、マクロ `:168` はテンプレート版を呼ぶ)。ユーザー決定 2026-10-26: 今回削除。
  `docs/docs/api-reference/schema-api.md:86` (ja 版も) の deprecation warning も落とす。
- R-R2: `Private/DataIndexerSchema_ColumnLayout.cpp:174-181` / `Private/DataIndexerSchema_Validation.cpp:169-176` /
  `Private/DataIndexerRepository.cpp:360-374` 「FName か authored name で property を探す」が 3 写し (Repository 版は property ごとに `ToString()`)。
  `FindPropertyByNameOrAuthoredName(const UScriptStruct&, FName)` を 1 つ。
- R-R3: `Private/DataIndexerSchema.cpp:182-186` ≡ `Validation.cpp:127-132` (UUserWidget 判定)、`Schema.cpp:245-249` ≡ `Validation.cpp:33-37`
  (FGuid 判定)、`Validation.cpp:14` の `IsVoidReturn` を `:206` で再定義。`Private/DataIndexerSchemaRowFunction.h` に集約。
- R-R4: `Private/DataIndexerDrivenCollection.cpp:4-5` `IPluginManager.h` / `CoreDelegates.h` 未使用。`DataIndexer.Build.cs:16,23` の
  `Projects` 依存はそのためだけ、`PropertyPath` public 依存はモジュール内未参照。
- R-R5: `Public/DataIndexerRepository.h:6` `AssetImportData.h` 無条件 include、cpp (`:11-13`) は `WITH_EDITORONLY_DATA` で再 include。前方宣言に。
- R-R6: `Private/DataIndexerTypes.cpp:39-67, 89-129, 207-230, 269-291` PrimaryKey / IndexKey / RowHandle / KeysHandle の Export/ImportTextItem がコピペ。
  GUID helper + "No asset specified / None" prefix helper。
- R-R7: `Public/DataIndexerTypes.h:128-130,157-161` / `Private/DataIndexerTypes.cpp:74-80` / `Public/DataIndexerSchema.h:97-132 vs 141-162` /
  `Private/DataIndexerSchema_Validation.cpp:21-27,65-74,83-105,141-149,154-188` / `Private/DataIndexerSchema.cpp:124-142`
  `WITH_EDITOR` と `WITH_EDITORONLY_DATA` が混在、`WITH_EDITOR` 内に `WITH_EDITORONLY_DATA` の再ガード。`WITH_EDITORONLY_DATA && !WITH_EDITOR` で
  `DevComment` / `ValidationContext` が未初期化。規則: データは `WITH_EDITORONLY_DATA`、コードは `WITH_EDITOR`、内側の再ガードは消す。
  t261026-91cd が足すガードはこの規則に合わせて残す (消さない)。
- R-R8: `Public/DataIndexerTypes.h:52-53, 96-97, 126-137, 145` `FGuid` / `FString` → key、`FDataIndexerIndex` ⇄ `FDataIndexerIndexKey` が implicit 変換。
  `BuildIndexFunctions.Find(IndexKey)` (`Schema.cpp:244`) がルックアップごとに FText 付き temporary を作る。
  **ユーザー決定 2026-10-26 (昇格レビュー): (a) を採る。** (a) `FString` / `FGuid` コンストラクタだけ `explicit`、
  `FDataIndexerIndex → FDataIndexerIndexKey` の暗黙変換は memory に「documented convention」とあり外部 Schema コードが使うので残す、
  (b) 全部 `explicit`。temporary の問題は (a) でも `Find` の引数側を `FDataIndexerIndexKey` 受けにすれば消える。
- R-R9: `Public/DataIndexerEditorInterface.h:16,31,49` `UINTERFACE` の U クラスに `DATAINDEXER_API` が無い。
- R-R10: `Private/DataIndexerFunctionLibrary.cpp:144-147` vs `:113-118` 未解決時の扱いが無言 false と `ThrowScriptException` で不揃い。`ThrowScriptException` に揃える。
- R-R11: `Public/DataIndexerRepository.h:23-31` runtime ヘッダに editor クラス 9 個の friend。設計変更なので本タスクでは**やらない**。KNOWLEDGE に 1 行。
- R-R12: `Private/DataIndexerSchema.cpp:6` `Blueprint/UserWidget.h` 無条件 include、使うのは `WITH_EDITOR` 内のみ。
- R-R13: `Private/DataIndexerTypes.cpp:258` `*RowDisplayName->ToString()` の TCHAR* 経由コピー。`Public/DataIndexerSchemaInterface.h:17-22` を `GetPtr<>()` 1 行に。

## Scope
R-R1〜R-R10, R-R12, R-R13。docs の deprecation 記述削除。

## Out of Scope
- R-R11 (friend 群)。
- 挙動変更。B-R3 の契約変更は t261026-2e18。
- `OwnerRepository` ガード (t261026-91cd)。

## Preconditions
- t261026-2e18 と t261026-91cd が done (同じファイル群を触る。R-R7 のガード統一は t261026-91cd が足すガードを前提に行う)。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. R-R1 → 定義とテンプレート版の `#pragma`/`UE_DEPRECATED` 周辺を整理、docs 2 ファイルの warning を消す。
2. R-R4 / R-R5 / R-R12 → include と Build.cs。Editor + Game ビルド。
3. R-R9 → API マクロ。
4. R-R2 / R-R3 → helper 抽出。3 箇所 + 二重定義を置換。
5. R-R6 → Export/ImportTextItem helper。既存の `DataIndexerSerializerTests` / `SearchableNameTests` で round-trip を確認。
6. R-R8 → `explicit`。コンパイルエラーになった暗黙変換箇所を明示化 (挙動同一)。
7. R-R7 → ガード規則統一。`WITH_EDITORONLY_DATA && !WITH_EDITOR` 構成は手元でビルドできないので、該当メンバの初期化子を型定義側に置く
  (`FText DevComment;` は既定で空、`TOptional` は未設定) ことで未初期化を潰す。
8. R-R10 / R-R13。
9. full (全群) を回す。

## Acceptance Criteria
- `rg 'UE_DEPRECATED' Plugins/DataIndexer/Source/DataIndexer/` が 0 件 (現在 1)。`rg -i 'deprecat' docs/docs/api-reference/schema-api*.md` が 0 件。
- `rg 'IPluginManager|CoreDelegates' Plugins/DataIndexer/Source/DataIndexer/` が 0 件 (現在 2)。`DataIndexer.Build.cs` に `Projects` が無い。
  - 修復 2026-10-28 (PLAN-REPAIRABLE): 書かれたとおりの rg は `FBlueprintCoreDelegates::ThrowScriptException` (起票時点で既に 3 件、R-R10 が 1 件足す) に部分一致する。
    起票時の「現在 2」は `DataIndexerDrivenCollection.cpp:4-5` の include 2 件を数えたもの。判定は `rg 'IPluginManager|Misc/CoreDelegates|\bFCoreDelegates\b' Plugins/DataIndexer/Source/DataIndexer/` が 0 件。
- `FindPropertyByNameOrAuthoredName` が 1 箇所で定義され 3 箇所から呼ばれる (`rg` 4 件、現在 0)。
- `rg 'IsVoidReturn' Plugins/DataIndexer/Source/DataIndexer/Private/` の定義が 1 件 (現在 2)。
- `rg 'DATAINDEXER_API' Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerEditorInterface.h` が U/I 両クラス分 (現在 I のみ)。
- `FDataIndexerIndexKey` を `FString` から暗黙構築するコードがコンパイルエラーになる (テスト用 static_assert で `!std::is_convertible_v<FString, FDataIndexerIndexKey>`)。
  `FDataIndexerIndex → FDataIndexerIndexKey` は暗黙のまま (`std::is_convertible_v` true を static_assert)。
- 全自動テスト 54 件が通る。

## Tests
- 結果 (2026-10-28): `Automation RunTests DataIndexerEd` (全群) → Success 69 / Fail 0。static_assert 5 本は Editor ビルドで成立 (対照: explicit を外すと C2607)。full は run 終端でも回る。
- 新規テスト行なし (挙動不変のリファクタ。既存 54 件が回帰を取る)。`std::is_convertible_v` の `static_assert` だけ `DataIndexerSchemaTests.cpp` に置く。
- full: `DataIndexerEd` 全群。Game ターゲットもビルド。

## Verification
- full の log。GUI 不要。
- 2026-10-28: `Saved/Logs/DataIndexerProject.log` で Success 69 / Fail 0、`TEST COMPLETE. EXIT CODE: 0`。

## Execution Log
2026-10-28 (dispatch 10/18、branch `fix/audit-2026-10`、両 repo)。

- 前提: t261026-2e18 は done。t261026-91cd は verify_pending (GUI 検証のみ残り) の上に構築。91cd のガード (`DataIndexer::Delegates`、`FindOwnerRepository`、`MarkSearchableName`) は触っていない。
- R-R1: `DataIndexerSchema.h` の `UE_DEPRECATED RegisterFunction_BuildIndex( Index, FName )` を削除。`BindBuildIndexFunction` のコメントから「deprecated overload」を削除。
  プラグイン外 (`Source/`、`Plugins/DataIndexerToolset`、`DataIndexerEd`) に 2 引数の呼び手が無いことを rg で確認。docs en/ja の warning ブロック (3 行ずつ) を削除。
- R-R4: `DataIndexerDrivenCollection.cpp` の `Interfaces/IPluginManager.h` / `Misc/CoreDelegates.h` を削除。`DataIndexer.Build.cs` から `Projects` と public `PropertyPath` を削除
  (runtime module 内に PropertyPath の参照なし。`DataIndexerEd.Build.cs` は自前で `PropertyPath` に依存している)。
- R-R5: `DataIndexerRepository.h` の `EditorFramework/AssetImportData.h` を前方宣言 `class UAssetImportData;` に置換。cpp は既存の `WITH_EDITORONLY_DATA` include のまま。
  Ed で `UAssetImportData` を使う 2 ファイル (`DataIndexerEditorToolkit.cpp` / `DataIndexerJsonSupport.cpp`) は自前で include 済み。
- R-R12: `DataIndexerSchema.cpp` の `Blueprint/UserWidget.h` を `#if WITH_EDITOR` 内へ。`DataIndexerSchema_Validation.cpp` の同じ無条件 include も `#if WITH_EDITOR` 内へ移した。
- R-R9: `UDataIndexerInterface_PropertyResolver` / `_CellContext` / `_EditorData` に `DATAINDEXER_API`。
- R-R2: `DataIndexer::FindPropertyByNameOrAuthoredName( const UScriptStruct&, FName )` を `DataIndexerSchemaRowFunction.h` に inline 定義 (`#if WITH_EDITOR`、呼び手 3 つとも editor)。
  `FindPropertyByName` を先に引き、外れたら authored name を比較する。`ToString()` はループの外で 1 回。Repository 版にあった `IsA<UUserDefinedStruct>` の分岐は落とした
  (native struct の `GetAuthoredNameForField` は `GetName()` を返すので、`FindPropertyByName` が外れた名前は authored name でも一致しない。結果は同じ)。
  `DataIndexerRepository.cpp` の `StructUtils/UserDefinedStruct.h` はこれで未使用になったので削除。
- R-R3: `IsVoidReturn` / `IsGuidReturn` を `DataIndexerSchemaRowFunction.h` に inline、`IsUserWidgetReturn` は宣言だけ header (`#if WITH_EDITOR`)、定義は `UserWidget.h` を include する
  `DataIndexerSchema_Validation.cpp`。Schema.cpp の 2 個のラムダ、Validation.cpp の 3 個のラムダと匿名名前空間の `IsVoidReturn` を置換。
- R-R6: `DataIndexerTypes.cpp` に匿名名前空間の `ExportGuidText` / `ImportGuidText` / `ExportMissingHandleText`。PrimaryKey / IndexKey の Export/Import と
  RowHandle / KeysHandle の "No asset specified" / "%s: None" をそれに置換。`FDataIndexerIndex::ExportTextItem` は別ロジックなので触らない。
- R-R8 (ユーザー決定 (a)): `FDataIndexerPrimaryKey( FGuid )` / `( FString )`、`FDataIndexerIndexKey( FGuid )` / `( FString )`、`FDataIndexerIndex( FGuid, FText )` を `explicit`。
  `FDataIndexerIndex → FDataIndexerIndexKey` の変換演算子は暗黙のまま。
  既定: `FDataIndexerIndex( const FDataIndexerIndexKey&, FText )` も暗黙のまま (FGuid/FString のコンストラクタではないので (a) の対象外と読んだ)。explicit にするコストは呼び手の修正。
  コンパイルエラーは 2 箇所だけ: `DataIndexerSchema.cpp` の `InvalidKey.IndexKey = FGuid::NewGuid()` と `K2Node_DataIndexerSwitchPrimaryKey.cpp:162` の `PinKeys.Remove( { FString } )`。
  どちらも明示構築に直した (挙動同一)。sample `Source/` と `DataIndexerToolset` はエラー無し (同じビルドでコンパイルされた)。
  `CallBuildIndex` の `BuildIndexFunctions.Find( IndexKey )` を `FindByHash( GetTypeHash( IndexKey ), IndexKey )` に置換。`FDataIndexerIndex` の temporary (FText 付き) を作らない。
  `GetTypeHash( FDataIndexerIndex )` は `GetTypeHash( IndexKey )` で、比較は既存の `FDataIndexerIndex == FDataIndexerIndexKey`。
  `DataIndexerSchemaTests.cpp` に static_assert 5 本 (FString/FGuid → IndexKey/PrimaryKey が不可 4 本、Index → IndexKey が可 1 本)。
  対照: `FDataIndexerIndexKey( const FString& )` から `explicit` を外して Editor をビルド → `DataIndexerSchemaTests.cpp(13,16): error C2607` (1 本目の static_assert) で失敗。戻して再ビルド成功。
- R-R7: 規則「データは `WITH_EDITORONLY_DATA`、コードは `WITH_EDITOR`、`WITH_EDITOR` 内の `WITH_EDITORONLY_DATA` 再ガードは消す」を適用。
  - `FDataIndexerIndex` ctor の `DevComment` 初期化、`ToString()`、`ExportTextItem` の `DevComment` 参照: `WITH_EDITOR` → `WITH_EDITORONLY_DATA`。
  - `UDataIndexerSchema` ctor の `ValidationContext` 生成: `WITH_EDITOR` → `WITH_EDITORONLY_DATA` (メンバは EODATA)。
  - `CallRowValidationFunction` の宣言 (Schema.h) と定義 (Validation.cpp): `WITH_EDITORONLY_DATA` → `WITH_EDITOR` (コード)。
  - 内側の再ガードを削除: Schema.cpp `RekeyInvalidBuildIndexFunctions`、Validation.cpp の `TryValidateBuildIndexFunction` / `IsDataValid` 4 箇所 / `IsRowValid`。
  - Step 7 の「初期化子を型定義側に」: `FText DevComment` は既定で空、`TObjectPtr ValidationContext` は既定で null に初期化される。ctor のガードを EODATA にしたので
    `WITH_EDITORONLY_DATA && !WITH_EDITOR` 構成でも `DevComment` は引数で、`ValidationContext` は生成で埋まる。追加の初期化子は不要と判断した。
  - `WITH_EDITORONLY_DATA && !WITH_EDITOR` 構成は手元でビルドできない。Editor (両方 1) と Game (両方 0) でビルドした。
- R-R10: `execGetRowsHandleValue` で Query が解決できない (`!QueryProperty || !QueryPtr || !QueryProperty->Struct`) ときに `ThrowScriptException` (`execGetRowHandleValue` と同じ形)。
  `GenericGetRowsHandleKeys` は `const FStructProperty&` 受けにして内側のガードを削除。
  既定: `!QueryProperty->Struct` も例外の条件に入れた。根拠: 旧ヘルパーはこれも「未解決」として同じ無言 false にしていた。変えるコストは条件 1 つ。
- R-R13: `FDataIndexerRowHandle::ToString` の `*RowDisplayName->ToString()` → `RowDisplayName->ToString()`。`TNativeSchemaInterface::FindRow` を `GetPtr<>()` の 1 行に。
- R-R11: やらない (Out of Scope)。KNOWLEDGE.md の監査節に 1 行。
- AC の修復 (PLAN-REPAIRABLE): 2 番目の AC の rg は `FBlueprintCoreDelegates` に部分一致し、R-R10 の要求と両立しない。判定を include と `FCoreDelegates` に限った文言を AC 直下に追記した。
- AC「全自動テスト 54 件」は古い。この run の時点で DataIndexerEd は 69 件。69 件で判定した。
- ビルド: Editor 1 回目は K2Node の 1 エラーで失敗 → 修正後成功 (21 秒)。Game ターゲット成功 (23 秒、`Module.DataIndexer.cpp` と sample を再コンパイル)。
  対照ビルド後の再ビルド 1 回目は `Module.DataIndexerEd.cpp: Exited with error code -1` (一過性。dispatch の注意どおり 1 回再実行) → 2 回目成功。
- テスト: `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests DataIndexerEd; Quit"` → `Result={Success}` 69 件、`Result={Fail` 0 件、`TEST COMPLETE. EXIT CODE: 0`。full (全群) を兼ねる。
- diff 確認 (stat): submodule 16 ファイル +159/-214、親は docs 2 ファイル・KNOWLEDGE.md・タスクファイル。Scope 外のソースは `K2Node_DataIndexerSwitchPrimaryKey.cpp` 1 行 (Step 6 の明示化) と
  `DataIndexerSchemaTests.cpp` (Tests 節が指定) だけ。
- critic round 1 (task-critic):

  ```
  PASS
  NOTE: …DataIndexerFunctionLibrary.cpp:138 — `GetRowsHandleValue` が無言 false ではなく例外を投げるようになった。これは挙動変更である。ただし task の R-R10 が明示的に要求しているので、Out of Scope の「挙動変更」には当たらない。`!QueryProperty->Struct` を例外の条件に入れた件は作者の判断である。
  NOTE: …DataIndexerSchema.cpp:21 / …DataIndexerTypes.h:125 — `WITH_EDITORONLY_DATA && !WITH_EDITOR` 構成はビルドで確かめていない。作者の Execution Log もビルドできないと書いている。diff だけでは確かめられない。
  NOTE: …DataIndexerRepository.cpp:377 — 呼び出しは名前空間を付けずに `FindPropertyByNameOrAuthoredName` を呼ぶ。名前の解決は同じファイル :19 の `using namespace DataIndexer;` に依存している。他の 2 箇所は `DataIndexer::` を付けている。命名の揃いの問題で、動作には影響しない。
  NOTE: …DataIndexerRepository.cpp:377 — Repository 版にあった `IsA<UUserDefinedStruct>` の分岐が消えた。結果は同じという作者の主張は妥当である。根拠: native struct の `GetAuthoredNameForField` は `GetName()` を返す。名前の比較はどちらの経路も大文字小文字を区別しない。
  NOTE: …t261026-a134-runtime-refactor.md:83 — AC は「54 件」と書いている。実測は 69/69 で、AC の件数が古い。69 件すべて通っているので AC は満たしている。
  ```

## Summary
runtime module の R-R1〜R-R10, R-R12, R-R13 を実施。deprecated `RegisterFunction_BuildIndex( Index, FName )` と docs の warning を削除。未使用 include と `Projects` / `PropertyPath` 依存を削除。
property 検索と戻り値バリデータを `DataIndexerSchemaRowFunction.h` に集約。Export/ImportTextItem の写しを helper 化。キーの FString/FGuid コンストラクタを `explicit` にした。
`WITH_EDITOR` / `WITH_EDITORONLY_DATA` を「データ/コード」で揃えた。GetRowsHandleValue の未解決 Query は例外にした。R-R11 は KNOWLEDGE に 1 行。Editor・Game ビルド成功、DataIndexerEd 69/69。

## Changed Files
submodule `Plugins/DataIndexer`:
- Source/DataIndexer/DataIndexer.Build.cs
- Source/DataIndexer/Private/DataIndexerDrivenCollection.cpp
- Source/DataIndexer/Private/DataIndexerFunctionLibrary.cpp
- Source/DataIndexer/Private/DataIndexerRepository.cpp
- Source/DataIndexer/Private/DataIndexerSchema.cpp
- Source/DataIndexer/Private/DataIndexerSchemaRowFunction.h
- Source/DataIndexer/Private/DataIndexerSchema_ColumnLayout.cpp
- Source/DataIndexer/Private/DataIndexerSchema_Validation.cpp
- Source/DataIndexer/Private/DataIndexerTypes.cpp
- Source/DataIndexer/Public/DataIndexerEditorInterface.h
- Source/DataIndexer/Public/DataIndexerRepository.h
- Source/DataIndexer/Public/DataIndexerSchema.h
- Source/DataIndexer/Public/DataIndexerSchemaInterface.h
- Source/DataIndexer/Public/DataIndexerTypes.h
- Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSwitchPrimaryKey.cpp
- Source/DataIndexerEd/Private/Tests/Core/DataIndexerSchemaTests.cpp

親 repo:
- docs/docs/api-reference/schema-api.md
- docs/docs/api-reference/schema-api.ja.md
- .agents/tasks/KNOWLEDGE.md
- .agents/tasks/done/t261026-a134-runtime-refactor.md

## Blockers
None
