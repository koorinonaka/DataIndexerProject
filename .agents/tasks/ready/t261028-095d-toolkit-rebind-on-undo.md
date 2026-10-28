---
status: ready
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

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし
