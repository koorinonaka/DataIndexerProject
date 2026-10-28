---
status: ready
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
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし
