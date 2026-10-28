---
status: done
order: 180
tier: standard
screen: required
agent: any
created_by: task-create
---

# Undo/Redo で RowStruct や Schema が戻ったとき、Toolkit の schema delegate を張り直す

## Goal
Repository の SchemaClass 変更 (行リセット込み) を Ctrl+Z / Ctrl+Y で戻したあとも、Repository エディタの Data View が
「いま EditorData が持つ RowStruct (UDS) と Schema Blueprint」の変更に追随する。

## Context
- 由来: root/t261026-6bf4 (B-E14、commit `496cd8b` 以前の同 run で実装済み・verify_pending) の critic NOTE 2。
  6bf4 は SchemaClass 変更時の再束縛を `OnAssetFinishedChangingProperties` → `HandleSchemaClassChanged` に足したが、
  undo/redo はこの経路を通らない。
- 現状の機構 (2026-10-28 時点。行番号がずれていたら関数名で探す):
  - `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp`
    - `BindSchemaDelegates` (528 行付近): `EditorData->GetRowStruct()` が UDS なら `OnStructChanged()` に、
      `Schema->GetClass()->ClassGeneratedBy` が UBlueprint なら `OnChanged()` に lambda を足す。
      束縛先を `BoundRowStruct` / `BoundSchemaBlueprint` (`TWeakObjectPtr`、`DataIndexerEditorToolkit.h`) に覚える。
    - `UnbindSchemaDelegates` (566 行付近): 覚えた先から `RemoveAll(this)` して両方 `Reset()`。
    - `HandleSchemaClassChanged` (580 行付近): `ResolveStructureMismatch` → Unbind → Bind。
    - `InitEditor` 内 (236 行付近): `ViewModel->GetOnAssetChanged().AddSPLambda( this, [this] { HandleModelRowsChanged( true ); } )`。
  - `Views/DataIndexerViewModel.h:20` `FViewModel` は `FSelfRegisteringEditorUndoClient`。`PostUndo` / `PostRedo`
    (`DataIndexerViewModel.cpp:71-79`) は `OnAssetChanged.Broadcast()`。`NotifyAssetChangedExternally` も同じ delegate を broadcast する。
- 不具合: SchemaClass 変更 → Yes でリセット (`ResolveStructureMismatch` は `Repository.Modify()` / `EditorData.Modify()` 済みの
  トランザクション) → 再束縛で新 UDS に bind → Ctrl+Z で `RowStruct` と `SchemaClass` は旧値に戻るが、delegate は新 UDS / 新 Blueprint に
  bind されたまま。以後、旧 UDS を編集しても Data View は更新されない。リセットしない SchemaClass 変更の undo でも Blueprint 側が同様に古くなる。
- 判断 (Advisor 判定、2026-10-28):
  - Toolkit に `FEditorUndoClient` を新設しない。`FViewModel` の undo client が既に `OnAssetChanged` を broadcast し、Toolkit は
    それを購読済み。そこに相乗りする。
  - 無条件に張り直さず、比較してから張り直す。`OnAssetChanged` は UDS の `OnStructChanged` lambda の中
    (`NotifyAssetChangedExternally`) からも発火する。比較を先にすれば、何も変わっていないときにブロードキャスト中の delegate へ
    `RemoveAll` / `Add` をしない。
  - RowStruct と Schema Blueprint の両方を比較する。
- 重複チェック通過 (20件対象)。

## Scope
- `DataIndexerEditorToolkit.h/.cpp` に `RebindSchemaDelegatesIfChanged()` (private) を足す。
- `InitEditor` の `GetOnAssetChanged()` lambda からそれを呼ぶ。

## Out of Scope
- `ResolveStructureMismatch` の再実行 (undo は行と RowStruct を一緒に戻すので不一致は生じない)。
- `FViewModel` の undo client の構造 (root/t261026-21a2 の範囲)。
- `HandleSchemaClassChanged` の既存経路。

## Preconditions
- 木に `BindSchemaDelegates` / `UnbindSchemaDelegates` / `BoundRowStruct` / `BoundSchemaBlueprint` があること (6bf4 の実装。verify_pending のままでよい)。

## Steps
1. `DataIndexerEditorToolkit.h` の `UnbindSchemaDelegates` 宣言の近くに `void RebindSchemaDelegatesIfChanged();` を足す。
2. `DataIndexerEditorToolkit.cpp` に実装する:
   - `UDataIndexerEditorData::Get( *Repository )` が null なら何もしない。
   - 現在値: `Cast<const UUserDefinedStruct>( EditorData->GetRowStruct() )` と、`EditorData->GetSchema()` があれば
     `Cast<UBlueprint>( Schema->GetClass()->ClassGeneratedBy )` (無ければ null)。`BindSchemaDelegates` と同じ取り方にする。
   - どちらかが `BoundRowStruct.Get()` / `BoundSchemaBlueprint.Get()` と違えば `UnbindSchemaDelegates(); BindSchemaDelegates();`。
3. `InitEditor` の `GetOnAssetChanged()` lambda を `{ RebindSchemaDelegatesIfChanged(); HandleModelRowsChanged( true ); }` にする。

## Acceptance Criteria
- `rg 'RebindSchemaDelegatesIfChanged' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp` が 2 件以上 (定義と `GetOnAssetChanged` lambda からの呼び出し。現在 0)。
- 新関数は比較が一致したときに `UnbindSchemaDelegates` / `BindSchemaDelegates` を呼ばない (コード上)。
- GUI: UDS RowStruct の Schema A を持つ Repository で SchemaClass を UDS RowStruct の Schema B に変え、Yes でリセット → Ctrl+Z →
  Schema A の UDS にフィールドを足すと、開いている Repository エディタの Data View に列が出る (現在: 出ない)。
- GUI: 続けて Ctrl+Y → Schema B の UDS にフィールドを足すと Data View に列が出る。

## Tests
- Toolkit は asset editor を開かないと構築できず、既存の自動テストに Toolkit を立てる口は無い。自動テストは足さない
  (Tests 4 問の 4 で、テスト専用の口が要るため)。
- quick: Editor ターゲットのビルド + `Automation RunTests DataIndexerEd.Repository` (既存の回帰が無いこと)。

## Verification
- 上の GUI 2 項目を UE MCP か `windows-screenshot` で撮る (Ctrl+Z 後と Ctrl+Y 後の Data View 各 1 枚)。
- 済 (2026-10-28 sweep、ALICE)。AC の「フィールドを足すと列が出る」は修正と関係なく起きないので (Execution Log)、同じ帰結「Data View が束縛中の UDS の変更に追随する」をフィールドの改名で確かめた。
  - Ctrl+Z 後: `02-undo-before-edit.png` (MemberVar_0 列あり) → S_V095B の MemberVar_0 を改名 → `03-undo-after-rename-B.png` (列が消える)。
  - Ctrl+Y 後: `04-redo-before-edit.png` → S_V095A の MemberVar_0 を改名 → `05-redo-after-rename-A.png` (列が消える)。
  - 証跡は `evidence/t261028-095d/`。

## Execution Log
- 2026-10-28 (dispatch 19/21, branch `fix/audit-2026-10`): Preconditions 確認。`BindSchemaDelegates` / `UnbindSchemaDelegates` /
  `BoundRowStruct` / `BoundSchemaBlueprint` は木にある (6bf4 の実装、verify_pending の上に構築)。
- 実装: Steps 1–3 どおり。`RebindSchemaDelegatesIfChanged()` は EditorData null で return、現在の UDS と Schema Blueprint を
  `BindSchemaDelegates` と同じ取り方で求め、両方が `BoundRowStruct.Get()` / `BoundSchemaBlueprint.Get()` と一致すれば
  Unbind/Bind を呼ばずに return する。`InitEditor` の `GetOnAssetChanged()` lambda から `HandleModelRowsChanged( true )` の前に呼ぶ。
- ビルド: `Build.bat DataIndexerProjectEditor Win64 Development` は 1–3 回目が `fatal error C1001` (内部コンパイラエラー、毎回別の
  エンジンヘッダ行: SharedPointerInternals.h(885) / SparseSet.h.inl(1391) など) で exit 6、各 15–16 秒。空きメモリ 40GB。
  4 回目は同じソースで `Result: Succeeded` (21.7 秒)。発生位置が毎回違うため、本変更ではなくコンパイラの一過性と判断した。
- quick: `Automation RunTests DataIndexerEd.Repository` → Found 3 tests、3 件とも `Result={Success}`、EXIT CODE 0。
- Acceptance: `rg 'RebindSchemaDelegatesIfChanged' …/DataIndexerEditorToolkit.cpp` → 2 件 (定義・lambda 呼び出し)。比較一致時の早期 return を
  コードで確認。GUI 2 項目は UE MCP (127.0.0.1:8001) がこの run で停止しているため未実施。
- 自己レビュー: `git diff --stat` = Toolkit.cpp +29/-1、Toolkit.h +1。Scope 外のパス無し。
- critic round 1 (task-critic):

  ```
  PASS
  NOTE: DataIndexerEditorToolkit.cpp:585 — I checked the first two Acceptance Criteria in the code. `rg 'RebindSchemaDelegatesIfChanged'` finds 2 hits: the definition at line 585 and the call from the `GetOnAssetChanged` lambda at line 239. When both values match, the new function returns at lines 599-602, before `UnbindSchemaDelegates` / `BindSchemaDelegates` run. Lines 595-597 read the current values the same way `BindSchemaDelegates` does at lines 541 and 554-556.
  NOTE: t261028-095d-toolkit-rebind-on-undo.md:66-68 — The diff cannot show the two GUI criteria (after Ctrl+Z / Ctrl+Y, adding a field to the UDS makes a column appear in Data View). The author has not run them yet; this is listed under Blockers. The screenshots still need to be taken.
  NOTE: DataIndexerEditorToolkit.cpp:604-605 — When the RowStruct is not a UDS and the Schema is not a Blueprint, both bound pointers stay null. In that case the comparison sees null on both sides and skips the rebind, which is correct. One gap: if a bound object is garbage-collected, its `TWeakObjectPtr` turns stale and `Get()` returns null. That differs from a live current value, so the function rebinds. This does no harm.
  No removed identifiers. The only `-` line is the old lambda body, and its `HandleModelRowsChanged( true )` call is kept. The diff does not touch `HandleSchemaClassChanged`, `ResolveStructureMismatch` or `FViewModel` (Out of Scope). The project has no CLAUDE.md.
  ```
- 2026-10-28 verify sweep (ALICE、UE MCP 127.0.0.1:8001、ユーザー許可の GUI 操作)。一時資産は `/Game/__Verify` に作り、終了時に全部消した (`Content/__Verify` は空)。
  - 準備: Python で UDS `S_V095A` / `S_V095B` (各 MemberVar_0: bool) と、Schema BP `DIS_V095A` (RowStruct=S_V095A) / `DIS_V095B` (RowStruct=S_V095B) を作った。MCP `DataIndexerToolset.CreateRepository` で `DI_V095` (SchemaClass=DIS_V095A) を作った。
  - SchemaClass の変え方: 詳細パネルの Schema Class ピッカーは RowStruct が同じ schema しか出さない (`FDataIndexerSchemaClassFilter`、DIS_V095A の 1 件だけ表示)。そこで Schema Class 行の右クリック → ペースト (クリップボードにクラスパス) で変えた。`OnAssetFinishedChangingProperties` → `HandleSchemaClassChanged` → リセット確認 (modal) → はい、の経路は AC と同じ。確認ダイアログは `01-reset-prompt.png`。
  - AC の書き換え (既定): 新しい UDS フィールドは Schema の Column Layout で未チェックなので、Data View の列にならない。対照として、S_V095B に MemberVar_1 を足してから Repository エディタを閉じて開き直した。開き直しても列は MemberVar_0 だけだった (`00-control-added-field-not-a-column-on-fresh-open.png`)。つまり「足すと列が出る」は束縛の正否と関係なく起きない。代わりに、チェック済みの MemberVar_0 を struct エディタで改名した。束縛が生きていれば `OnStructChanged` → `NotifyAssetChangedExternally` → 列の作り直しで、列が消える。ヘッダの作り直しは `OnAssetChanged` (PostUndo / PostRedo / NotifyAssetChangedExternally) だけが起こし、タブ切替には繋がっていない。
  - 実行した順序 (AC の A/B の役は入れ替わる): DIS_V095B の状態から DIS_V095A をペースト (リセット はい) → `TRANSACTION UNDO` (ログ `Undo プロパティを貼り付け`、SchemaClass=DIS_V095B_C) → S_V095B の MemberVar_0 を `B_AfterUndo` に改名 → 列が消えた (02→03)。
  - redo: 再度 DIS_V095A をペースト → UNDO → `TRANSACTION REDO` (ログ `Redo プロパティを貼り付け`、SchemaClass=DIS_V095A_C) → S_V095A の MemberVar_0 を `A_AfterRedo` に改名 → 列が消えた (04→05)。
  - 改名で struct の変更が出るので、改名の後の Ctrl+Y は redo 履歴が消えて成立しない。このため undo と redo は別の周回で確かめた。
  - 最初の周回の UNDO は効かなかった (`Undo` のログ行なし)。原因は、UNDO を ProgrammaticToolset の `execute_tool_script` の中から打ったこと。スクリプト実行中は `end_transaction()` が 1 を返す (スクリプト自身のトランザクション)。直接 SlateInspector `Type` で打つと、ペースト直後も 0 で、UNDO が効いた (2/2)。
  - 観察 (範囲外・既存): toolbar 右上の「Schema: <名前>」は SchemaClass を変えても更新されない (f3b1 Execution Log 参照)。

## Summary
`FEditorToolkit::RebindSchemaDelegatesIfChanged()` を追加し、`ViewModel` の `OnAssetChanged` (undo/redo の `PostUndo`/`PostRedo` も含む)
のたびに RowStruct UDS と Schema Blueprint を束縛先と比較して、違うときだけ schema delegate を張り直す。
ビルドと Repository テストは通過。GUI 検証は 2026-10-28 の sweep で済。Ctrl+Z 後と Ctrl+Y 後のどちらでも、束縛中の UDS のフィールド改名が Data View の列に反映された。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp`

## Blockers
None
