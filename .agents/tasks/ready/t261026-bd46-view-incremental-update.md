---
status: ready
order: 110
tier: standard
screen: required
agent: any
created_by: task-create
---

# Data View を DirtyKeys 単位の差分更新にする (X2)

## Goal
セル 1 個の編集・フィルタ 1 打鍵・列の並べ替えで、Data View が可視行×列の widget を全再生成しない。`SListView` の widget 再利用が効き、
ヘッダ再構築の broadcast が 1 回になる。

## Context
監査 `.agents/audit-2026-10-26.md` の X2 = P-V1, P-V2, P-V5, P-V8, P-V11, B-V8。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` 相対、行番号は 2026-10-26 時点。エンジン根拠は UE 5.8 実測。

- `SDataIndexerView.cpp:266` `// TODO: DirtyKeyでUpdate` — `Tick` が `DirtyKeys` 非空なら `RepopulateEntries(false)` で全再構築。
- P-V1: `DataIndexerViewTypes.cpp:428-429` + `SDataIndexerView.cpp:262-282` `BuildRows` が毎回 `FRowEntry` と `TSharedRef<FRepositoryKey>` を新規確保 →
  `VisibleRows` のポインタが変わる → `SListView::ItemToWidgetMap` (`SListView.h:1163`、ポインタ同一性) 全ミス → 可視行×列の
  `SViewRow` / `SViewCell` を全再生成 (`CreatePropertyWidget` → `Schema->CustomizePropertyCellWidget`)。トリガー: セル編集
  (`NotifyPostChange → OnRowsUpdated`)、Undo、フィルタ 1 打鍵、行追加/削除/移動。
  Fix: `BuildRows` で旧 `FRowEntry` をキーで再利用 (`BuildColumns` の `OldEntries` と同じ手)。`DirtyKeys` が具体キーなら該当 `FRowEntry::Cells`
  だけ再計算し、`RequestListRefresh` せず該当 `SViewRow` に再生成を通知。EmptyKey / 追加 / 削除 / 移動のみ全再構築。
  依存: 行再利用後は `SDataIndexerViewCell.cpp:682-683` の LineNumber 固定テキストと `CreateAsSimpleText` の `.Text(固定)` が陳腐化 → TAttribute 化。
- P-V2: `SDataIndexerView.cpp:575-580` `HandleModelVisibleUpdated` が `DirtyKeys` に EmptyKey → フィルタ 1 打鍵で全再構築。別フラグ
  (`bVisibleRowsDirty`) で `RequestListRefresh` + 桁数チェックだけ。
- P-V5: `SDataIndexerViewHeader.cpp:245-396` `BuildColumns` が `ClearColumns` (broadcast) + `AddColumn`×(5+N+1) (各 broadcast、`SHeaderRow.cpp:634,662`) +
  `SetShowGeneratedColumn`×(5+N) (変化時 broadcast、`:1253`) → `ColumnsChanged` ごとに全 `SMultiColumnTableRow::GenerateColumns`
  (`STableRow.h:1384`) → 可視セル全再生成が ~2N 回。`SDataIndexerView.cpp:562` の「ちらつき」の原因。`AddColumns` (`SHeaderRow.cpp:619`、一括 1 broadcast)
  で構築。列可視トグル (`SDataIndexerView.cpp:942,995,1002`) は `BuildColumns` でなく `SetShowGeneratedColumn` + `MinContentWidth` 再計算だけ。
- P-V8: `DataIndexerViewTypes.cpp:417-424` `BuildRows` の上書きマーク loop が重複キーごとに `OutRowEntries` 全走査 → O(rows²)。`:445-472` セルごとに
  `ComputeCellText` (ExportTextItem) + `FCellTextProbe` で `CustomizePropertyCellWidget` + `MakeShared<FCellEntry>` + FText×2。
  `TMap<FPrimaryKey, FRowEntryPtr>` で上書きマーク、プローブは差分更新で行単位に。
- P-V11: `SDataIndexerView.cpp:1005-1009` `HandleColumnReordered` → `RepopulateEntries(false)` → `RefreshColumns` が並べ替えだけで true →
  `:530-556` の rows×cols FontMeasure 再実行 + `BuildColumns`。戻りを「集合変化」と「順序変化」に分け、順序変化は `BuildColumns` だけ。
- B-V8: `SDataIndexerViewCell.cpp:454-463, 106-134` + `SDataIndexerView.cpp:588-602` InlineTransient 編集確定 → 行再生成で `SViewCell` 破棄だが
  `ExitEditMode` を通らず `ViewModel->bIsEditingCell` が true 固定 → `CanFill` (`DataIndexerViewModel_FillHandle.cpp:34`) / `CanClearSelection` が Undo まで false。
  行再利用で根本が消えるが、保険として `SViewCell` デストラクタで `SetEditingCell(false)`。

## Scope
- `FViewModel::BuildRows` / `RefreshRows` を「旧エントリ再利用 + DirtyKeys 該当行だけ再計算」に。`FRowEntry` にキー → エントリのマップを足す
  (t261026-dd8a が同じマップを使う)。
- `SView::Tick` の Invalidate を DirtyKeys の種類で分岐。`HandleModelVisibleUpdated` を別フラグに。
- `SViewHeader::BuildColumns` を `AddColumns` 一括に、可視トグルを `SetShowGeneratedColumn` に、`RefreshColumns` の戻りを 2 値に。
- `SViewCell` の LineNumber / SimpleText を TAttribute に。デストラクタで `SetEditingCell(false)`。

## Out of Scope
- 毎フレームの線形探索 (t261026-dd8a)。
- ViewModel 責務整理 (t261026-ed01)。
- セル widget の中身 (Schema の `CustomizePropertyCellWidget` 契約)。

## Preconditions
- t261026-57d1 (View bug) が done (同じ `SDataIndexerView.cpp` / `_RowOps.cpp` を触る)。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. `FViewModel` に `TMap<FRepositoryKey, FRowEntryPtr> RowEntryByKey` を足し、`BuildRows` で旧エントリを引いて再利用。上書きマークもこのマップで。
2. `OnRowsUpdated` が運ぶキーを `DirtyKeys` に積む経路を確認し、`SView::Tick` で「具体キーのみ」なら該当 `FRowEntry::Cells` 再計算 +
   `CellsListView->WidgetFromItem` で得た `SViewRow` に `RebuildCells()` (新設) を呼ぶ。EmptyKey を含むときだけ `RepopulateEntries`。
3. `HandleModelVisibleUpdated` を `bVisibleRowsDirty` に分離。
4. `SViewCell` の固定テキストを TAttribute 化、デストラクタで `SetEditingCell(false)`。
5. `D:\Apps\Epic Games\UE_5.7\Engine\Source\Runtime\Slate\Private\Widgets\Views\SHeaderRow.cpp` に `AddColumns` (一括 1 broadcast) があるか確認
   (5.8 では `:619`)。無ければ 5.7 側は `ClearColumns` + `AddColumn` のままにし `UE_VERSION_NEWER_THAN_OR_EQUAL(5,8,0)` で分岐。
   `BuildColumns` を `AddColumns` 一括に。可視トグル 3 箇所を `SetShowGeneratedColumn` + `MinContentWidth` 再計算に。`RefreshColumns` の戻りを
   `enum { None, OrderChanged, SetChanged }` に。
6. `DataIndexerEd.ViewModel` + `DataIndexerEd.View` を回し、GUI で Verification。

## Acceptance Criteria
- `BuildRows` を同じ行集合で 2 回呼ぶと、変更の無いキーの `FRowEntry` ポインタが同一 (現在: 毎回新規)。
- セル 1 個を編集したとき `SViewCell::Construct` が呼ばれるのはその行の分だけ (ログカウンタで確認、現在: 可視行×列)。
- フィルタ入力中に `RepopulateEntries` が呼ばれない (現在: 1 打鍵ごと)。
- `rg 'AddColumns' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.cpp` が 1 件以上 (現在 0)。
  列可視トグルで `ClearColumns` が呼ばれない。
- 列ドラッグ並べ替えで `FontMeasure` の rows×cols 再計算が走らない。
- InlineTransient セルを編集確定した直後に Fill handle が有効 (現在: Undo まで無効)。
- `rg 'TODO: DirtyKey' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 1)。

## Tests
- `DataIndexerViewModelTests.cpp` に行: (a) `BuildRows` 2 回で未変更キーのポインタ同一、(b) 1 キー dirty で再計算されるエントリが 1 つ。
  修正前コードで (a) が落ちることを確かめる。
- quick: `DataIndexerEd.ViewModel` + `DataIndexerEd.View`。

## Verification
- 200 行×20 列の Repository (テスト用に生成) でセル編集・フィルタ入力・列トグルを行い、`stat Slate` の `Num Widgets` / tick 時間の
  before/after スクリーンショット。ちらつき (`SDataIndexerView.cpp:562` のコメント) が消えたことを動画 or 連続スクショで。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
