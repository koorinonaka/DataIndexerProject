---
status: done
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
- `rg 'SelectCellRect'` が 1 定義 + 2 呼び手 (`PasteCells`, `CommitFillHandleDrag`)。Clipboard/FillHandle は範囲選択の状態を直接書かない。
- `SDataIndexerDictionaryComboBox.cpp` の `clang-format off/on` が対になる。
- 全自動テスト (`DataIndexerEd` 全群) が通る。

## Tests
- 新規テスト行なし (挙動不変)。既存 `ViewModel` 8 件 + `View` 1 件が回帰を取る。
- full: `DataIndexerEd` 全群。

実行結果 (2026-10-28):
- `Build.bat DataIndexerProjectEditor Win64 Development`: 1 回目は `fatal error C1001` (`SharedPointerInternals.h(478)`, `Module.DataIndexerEd.cpp`, 18 秒)。run notes どおり一過性として再実行した。2 回目は `Result: Succeeded` (22 秒)。
- `Automation RunTests DataIndexerEd`: `Result={Success}` 75 件、`Result={Fail` 0 件、`TEST COMPLETE. EXIT CODE: 0`。
  - うち `DataIndexerEd.ViewModel.*` 14 件、`DataIndexerEd.View.*` 1 件。すべて Success (起票時の「8 件」より増えている)。
  - テスト開始前の `LogAutomationTest: Error: Condition failed` 19 行は、エンジン起動時の `UnifiedErrorTest` 自己診断の出力。本タスクとは無関係。
- full は run 終端で orchestrator が回す。ここでも全群 75 件を回した。

## Verification
残り (GUI。UE MCP `127.0.0.1:8001` が本 run で停止中のため未実施):
- smoke 5 操作のスクリーンショット (選択矩形、貼り付け後、フィル後、列ドラッグ中のデコレータ、行ドラッグ中のデコレータ)。
- 重点 1: 貼り付け後とフィル後の選択矩形が以前と同じ範囲か (`SelectCellRect` 経由に変更)。
- 重点 2: ドラッグデコレータの見た目 (`FTextDragDropOp` に統合。行デコレータは旧 `SHorizontalBox` 1 スロットの包みを外した)。
- 済 (2026-10-28 verify sweep、ALICE): smoke 5 操作と重点 1・2。証跡 `.agents/tasks/evidence/t261026-ed01/`。

## Execution Log
- 2026-10-28 着手。前提 t261026-57d1 / bd46 / dd8a は `verify_pending` (検証未完) の上に構築した。作業ブランチは両 repo とも `fix/audit-2026-10`。
- R-V9 死にコードを削除した:
  - `ToggleSelection`、`GetFillHandleFlags` + `EFillHandleFlags`、`GetSelectionAnchor`。
  - `SViewHeader::OnMouseMove` (no-op。基底 `SHeaderRow` は override しておらず、既定も Unhandled)。
  - `SViewRow::OnPaint` (基底への素通し)。
  - `SViewCell` RMB の同一 3 分岐を 1 つに。
  - `HandleRowDragEnter` の両分岐を `ClearDropInsertZone()` 1 行に。
  - `TryGetDropBoundaryLineY` の常 true 条件 `!PrevRowBottomY.IsSet()`。
  - `BuildColumnsRecursive` の `VisitedStructs.Contains` 二重。
- R-V9 のうち `DataIndexerViewModel.cpp:134` の常 true (旧 `RefreshRows` の `OldRows != RowEntries`) は、t261026-bd46 が書き換え済みで既に無い。
- R-V11: `FCanExecuteAction::CreateLambda([]{return true;})` 6 箇所を `FCanExecuteAction()` にした。未バインドなら `FUIAction::CanExecute` は true を返す。
- R-V12: 列名整形の IIFE を `LOCTEXT` 2 個にした (`EditorColumn_Visibility` "Visibility"、`EditorColumn_ReferenceViewer` "Reference Viewer")。英語の表示文字列は同じ。
- R-V13: `SDataIndexerDictionaryComboBox.cpp:27` の `off` を `on` にした。並びは 13 off / 27 on / 70 off / 75 on。
- R-V10: `HasSelection() && GetRowSelection().IsEmpty()` 4 箇所を `Selection.IsCellSelection()` にした。
- R-V4: `ResolveContainerMemory` テンプレートを `DataIndexerViewTypes.h` へ移した (ViewTypes.cpp からも使うため)。ViewTypes.cpp の const 版と `SViewRow::GetProviderForColumn` の手書きループを削除した。
- R-V5: `FStructViewDataProvider` を `DataIndexerViewTypes.h` に 1 つ置き、Row と Cell の 2 定義を削除した。
- R-V2:
  - `ResolveCellValue(const FColumnEntry&, T* RowMemory)` を ViewTypes.h に新設した。
  - `ExportCellValue` / `ImportCellValue` / `ShowViewNotification` を ViewModel.cpp に新設した。CopyCells / PasteCells / FillDown がこれを経由する。
  - 行の読取専用検査は `CheckRowsEditable` 1 つにした (PasteCells と Fill Down が使う)。
  - Fill Down の書込みループは `FillDownTargets` 1 つにした。FillDown / FillDownRows はエンティティの生成だけを渡す。
- R-V1 関数の移動:
  - `GetFillHandleCellKey` / `GetFillHandleColumnRange` / `IsMouseOverSelectedRow` / `IsRowInAnySelection` → `_FillHandle.cpp`。
  - `SourceRowHasAnyFlag` / `HasLocalRow` / `IsOverridingParentRow` → `DataIndexerViewModel.cpp` (行関数の隣)。
  - `GetColumnEntryIndex` / `GetVisibleColumnIndices` / `SetTableColumnOrder` / `GetTableColumnIndex` / `GetTableColumnNameAt` → `DataIndexerViewModel.cpp` (列関数の隣)。
  - `GetRootDefinitionOrder` は先行タスクで消えていて、既に存在しない。
- R-V1 `SelectCellRect(TopLeft, BottomRight)` を `_RangeSelection.cpp` に新設した。
  - セル矩形を選び、セルドラッグを離した後と同じ状態にする: `PreDragSelection` 空、`SelectionAnchor = TopLeft`、`RangeSelectMode = Inactive`、`bCellSelectMode = true`。
  - 呼び手は `PasteCells` と `CommitFillHandleDrag` (偽ドラッグ `Begin → Extend → End` を置換)。
  - **AC「`SelectCellRect` が 1 定義 + 3 呼び手」は書かれたとおりには満たさない。結果は 1 定義 + 2 呼び手。** 3 つ目に挙がった `PasteRow` は行選択を作る。単一行のクリップボードでは飛び飛びの選択行にも貼るので、セル矩形では表せない。タスク文の想定違いと判断した。
  - 代わりに確かめた強い同値: `rg 'RangeSelectMode|SelectionAnchor|PreDragSelection' DataIndexerViewModel_{Clipboard,FillHandle}.cpp` が 0 件。Clipboard と FillHandle は範囲選択の状態を直接書かない。
  - `PasteRow` は `EndRangeSelection()` (RangeSelectMode と PreDragSelection を戻す) を呼んでから `SetSelection(行)` する。`bCellSelectMode` は触らない。PasteRow は行選択からしか走らないので、値は元のまま。
  - `PasteCells` の選択範囲: 旧実装は `TargetColumns` (ColumnEntries 上の表示列) で作っていた。`SelectCellRect` は `GetVisibleColumnIndices(先頭列, 末尾列)` で同じ集合を作る。
  - `CommitFillHandleDrag` の選択範囲: 旧実装は `TableColumnOrder` 経由だった。データ列は固定列を挟まず連続するので、同じ集合になる。
- R-V8: `CollectVisibleRows(A, B)` (行範囲の収集。ExtendRangeSelection の 2 箇所と CommitFillHandleDrag が使う) と `ToggleInPlace` テンプレート (XOR 4 箇所) を新設した。
- R-V3:
  - `ForEachVisibleColumnLayout(TotalWidth, TFunctionRef<bool(const FColumn&, float X, float W)>)` を新設した。`OnPaint` / `GetColumnCenterXLocal` / `GetColumnRangeBoundsLocal` の 3 重走査を置換した。
  - 未使用の `GetColumnRangeCenterXLocal` を削除した。
  - 固定幅 16/18/26 の 2 重と `UpdateLineNumberWidth ≡ BuildColumns` は、t261026-57d1 が `RecomputeMinContentWidth` に置換済み。リテラルは `AddColumn` に 1 箇所ずつしか残っていない。1 回しか使わない値なので、定数化は見送った。
- R-V6:
  - エンジンの `FDecoratedDragDropOp::GetDefaultDecorator` を読んだ (`Editor/UnrealEd/Public/DragAndDrop/DecoratedDragDropOp.h`。5.8 と 5.7 で同一)。
  - 既定のデコレータは `SBorder` に `Padding(8)` が無く、アイコン用の `SImage` スロット (右 3px) を持つ。見た目が変わるので、自前のテキストデコレータを残した。
  - 新ヘッダ `SDataIndexerViewDragHandle.h` に 2 つの基底を置いた。`FTextDragDropOp` はデコレータとカーソルを持つ。`SDragHandle` は `OnMouseButtonDown` / `OnDragDetected` / `OnCursorQuery` を持ち、派生は `MakeDragOp()` だけを実装する。
  - 行ハンドルの古いコメント (「OnMouseButtonDown で BeginRowSelection」) を実装に合わせて直した。
- R-V7: `SViewCell::OnPaint` の選択枠とフィルプレビュー枠を `DrawEdges(Edges, Color)` ラムダ 1 本にした。`OutlineOutset = 0` の算術を除いた。
- 軽微 fold (t261026-57d1 由来): `SViewHeader::HandleColumnWidthChanged` は列のドラッグ後に `MinContentWidth` を再計算していなかった。`RecomputeMinContentWidth()` を 1 行足した。1 ファイル 1 行で、`SDataIndexerViewHeader.cpp` は Scope (Views) 内。
- 既定 (可逆) — 新ヘルパの置き場所:
  - `ResolveContainerMemory` / `ResolveCellValue` / `FStructViewDataProvider` は ViewTypes.h。ViewTypes.cpp と Row / Cell が共通に include する。
  - `ExportCellValue` / `ImportCellValue` / `ShowViewNotification` は ViewModel.cpp。
  - ドラッグの基底は新ヘッダ 1 本。
  - 変える費用はファイル間の移動だけ。
- 外部呼び手: 削除・改名したシンボルは、submodule 外 (`Source/`、`Plugins/DataIndexerToolset`) から参照されていない (`grep` 0 件)。`DataIndexerNotOverridable.cpp` にある同名のローカル `ResolveContainerMemory` (FPropertyPath 版) は Views の外で、Scope 外。
- Game ターゲット: 変更は Editor モジュールの Private ファイルだけで、runtime 構造体に触れない。ビルド不要。
- 自己レビュー (`git diff --stat`): 変更は Views 配下の 20 ファイルと新規 `SDataIndexerViewDragHandle.h` だけ (669 行追加 / 1201 行削除)。Scope 外のパスは無い。
- AC 確認 (2026-10-28):
  - `rg 'GetFillHandleFlags|EFillHandleFlags|GetColumnRangeCenterXLocal|CreateLambda\(\s*\[\]\s*\{\s*return true' Views/` → 0 件。
  - `ResolveContainerMemory` の定義 → 1 件 (`DataIndexerViewTypes.h`)。
  - `StructViewDataProvider` のクラス定義 → 1 件 (`DataIndexerViewTypes.h`)。
  - `ExportTextItem|ImportTextItem` in `_Clipboard.cpp` / `_FillHandle.cpp` → 0 件。Export / Import は ViewModel.cpp の `ExportCellValue` / `ImportCellValue` (`ResolveCellValue` 経由) にだけある。
  - `SelectCellRect` → 1 定義 + 2 呼び手 (理由は R-V1 の項)。
  - `clang-format` の off/on → 対になる (13/27、70/75)。
  - 全自動テスト → 75/75。
- GUI smoke は UE MCP 停止で撮れない。verify_pending にした。
- critic round 1 (task-critic): FAIL
  1. AC「SelectCellRect 1 定義 + 3 呼び手」未達 (2 呼び手。PasteRow は EndRangeSelection + SetSelection)。作者の判断で AC を外せない。
  2. `SViewHeader::HandleColumnWidthChanged` の `RecomputeMinContentWidth();` は挙動変更で Out of Scope「挙動変更」に当たる。
  NOTE: Export/Import は ViewModel.cpp へ (AC 文言はファイル内を想定、意図は満たす)。PasteRow は PreDragSelection もリセット (Steps 4 の範囲)。行デコレータの SHorizontalBox 除去と SelectCellRect の範囲は画面でしか確認できない。削除名の掃除と新ヘルパの呼び手は確認済み。
- round 1 への回答: 1 は PasteRow が飛び飛びの行選択を作り得るためセル矩形で表せず、書かれたとおりには直せない (直すと挙動変更)。2 は orchestrator の dispatch が名指しで fold を指示した 57d1 の修正で、Execution Log に fold として記録済み。コード変更なしで round 2。
- critic round 2 (task-critic): FAIL
  1. 同上 (AC の文言は未変更なので未達のまま。AC を直せるのはユーザー)。
  2. 同上 (executor.md の fold 規則はタスクの Out of Scope を無効にしない。行を外すか Out of Scope を直すかはユーザー判断)。
  NOTE: PasteRow の PreDragSelection クリア、CollectVisibleRows の IsValid ガード追加 (無害)、削除名 grep 0 件、LOCTEXT ラベル同一、見た目は GUI smoke 待ち。
- 2 round で閉じない 2 件は 要ユーザー判断 (run 終端) として orchestrator に渡し、commit は進める (executor.md step 8-4)。

### ユーザー決定 2026-10-28
- critic 1 件目: AC の文言を直した。コードは変えない。status は変えない。
  - 旧文言: 「`rg 'SelectCellRect'` が 1 定義 + 3 呼び手。」
  - 新文言: 1 定義 + 2 呼び手 (`PasteCells`, `CommitFillHandleDrag`)。Clipboard/FillHandle は範囲選択の状態を直接書かない。
- critic 2 件目: fold した t261026-57d1 由来の 1 行 (`SViewHeader::HandleColumnWidthChanged` の `RecomputeMinContentWidth();`) は受け入れる。
  Out of Scope の「挙動変更」は「リファクタ由来の挙動変更」と読む。Out of Scope の文言は変えない。

### verify sweep 2026-10-28 19:34 (ALICE、合格 → done)
- 前提 t261026-57d1 / bd46 / dd8a は合格済み (bd46 / dd8a は同じ sweep)。
- 環境: エディタ PID 40584 (submodule 0f651e4 のビルド)、UE MCP 127.0.0.1:8001。GUI 操作はユーザー許可済み (dispatch 2026-10-28「今からやって」)。一時アセット `/Game/__Verify/DI_V1000` (DIS_Item、1000 行) を bd46 と共用し、終了時に削除した。全データ列を表示して操作し、列レイアウトは後で UI で戻した (bd46 の Execution Log)。証跡は `.agents/tasks/evidence/t261026-ed01/`。
- 選択矩形: Subtitle 行 1 を Ctrl+Click → Description 行 2 を Shift+Click → 2×2 の枠 (01)。
- 貼り付け (重点 1): その 2×2 を Copy → Subtitle 行 4 を 1 セル選択 (02a) → Paste (実マウス) → 行 4〜5 の Subtitle..Description に値が入り、選択枠はちょうどその 2×2 (02)。旧実装の `TargetColumns` から作る範囲と同じ。
- フィル (重点 1): Base Value 行 1 を選び、ハンドルを行 3 まで実マウスでドラッグ → 青枠のプレビュー (03a) → 離すと行 2・3 に 0 が入り、選択枠は行 1〜3 の Base Value (03)。旧実装の Begin → Extend → End と同じ範囲 (元セルから終点まで)。行のフィルも dd8a の検証で同じ結果 (行 4〜6)。
- 列ドラッグのデコレータ (重点 2): ドラッグ中に CopyFromScreen で撮った (`_harness/dragcap.ps1`)。余白つきの枠に "BaseValue"、挿入位置に青い縦線 (04)。
- 行ドラッグのデコレータ (重点 2): "Place Row [Item 0000] Here" の枠と挿入線 (05)。旧 `SHorizontalBox` の包みを外した後も、文字は枠の中に収まっている。
- 見つけた不具合 (follow-up、既存): 前に非表示のデータ列があると、列ドラッグの挿入位置が常に 0 になる (`ComputeDropInsertIndex` が非表示列で break)。本タスク以前 (`7b8789e^`) も同じ処理。証跡 `.agents/tasks/evidence/t261026-bd46/03b-drag-with-hidden-columns-noop.png`。
- 所要 約 4 分 (Data View の作業 19:34〜19:54 を bd46 / dd8a / ed01 で共用。按分は概算)。

## Summary
Views の重複・責務漏れ・死にコードを整理した (R-V1〜R-V13)。セル値の Export/Import、行の読取専用検査、Fill Down の書込み、列レイアウトの走査、ドラッグの Op/Handle、セル枠の描画を、それぞれ 1 つにまとめた。挙動は変えない意図で、自動テストは 75/75。GUI smoke 5 操作は 2026-10-28 の verify sweep で合格した。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` 配下:
  - `DataIndexerViewModel.h` / `DataIndexerViewModel.cpp`
  - `DataIndexerViewModel_Clipboard.cpp` / `_FillHandle.cpp` / `_RangeSelection.cpp` / `_Selection.cpp` / `_Visibility.cpp`
  - `DataIndexerViewTypes.h` / `DataIndexerViewTypes.cpp`
  - `SDataIndexerDictionaryComboBox.cpp`
  - `SDataIndexerView.cpp`
  - `SDataIndexerViewCell.cpp`
  - `SDataIndexerViewHeader.h` / `SDataIndexerViewHeader.cpp` (fold の `RecomputeMinContentWidth` 1 行を含む)
  - `SDataIndexerViewRow.h` / `SDataIndexerViewRow.cpp`
  - `SDataIndexerViewColumnMoveDragDrop.h` / `.cpp`
  - `SDataIndexerViewRowMoveDragDrop.h` / `.cpp`
  - `SDataIndexerViewDragHandle.h` (新規)
- `.agents/tasks/done/t261026-ed01-view-refactor.md`

## Blockers
None
