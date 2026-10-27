---
status: ready
order: 20
tier: standard
screen: none
agent: any
created_by: task-create
---

# JSON import を原子化し、失敗理由を返し、round-trip の非対称を直す (X5)

## Goal
JSON import (`ImportAsJSON` / `ImportFromDataTableJSON` / `ImportRowsFromJSON` / Reimport) が、途中失敗で
アセットを壊さず、トランザクションを cancel し、失敗理由を呼び手に返す。export → import の round-trip で
FText / `Name` 列 / 重複 PrimaryKey が黙って落ちない。

## Context
監査 `.agents/audit-2026-10-26.md` の X5 = B-E2, B-E3, B-E9, B-E10, B-E11, B-E12, B-E13, B-E16, R-E1, R-E12, R-E14, P-E2, P-E5。
重複チェック通過 (0件対象)。パスは `Plugins/DataIndexer/Source/` 相対、行番号は 2026-10-26 時点。

- B-E2: `DataIndexerEd/Private/DataIndexerJsonSupport.cpp:94` `ImportAsJSON` が live `EditorRows` を
  `JsonArrayStringToUStruct` に渡す。エンジン `JsonObjectConverter.h:393-414` は `SetNum(N)` してから in-place 変換し、
  最初の不正要素で `false`。途中失敗で N 個の default 行が残る。既存の `DataIndexerImportAtomicityTests.cpp` は
  `ImportFromDataTableJSON` だけを覆う。
- B-E3: `:252`, `:469`, `DataIndexerEditorFunctionLibrary.cpp:44` の 3 呼び手が失敗時に `FScopedTransaction` を
  `Cancel()` しない (`rg '\.Cancel\(' DataIndexerJsonSupport.cpp` 0 件、2026-10-26 確認)。`OutFailReason` は `:95` で nullptr。
- R-E1: 「transaction + ImportAsJSON + `FindFProperty("EditorRows")` + NotifyPostChange」が 3 写し (`:262-265`, `:476-478`,
  `EditorFunctionLibrary.cpp:49-50`)。2 つは `TEXT("EditorRows")` リテラル、1 つは `GET_MEMBER_NAME_CHECKED`。
- R-E12: `Public/DataIndexerEditorData.h:82-83` の `friend class FDataIndexerSerializer` が modifier を迂回して
  `EditorRows` を書く元。`friend FDataIndexerCompiler` は `GetRows()` が public なので不要。
- P-E2: `:349-407` `ImportFromDataTableJSON` が `Empty()` → 行ごと `InsertRow` → 行ごとに `FScopedRowsModifier`
  (Modify + `MarkRepositoryDirty` + 全 toolkit の `NotifyAssetChangedExternally`)。
- P-E5: `:161-167` + `:94` でファイル全体を 2 回パース (形式判定 + 本 import)。
- B-E9: export は `FText::ToString()` (`:63`)、import は `FTextStringHelper::ReadFromBuffer` (`:123`、エンジン
  `JsonObjectConverter.cpp:819-828`) → localized text の namespace/key が落ちる。直すと export の FText 値の文字列形式が
  `NSLOCTEXT("ns","key","text")` 形になり、旧 export との VCS diff が FText 列で全行変わる (docs は VCS diff を売りにしている)。
  旧ファイルの import は引き続き通る。**ユーザー決定 2026-10-26 (昇格レビュー): 変える。** docs の json-support に形式変更を 1 行足す。
- B-E10: `:43` `ImportRowEntityFromJson` が `JsonObjectToUStruct` の戻り値を無視し non-strict。行レベル (`:95` strict) と非対称。
- B-E11: `:378` `RowNameProperty == NAME_None` でも `Name` キーを剥がす。
- B-E12: `DataIndexerEditorData.cpp:447` keyed `InsertRow` が重複未検査。`:364` でパース不能 PrimaryKey を無言で新 GUID。
- B-E13: `DataIndexerRepositoryFactory.cpp:19-21` `bEditorImport = true` + `Formats "json"` だが `FactoryCreateFile/Text` 未実装。
  docs (`docs/docs/editor-guide/json-support.md:3,43-62`) は Content Browser コンテキストメニューの Import JSON と toolbar の
  Reimport しか約束していない → advisor 判断: `bEditorImport = false` にして `Formats` を外す (実装しない)。
- B-E16: `:459` `UE_LOG(LogTemp, …)`。他は `LogDataIndexer`。
- R-E14: `:384-408` 手動 `TArray<uint8>` + `InitializeStruct/DestroyStruct`。`:42` は既に `FInstancedStruct`。

## Scope
- `ImportAsJSON` はローカル `TArray<FDataIndexerEditorRow>` に deserialize → 成功時だけ `UDataIndexerEditorData::SetRows(TArray&&)`
  (新設、`FScopedRowsModifier` 経由) で swap。失敗時は `OutFailReason` を埋めて `false`。
- 3 呼び手は `FScopedTransaction` を持ち、失敗で `Cancel()`。Content Browser 経路は `FMessageDialog` か notification で理由を出す。
  `ImportRowsFromJSON` (BP 関数) は `OutFailReason` を返す引数を足す。
- `ImportFromDataTableJSON` もローカル配列を組んでから `SetRows` 1 回。重複 PrimaryKey とパース不能 PrimaryKey は失敗理由に含める。
- `friend class FDataIndexerSerializer` / `friend class FDataIndexerCompiler` を `DataIndexerEditorData.h` から外す。
- JSON パースを 1 回にし、`TArray<TSharedPtr<FJsonValue>>` を下へ渡す。
- FText は `WriteTextAsComplexString` 相当で export し、import と対称にする。
- `ImportRowEntityFromJson` は戻り値を見て strict (`EJsonObjectConversionFlags` で未知キーを失敗に) にする。
- `Name` キーの剥がしは `RowNameProperty != NAME_None` のときだけ。
- Factory の import 主張を外す。`LogTemp` を `LogDataIndexer` に。`:384-408` を `FInstancedStruct` に。

## Out of Scope
- 行ルックアップのマップ化 (t261026-ba24)。`SetRows` の中でマップを組むのはそちらが足す。
- Undo 通知経路 (t261026-21a2)。`SetRows` は同タスクが作る `NotifyRowsChanged()` を呼ぶ想定。先に走っていなければ
  `NotifyPostChange` を呼ぶ。
- JSON の形式仕様そのものの変更 (キー名・配列構造)。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。
- t261026-21a2 が先に done なら `NotifyRowsChanged()` を使う。必須ではない。

## Steps
1. `DataIndexerEditorData.h/.cpp` に `bool SetRows(TArray<FDataIndexerEditorRow>&& InRows)` を追加 (`FScopedRowsModifier` 内で `EditorRows = MoveTemp`)。
   friend 2 行を削除し、ビルドエラーになった箇所を `SetRows` / public getter に置換する。
2. `DataIndexerJsonSupport.cpp` の `ImportAsJSON` をローカル配列方式に書き換え、`OutFailReason` (FText*) を必須引数にする。
   `ImportRowEntityFromJson` を strict にし、`FText` の export/import を対称にする。`Name` 剥がしの条件を直す。
3. `ImportFromDataTableJSON` をローカル配列 + 重複/不正 PrimaryKey 検出 + `SetRows` 1 回に書き換える。
   `:384-408` を `FInstancedStruct` に置換する。
4. 形式判定の JSON パース結果を本 import に渡す (2 回目のパースを消す)。
5. 3 呼び手 (`:252`, `:469`, `EditorFunctionLibrary.cpp:44`) を共通の `ImportWithTransaction(...)` にまとめ、失敗で
   `Transaction.Cancel()` + 理由表示。`ImportRowsFromJSON` の BP シグネチャに `FText& OutFailReason` を足す。
6. `DataIndexerRepositoryFactory.cpp` の `bEditorImport` / `Formats` を外す。`:459` の `LogTemp` を直す。
7. テスト (Tests 参照) を足し、`DataIndexerEd.Serializer` + `DataIndexerEd.EditorFunctionLibrary` を回す。

## Acceptance Criteria
- `rg 'friend class' Plugins/DataIndexer/Source/DataIndexerEd/Public/DataIndexerEditorData.h` が 0 件 (現在 2)。
- `rg 'Transaction.Cancel\(\)' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerJsonSupport.cpp` が 1 件以上 (現在 0)。
- 3 要素目が不正な JSON を `ImportAsJSON` に渡すと `false` + 非空の `OutFailReason`、`EditorRows` は import 前のまま (現在: default 3 行)。
- 同じ PrimaryKey を 2 行持つ DataTable JSON の import が `false` で理由に重複キーを含む (現在: 2 行入る)。
- `NSLOCTEXT` 付き FText を export → import すると namespace/key が保持される (現在: 落ちる)。
- `rg 'LogTemp' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 1)。
- Content Browser の Import ダイアログのフィルタに DataIndexer の json が出ない。

## Tests
- `DataIndexerImportAtomicityTests.cpp` に行を足す: `ImportAsJSON` 途中失敗 → 行数と内容が不変。修正前コードで落ちることを確かめる。
- `DataIndexerSerializerTests.cpp` に行を足す: FText round-trip、`Name` プロパティ持ち struct の round-trip、重複 PrimaryKey 拒否。
- `DataIndexerEditorFunctionLibraryTests.cpp`: `ImportRowsFromJSON` 失敗時に `OutFailReason` 非空。
- quick: `DataIndexerEd.Serializer` + `DataIndexerEd.EditorFunctionLibrary`。

## Verification
- quick の log で `Automation Test Succeeded` を確認。GUI 不要。
- Reimport 失敗時に notification が出ることは log の `LogDataIndexer` 行で確認 (スクリーンショット不要)。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
