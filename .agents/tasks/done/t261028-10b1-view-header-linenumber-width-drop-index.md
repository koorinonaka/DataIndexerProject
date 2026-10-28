---
status: done
order: 240
tier: standard
screen: required
agent: any
created_by: task-create
---

# Data View ヘッダ: 行数の桁が変わったときの LineNumber 列幅のずれと、隠れた列があるときの列ドラッグ挿入位置 0 を直す

## Goal
- 行数の桁数が変わっても (例 10→9)、LineNumber 列の幅がヘッダと全行でそろう。
- 前に非表示のデータ列があっても、列ドラッグがマウス位置に応じた挿入位置へ列を動かす。

## Context
- ユーザー決定 2026-10-28: t261026-bd46 の verify sweep で見つかった 2 件の follow-up として起票を承認 (この回答が承認、`ready/` 直行)。
- 重複チェック通過 (25件対象、2 件とも)。
- 2 件を 1 本にした理由: どちらも `SDataIndexerViewHeader.cpp` を直し、同じ Data View のスクリーンショットで検証し、互いに依存しない。
- パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` 相対。行番号は 2026-10-28 時点。ずれていたら関数名で探す。

### 1. LineNumber 列幅 (bd46 で入った経路)
- 証跡: `.agents/tasks/evidence/t261026-bd46/06-after-delete-move-insert.png`。
- `SDataIndexerView.cpp` `SView::UpdateLineNumberDigitCount` は桁数だけが変わったとき `BuildColumns()` を避け (ClearColumns 起因のちらつき、同関数のコメント)、`SViewHeader::UpdateLineNumberWidth` → `SHeaderRow::SetColumnWidth( Column::LineNumber, … )` を呼ぶ。
- エンジン (UE 5.8、5.7 も同じ):
  - `SHeaderRow::SetColumnWidth` は `FColumn::SetWidth` だけで、ヘッダのウィジェットも `ColumnsChanged` も作り直さない。
  - LineNumber は `FixedWidth` 列 (`SViewHeader::BuildColumns`)。`STableRow.h` の `SMultiColumnTableRow::GenerateColumns` は Fixed 列を `SBox::WidthOverride( Column.Width.Get() )` と生成時の値で作る。ヘッダの `SHeaderRow::RegenerateWidgets` も Fixed は `WidthOverride( SomeColumn.GetWidth() - SplitterHandleSize )` の値渡し。
  - よって生成済みの行とヘッダは古い幅を持ち続ける。`RebuildCells` (`SView` の dirty 行ループ) で作り直した 1 行だけ新しい幅になり、2px ずれる。
- 列のトグル (`SHeaderRow::SetShowGeneratedColumn` が `ColumnsChanged` を broadcast) は全行を揃える。
- 使える口: `SHeaderRow::RefreshColumns()` (public、5.7 / 5.8 両方。`RegenerateWidgets` だけで `ColumnsChanged` は broadcast しない = ヘッダだけ直る)。行は `ViewModel->GetOnColumnsUpdated()` → `SViewRow::RebuildCells` (`SDataIndexerViewRow.cpp` の ctor で購読) で全行作り直せる。どれを使うかは実装者が決める。条件: `ClearColumns` を使わない (ちらつき)。

### 2. 列ドラッグ挿入位置 (既存の不具合、submodule の `7b8789e^` でも同じ)
- 証跡: `.agents/tasks/evidence/t261026-bd46/03b-drag-with-hidden-columns-noop.png`。`KNOWLEDGE.md`「UE MCP GUI ハーネス」にも既知の不具合として載っている。
- `SDataIndexerViewHeader.cpp` `SViewHeader::ComputeDropInsertIndex` は `ViewModel->GetColumnEntries()` (非表示列を含む) を回し、`GetColumnCenterXLocal` が値を返さない (非表示列) と `break` する。前に非表示列があると挿入位置は常に 0 になる。
- 挿入位置の使い手: `OnDrop` → `ViewModel->HandleColumnOrderChanged( SourceColumnName, DropInsertIndex )`。添字は非表示列を含む `GetColumnEntries` の添字。
- `ComputeDropIndicatorX` も `Entries[0]` / `Entries[DropIndex - 1]` / `Entries[DropIndex]` の範囲を `GetColumnRangeBoundsLocal` で読む。隣が非表示列だと値が無く、インジケータが出ない。`break` → `continue` だけでは直らない。インジケータは最も近い表示列で決める。

## Scope
- `SDataIndexerView.cpp` / `SDataIndexerViewHeader.cpp` (必要なら `SDataIndexerViewRow.cpp`) の上の 2 件。

## Out of Scope
- LineNumber 列以外の列幅の扱い。
- 列ドラッグの見た目 (デコレータ)。

## Preconditions
- t261026-bd46 が done (済)。
- 作業ブランチ `fix/audit-2026-10` (両 repo)。

## Steps
1. LineNumber: 桁数が変わったとき、ヘッダと生成済みの全行の LineNumber 幅を新しい値にする。`ClearColumns` は使わない。
2. 挿入位置: `ComputeDropInsertIndex` は非表示列を飛ばして数え、マウスより左にある最後の表示列の次を返す。`ComputeDropIndicatorX` は非表示列を飛ばして最も近い表示列の境界を使う。
3. ビルド、quick。
4. GUI で Verification。

## Acceptance Criteria
- 10 行の Repository で 1 行消して 9 行にした後、全行とヘッダの LineNumber 列の右端の x が同じ (現在: 作り直された 1 行だけ 2px ずれる)。
- 前に非表示のデータ列がある状態で、表示列を別の表示列の右へドラッグすると、その位置へ動く (現在: 挿入位置 0 で、動かないか先頭へ行く)。
- 同じ状態でドラッグ中のインジケータが落とす位置に出る。
- 非表示列が無いときの列ドラッグは今までどおり動く。

## Tests
- 自動テストなし。Tests 4 問: 守るのは挿入位置の計算。`ComputeDropInsertIndex` は private で、`FGeometry` と `FDragDropEvent` を要る。テストから呼ぶには本番が要らない口が要るので足さない。幅のずれは Slate の生成済みウィジェットにしか出ない。
- quick: ビルド + `DataIndexerEd.View` と `DataIndexerEd.ViewModel`。

## Verification
- `KNOWLEDGE.md`「UE MCP GUI ハーネス」の手順で行う。ヘッダのドラッグは実マウス、ドラッグ中の撮影は `_harness/dragcap.ps1`。
- 1. 10 行の一時 Repository を開き、1 行消す。LineNumber 列が全行でそろったスクリーンショット (06 と同じ構図)。
- 2. データ列を 1 本非表示にし、その右の列を別の位置へドラッグ。ドラッグ中 (インジケータ) と落とした後のスクリーンショット。
- 3. 全列表示でも同じドラッグが動くこと。
- 列の表示や並びは Saved/Config の SchemaLayouts に即時保存される。終わったら UI で戻す。一時アセットは消す。証跡は `.agents/tasks/evidence/t261028-10b1/`。
- 済 (2026-10-29 verify sweep): 1〜3 すべて。

## Execution Log
- 2026-10-28 (dispatch 2/3, run fix/audit-2026-10): 実装。
  - LineNumber: `SView::UpdateLineNumberDigitCount` の桁数変化経路で `UpdateLineNumberWidth()` の後に
    `ViewHeader->RefreshColumns()` (ヘッダだけ再生成、`ColumnsChanged` は出さない) と
    `ViewModel->GetOnColumnsUpdated().Broadcast( {} )` (購読中の全 `SViewRow::RebuildCells`) を呼ぶ。`ClearColumns` は使わない。
    `UpdateLineNumberWidth` 自体には入れていない: `HandleDisplayNameColumnWidthChanged` (グリップドラッグ中) からも呼ばれ、
    そこでヘッダを再生成するとドラッグ中のスプリッタを壊すため。
  - 挿入位置: `ComputeDropInsertIndex` は位置を持たない (非表示) 列を `continue` で飛ばし、マウスより右に中心がある最初の表示列で止まる。
    `ComputeDropIndicatorX` は挿入位置の左・右それぞれで最も近い表示列の境界を探し、両方→中点 / 右だけ→左端 / 左だけ→右端 (TotalWidth-1 で clamp)。
    非表示列が無いときは旧実装の 3 分岐と同じ値になる。
  - 既定 (可逆): 行の作り直しは `OnColumnsUpdated` の broadcast を選んだ (SView から `WidgetFromItem` を回すより短く、生成済みの全行に届く)。
    桁が変わるとき (10/100/1000 境界) だけ走る。変えるコストは数行。
- quick: `Build.bat DataIndexerProjectEditor Win64 Development` → `Result: Succeeded` (31s, 1 回目で成功)。
  `Automation RunTests DataIndexerEd.View+DataIndexerEd.ViewModel` (-nullrhi) → 15 件 Success / 0 Fail (View 1, ViewModel 14)。full は run 終端。
- GUI Verification は未実施: ユーザーが PC を使用中で、dispatch の指示により実マウスを動かす UE MCP SlateInspector は使えない (エディタも閉じたまま)。
- 自己レビュー: `git diff --stat` は submodule の `SDataIndexerView.cpp` / `SDataIndexerViewHeader.cpp` の 2 ファイルだけ。Scope 内。
- critic r1:
  ```
  PASS
  NOTE: SDataIndexerView.cpp:591-592 — AC1 は画面でしか確かめられない。GUI Verification 1 は未実施なので diff だけでは判定できない。`OnColumnsUpdated` の購読は `SDataIndexerViewRow.cpp:31` (`RebuildCells`) だけ。broadcast の副作用は行の作り直しだけ。
  NOTE: SDataIndexerViewHeader.cpp:424-482 — AC2〜AC4 は GUI 未検証。非表示列が無いとき新実装は旧実装の 3 分岐と同じ値。[A, 非表示 H, B] では挿入位置とインジケータの左右境界が正しく決まる。
  NOTE: 消えた識別子はローカルかエンジン API のみ。参照 0 件の定義・消えた文字列なし。Out of Scope への変更なし。
  ```
- verify sweep 2026-10-29 05:18 (ALICE、合格 → done):
  - 環境: エディタは HEAD のビルド (UnrealEditor-DataIndexerEd.dll 2026-10-28 21:29、submodule c9ca5c7 の木)。UE MCP 127.0.0.1:8001 を `evidence/_harness/mcp.py` で叩いた。ユーザーは不在で、実マウスの操作は dispatch で許可済み。証跡 `evidence/t261028-10b1/`。
  - 一時 Repository `/Game/__Verify/DI_V10` (ItemSchema、10 行) を作り、終わりに消した。
  - 計器: SlateInspector の Snapshot の pos (画面 px)。変えずに 2 回取って同じ値になった。
  - V1 / AC1: Row 05 を選んで Delete (10 → 9 行)。ヘッダの RowName は x 102 → 100、全行の先頭ボタンは 75 → 73、名前は 103 → 101。全行とヘッダが 2px そろって動いた。行の widget は同じ ref で、全行のセルが作り直された (`OnColumnsUpdated` → `RebuildCells`)。続けて 1 行だけ作り直した (詳細パネルで Row 06 の DisplayName を Row 66 に変更)。作り直されたのはその行のセルだけで、その行の位置も 73 / 101 のまま。修正前は作り直した行だけ 2px ずれていた (bd46 の 06)。`01-linenumber-measure.txt`、`01-after-delete-9rows-linenumber-aligned.png`。
  - V2 / AC2・AC3: Filters → Properties で Subtitle を非表示にした。実マウスで Description を Rarity と Base Value の間へドラッグした。押したままの撮影では、青いインジケータが画面 x 935–936 (Rarity と Base Value の境) に出た (02a)。離した後の並びは RowName, Type, Rarity, Description, Base Value, Metadata Tags (02b)。修正前は挿入位置が 0 になっていた。
  - V3 / AC4: Subtitle を表示に戻し (全列表示)、Description を Subtitle と Type の間へドラッグした。インジケータは x 367–368 に出た (03a)。並びは元の順 RowName, Subtitle, Description, Type, Rarity, Base Value, Metadata Tags に戻った (03b)。
  - SchemaLayouts: UI で元の並びと表示に戻した。ただし ini の行には `Subtitle (bVisibility=True)` と `ColumnOrder` が明示値として残った。そこで sweep の終わりにエディタを閉じ、検証前に保存しておいた行で戻した。エディタを起動し直した後も、その行は検証前と同じだった。

## Summary
LineNumber 列幅は桁数変化時にヘッダ (`RefreshColumns`) と生成済み全行 (`OnColumnsUpdated` → `RebuildCells`) を作り直して揃える。
列ドラッグの挿入位置とインジケータは非表示列を飛ばして表示列だけで決める。ビルドと View/ViewModel テストは通過。2026-10-29 の verify sweep で GUI 検証に合格した (桁が変わった後も全行とヘッダの LineNumber がそろう。非表示列があっても挿入位置とインジケータが正しい)。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerView.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.cpp`
- `.agents/tasks/evidence/t261028-10b1/` — verify sweep の証跡

## Blockers
None
