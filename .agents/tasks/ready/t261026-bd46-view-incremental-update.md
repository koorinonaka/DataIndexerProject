---
status: verify_pending
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
- 残り (2026-10-28、MCP 停止のため未実施):
  1. セル 1 個の編集で `SViewCell::Construct` がその行の列数ぶんだけ呼ばれる (一時ログカウンタで確認)。
  2. フィルタ入力中に `RepopulateEntries` が呼ばれない (一時ログ)。
  3. 列ドラッグ並べ替え・列可視トグルで表示が正しく、ちらつきが無い。
  4. InlineTransient セルの編集確定直後に Fill handle が有効。
  5. Undo / Redo 後にセルが空にならない (EmptyKey 経路の `RebuildList`)。
  6. 行の追加 / 削除 / 移動で行番号が正しく振り直される (LineNumber TAttribute)。

## Execution Log
- 2026-10-28 13:02 着手。前提 t261026-57d1 は verify_pending (検証未完) の上に構築。`release/v1.0.2` は main にマージ済み (作業ブランチ `fix/audit-2026-10` は main から)。
- Step 5 のエンジン確認: `SHeaderRow::AddColumns` は 5.8 (`SHeaderRow.cpp:619`、一括 1 broadcast) にあり、5.7 (`D:/Apps/Epic Games/UE_5.7/Engine/Source/Runtime/Slate` の `SHeaderRow.h` / `.cpp`) には無い → `UE_VERSION_NEWER_THAN_OR_EQUAL(5,8,0)` で分岐し、5.7 は `AddColumn` ループ。`SHeaderRow::Columns` は private で `FColumn` は常に `bIsVisible=true` で生成されるため、非表示列だけ `SetShowGeneratedColumn(false)` を個別に呼ぶ (その列数ぶん broadcast)。
- 実装:
  - `BuildRows` に `TMap<FRepositoryKey,FRowEntryPtr>& InOutRowEntryByKey` と `RecomputeKeys` (null=全行) を追加。旧エントリをキーで再利用し、セルは新規エントリ / RecomputeKeys 該当行だけ再計算。上書きマークは `TMap<FPrimaryKey,FRowEntryPtr>` (直前エントリだけマーク) で O(rows²) を解消。`FRowEntry::DisplayName` は再利用時に更新するため非 const。
  - `FViewModel::RowEntryByKey` を新設 (dd8a が使う)。`FindRowEntry` は dd8a の範囲なので線形のまま。`RefreshRows(RecomputeKeys)` は常に `UpdateVisibleRows` を呼ぶ (旧 `OldRows != RowEntries &&` は再利用で常に偽になり、フィルタ再判定と選択範囲の再構築が止まるため)。`RefreshColumns` の戻りを `EColumnsChange { None, OrderChanged, SetChanged }` に。
  - `SView::Tick`: EmptyKey (アセット変更 / Undo) → `RepopulateEntries` + `RebuildList` (エントリ再利用で行 widget が残るが、`SViewCell` は `OnAssetChanged` で自分を NullWidget にするため全再生成が要る)。具体キーのみ → `RepopulateEntries(false, &DirtyKeys)`、可視リストが変わったときだけ `RequestListRefresh`、dirty 行は `WidgetFromItem` → `SViewRow::RebuildCells()`。`bVisibleRowsDirty` だけ → 桁数チェック + `RequestListRefresh`。`OnRowsPostUpdated` は全分岐で発火。
  - `HandleModelVisibleUpdated` は `bVisibleRowsDirty` を立てるだけ。`HandleColumnReordered` は `RefreshColumns` + `BuildColumns` だけ (行の再構築・FontMeasure なし)。列可視トグル 3 箇所は `SViewHeader::SetColumnShown` (`SetShowGeneratedColumn` + `RecomputeMinContentWidth`)。
  - `SViewRow::RebuildCells()` 新設 (`OnColumnsUpdated` も同じ関数を使う)。行 widget を再利用するので、行メモリを直接持つ `RowEntityProvider` を再生成のたびに解決し直す。
  - `SViewCell`: LineNumber を `Text_Lambda` (RowsIndexOf)、`CreateAsSimpleText` を `FCellEntry::DisplayName` を読む `Text_Lambda` に (引数はフォールバック。ViewTypes.h のコメントどおり DisplayName は CreateAsSimpleText に渡される文字列)。毎フレームの線形探索を増やさないよう、セルに `TWeakPtr<FRowEntry>` を保持。デストラクタで InlineTransient なら `SetEditingCell(false)` (B-V8 の保険)。
- 既定 (可逆): 行の追加 / 削除 / 移動は「具体キー」経路で扱い、`RequestListRefresh` で新規行だけ生成する。全再生成は EmptyKey のみ。変更コストは `SView::Tick` の分岐 1 箇所。
- 既知の残り: `BuildColumns` は RowName 列幅のため全行の DisplayName を測る (rows×1)。列並べ替えでも走る。受け入れ条件は rows×cols のセル計測なので満たすが、ここは未対処。
- テスト (Tests 4 問): (1) 未変更キーの FRowEntry ポインタ維持と、dirty 行だけのセル再計算。(2) BuildRows が毎回新規確保に戻る回帰で落ちる。(3) 既存テストはポインタ同一性を見ない (hollow でも既存 12 件は緑)。(4) `RecomputeKeys` は本番 (`SView::Tick`) が使う口。
- red control: `OldEntryByKey.FindRef` を「常に新規」に hollow してビルド → `DataIndexerEd.ViewModel` 13 件中 12 成功 / 1 失敗 (`BuildRows.ReusesUnchangedEntries` の 3 assert: "Full rebuild keeps every FRowEntry pointer"、"Dirty rebuild keeps every FRowEntry pointer"、"Exactly one entry recomputed its cells" = 4)。件数とセル存在の対照 assert は通過。復元して再ビルド。
- quick: `Build.bat DataIndexerProjectEditor Win64 Development` 成功 (2 回、各 ~22 s)。`Automation RunTests DataIndexerEd.ViewModel+DataIndexerEd.View` → 14 Success / 0 Fail。追加で `Automation RunTests DataIndexerEd` (全群) → 74 Success / 0 Fail。full は run 終端。
- 受け入れ: `rg 'AddColumns' SDataIndexerViewHeader.cpp` 2 件 (旧 0)。`rg 'TODO: DirtyKey' Plugins/DataIndexer/Source/DataIndexerEd/` 0 件。トグル 3 箇所から `BuildColumns` (= `ClearColumns`) 呼び出しが消えた。並べ替え経路に FontMeasure なし。フィルタ経路は `RepopulateEntries` を呼ばない (コード上)。Construct 回数・Fill handle・ちらつきは GUI 検証待ち。
- 自己レビュー (stat): submodule 13 ファイル (+333/-108)、すべて Scope 内 (Views 配下と ViewModel テスト)。parent はタスクファイルと submodule pointer のみ。
- critic r1 (task-critic):
  ```
  PASS
  NOTE: SDataIndexerViewCell.cpp:527-532 — LineNumber の Text_Lambda は毎フレーム RowsIndexOf (線形探索) を可視行ごとに呼ぶ。Step 4 が TAttribute 化を求めているので FAIL ではない。線形探索の解消は t261026-dd8a の範囲。
  NOTE: SDataIndexerView.cpp:1013-1017 — HandleColumnReordered は RefreshRows と RequestListRefresh を呼ばなくなった。行の列再生成はヘッダの ColumnsChanged broadcast 頼み。正しく動くかは GUI でしか確かめられない (Verification 3)。
  NOTE: SDataIndexerView.cpp:599-604 — 可視行の更新は RequestListRefresh だけになり、RefreshColumns / RefreshRows は走らない。呼び手 (UpdateVisibleRows) は VisibleRows を確定させてから broadcast するので、コード上は問題ない。
  NOTE: AC「セル編集で Construct が 1 行分だけ」「InlineTransient 確定後に Fill 有効」とちらつきはコードだけでは判定できない。画面確認が残る。
  掃除の結果: 外部の参照切れなし。追加した定義はすべて参照あり。シグネチャ変更の呼び手 (テスト含む) もすべて追従。
  ```

## Summary
Data View を差分更新にした。`BuildRows` は旧 `FRowEntry` をキーで再利用し、dirty 行だけセルを再計算する。`SView::Tick` は EmptyKey だけ全再生成し、具体キーは該当 `SViewRow::RebuildCells`、フィルタは `RequestListRefresh` だけにした。ヘッダは `AddColumns` 一括 (5.7 は分岐)、列トグルは `SetShowGeneratedColumn`、列並べ替えはヘッダ再構築だけ。LineNumber / SimpleText は TAttribute、`SViewCell` デストラクタで編集フラグを戻す。GUI の Verification が残る。

## Changed Files
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewTypes.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewTypes.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerView.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerView.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewRow.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewRow.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewCell.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewCell.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/ViewModel/DataIndexerViewModelTests.cpp
- .agents/tasks/ready/t261026-bd46-view-incremental-update.md

## Blockers
検証待ち: UE MCP (127.0.0.1:8001) がこの run で停止しており、エディタ GUI の Verification を実施できない。残りは Verification 節の GUI 項目だけ (実装は完了)。
