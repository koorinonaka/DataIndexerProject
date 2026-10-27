---
status: ready
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
- `FViewModel::PostUndo/PostRedo` と `FEditorUndoClient` 継承を削除する (EditorData が broadcast すれば不要)。
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
3. `DataIndexerViewModel.h/.cpp` から `FEditorUndoClient` 継承と `PostUndo/PostRedo` を削除する。
4. `DataIndexerViewModel_Visibility.cpp` `UpdateVisibleRows` の末尾で `RebuildSelectionBounds()` を無条件に呼ぶ。
   既存の「個数が変わった時だけ SetSelection」分岐はそのまま残す (選択集合の縮小はそこが担う)。
5. `DataIndexerEd.EditorData` / `DataIndexerEd.ViewModel` 群の自動テストに回帰行を足す (Tests 参照)。

## Acceptance Criteria
- `rg 'PostEditUndo' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorData.cpp` が 1 件以上 (現在 0)。
- `rg 'FEditorUndoClient' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` が 0 件 (現在 1)。
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

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
