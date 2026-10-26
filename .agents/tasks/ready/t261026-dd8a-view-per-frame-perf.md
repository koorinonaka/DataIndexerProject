---
status: ready
order: 120
tier: standard
screen: required
agent: any
created_by: task-create
---

# Data View の毎フレーム線形探索・AssetRegistry 問い合わせ・Details 全再構築を消す

## Goal
OnPaint / TAttribute / マウス移動ごとに走る O(rows) 探索、AssetRegistry `GetReferencers`、Details `ForceRefresh`、
O(rows²) のソート・フィルタを、マップとキャッシュで定数時間にする。

## Context
監査 `.agents/audit-2026-10-26.md` の P-V3, P-V4, P-V6, P-V7, P-V9, P-V10, P-V12, P-V13, P-V14。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/` 相対、行番号は 2026-10-26 時点。

- P-V3: `DataIndexerViewModel.cpp:43-70`, `DataIndexerViewModel_Selection.cpp:118-129` `RowsIndexOf` / `FindRowEntry` / `FindColumnEntry` /
  `GetColumnEntryIndex` が線形。呼び手がセル毎・フレーム毎: `SDataIndexerViewCell.cpp:563,599` OnPaint → `GetSelectionEdgeFlags` /
  `GetFillPreviewEdgeFlags` (→ `_FillHandle.cpp:342-358` `IsFillPreviewRow` 線形); `SDataIndexerViewCell.cpp:136-151` `GetCellTextColor` →
  `SDataIndexerViewRow.cpp:268-290` `GetRowTextColor` → `IsCommentOut` → `EditorData->FindRow` + `Algo::AnyOf(RowSelection)` + `GetOverridden` →
  `FindRowEntry` → `IsOverridingParentRow` (`_Visibility.cpp:44-58` 線形); `SDataIndexerViewRow.cpp:89-122` border lambda → `FindRow`×2/行/フレーム。
  Fix: `UpdateVisibleRows` / `RefreshRows` / `RefreshColumns` で `TMap<FRepositoryKey,int32> VisibleIndexByKey`、`TMap<FName,int32> ColumnIndexByName`
  を構築 (`RowEntryByKey` は t261026-bd46 が作る)。`FRowEntry` に `bOverridesParent` / `bCommentOut` をキャッシュ (`OnRowsUpdated` で更新)。
- P-V4: `SDataIndexerViewHeader.cpp:365` `.ManualWidth(ViewModel, &FViewModel::GetColumnWidth, Name)` TAttribute → `STableRow.h:1442` で各行セルが
  bind → 可視行×列×フレームで `GetColumnWidth` (`DataIndexerViewModel.cpp:137-157`: 線形 Find + `SchemaLayout->GetColumnWidth` →
  settings の `TMap<FSoftClassPath,…>.Find` 文字列ハッシュ)。`FColumnEntry::ResolvedWidth` にキャッシュ、`HandleColumnWidthChanged` /
  `ResetSchemaLayout` / `SetViewportWidth` 変化時のみ再計算。
- P-V6: `SDataIndexerSelectionDetails.cpp:200` `HandleSelectionChanged` が毎回 `ForceRefresh`。`OnSelectionChanged` は `ExtendRangeSelection` の各セル進入
  (`SDataIndexerViewCell.cpp:223`)、`ExtendFillHandleDrag` の各行 (`_FillHandle.cpp:170,190,233`)、Toolkit `HandleModelRowsChanged(true)`
  (`DataIndexerEditorToolkit.cpp:461-477`) でも発火。フィルプレビュー変化を専用デリゲートに分離、Details 更新は `EndRangeSelection` / 次 Tick に遅延。
- P-V7: `DataIndexerViewModel_Visibility.cpp:132-153, 85-86, 160, 175` Sort 比較子が `GetRootDefinitionOrder` (線形) を比較ごと 2 回 → O(rows² log rows)。
  `:126-130` の `RowOrderByKey` は tie-break にしか使っていない。フィルタ述語の `IsOverridingParentRow` 線形 → O(rows²)。選択フィルタ
  `VisibleRows.ContainsByPredicate` → O(sel×rows)。`FirstIndexByPrimaryKey` を 1 パスで作り比較子で使う。
- P-V9: `SDataIndexerViewCell.cpp:780-785` ReferenceViewer 列の `IsEnabled` lambda が `IsRowReferred` → AssetRegistry `GetReferencers`
  (`DataIndexerViewTypes.cpp:477-487`) を可視行×フレーム。既定表示の列。`FRowEntry::bReferred` をキャッシュ (`BuildRows` / `OnRowsUpdated` で更新)。
  `_Visibility.cpp:96-97` の `bShowOnlyUnreferredRows` も同キャッシュ。
- P-V10: `SDataIndexerViewFillHandle.cpp:158-174, 60-156` OnPaint 毎フレーム `IsMouseOverSelectedRow` → `ComputeHandleLocalPos` → `GetFillHandleCellKey`
  (`_Selection.cpp:339-404`: `CanFill` → `FindRowEntries` O(rows) 確保) + `GetColumnRangeBoundsLocal` (列 2 パス)。`SDataIndexerView.cpp:291,374,408`
  がマウス移動ごとに同じ計算。OnPaint で求めた HandleCenter を mutable キャッシュ (ヘッダのコメントが既に「1 フレーム遅れ許容」)。
- P-V12: `SDataIndexerViewHeader.cpp:85, 123-124` OnPaint で列ごとに `IsColumnActive` (`_Selection.cpp:85-95`、セル選択全走査)×2。
  OnPaint 冒頭で `TSet<FName> ActiveColumns` を 1 回。
- P-V13: `DataIndexerViewModel_Visibility.cpp:107-120` フィルタ述語で行×列×2 の `FText::ToString()` を打鍵ごと。`FCellEntry` に検索用 FString。
- P-V14: `DataIndexerViewModel_RowOps.cpp:120, 133` `DeleteRow` が毎回 `CollectRepositoriesIncluding` (候補 Repository を `GetAsset()` でロード、
  `DataIndexerViewTypes.cpp:564`) + 行ごと `IsRowReferred`。結果を Toolkit 寿命でキャッシュ、AssetRegistry 更新で無効化。

## Scope
上の 9 項目。マップ 2 つと `FRowEntry` のキャッシュ 3 フラグ、`FColumnEntry::ResolvedWidth`、`FCellEntry::SearchText`。

## Out of Scope
- 差分更新そのもの (t261026-bd46)。
- EditorData 側のマップ (t261026-ba24、done 済みの前提)。
- 責務整理 (t261026-ed01)。関数の置き場は現状のまま。

## Preconditions
- t261026-bd46 が done (`RowEntryByKey` と `FRowEntry` 再利用を前提にキャッシュを置く。`SDataIndexerViewCell` paint を両方が書き換える)。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. `FViewModel` に `VisibleIndexByKey` / `ColumnIndexByName` を足し、`RowsIndexOf` / `FindColumnEntry` / `GetColumnEntryIndex` / `GetTableColumnIndex` をマップ参照に。
2. `FRowEntry` に `bOverridesParent` / `bCommentOut` / `bReferred` を足し、`BuildRows` と `OnRowsUpdated` で埋める。`GetRowTextColor` /
   `IsOverridingParentRow` / `IsRowReferred` の呼び手をフラグ参照に。
3. P-V7 → Sort 比較子と フィルタ述語。P-V13 → `FCellEntry::SearchText`。
4. P-V4 → `FColumnEntry::ResolvedWidth`。P-V12 → `ActiveColumns`。P-V10 → HandleCenter キャッシュ。
5. P-V6 → `OnFillPreviewChanged` デリゲートを分離し、`SSelectionDetails` は `OnSelectionChanged` を次 Tick にまとめる。
6. P-V14 → Toolkit に `CachedReferencingRepositories` + AssetRegistry `OnAssetAdded/Removed` で無効化。
7. `DataIndexerEd.ViewModel` を回し、GUI で Verification。

## Acceptance Criteria
- `rg 'IndexOfByPredicate|FindByPredicate|ContainsByPredicate' Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerViewModel*.cpp` が
  現在値より減り、`RowsIndexOf` / `FindColumnEntry` / `GetColumnEntryIndex` の実装に残らない。
- `SDataIndexerViewCell::OnPaint` から `EditorData->FindRow` / `GetReferencers` への呼び出し経路が無い (静的に `rg` + 読みで確認、現在: あり)。
- 範囲選択ドラッグ中に `SSelectionDetails::ForceRefresh` が 1 セル進入ごとに呼ばれない (ログカウンタ、現在: 毎セル)。
- 1000 行の Repository で `UpdateVisibleRows` が `GetRootDefinitionOrder` を行数以上呼ばない (カウンタ、現在: O(n² log n))。

## Tests
- `DataIndexerViewModelTests.cpp` に行: `RowsIndexOf` がマップ経由で `VisibleRows` の index と一致、`bOverridesParent` が `OnRowsUpdated` 後に更新される。
  修正前コードでも通る行は足さない (構造の AC は `rg` で見る)。
- quick: `DataIndexerEd.ViewModel`。

## Verification
- 1000 行 Repository で `stat Slate` / `stat Game` の before/after スクリーンショット (範囲選択ドラッグ中、フィルタ入力中)。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
