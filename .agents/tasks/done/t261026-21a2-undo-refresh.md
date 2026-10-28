---
status: done
order: 10
tier: standard
screen: required
agent: any
created_by: task-create
---

# Undo/Redo 後に Data View・派生 toolkit・選択範囲を更新する (X6)

## Goal
Ctrl+Z / Ctrl+Y の後、開いている Data View・派生 Repository の toolkit・`UDataIndexerDrivenCollection` が
行の変化を反映し、セル選択範囲 (`SelectionBounds`) が現在の行順に追随する。

## Context
監査 `.agents/audit-2026-10-26.md` の X6 = B-E1 + B-V2 + R-E2。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/` 相対。行番号は 2026-10-26 時点 (`release/v1.0.2`)。ずれていたら関数名で探す。

- B-E1: `DataIndexerEd/Private/Views/DataIndexerViewModel.cpp:18-26` の `FViewModel::PostUndo/PostRedo`
  (`FEditorUndoClient`) は、誰も `GEditor->RegisterForUndo` を呼ばないので死んでいる (`rg RegisterForUndo` 0 件、
  2026-10-26 確認)。`UDataIndexerEditorData` に `PostEditUndo` override が無い (`rg PostEditUndo` は K2Node 2 クラスのみ)。
  undo 経路は `UObject::PostEditUndo → PostEditChange → PostEditChangeProperty(null)` を通り、
  `DataIndexerEd/Private/DataIndexerEditorData.cpp:92` の `OnEditorRowsUpdated` broadcast に届かない。
  `:87-89` のコンパイル済みキャッシュ無効化は無条件なので動いている。
- R-E2: `DataIndexerEditorData.cpp:83-111` で `PostEditChangeProperty` と `NotifyPostChange` (`FNotifyHook`) が
  両方 dirty + broadcast を書いている。`PostEditUndo` が 3 つ目の呼び手になるので、内部 `NotifyRowsChanged()` に
  まとめてから 3 箇所がそれを呼ぶ (advisor 判断: 同じ関数を触るこのタスクで統合する)。
- B-V2: `DataIndexerEd/Private/Views/DataIndexerViewModel_Visibility.cpp:155-184` の `UpdateVisibleRows` は、
  選択「個数」が変わった時しか `SetSelection → RebuildSelectionBounds` を呼ばない。Undo・親アセット変更・ソートで
  `VisibleRows` の順序だけ変わると `SelectionBounds` (行 index) が古いまま → `CopyCells`
  (`DataIndexerViewModel_Clipboard.cpp:155-162`)、`PasteCells` (`:432-440`)、`FillDown`
  (`DataIndexerViewModel_FillHandle.cpp:450-455`) が `VisibleRows[Bounds]` で別の行を読み書きする。
- 派生 toolkit への配線は `DataIndexerEd/Private/DataIndexerEditorToolkit.cpp:564` (`OnEditorRowsUpdated` 購読) に既にある。
  broadcast さえ出れば届く。

## Scope
- `UDataIndexerEditorData::PostEditUndo()` を追加し、キャッシュ無効化 + `OnEditorRowsUpdated` broadcast を行う。
- `PostEditChangeProperty` / `NotifyPostChange` / `PostEditUndo` が共通の private `NotifyRowsChanged()` を呼ぶ形にする。
- `FViewModel` の undo client (`FSelfRegisteringEditorUndoClient` 継承と `PostUndo/PostRedo`) は残す。自分の Data View を undo 後に更新する唯一の経路 (Execution Log の advisor 判定参照)。
- `FViewModel::UpdateVisibleRows` の末尾で無条件に `RebuildSelectionBounds()` を呼ぶ。

## Out of Scope
- 行ルックアップのマップ化 (t261026-ba24)。`PostEditUndo` にマップ再構築を足すのはそちら。
- View の差分更新 (t261026-bd46)。
- JSON import の通知経路 (t261026-25fc)。`NotifyRowsChanged()` の名前だけ揃える。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み (README 品質基準)。
- 作業ブランチを両 repo で `main` から切る。

## Steps
1. `DataIndexerEditorData.h` に `virtual void PostEditUndo() override;` と private `void NotifyRowsChanged();` を宣言する。
2. `DataIndexerEditorData.cpp` で `NotifyRowsChanged()` を実装 (現在の `PostEditChangeProperty` の dirty + broadcast 部分を移す)。
   `PostEditChangeProperty` / `NotifyPostChange` / `PostEditUndo` からそれを呼ぶ。`PostEditUndo` は `Super::PostEditUndo()` の後。
3. `DataIndexerViewModel.h/.cpp` の undo client は触らない (Scope 参照)。
4. `DataIndexerViewModel_Visibility.cpp` `UpdateVisibleRows` の末尾で `RebuildSelectionBounds()` を無条件に呼ぶ。
   既存の「個数が変わった時だけ SetSelection」分岐はそのまま残す (選択集合の縮小はそこが担う)。
5. `DataIndexerEd.EditorData` / `DataIndexerEd.ViewModel` 群の自動テストに回帰行を足す (Tests 参照)。

## Acceptance Criteria
- `rg 'PostEditUndo' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorData.cpp` が 1 件以上 (現在 0)。
- `rg 'FSelfRegisteringEditorUndoClient' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel.h` が 1 件 (自分の Data View の undo 経路として残す)。
- Data View で行を削除 → Ctrl+Z で行が戻り、派生 Repository の toolkit も同時に行が戻る (現在: 戻らない)。
- 行を Move → セル範囲選択 → Ctrl+Z → Ctrl+C した内容が、見えている選択セルと一致する (現在: 別行が入る)。

## Tests
- `DataIndexerViewModelTests.cpp` に行を足す: `VisibleRows` の順序だけ変える操作の後、`GetSelectionBounds()` が
  選択セルの現在 index と一致する。修正前のコードで落ちることを確かめる。
- `DataIndexerEd.EditorData` 群: `PostEditUndo()` 呼び出し後に `OnEditorRowsUpdated` が 1 回発火する。
- quick: `DataIndexerEd.EditorData` + `DataIndexerEd.ViewModel`。

## Verification
- エディタで親 Repository と派生 Repository を両方開き、親で行削除 → Ctrl+Z。両 Data View のスクリーンショット。
- Move → 範囲選択 → Ctrl+Z → Ctrl+C → テキストエディタに貼った内容のスクリーンショット。
- 済 (2026-10-28 sweep): 上の 2 項目。証跡 `.agents/tasks/evidence/t261026-21a2/` (01-04 削除と Undo、05-06 Move と範囲選択と Copy)。

## Execution Log
- 1 回目 (2026-10-26 run) は main の Editor ビルド破損 (sample schema と新 `IsRowValid` シグネチャ) で deferred。親 c58b690 で解消後に再 dispatch。
- PLAN-REPAIRABLE (advisor 判定、1 回目で取得): Context B-E1 「誰も `GEditor->RegisterForUndo` を呼ばない」は誤り。
  `FViewModel` は `FSelfRegisteringEditorUndoClient` を継承し (`Views/DataIndexerViewModel.h:19`)、その ctor が `RegisterForUndo(this)` を呼ぶ。
  `PostUndo → OnAssetChanged → SView::HandleModelAssetChanged` は生きている。`FEditorToolkit::HandleEditorDataUpdated`
  (`DataIndexerEditorToolkit.cpp:566-567`) は自 repository を飛ばすので、自分の Data View を undo 後に更新するのはこの経路だけ。
  削除すると自分の view が更新されなくなる。advisor: Scope 1/2/4 と Tests はそのまま、undo client は残す。
  書き換えた計画欄 (原文):
  - Scope 3: 「`FViewModel::PostUndo/PostRedo` と `FEditorUndoClient` 継承を削除する (EditorData が broadcast すれば不要)。」
  - Steps 3: 「`DataIndexerViewModel.h/.cpp` から `FEditorUndoClient` 継承と `PostUndo/PostRedo` を削除する。」
  - AC 2: 「`rg 'FEditorUndoClient' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` が 0 件 (現在 1)。」
- 実装:
  - `UDataIndexerEditorData::NotifyRowsChanged()` (private) = `MarkRepositoryDirty` + `OnEditorRowsUpdated` broadcast。
    `NotifyPostChange` と新規 `PostEditUndo()` (`Super::PostEditUndo()` の後) が呼ぶ。`PostEditChangeProperty` は
    `EditorRows` の時だけ `NotifyRowsChanged()`、それ以外は dirty のみ (broadcast 条件は従来どおり)。
    `PostEditUndo` 内で Super 経由の dirty が二重になるが無害。
  - `FViewModel::UpdateVisibleRows` の選択フィルタ後に無条件 `RebuildSelectionBounds()`。個数変化時の `SetSelection` 分岐は残した。
- テスト (Tests 4 問): (1) undo で rows broadcast が出る / 行順変化で `SelectionBounds` が追随する。(2) `PostEditUndo` 削除・
  `RebuildSelectionBounds` 呼び出し削除で落ちる。(3) 既存は undo 経路も並べ替え後の bounds も見ていない。(4) 追加の口なし
  (`PostEditUndo` は UObject 経由で呼ぶ = transaction system と同じ境界)。
  - 新規 `Tests/Core/DataIndexerEditorDataUndoTests.cpp`: `DataIndexerEd.EditorData.PostEditUndoBroadcastsRowsUpdated`。
  - `DataIndexerViewModelTests.cpp`: `DataIndexerEd.ViewModel.Selection.BoundsFollowRowOrder`。1 回目の下書きは SchemaClass 未設定で
    "Value" 列が作られず、対照 assert (並べ替え前の bounds) が落ちた。`UDataIndexerBuildIndexSchemaTests` と行ごとに別の値 (1/2/3) を
    与えて修正。
- 修正前の赤 (テストのみ適用、修正前ソース): `PostEditUndoBroadcastsRowsUpdated` Fail「expected 1, was 0」。
  `BoundsFollowRowOrder` Fail、対照 3 件は通過し並べ替え後の 2 件 (「Bounds follow the selected row after reorder」
  「Row now at the old index is not selected」) だけが落ちた。
- 修正後 quick: `Automation RunTests DataIndexerEd.EditorData+DataIndexerEd.ViewModel` → Found 11、Success 11 / Fail 0、EXIT CODE 0。
- 参考に全群 `Automation RunTests DataIndexerEd` → Found 56、Success 55 / Fail 1。落ちたのは
  `DataIndexerEd.Migration.FailedImportKeepsExistingRows` (「Malformed import should fail」)。対照: submodule を stash したツリーで
  同テストを実行 → Fail (54 件中 53 Success / 1 Fail)。このタスクの変更とは無関係。対象は t261026-25fc (JSON import atomic)。
- ビルド: Editor ターゲット。途中 2 回 MSVC 内部コンパイラエラー (C1001、`SharedPointer.h` / `STableRow.h`) → 再実行で成功。一過性。
- fold (軽微、ソース以外): `.agents/audit-2026-10-26.md` の X6 / B-E1 行の「`RegisterForUndo` 未呼び出し」を訂正 (2 行)。
- 既定 (可逆): undo client は現状維持 (advisor 判定)。変更案は run 終端の 要ユーザー判断 として報告。
- 自己レビュー: `git diff --stat` は Scope の 4 ソース + テスト 2 + task/audit のみ。Views/ の undo client は無変更。
- 検証待ち: `screen: required` の GUI 2 項目。UE MCP server (`127.0.0.1:8001`) が ECONNREFUSED (エディタ未起動) で、この dispatch では
  エディタ操作とスクリーンショットができない。
- critic round 1: PASS
  ```
  PASS
  NOTE: t261026-21a2-undo-refresh.md:40,56,63 — The author rewrote Scope 3, Steps 3 and Acceptance Criterion 2 inside this same diff. The old AC asked for `FEditorUndoClient` to have 0 hits under Views/. The new AC asks for `FSelfRegisteringEditorUndoClient` to have 1 hit. The diff meets the rewritten AC (`Views/DataIndexerViewModel.h:19`). It does not meet the original one. The reason for the change is in the Execution Log, but the user has not approved the rewritten plan.
  NOTE: t261026-21a2-undo-refresh.md:64-65 — AC 3 and AC 4 can only be checked in the editor. The diff cannot show them. The GUI verification is still pending.
  NOTE: DataIndexerEditorData.cpp:97-102 — `PostEditUndo` broadcasts `OnEditorRowsUpdated` on every undo/redo of this object, including undo of non-row properties. The repository is also marked dirty twice there (once through Super). Both are harmless; this is not a failure.
  NOTE: DataIndexerEditorData.cpp:83-95 — `PostEditChangeProperty` behaves the same as before: dirty always, broadcast only for `EditorRows`. Removed-name sweep: no identifier was dropped. `NotifyPreChange` / `NotifyPostChange` were only moved. The new `NotifyRowsChanged` has 3 callers (lines 89, 101, 121), and `RebuildSelectionBounds` keeps its existing callers. Nothing listed in Out of Scope is touched.
  ```

### ユーザー決定 2026-10-28
- undo client は現状維持。`FViewModel` の `FSelfRegisteringEditorUndoClient` 継承と `PostUndo/PostRedo` は残す。
  実行者が書き直した Scope 3 / Steps 3 / AC 2 (critic round 1 の 1 つ目の NOTE) をユーザーが承認した。status は変えない。

### verify sweep 2026-10-28 16:55 (環境不可、verify_pending 継続)
- エディタ (PID 9372) は起動中で、UE MCP (127.0.0.1:8001) に接続できた。`UnrealEditor-DataIndexerEd.dll` (15:50) はエディタ起動 (16:34) より前のビルドなので、HEAD の実装が載っている。
- ユーザーが作業中だった (最終入力 0.8 秒前、前面は Chrome リモート デスクトップ)。dispatch の条件は「ユーザーの作業を妨げない」。
- SlateInspectorToolset の Click / Hover / Drag は `FSlateApplication::SetCursorPos` で実カーソルを動かす (`SlateInspectorToolset.cpp:115, 317, 589`)。PressKey / Type は Slate へキーを注入する。`OpenEditorForAsset` はエディタのウィンドウを前面に出す恐れがある。
- Verification の 2 項目は Data View を開き、行削除・Ctrl+Z・範囲選択・Ctrl+C を要する。上の制限の下では実施できないので、未実施のまま残した。

### verify sweep 2026-10-28 18:44 (ALICE、合格 → done)
- 環境: エディタ PID 9372 (15:50 ビルド = submodule 0f651e4)、UE MCP 127.0.0.1:8001。GUI 操作はユーザー許可済み (dispatch 2026-10-28「今からやって」)。一時アセットは `/Game/__Verify` に作り、終了時に削除した。証跡は `.agents/tasks/evidence/t261026-21a2/`。
- 一時アセット: DI_Ability → DI_VAbility、DI_Ability_Mage → DI_VAbility_Mage (ParentRepositories を DI_VAbility に差し替え)。
- AC3: 親で Defence 行を Delete → 親は Attack / Heal、派生からも Defence が消えた (01, 02)。親で Ctrl+Z → 親に Defence が戻った (03)。派生タブにも Defence が戻った (04)。
- AC4: 親で Attack を Ctrl+J で 1 つ下へ (Defence / Attack / Heal)。Attack 行の Description..AbilityType を Ctrl+クリック → Shift+クリックで矩形選択 (05)。Ctrl+Z で行順が Attack / Defence / Heal に戻り、選択枠も Attack 行 (行 1) に移った (06)。Ctrl+C の中身は Attack のセル ("A basic physical attack." / NewEnumerator0、06-clipboard-after-undo.txt)。旧 index の行 (Defence) ではない。テキストエディタへの貼り付けの代わりに、クリップボード本文をファイルに残した。
- 限界: 2 つの Data View は同じウィンドウのタブで、同時には表示していない。派生はタブを切り替えて撮った。タブ有効化で view を作り直すコードは無い (`rg OnTabActivated` 0 件)。
- 所要 約 6 分。

## Summary
`UDataIndexerEditorData::PostEditUndo` を追加し、undo/redo で `OnEditorRowsUpdated` を broadcast するようにした
(派生 toolkit・DrivenCollection が追随)。dirty + broadcast は `NotifyRowsChanged()` に集約。`UpdateVisibleRows` が毎回
`SelectionBounds` を再構築するので、行順だけ変わっても Copy/Paste/FillDown が正しい行を指す。`FViewModel` の undo client は
自分の Data View の更新経路なので残した。GUI 検証が残り verify_pending。
GUI 検証は 2026-10-28 の verify sweep で合格した。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Public/DataIndexerEditorData.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorData.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel_Visibility.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerEditorDataUndoTests.cpp` (新規)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/ViewModel/DataIndexerViewModelTests.cpp`
- `.agents/audit-2026-10-26.md` (fold: X6 / B-E1 訂正)

## Blockers
None
