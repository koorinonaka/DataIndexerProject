---
status: ready
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
- `FindPropertyByNameOrAuthoredName` が 1 箇所で定義され 3 箇所から呼ばれる (`rg` 4 件、現在 0)。
- `rg 'IsVoidReturn' Plugins/DataIndexer/Source/DataIndexer/Private/` の定義が 1 件 (現在 2)。
- `rg 'DATAINDEXER_API' Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerEditorInterface.h` が U/I 両クラス分 (現在 I のみ)。
- `FDataIndexerIndexKey` を `FString` から暗黙構築するコードがコンパイルエラーになる (テスト用 static_assert で `!std::is_convertible_v<FString, FDataIndexerIndexKey>`)。
  `FDataIndexerIndex → FDataIndexerIndexKey` は暗黙のまま (`std::is_convertible_v` true を static_assert)。
- 全自動テスト 54 件が通る。

## Tests
- 新規テスト行なし (挙動不変のリファクタ。既存 54 件が回帰を取る)。`std::is_convertible_v` の `static_assert` だけ `DataIndexerSchemaTests.cpp` に置く。
- full: `DataIndexerEd` 全群。Game ターゲットもビルド。

## Verification
- full の log。GUI 不要。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
