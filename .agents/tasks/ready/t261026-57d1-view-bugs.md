---
status: verify_pending
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
- 2026-10-28 着手。前提: `release/v1.0.2` は main にマージ済み (run 開始時に確認済み、作業ブランチ `fix/audit-2026-10` は両 repo とも main から)。git status は両 repo とも clean だった。
- B-V5: `PasteRow` の選択なし return を clipboard 解析と `FScopedTransaction` の前へ移した。
  - AC 4 は修正前から成り立っていた。エンジンが空のトランザクションを捨てるから (`FTransaction::IsTransient` が変更なしを transient と判定し、`UTransBuffer::End` が undo buffer から外す。`EditorTransaction.cpp:515` / `:1333`)。代わりに主張できる強い等価: `PasteRow` では return が `FScopedTransaction` の生成より前にある (コードで確認)。この AC の回帰テストは足していない (修正前も通るので何も証明しない)。
- B-V3: `PasteRow` は書き込み先を `Targets` に集め、トランザクション前に `CanEditRow` で全件検査する。`PasteCells` は `FillDown` と同形の事前検査ループをトランザクション前に置いた。1 行でも読み取り専用なら通知して何も変えない。
- B-V1: `FRowsRemovedDelegate` を TwoParams (`AffectedRows`, `MinIndexBeforeRemove`) にした。`DeleteRow` は `ClearSelection` の前に削除行の最小 `RowsIndexOf` を取る。`SView::HandleModelRowsRemoved` はその index で次の選択を決める。読み手が消えた `SView::LastSelectionIndex` は削除した。束縛側 `DataIndexerEditorToolkit.cpp:267` の lambda も引数を足した (同じモジュール。シグネチャ変更に伴う機械的な修正。`Views/` の外だが Scope の B-V1 に必要)。`rg GetOnRowsRemoved` で sample `Source/` と `Plugins/DataIndexerToolset` に束縛が無いことを確認した。
- B-V4: 空き領域判定を `SView::IsOverEmptyListArea` 1 箇所にまとめた。条件は「`CellsListView` の上」かつ「`ViewHeader` の上でない」かつ「行の上でない」。ヘッダ行は `STableViewBase` が list view の子として持つので、list view の geometry に含まれる。そのためヘッダを明示的に除外した。スクロールバーは list view の外にあるので判定から外した。`OnMouseButtonDown` の override (重複判定) は削除した。
  - 既定 (可逆): Shift/Ctrl+クリックで空き領域を押しても選択は消えない。以前は bubble 経路の `OnMouseButtonDown` が修飾キーを見ずに消していた。preview 経路は元から plain LMB だけを対象にしている。戻すには `OnMouseButtonDown` を復活させる (数行)。
- B-V6: `SViewHeader::RecomputeMinContentWidth` 1 箇所で `GetColumns()` を走査し、`bIsVisible` の列 (固定列・データ列とも) の `GetWidth()` を足す。`Column::Blank` の width は fill 係数なので除く。`UpdateLineNumberWidth` と `BuildColumns` の重複した計算をこれに置き換えた。`FColumn::bIsVisible` / `GetWidth()` は UE 5.7 にもある (`SHeaderRow.h:249, 304`)。
- B-V9: `FViewModel::GetVisibleColumnIndices(Min, Max)` を足した。適用箇所: `ExtendRangeSelection` の矩形 (非表示列を飛ばす)、`RebuildSelectionBounds` の矩形判定 (可視列数で数える)、`CopyCells` の列、`PasteCells` の列対応 (clipboard の列を Min.X 以降の可視列へ順に対応させる)、`FillDown` の読み書き 2 ループ、Fill プレビュー、Fill ハンドルの中央列 2 箇所 (可視列の中央)、`GetSelectionEdgeFlags` の左右の隣 (次の可視列)。中央列と隣接は見た目のための修正。直さないと、非表示列が中央のときハンドルが消え、非表示列の位置に選択の縦線が出る。
  - UE 5.7: `GetScreenSpacePosition` / `IsUnderLocation` の型は 5.7 と同じ (`Events.h:962`、`Geometry.h:421`)。
- B-V10: `FillHandleOriginalBounds` (`FIntRect`) を廃止した。Begin 時に、矩形選択でも行選択でも `FillHandleOriginalRows` (行キーの snapshot) を作る。列範囲は `FillHandleOriginalColumnRange` (`FIntPoint`) に分けた。Extend と Commit は行範囲を `RowsIndexOf` で解決する。
- B-V7: `UpdateSort` / `GetSortMode` / `FSortContext` と比較子の Descending 分岐を削除した (ユーザー決定 2026-10-26)。`FSelectionBoundsFollowRowOrderTest` (t261026-21a2 が足したテスト) は並べ替えに `UpdateSort` を使っていた。並べ替えを `MoveRow({First}, 2)` + `RefreshRows()` に替えた。同じ `UpdateVisibleRows → RebuildSelectionBounds` 経路を通る。旧位置にある行が Last から Middle に変わるので、最後の assert の対象を Middle にした。期待値の変更ではなく、並べ替え手段の変更に伴う対象行の変更。
- テスト (`DataIndexerViewModelTests.cpp`)。Tests 4 問の答え:
  - (a) `Clipboard.PasteCellsRejectsReadOnlyRows`。守る振る舞い: 読み取り専用行を含む Paste は 0 行を変える。回帰の例: 検査が書き込みループ内に戻ると落ちる。既存テストに Paste のものは無い。制御: 同じ clipboard 経路で全行編集可の範囲には書ける (10 になる)。順序の制御: 編集可の行の直下に読み取り専用行がある。Task の「`false` を返す」は採らなかった。`PasteCells` は void で、`FExecuteAction` として束縛されている。代わりに行の値が変わらないことを assert する。
  - (b) `Selection.RangeSkipsHiddenColumns`。仮想列 schema の隣接 3 列のうち中央を非表示にして、両端をドラッグする。非表示列が選択に無いこと、矩形として扱われること、`CopyCells` のヘッダが可視 2 列だけであることを assert する (AC 6 を兼ねる)。`SetColumnVisible` は editor 設定を `SaveConfig` するので、最後に `ResetSchemaLayout` で戻す。clipboard は `FScopedClipboardRestore` で保存して戻す。
  - (c) `RowOps.DeleteRowReportsIndexBeforeRemove`。3 行の中央を `DeleteRow` すると `OnRowsRemoved` が 1 を運ぶ。親コミットではデリゲートの引数の数が違うのでコンパイルできない。修正前の red は取れない。
- 修正前 red (B-V3/B-V9 を入れる前の build、他の修正とテストは入れた状態):
  - (a) Fail: `Expected 'Editable row before the read-only row is unchanged' to be 3, but it was 30`。狙った理由で落ちた。
  - (b) 初回は fixture の不備で落ちた (`FDataIndexerNotOverridableRow` + `UDataIndexerSchemaTests` ではデータ列が 3 本できない)。仮想列 schema に替えて再 build した。その結果 Fail: `Expected 'Hidden column is not selected' to be false` と `Copied header has the visible columns only: The two values are not equal`。矩形の assert は修正前も通る (列が連続しているため)。
- ビルド: `Build.bat DataIndexerProjectEditor Win64 Development` を 4 回実行した。1 回目は C1001 (`UnrealType.h(2000)`) で落ちた。run notes どおり再実行して成功した (22.45s)。fixture 修正後に 22.6s、全修正後に 22.4s。Game ターゲットは build していない (runtime 構造体と `WITH_EDITORONLY_DATA` に触っていない)。
- quick: `Automation RunTests DataIndexerEd` (全群) → 67 Success / 0 Fail (`Saved/Logs/DataIndexerProject.log`、`TEST COMPLETE. EXIT CODE: 0`)。前タスク後は 64 本で、この task が 3 本足した。full は run 終端で実行する。
- AC 7: `rg 'UpdateSort|GetSortMode' Plugins/DataIndexer/Source/DataIndexerEd/` → 0 件 (rg exit 1)。
- GUI Verification は未実施。UE MCP (127.0.0.1:8001) がこの run では落ちていて、エディタ操作とスクリーンショットができない。
- diff の自己 review (stat): submodule の 13 ファイル、+381/-177。`Views/` 以外は `DataIndexerEditorToolkit.cpp` (1 行、B-V1 の束縛) とテスト 1 ファイルだけ。どちらも上に記録した。
- critic r1 (task-critic):
  ```
  PASS
  NOTE: t261026-57d1-view-bugs.md:65-69 — AC 1-5 only show on screen (selection after Delete, Paste result, header resize, Undo history, horizontal scroll edge). The diff makes them plausible: `MinIndexBeforeRemove` is taken before `ClearSelection`, both pastes check read-only rows before the transaction, `IsOverEmptyListArea` excludes `ViewHeader`, `PasteRow` returns before `FScopedTransaction`, and `RecomputeMinContentWidth` counts only `bIsVisible` columns. The Verification screenshots are still missing.
  NOTE: DataIndexerViewModelTests.cpp:481-490 — The diff edits `FSelectionBoundsFollowRowOrderTest`, a test from t261026-21a2. The author swapped the reorder method `UpdateSort` for `MoveRow` because B-V7 deletes `UpdateSort`. The asserted behaviour stays the same. Selection-bounds Undo behaviour (Out of Scope) is not touched.
  NOTE: SDataIndexerView.cpp:286 — With `OnMouseButtonDown` removed, a Shift/Ctrl+LMB click in the empty list area no longer clears the selection. Step 4 asks to remove that duplicate handler, and the Execution Log records the effect, so this is not a FAIL.
  NOTE: SDataIndexerViewHeader.cpp:184-187 — A schema column resize (`HandleColumnWidthChanged`) still does not recompute `MinContentWidth`. The pre-diff code behaved the same way, so the diff did not cause it.
  NOTE: Sweeps found nothing. These removed names have no references left outside `.agents` docs: `UpdateSort`, `GetSortMode`, `SortContext`, `LastSelectionIndex`, `FillHandleOriginalBounds` and the `OnMouseButtonDown` override. `HorizontalScrollBar` and `VerticalScrollBar` are still used at `SDataIndexerView.cpp:40-198`. Both `GetOnRowsRemoved` binders were updated (`DataIndexerEditorToolkit.cpp:267`, `SDataIndexerView.cpp:36`). Every new definition has a caller: `GetVisibleColumnIndices`, `IsOverEmptyListArea`, `RecomputeMinContentWidth`, `FScopedClipboardRestore`, `GetAutomationRowValue`.
  ```

## Summary
Data View の 8 件 (B-V1, B-V3, B-V4, B-V5, B-V6, B-V7, B-V9, B-V10) を修正した。
- 削除後の選択: 削除前の最小 index で決める。
- Paste: 読み取り専用行を含むときは何も書かない。
- 空き領域クリック: 判定は 1 箇所。ヘッダ・ツールバー・ステータスバーのクリックでは選択が消えない。
- 横スクロール幅: 非表示列を数えない。
- 矩形選択・Copy・Paste・FillDown: 非表示列を含めない。
- Fill ドラッグ: 元の行をキーで持つ。
- 列ソート API: 削除した。
自動テスト 3 本を足し、DataIndexerEd 全 67 本が通る。GUI での確認 (AC 1〜6 のスクリーンショット) は残っている。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel_Clipboard.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel_FillHandle.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel_RangeSelection.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel_RowOps.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel_Selection.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel_Visibility.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerView.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerView.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/ViewModel/DataIndexerViewModelTests.cpp`
- `.agents/tasks/ready/t261026-57d1-view-bugs.md`

## Blockers
検証待ち。理由: UE MCP server (127.0.0.1:8001) がこの run では接続できない (ECONNREFUSED)。エディタの GUI 操作とスクリーンショットが撮れない。残りは Verification の 6 枚だけ。AC 4 は上記のとおりエンジン側で成り立っているので、Undo 履歴のスクリーンショットは確認用。
