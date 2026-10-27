---
status: ready
order: 160
tier: standard
screen: required
agent: any
created_by: task-create
---

# Views の責務整理・重複排除・死にコード削除

## Goal
ViewModel 7 分割の責務漏れ、セル値 Export/Import の 4 重、ヘッダの列走査 3 重、`ResolveContainerMemory` 3 重、DragDrop の コピペ、
死にコードを消す。挙動は変えない。

## Context
監査 `.agents/audit-2026-10-26.md` の R-V1〜R-V13。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` 相対、行番号は 2026-10-26 時点 (先行 3 タスクでずれる。関数名で探す)。

- R-V1: `DataIndexerViewModel_Selection.cpp:339-438,446-517` / `DataIndexerViewModel_Visibility.cpp:7-61` / `_Selection.cpp:118-149` /
  `_Clipboard.cpp:312,529-530` / `_FillHandle.cpp:313-315` 責務漏れ。`GetFillHandleCellKey` / `GetFillHandleColumnRange` / `IsMouseOverSelectedRow` /
  `IsRowInAnySelection` → `_FillHandle.cpp`。`GetRootDefinitionOrder` / `SourceRowHasAnyFlag` / `HasLocalRow` / `IsOverridingParentRow` →
  `DataIndexerViewModel.cpp`。`GetColumnEntryIndex` / `SetTableColumnOrder` / `GetTableColumnIndex` → 列関数の隣。Clipboard が `RangeSelectMode` /
  `SelectionAnchor` を直接書き (`PreDragSelection` / `bCellSelectMode` は未リセット)、FillHandle は `BeginCellSelection → Extend → End` で偽ドラッグ →
  `_RangeSelection.cpp` に `SelectCellRect(TopLeft, BottomRight)` を 1 つ置き `PasteCells` / `PasteRow` / `CommitFillHandleDrag` から呼ぶ。
- R-V2: `_Clipboard.cpp:175-188,468-492` / `_FillHandle.cpp:460-483,534-563` 「列の PropertyPath から行メモリの葉値ポインタを解決して Export/Import」が 4 重。
  `FillDown` / `FillDownRows` の事前検査+適用 loop 同形 (`:486-499 ≡ :626-639`, `:504-516 ≡ :644-656`)。t261026-57d1 が Paste に足した検査ループも同形。
  `ShowFillNotification` (`_FillHandle.cpp:16-25`) ≡ `ShowPasteNotification` (`_Clipboard.cpp:57-66`)。
  `ResolveCellValue(const FColumnEntry&, uint8* RowMemory)` + Export/Import 2 関数 + 通知 1 関数。
- R-V3: `SDataIndexerViewHeader.cpp:66-133, 529-575, 588-645` 可視列の X/幅走査が 3 重。`UpdateLineNumberWidth` `:222-228` ≡ `BuildColumns` `:400-406`。
  固定幅 16/18/26 がリテラルで 2 重 + `AddColumn`。`GetColumnRangeCenterXLocal` (`:577-586`) 未使用 (`rg` 2026-10-26: 宣言+定義のみ)。
  `ForEachVisibleColumnLayout(TotalWidth, TFunctionRef<bool(const FColumn&, float X, float W)>)` 1 本。
- R-V4: `DataIndexerViewModel.h:300-325` / `DataIndexerViewTypes.cpp:68-94` / `SDataIndexerViewRow.cpp:339-352` `ResolveContainerMemory` 3 重。テンプレート 1 本に。
- R-V5: `SDataIndexerViewRow.cpp:26-45` ≡ `SDataIndexerViewCell.cpp:33-52` `FStructViewDataProvider` / `FCellStructViewDataProvider` 同一。
- R-V6: `SDataIndexerViewRowMoveDragDrop.cpp:15-66,114-117` ≡ `SDataIndexerViewColumnMoveDragDrop.cpp:13-54,66-69` Op + Handle コピペ。
  `FDecoratedDragDropOp` は `CurrentHoverText` / `CurrentIconBrush` で同じ見た目を標準で出す (エンジン側の既定描画は未実測 → 実装者が確認し、
  見た目が変わるなら自前 decorator を残して Handle だけ共通化)。
- R-V7: `SDataIndexerViewCell.cpp:563-632` エッジ描画が色違いで 2 重、`OutlineOutset = 0` の死んだ算術。
- R-V8: `DataIndexerViewModel_RangeSelection.cpp:104-115 ≡ 154-165`, `39-47/68-76/120-131/206-217` 行範囲収集 2 重、XOR トグル 4 重。
- R-V9: 死にコード — `_Selection.cpp:251-337` `GetFillHandleFlags` + `EFillHandleFlags` (`.h:95-100,103,296`)、`_Selection.cpp:51-71` `ToggleSelection`、
  `.h:142` `GetSelectionAnchor`、`SDataIndexerViewHeader.cpp:34-37` no-op `OnMouseMove`、`SDataIndexerViewRow.cpp:244-250` 素通し `OnPaint`、
  `SDataIndexerViewCell.cpp:275-297` RMB 3 分岐同一、`SDataIndexerViewRow.cpp:613-622` 両分岐同じ、`SDataIndexerView.cpp:778` 常 true、
  `DataIndexerViewTypes.cpp:229-232` Contains 二重、`DataIndexerViewModel.cpp:134` 常 true (`rg` 2026-10-26 で未使用を確認済み)。
- R-V10: `_Clipboard.cpp:72,82,201,214` `HasSelection() && GetRowSelection().IsEmpty()` 4 回 = `Selection.IsCellSelection()`。
- R-V11: `SDataIndexerView.cpp:852,862,871,904,927,944` `FCanExecuteAction::CreateLambda([]{return true;})` 6 回。未バインドで実行可、削除。
- R-V12: `SDataIndexerView.cpp:879-901` 定数列名 2 個のための IIFE → `FName::NameToDisplayString` か LOCTEXT 2 個。
- R-V13: `SDataIndexerDictionaryComboBox.cpp:27` `// clang-format off` が `on` の誤記 (`:13 off, :27 off, :70 off, :75 on`)。

## Scope
R-V1〜R-V13。

## Out of Scope
- 挙動変更。
- 差分更新・perf (先行タスク)。

## Preconditions
- t261026-57d1, t261026-bd46, t261026-dd8a が done (同じファイル群)。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. R-V9 / R-V11 / R-V13 / R-V10 → 削除・置換。
2. R-V4 / R-V5 → テンプレートと provider の統合。
3. R-V2 → `ResolveCellValue` + Export/Import + 通知。Clipboard / FillHandle / Paste 検査ループを置換。
4. R-V1 → 関数を所属ファイルへ移動、`SelectCellRect` を新設して 3 呼び手を置換。`PreDragSelection` / `bCellSelectMode` のリセット漏れもここで直す。
5. R-V8 → `CollectVisibleRows` + `ToggleInPlace`。
6. R-V3 → `ForEachVisibleColumnLayout`、定数化、未使用削除。
7. R-V6 → `FDecoratedDragDropOp` の既定描画を実測し、Handle 基底 + `MakeDragOp()`。
8. R-V7 / R-V12。
9. full を回し、GUI で smoke (選択・貼り付け・フィル・列ドラッグ・行ドラッグ)。

## Acceptance Criteria
- `rg 'GetFillHandleFlags|EFillHandleFlags|GetColumnRangeCenterXLocal|CreateLambda\(\s*\[\]\s*\{\s*return true' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` が 0 件 (現在 1 件以上ずつ)。
- `rg 'ResolveContainerMemory' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` の定義が 1 件 (現在 3)。
- `rg 'StructViewDataProvider' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` のクラス定義が 1 件 (現在 2)。
- `rg 'ExportTextItem|ImportTextItem' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel_{Clipboard,FillHandle}.cpp` が
  `ResolveCellValue` 経由の 2 関数内だけ (現在 4 箇所)。
- `rg 'SelectCellRect'` が 1 定義 + 3 呼び手。
- `SDataIndexerDictionaryComboBox.cpp` の `clang-format off/on` が対になる。
- 全自動テスト (`DataIndexerEd` 全群) が通る。

## Tests
- 新規テスト行なし (挙動不変)。既存 `ViewModel` 8 件 + `View` 1 件が回帰を取る。
- full: `DataIndexerEd` 全群。

## Verification
- smoke 5 操作のスクリーンショット (選択矩形、貼り付け後、フィル後、列ドラッグ中のデコレータ、行ドラッグ中のデコレータ)。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
