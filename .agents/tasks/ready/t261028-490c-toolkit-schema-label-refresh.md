---
status: ready
order: 250
tier: standard
screen: required
agent: any
created_by: task-create
---

# Repository エディタ右上の「Schema: <名前>」を SchemaClass の変更と Undo に追従させる

## Goal
Repository エディタで SchemaClass を変えたとき、または Ctrl+Z で戻したとき、右上の「Schema: <名前>」とその横のボタン (Content Browser で探す・エディタで開く) が新しい schema を指す。

## Context
- ユーザー決定 2026-10-28: t261026-f3b1 / t261028-095d の verify sweep で見つかった既存の不具合の follow-up として起票を承認 (この回答が承認、`ready/` 直行)。
- 重複チェック通過 (25件対象)。最も近いのは `done/t261026-6bf4-editor-core-bugs.md` (dup 0.44)。6bf4 は Schema 差し替えの別の bug で、表示ラベルは扱っていない。`done/t261028-095d-toolkit-rebind-on-undo.md` は Undo で delegate を張り直すタスクで、表示は扱っていない。
- コード (`Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp`、2026-10-28 時点。ずれていたら関数名で探す):
  - `FEditorToolkit::PostRegenerateMenusAndToolbars` が overlay を組み、`SetMenuOverlay` する。名前は `.Text( FText::FromName( ClassGeneratedBy->GetFName() ) )` の値渡し。2 つのボタンも `ClassGeneratedBy` を lambda に捕まえる。ネイティブ schema なら `FEditorClassUtils::GetSourceLink( SchemaClass )`。Schema が null なら overlay を作らずに return する。
  - `PostRegenerateMenusAndToolbars` はエンジンの `FAssetEditorToolkit::RegenerateMenusAndToolbars` の末尾で呼ばれる。toolkit 作成時の 1 回だけで、SchemaClass が変わっても呼ばれない。
  - SchemaClass 変更の通知先: `OnAssetFinishedChangingProperties` → `HandleSchemaClassChanged` (詳細パネルの変更)、`RebindSchemaDelegatesIfChanged` (Undo / Redo、095d が足した)。
  - `FAssetEditorToolkit::RegenerateMenusAndToolbars()` と `SetMenuOverlay` は public (UE 5.8 / 5.7 両方の `AssetEditorToolkit.h`)。
- 注意: `RebindSchemaDelegatesIfChanged` は schema の Blueprint を比べる。ネイティブ schema 同士の切り替えは Blueprint が両方 null なので「変化なし」になる。表示の更新をここに載せるなら、比較を schema クラスでもするか、表示用に別に比べる。

## Scope
- overlay を組む処理を関数に切り出し、SchemaClass が変わったとき (詳細パネルの変更と Undo / Redo) に組み直す。
- Schema が null から非 null になったときも overlay を出す。非 null から null なら消す。

## Out of Scope
- overlay の見た目・文言の変更。
- `HandleSchemaClassChanged` の不整合チェック (`ResolveStructureMismatch`) の挙動。

## Preconditions
- t261028-095d が done (済)。
- 作業ブランチ `fix/audit-2026-10` (両 repo)。

## Steps
1. overlay の組み立てを `FEditorToolkit` の関数 (例 `RefreshSchemaOverlay`) に切り出す。`PostRegenerateMenusAndToolbars` はそれを呼ぶ。
2. `HandleSchemaClassChanged` と `RebindSchemaDelegatesIfChanged` (schema が変わったとき) から呼ぶ。ネイティブ schema 同士の切り替えも拾えるように比較を見直す。
3. Schema が null のときは空の overlay にする。
4. ビルド、quick。
5. GUI で Verification。

## Acceptance Criteria
- Repository の Schema Class を別の schema に変えると、エディタを開き直さずに右上の名前が新しい schema 名になる (現在: 開いたときの名前のまま)。
- その後 Ctrl+Z で、名前が元の schema 名に戻る。
- 変えた後の「エディタで開く」ボタンが新しい schema を開く。

## Tests
- 自動テストなし。Tests 4 問: 守るのはメニュー overlay の表示。toolkit の overlay を読む口はテストから無く、足すと本番が要らない口になる。
- quick: ビルド + `DataIndexerEd.Repository`。

## Verification
- `KNOWLEDGE.md`「UE MCP GUI ハーネス」の手順で行う。Schema Class ピッカーは RowStruct が同じ schema しか出さない。別の RowStruct の schema へ変えるときは右クリック → ペーストで、リセット確認の modal の前に `bgyes.ps1` を background で走らせる。Undo は Cmd 欄の `TRANSACTION UNDO`、前に `end_transaction()` が 0 を確かめる。
- 1. 一時 Repository を開き、右上のスクリーンショット。
- 2. Schema Class を変えた後のスクリーンショット (名前が変わる)。「エディタで開く」を押し、開いた schema を確かめる。
- 3. `TRANSACTION UNDO` 後のスクリーンショット (名前が戻る)。
- 一時アセットは終わりに消す。証跡は `.agents/tasks/evidence/t261028-490c/`。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし
