---
status: done
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
- 済 (2026-10-29 verify sweep): 1〜3 すべて。

## Execution Log
- 2026-10-28 実装 (dispatch 3/3、ブランチ `fix/audit-2026-10`):
  - `PostRegenerateMenusAndToolbars` の overlay 組み立てを `FEditorToolkit::RefreshSchemaOverlay()` に切り出した。`PostRegenerateMenusAndToolbars` はそれを呼ぶだけ。
  - `RefreshSchemaOverlay` は組んだ schema クラスを `OverlaySchemaClass` (`TWeakObjectPtr<const UClass>`) に記録する。Schema が null なら `SetMenuOverlay( SNullWidget::NullWidget )` で overlay を消す (以前は return するだけで古い overlay が残り得た)。
  - `HandleSchemaClassChanged` (詳細パネルの変更) の末尾で `RefreshSchemaOverlay()` を呼ぶ。
  - `RebindSchemaDelegatesIfChanged` (Undo / Redo) は、Blueprint 比較の early return の前に、現在の schema クラスと `OverlaySchemaClass` をクラスで比べ、違えば `RefreshSchemaOverlay()` を呼ぶ。ネイティブ schema 同士の切り替え (Blueprint が両方 null) もこれで拾う。delegate の rebind 判定 (Blueprint 比較) は変えていない。
  - `SetMenuOverlay` はエンジン側で `MenuOverlayWidgetContent->SetContent` による置き換え。UE 5.8 / 5.7 の `SStandaloneAssetEditorToolkitHost.cpp` で同じ実装を確認した。StandaloneHost が未生成の時期 (InitEditor 中) は no-op で、その後の `PostRegenerateMenusAndToolbars` が無条件に組み直す。
- quick: `Build.bat DataIndexerProjectEditor Win64 Development` → `Result: Succeeded` (30s、1 回、C1001 なし。`UnrealEditor-DataIndexerEd.dll` 21:29:46 更新)。`Automation RunTests DataIndexerEd.Repository` → Success 3 / Fail 0、`EXIT CODE: 0`。full は run 終端。
- 自動テストなし (Tests の 4 問どおり: overlay を読む口が無く、足すと本番が要らない口になる)。
- verify_pending の理由: ユーザーが PC を使用中で、dispatch 指示により GUI 検証 (UE MCP SlateInspector が実カーソルを動かす) を禁止されている。Verification 1-3 が残る。
- 自己レビュー: `git diff --stat` = `DataIndexerEditorToolkit.cpp` +17/-4、`.h` +3。どちらも Scope の対象ファイル。

- critic r1 (task-critic):
  ```
  PASS
  NOTE: DataIndexerEditorToolkit.cpp:606 — AC 1-3 は diff だけでは確かめられない (GUI 検証が要る)。配線は揃っている: HandleSchemaClassChanged が RefreshSchemaOverlay() を呼ぶ (630)。Undo/Redo は OnAssetChanged → RebindSchemaDelegatesIfChanged (245) がクラスを比べて RefreshSchemaOverlay() を呼ぶ。ボタンの lambda は組み直しごとに新しい ClassGeneratedBy を捕まえる (166, 189)。
  NOTE: DataIndexerEditorToolkit.cpp:117 — RefreshSchemaOverlay は Repository->GetSchema()、602 は EditorData->GetSchema() を使うが、後者は GetRepository().GetSchema() を返すだけで同じ値。
  NOTE: diff が消すのは check( SchemaClass ) だけ。新しい名前は両方使われている。Out of Scope (ResolveStructureMismatch、overlay の見た目・文言) は触っていない。
  ```

- verify sweep 2026-10-29 05:22 (ALICE、合格 → done):
  - 環境: エディタは HEAD のビルド (UnrealEditor-DataIndexerEd.dll 2026-10-28 21:29、submodule c9ca5c7 の木)。UE MCP を `evidence/_harness/mcp.py` で叩いた。ユーザーは不在で、実マウスの操作は dispatch で許可済み。証跡は `evidence/t261028-490c/` (`verify.txt` と 01〜04)。
  - 一時アセット: `/Game/__Verify/DIS_VItem2` (DIS_Item の複製。RowStruct が同じ ItemRow なので、ピッカーに出てリセットの modal も出ない) と、`/Game/__Verify/DI_V490` (schema DIS_Item_C)。終わりに消した。
  - V1: 開いた直後の右上は `Schema: DIS_Item` だった (01)。
  - V2 / AC1・AC3: Asset Details の Schema Class ピッカーで、DIS_VItem2 を実マウスで選んだ。エディタを開き直さずに、右上が `DIS_VItem2` になった (02)。続けて右上の 2 つ目のボタン (エディタで開く) を押した。`GetOpenAssets` に `/Game/__Verify/DIS_VItem2` が増え、DIS_VItem2 の BP エディタが開いた (03)。
  - V3 / AC2: Repository のタブに戻り、`end_transaction()` が 0 であることを確かめた。Cmd 欄で `TRANSACTION UNDO` を打った (`Undo Schema Class を編集`)。右上は `DIS_Item` に戻った (04)。

## Summary
Repository エディタ右上の「Schema: <名前>」overlay を `RefreshSchemaOverlay()` に切り出し、詳細パネルでの SchemaClass 変更と Undo / Redo (クラス比較、ネイティブ同士も可) で組み直すようにした。Schema が null なら overlay を消す。ビルドと `DataIndexerEd.Repository` は緑。2026-10-29 の verify sweep で GUI 検証に合格した (変更で名前が変わる・Undo で戻る・「エディタで開く」が新しい schema を開く)。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.h`
- `.agents/tasks/evidence/t261028-490c/` — verify sweep の証跡

## Blockers
None
