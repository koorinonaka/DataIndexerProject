---
status: ready
order: 40
tier: standard
screen: required
agent: any
created_by: task-create
---

# Data View の選択・貼り付け・削除まわりの bug 修正

## Goal
行削除後の選択位置、部分適用される Paste、UI 操作で消える選択、非表示列の暗黙編集、空トランザクション、横スクロール幅の
空白を直し、死んだ列ソート API を消す。

## Context
監査 `.agents/audit-2026-10-26.md` の B-V1, B-V3, B-V4, B-V5, B-V6, B-V7, B-V9, B-V10。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` 相対、行番号は 2026-10-26 時点。

- B-V1: `DataIndexerViewModel_RowOps.cpp:189` `DeleteRow` が `ClearSelection()` → `OnSelectionChanged` →
  `SDataIndexerView.cpp:667-670` `HandleModelSelectionChanged` が `LastSelectionIndex = INDEX_NONE` → その後 `OnRowsRemoved`
  (`SDataIndexerView.cpp:611-632`) が `LastSelectionIndex` を capture → 常に INDEX_NONE → 削除後は行 0 を選択してスクロール。
  修正: `OnRowsRemoved` の引数で削除前の最小可視 index を渡す。再構築前なので `RowsIndexOf(AffectedRows)` がまだ引ける。
- B-V3: `DataIndexerViewModel_Clipboard.cpp:290-307, 432-445` `PasteRow` / `PasteCells` が読み取り専用行をループ内で検出して return。
  先行行は `UpdateRow` 済み、`OnRowsUpdated` 未 broadcast。`PasteCells` は選択範囲を超えて貼る (`:424-426`) ので
  `CanPasteCells` (`:318-334`) の事前検査が届かない行がある。`FillDown` (`DataIndexerViewModel_FillHandle.cpp:486-499`) と同じ
  事前検査ループをトランザクション開始前に。
- B-V4: `SDataIndexerView.cpp:304-318` `OnPreviewMouseButtonDown` が「行の上でない LMB」で `ClearSelection` + `EndRangeSelection`。
  ヘッダのリサイズグリップ・列ヘッダ・検索ボックス・Filters/Reset Layout ボタン・ステータスバーも該当。`:324-343` の
  `OnMouseButtonDown` にも同じスクロールバー判定が重複。`CellsListView->GetPaintSpaceGeometry().IsUnderLocation` でリストビュー領域内に限定し、判定を 1 箇所に。
- B-V5: `DataIndexerViewModel_Clipboard.cpp:262-270` `FScopedTransaction` を `SelectedRows.IsEmpty()` の return より前に生成。
- B-V6: `SDataIndexerViewHeader.cpp:222-228, 400-406` `MinContentWidth` が非表示列も合算 (`IsColumnVisible` 無視)、固定列の非表示も無視。
- B-V7: `DataIndexerViewModel_Visibility.cpp:239-249` + `DataIndexerViewModel.h:155-156` `UpdateSort` / `GetSortMode` は呼び手ゼロ
  (`rg` 2026-10-26 確認、ヘッダは SortMode/OnSort を bind しない)。比較子 (`:137-146`) も `ColumnName` を見ない。
  ユーザー決定 2026-10-26: 削除する。列ソートが要るときは別タスク。
- B-V9: `DataIndexerViewModel_RangeSelection.cpp:177-201` 矩形を `TableColumnOrder` の index 範囲で作るので間の非表示列も選択に入る →
  `CopyCells` が非表示列を出力、`PasteCells` が非表示列ヘッダ一致を要求、`FillDown` が非表示列へ書く。
- B-V10: `DataIndexerViewModel_FillHandle.cpp:99, 151, 261-262` `FillHandleOriginalBounds` が `VisibleRows` index。ドラッグ中に外部変更
  (`OnAssetChanged → Tick → UpdateVisibleRows`) が入ると Commit で別行へ。Begin 時に `FRepositoryKey` でスナップショット、Commit 時に `RowsIndexOf` で解決。

## Scope
- 上の 8 項目。B-V7 は `UpdateSort` / `GetSortMode` / 比較子の `SortContext` 参照を削除。
- B-V3 の事前検査は `FillDown` のループと同形で書く (共通化は t261026-ed01 がやる。ここでは複製で可)。

## Out of Scope
- `SelectionBounds` の Undo 追随 (t261026-21a2)。
- View の差分更新・perf (t261026-bd46, t261026-dd8a)。
- ViewModel の責務整理・重複排除 (t261026-ed01)。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. B-V5 → `PasteRow` の早期 return をトランザクション生成の前へ。
2. B-V3 → `PasteRow` / `PasteCells` にトランザクション前の読み取り専用検査ループを足し、1 行でも拒否なら何もせず通知。
3. B-V1 → `OnRowsRemoved` デリゲートに「削除前の最小可視 index」を足し、`SDataIndexerView::HandleModelRowsRemoved` がそれで次選択を決める。
4. B-V4 → `OnPreviewMouseButtonDown` の ClearSelection 条件をリストビュー領域内 + スクロールバー外に限定。`OnMouseButtonDown` の重複判定を消す。
5. B-V6 → `MinContentWidth` の合算を `IsColumnVisible` と固定列の表示状態で絞る。
6. B-V9 → `BuildCellRect` (名前は実コードに合わせる) で `IsColumnVisible` でない列を除外。`RebuildSelectionBounds` の矩形判定も可視列数で。
7. B-V10 → `FillHandleOriginalBounds` を `TArray<FRepositoryKey>` スナップショットに。
8. B-V7 → 死にコード削除。
9. `DataIndexerEd.ViewModel` を回し、GUI で Verification。

## Acceptance Criteria
- 行 3 を選択して Delete → 行 3 (旧 4) が選択されている (現在: 行 0)。
- 読み取り専用行 (継承行) を含む範囲へ Paste → どの行も変わらず通知が出る (現在: 先行行だけ変わる)。
- 列ヘッダの境界をドラッグして幅を変えてもセル選択が残る (現在: 消える)。
- 選択なしで Ctrl+V → Undo 履歴に "Paste Row" が積まれない (現在: 積まれる)。
- 列を 2 本非表示にすると横スクロールの最大幅がその分縮む (現在: 空白が残る)。
- 非表示列をまたぐ矩形選択を Ctrl+C した CSV に非表示列が含まれない (現在: 含まれる)。
- `rg 'UpdateSort|GetSortMode' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 4)。

## Tests
- `DataIndexerViewModelTests.cpp` に行: (a) 読み取り専用行を含む `PasteCells` が 0 行変更で `false`、(b) 非表示列をまたぐ矩形選択の
  `CellSelection` に非表示列キーが無い、(c) `DeleteRow` 後に `OnRowsRemoved` が削除前 index を運ぶ。各行、修正前コードで落ちることを確かめる。
- quick: `DataIndexerEd.ViewModel`。

## Verification
- 上の AC 6 項目をエディタで操作し、各 1 枚スクリーンショット (削除後の選択行、Paste 後の表、リサイズ後の選択、Undo 履歴、横スクロール端、貼り付け CSV)。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
