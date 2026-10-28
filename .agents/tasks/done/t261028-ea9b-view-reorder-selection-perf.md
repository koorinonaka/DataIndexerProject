---
status: done
order: 200
tier: standard
screen: none
agent: any
created_by: task-create
---

# Data View: 列並べ替えで RowName 列幅を測り直さない・行文字色の選択判定を O(1) にする

## Goal
1. 列の並べ替えで、全行の DisplayName を FontMeasure で測る処理が走らない。
2. `SViewRow::GetRowTextColor` (セルごと毎フレーム) が行選択の要素数に比例する走査をしない。

## Context
- 由来: root/t261026-dd8a (Execution Log 133 行「bd46 からの引き継ぎ」) と root/t261026-bd46 (Execution Log「既知の残り」)。
  どちらも verify_pending。bd46 の受け入れ記述「並べ替え経路に FontMeasure なし」は、下の (a) のため実際には偽。
- (a) 現状 (2026-10-28 時点。行番号がずれていたら関数名で探す):
  - `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.cpp` `SViewHeader::BuildColumns` (216 行付近) の先頭:
    `!bDisplayNameColumnWidthResized` なら `ViewModel->GetRowEntries()` の全行の `DisplayName` を測り `DisplayNameColumnWidth` を決める。
  - `BuildColumns` の呼び手は 3 つだけ (`rg 'BuildColumns\(' Views/` で確認):
    `SDataIndexerView.cpp` `SView::RepopulateEntries` の first/SetChanged 分岐 (557 行付近、直前に全セルを測る)、
    同関数の OrderChanged 分岐 (562 行付近)、`SView::HandleColumnReordered` (1001 行付近)。
  - よって行名の変更だけでは今も RowName 幅は測り直されない。測り直しを first/SetChanged 分岐に限っても、失う挙動は無い。
- (b) 現状:
  - `Views/SDataIndexerViewRow.cpp` `SViewRow::GetRowTextColor` (245 行付近) が
    `Algo::AnyOf( ViewModel->GetRowSelection(), ... ThisKey.PrimaryKey == RepositoryKey.PrimaryKey )` を呼ぶ。
    `SViewCell` (`SDataIndexerViewCell.cpp:139`) がセルの文字色としてこれを毎フレーム呼ぶ。
  - `Views/DataIndexerViewTypes.h` `FSelectionState` は private メンバを `MakeRowSelection` / `MakeCellSelection` / `Empty()` でしか作らない
    不変値。選択が変わるたびに新しいインスタンスが入る (`DataIndexerViewModel_Selection.cpp` `SetSelection`)。
  - `TSet<FPrimaryKey>` は既に使われている (`DataIndexerEditorData.cpp` `DeleteRows`)。ハッシュはある。
- 判断 (Advisor 判定、2026-10-28):
  - (a) 計測ループを `BuildColumns` から `SViewHeader::MeasureDisplayNameColumnWidth()` (public) へ出し、first/SetChanged 分岐だけが
    `BuildColumns` の前に呼ぶ。行編集での測り直しは今も無いので範囲外。
  - (b) `FSelectionState::MakeRowSelection` で `TSet<FPrimaryKey> RowPrimaryKeys` を作って持つ。不変値なので無効化は要らない。
  - (a) と (b) は同じ Views の perf で、検証も同じ構造確認なので1本にした。
- perf の受け入れは構造で書く (tasks README「証跡」)。
- 重複チェック通過 (20件対象)。

## Scope
- (a) `SDataIndexerViewHeader.h/.cpp`、`SDataIndexerView.cpp`。
- (b) `DataIndexerViewTypes.h/.cpp`、`SDataIndexerViewRow.cpp`。

## Out of Scope
- 行名変更時に RowName 幅を測り直すこと (今も無い挙動)。
- `bDisplayNameColumnWidthResized` の意味 (手で幅を変えたら自動計測しない) は変えない。
- `IsSelected` など他の選択判定。

## Preconditions
- 木に `SViewHeader::BuildColumns` の DisplayName 計測ループと `FSelectionState::MakeRowSelection` があること。

## Steps
1. (a) `SDataIndexerViewHeader.h` に `void MeasureDisplayNameColumnWidth();` (public) を足す。`BuildColumns` 先頭の
   `if ( !bDisplayNameColumnWidthResized ) { ... }` ブロックをそのまま新関数へ移す。
2. (a) `SDataIndexerView.cpp` `RepopulateEntries` の first/SetChanged 分岐で、`ViewHeader->BuildColumns()` の直前に
   `ViewHeader->MeasureDisplayNameColumnWidth()` を呼ぶ。OrderChanged 分岐と `HandleColumnReordered` は変えない。
3. (b) `DataIndexerViewTypes.h` `FSelectionState` に private `TSet<FPrimaryKey> RowPrimaryKeys` と
   public `bool ContainsPrimaryKey( const FPrimaryKey& Key ) const { return RowPrimaryKeys.Contains( Key ); }` を足す。
   `DataIndexerViewTypes.cpp` `MakeRowSelection` で `Rows` の各 `PrimaryKey` を入れる。
4. (b) `SDataIndexerViewRow.cpp` `GetRowTextColor` の `Algo::AnyOf(...)` を `ViewModel->GetSelection().ContainsPrimaryKey( RepositoryKey.PrimaryKey )` に置き換える。
   ほかに使い手が無ければ `#include "Algo/AnyOf.h"` を消す。

## Acceptance Criteria
- `rg 'FontMeasure' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.cpp` の一致が
  `BuildColumns` の本体に無い (現在 1 箇所ある)。計測は `MeasureDisplayNameColumnWidth` にだけある。
- `MeasureDisplayNameColumnWidth` の呼び手が `SView::RepopulateEntries` の first/SetChanged 分岐だけ (現在 関数自体が 0 件)。
- `rg 'Algo::AnyOf' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewRow.cpp` が 0 件 (現在 1)。
- `rg 'ContainsPrimaryKey' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` が 2 件以上 (宣言・GetRowTextColor の呼び出し。現在 0)。

## Tests
- 候補: `Private/Tests/ViewModel/DataIndexerViewModelTests.cpp` の `DataIndexerEd.ViewModel.Selection.*` に、別 Repository の同じ PrimaryKey を持つキーで行選択したとき
  `ContainsPrimaryKey` が真、セル選択・空選択では偽、を確かめる行を足す。足す前に Tests 4 問に答え、修正前のコードで落ちることを確かめる。
- (a) は Slate のフォント計測に依存し、自動テストの口が無い。構造の AC で受ける。
- quick: Editor ターゲットのビルド + `Automation RunTests DataIndexerEd.View` と `DataIndexerEd.ViewModel`。

## Verification
- 上の AC を `rg` の出力で証跡に残す。計測値は任意 (`TRACE_CPUPROFILER_EVENT_SCOPE` を付けられれば添える)。

## Execution Log
- 2026-10-28 (dispatch 21/21、ブランチ `fix/audit-2026-10` 両 repo)。開始時 両 repo とも clean。前提の計測ループと `MakeRowSelection` は木にあった (ed01 後も関数名どおり、行番号もほぼ同じ)。
- (a) `SViewHeader::BuildColumns` 先頭の `if ( !bDisplayNameColumnWidthResized ) {...}` を `MeasureDisplayNameColumnWidth()` (public) へそのまま移した。`SView::RepopulateEntries` の first/SetChanged 分岐だけが `BuildColumns()` の直前に呼ぶ。OrderChanged 分岐と `HandleColumnReordered` は変えていない。
- (b) `FSelectionState` に private `TSet<FPrimaryKey> RowPrimaryKeys` と public `ContainsPrimaryKey()` を足し、`MakeRowSelection` で埋める。`SViewRow::GetRowTextColor` の `Algo::AnyOf` を置き換え、使い手が無くなった `#include "Algo/AnyOf.h"` を消した。
- テスト (Tests 4 問: 守る契約 = 行選択の PrimaryKey 判定は Repository を問わず真・セル/空選択は偽、並べ替え経路は RowName 幅を測り直さない。回帰 = `RowPrimaryKeys` を埋め忘れる / 計測を `BuildColumns` に戻す。既存テストはどちらも見ていない。本番に無い口は要らない — 両関数とも本番が呼ぶ):
  - `DataIndexerEd.ViewModel.Selection.Lifecycle` に `ContainsPrimaryKey` の真1・偽3 を追加。
  - `DataIndexerEd.View.RowNameColumn.AutoWidthAndManualResize` は `BuildColumns` だけで自動幅を期待していたため、分離後に落ちた (期待値 >200 が 200)。期待値は変えず、呼び順を本番の first/SetChanged 分岐と同じ `MeasureDisplayNameColumnWidth()` + `BuildColumns()` に直した (3 箇所)。そのうえで「`BuildColumns` 単独では 200 のまま」の assert を 1 行足した。手動幅後の再構築も Measure 経由にしたので、`bDisplayNameColumnWidthResized` を尊重することも見ている。
- 赤の確認 (hollow):
  - `MakeRowSelection` の `RowPrimaryKeys.Add` を空にしたビルド → `Selection.Lifecycle` が `Expected 'Row selection contains its PrimaryKey' to be true` の 1 件だけで Fail (Selection 群の他 2 本は Success)。
  - `BuildColumns` 先頭に `MeasureDisplayNameColumnWidth()` を戻したビルド → `RowNameColumn.AutoWidthAndManualResize` が `Expected 'Column reorder (BuildColumns alone) does not re-measure RowName' to be 200.000000, but it was 466.000000` の 1 件だけで Fail。
  - どちらも元に戻して再ビルド (`rg HOLLOW` 0 件)。
- quick: `Build.bat DataIndexerProjectEditor Win64 Development` Succeeded (C1001 内部コンパイラエラー 1 回 → 再実行で成功、`DataIndexerNotOverridableTests.cpp(265)` で本タスク無関係)。`Automation RunTests DataIndexerEd.View+DataIndexerEd.ViewModel` → Found 15、Success 15 / Fail 0。full は run 終端。
- AC の証跡 (`rg`):
  - AC1: `SDataIndexerViewHeader.cpp` の `FontMeasure` は 170/186 (`ComputeLineNumberWidth`、行番号列の 1 文字列だけ。既存) と 220/226 (`MeasureDisplayNameColumnWidth`)。`BuildColumns` (231 行〜) の本体には 0 件。DisplayName の計測は `MeasureDisplayNameColumnWidth` にだけある。
  - AC2: 本番の呼び手は `SDataIndexerView.cpp:557` (first/SetChanged 分岐) の 1 件だけ。ほかはテスト 3 件 (`DataIndexerViewModelTests.cpp:84/101/116`)。
  - AC3: `rg 'Algo::AnyOf' SDataIndexerViewRow.cpp` 0 件 (rc=1)。
  - AC4: `ContainsPrimaryKey` 2 件 (`DataIndexerViewTypes.h:159` 宣言、`SDataIndexerViewRow.cpp:253` 呼び出し)。
- 自己レビュー: `git diff --stat` は Scope の 6 ファイル + テスト 1 ファイル、計 +32/-4。テストファイルは Scope 外だが、Tests 節が名指ししているファイルで、(a) の API 分離に呼び順を合わせる変更 (期待値は不変)。
- 計測値は付けていない (README「証跡」: perf は構造で受ける)。
- critic r1 (task-critic):
  ```
  PASS
  NOTE: DataIndexerViewModelTests.cpp:84 — AC2 says MeasureDisplayNameColumnWidth must be called only from the first/SetChanged branch. The only production caller is that branch (SDataIndexerView.cpp:557). The other 3 callers are tests (lines 84/101/116), so I read AC2 as met.
  NOTE: DataIndexerViewModelTests.cpp:721 — This test calls BuildColumns() alone. It only checks column order and selection, not RowName width, so the split does not break it.
  NOTE: DataIndexerViewModelTests.cpp — This file is outside Scope, but Out of Scope does not list it and the Tests section names it, so it is not a FAIL.
  NOTE: Removed-name sweep: Algo::AnyOf and Algo/AnyOf.h have 0 hits in SDataIndexerViewRow.cpp. GetRowSelection still has many other callers, so nothing became unreferenced. RowSelection is set only in MakeRowSelection (DataIndexerViewTypes.cpp:22), so RowPrimaryKeys stays in sync.
  NOTE: The performance gain can only be measured at runtime. The diff shows the structure only, which is what the task asks for.
  ```

## Summary
列の並べ替え (`HandleColumnReordered` / OrderChanged) は RowName 幅を測らなくなった。全行の DisplayName 計測は `SViewHeader::MeasureDisplayNameColumnWidth()` に移り、first/SetChanged 分岐だけが呼ぶ。`GetRowTextColor` の上書き先マークは `FSelectionState::ContainsPrimaryKey` (TSet 引き) で O(1) になった。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerView.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewTypes.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewTypes.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewRow.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/ViewModel/DataIndexerViewModelTests.cpp`

## Blockers
None
