---
status: done
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
残り (GUI。UE MCP 127.0.0.1:8001 がこの run で停止していて実行できない):
- 1000 行 Repository で `stat Slate` / `stat Game` の before/after スクリーンショット (範囲選択ドラッグ中、フィルタ入力中)。
- AC3: `log LogDataIndexer Verbose` を有効にして範囲選択ドラッグし、`SSelectionDetails: ForceRefresh #N` がドラッグ 1 回につき 1 行 (マウスを離した次の tick) だけ出ることを数える。
- fill handle: 選択行へのホバーでハンドルが出る。ハンドル上でカーソルが ResizeUpDown になる。ドラッグでプレビュー (背景と青枠) が出て、離すと消える (HandleCenter キャッシュと OnFillPreviewChanged 分離の目視)。
- CommentOut トグルで行の文字色・背景・Visibility チェックが次の tick で切り替わる (bCommentOut キャッシュ)。
- t261026-ba24 から引き継いだ Reload 経路の compiled-repository cache の GUI 確認。
- 済 (2026-10-28 verify sweep、ALICE): AC3、fill handle、CommentOut、Reload 経路。`stat Slate` の before は旧バイナリが無く撮れない (README「perf は構造の AC で足りる」、AC4 は静的に成立)。証跡 `.agents/tasks/evidence/t261026-dd8a/`。

## Execution Log
- 2026-10-28 13:17 着手。前提 t261026-bd46 は verify_pending (検証未完) の上に構築。`release/v1.0.2` は両 repo の main に merge 済み (`git merge-base --is-ancestor` で確認)。作業ブランチは両 repo とも `fix/audit-2026-10`。
- 着手前の基準: AC1 の `rg -c 'IndexOfByPredicate|FindByPredicate|ContainsByPredicate' DataIndexerViewModel*.cpp` = 11 (Visibility 4 / ViewModel 4 / RowOps 2 / Selection 1)。
  AC2 の「現在: あり」の経路: `SViewCell::OnPaint` 自体は `GetSelectionEdgeFlags` / `GetFillPreviewEdgeFlags` だけを呼ぶ。`FindRow` / `GetReferencers` は子の paint で評価される TAttribute から届いていた。経路は 3 つ: `GetCellTextColor → ViewTextColor::IsCommentOut → EditorData->FindRow`、SViewRow の border / foreground lambda (`FindRow` ×2/行)、ReferenceViewer 列の `IsEnabled` lambda → `IsRowReferred` → `GetReferencers`。
  `UDataIndexerEditorData::FindRow` は ba24 で既に O(1) (`RowIndexByKey`)。P-V3 の効果は、paint 経路から呼び出しそのものを消すこと。
- 実装 (9 項目):
  - P-V3: `FViewModel` に `VisibleIndexByKey` (UpdateVisibleRows で sort 後・選択フィルタ前に構築)、`ColumnIndexByName` (RefreshColumns で ApplySavedColumnOrder 後)、`TableColumnIndexByName` (SetTableColumnOrder) を追加。`RowsIndexOf` / `FindColumnEntry` / `GetColumnEntryIndex` / `GetTableColumnIndex` をマップ参照にした。`FindRowEntry` は bd46 の `RowEntryByKey.FindRef`。
    `FRowEntry` に `bCommentOut` / `bOverridesParent` / `bReferred` を追加し、`BuildRows` で埋める。bCommentOut は全行を毎回、bOverridesParent は全行の O(n) 後処理、bReferred はセルを再計算する行だけ。
    `IsOverridingParentRow` はフラグ参照。`ViewTextColor::IsCommentOut` を削除し、`SViewRow::IsCommentOut()` (フラグ参照) へ移した。SViewRow の border / foreground lambda、Visibility チェックボックス、`GetOverridden` もフラグ参照。
    `IsFillPreviewRow` は `FillPreviewRows` (TSet。FillPreviewCells と一緒に clear) の Contains。`CanFill` の `ArrayFrom` (全 RowEntries 走査 + 確保) は、選択キーの `RowEntryByKey.Contains` に置き換えた。`SourceRowHasAnyFlag` の `GetRows().FindByPredicate` は `FindRow` に置き換えた。
  - P-V4: `FColumnEntry::ResolvedWidth` (mutable TOptional。GetColumnWidth が遅延で埋める)。リセットは `HandleColumnWidthChanged` (その列)、`ResetSchemaLayout`、`SetViewportWidth` (値が変わったときだけ)。BuildColumns は毎回新しい entry を作るので、RefreshColumns 後は空から始まる。SchemaLayout の幅を書くのは `FViewModel::HandleColumnWidthChanged` だけ (`rg SetColumnWidth` で確認)。
  - P-V6: `OnFillPreviewChanged` デリゲートを分離した (ExtendFillHandleDrag / Commit / Cancel のプレビュー変化)。SViewCell と SFillHandleWidget が購読する。`SSelectionDetails::HandleSelectionChanged` は、範囲選択ドラッグ中なら active timer で保留し、ドラッグ終了後の tick で 1 回だけ `ApplySelection` する。ForceRefresh の前に `UE_LOG(LogDataIndexer, Verbose, "SSelectionDetails: ForceRefresh #%d")` のカウンタを置いた (AC3 用)。
  - P-V7: `FirstIndexByPrimaryKey` を 1 パスで作り、Sort 比較子で使う。`GetRootDefinitionOrder` は削除。フィルタ述語は `bOverridesParent` / `bReferred` を参照する。選択フィルタの `ContainsByPredicate` ×2 は `VisibleIndexByKey.Contains`。
  - P-V9: ReferenceViewer 列の `IsEnabled` と `bShowOnlyUnreferredRows` は `FRowEntry::bReferred` を参照する。
  - P-V10: `SFillHandleWidget::OnPaint` が `ComputeHandleLocalPos` を 1 回計算して `CachedHandlePos` (mutable) に置く。`GetHandleLocalPosForHitTest` (SView の OnMouseMove / OnCursorQuery / OnPreviewMouseButtonDown) はそれを返す。計算は表示判定の early return より前に置いた (ハンドル非表示中もホバー検出に位置が要るため)。
  - P-V12: `SViewHeader::OnPaint` 冒頭で `TSet<FName> ActiveColumns` を 1 回作る。呼び手が無くなった `FViewModel::IsColumnActive` は削除。
  - P-V13: `FCellEntry::SearchText` (DisplayName + 改行 + FilterText) をコンストラクタで作る。フィルタ述語はセルごとに 1 回の `Contains`。
  - P-V14: `FViewModel::CachedIncludingRepositories` (TOptional<TSet<TWeakObjectPtr>>)。AssetRegistry の `OnAssetAdded` / `OnAssetRemoved` / `OnAssetRenamed` でリセットする (ctor で AddRaw、dtor で RemoveAll)。置き場は Toolkit ではなく FViewModel: DeleteRow が FViewModel にあり、寿命は Toolkit と同じ。
- 既定で進めた選択 (どれも戻せる):
  1. DeleteRow の `IsRowReferred` は bReferred に置き換えず、live のまま。根拠: 削除という破壊操作の警告で、払うのは選択行数だけ。bReferred は他アセットの保存で参照が増えても、次の全再構築まで古い。変えるコスト: 1 行。
  2. `CachedIncludingRepositories` は AR の add/remove/rename でしか消えない。別の子 Repository の ParentRepositories をメモリ上で書き換えただけ (保存・追加なし) だと、「他アセットが override している」警告が次の AR イベントまで古い。削除は undo できる。変えるコスト: `FCoreUObjectDelegates::OnObjectPropertyChanged` で UDataIndexerRepository を見て Reset する数行。
  3. `bReferred` はセルを再計算する行だけ更新する (新規行・RecomputeKeys・全再構築 = 開いた時 / undo / 列集合の変化)。他アセットが新たに参照しても、ReferenceViewer ボタンと「未参照のみ」フィルタは次の全再構築まで古い (以前は毎フレーム live)。全行を毎回更新すると、1 編集ごとに行数回の GetReferencers が走る。
  4. SSelectionDetails の保留は「範囲選択ドラッグ中だけ」にした。Steps の「次 Tick にまとめる」を全変更には広げなかった。根拠: Details は行メモリを指す FStructOnScope を持つ。行の追加・削除・undo の直後に旧ポインタを 1 tick でも残すのは危険。Toolkit の `HandleModelRowsChanged(true)` は `bImmediate=true` で即時適用する。
  5. `ResolvedWidth` は、同じスキーマの SchemaLayout を別エディタが書き換えても無効化されない (次の列再構築かビュー幅の変化まで古い)。
- Tests 4 問 (新規 `DataIndexerEd.ViewModel.BuildRows.CachedFlagsFollowEditorData`):
  1. 守る振る舞い: CommentOut / override の状態が、別の行だけを RecomputeKeys に入れた部分再構築でも FRowEntry のフラグに反映される。
  2. 落ちる回帰: フラグ更新を `bComputeCells` の内側へ入れる (RecomputeKeys 外の行が古いまま残る)。または後処理を落とす。
  3. 既存が取りこぼす理由: `OverrideRows.CannotMove` (L303) は全再構築直後の IsOverridingParentRow だけを見る。CommentOut のキャッシュと部分再構築は、どのテストも見ていない。
  4. 新しい口は不要 (`FindRowEntry` / `ToggleRow` / `RefreshRows(&Keys)` には本番の呼び手がある)。
  Tests 節の「RowsIndexOf がマップ経由で VisibleRows の index と一致」の行は足さなかった。修正前でも通る行で、既存の `MoveRows.MaintainsRelativeOrder` (L487-488)・`OverrideRows.MaintainDefinitionOrder`・`RowOps.DeleteRowReportsIndexBeforeRemove` (L728)・`Selection.BoundsFollowRowOrder` が、並べ替え・移動・削除後の RowsIndexOf を既に検査している (同じ節の「修正前コードでも通る行は足さない」に従った)。
- quick gate (`Build.bat DataIndexerProjectEditor Win64 Development` + `Automation RunTests`):
  - build 1 (9 項目): Succeeded 23.2s。`DataIndexerEd.View` → Success=15 Fail=0 (新テストを含む)。
  - hollow-out の対照 (bCommentOut の代入を `continue` の後へ移し、bOverridesParent を常に false にした): build 23.0s、`DataIndexerEd.View` → Fail=2 Success=13。新テストは予測した 3 assert だけが落ちた (L568 Shared child overrides / L574 CommentOut outside RecomputeKeys / L583 New overriding row)。もう 1 件は既存の `OverrideRows.CannotMove` (L305)。
  - 復元後の build 22.9s、`DataIndexerEd.View+DataIndexerEd.Repository` → Success=18 Fail=0。
  - full は run 終端。Game target は不要 (DataIndexerEd の Private だけを変更し、runtime 構造体に触れない)。
  - UE 5.7: `IAssetRegistry::Get()` と `OnAssetRenamed()` が 5.7 の `IAssetRegistry.h` にもある (L266, L937) ことを確認した。
- Acceptance Criteria:
  - AC1: `rg 'IndexOfByPredicate|FindByPredicate|ContainsByPredicate' DataIndexerViewModel*.cpp` は 11 → 2。残りは RowOps.cpp の MoveRow 内 `ThisEntries.IndexOfByPredicate` ×2 で、対象の関数ではない。`RowsIndexOf` / `FindColumnEntry` / `GetColumnEntryIndex` はマップ参照。満たす。
  - AC2: `rg 'FindRow\(|IsRowReferred|GetReferencers|IsCommentOut\(' SDataIndexerViewCell.cpp SDataIndexerViewRow.cpp DataIndexerViewVisuals.cpp` の残りは 3 箇所: SViewCell L974 (inline 編集の provider 生成)、SViewRow L105 (RefreshRowEntityProvider = セル再構築時)、L337 (GetProviderForColumn = ウィジェット生成時)。どれも paint で評価される TAttribute ではない。IsCommentOut は FRowEntry のフラグ参照。満たす (静的)。
  - AC3: 構造は実装済み (ドラッグ中は保留、ログカウンタあり)。ドラッグでの実測は GUI なので verify_pending。
  - AC4: `GetRootDefinitionOrder` を削除した (`rg GetRootDefinitionOrder` 0 件)。UpdateVisibleRows は FirstIndexByPrimaryKey を 1 パス (行数回の Map 追加) で作るだけ。カウンタは回していない。関数そのものが無いので、呼び出し数は静的に 0。満たす (静的)。
- 差分レビュー (stat): submodule 20 files / +317 -167。全て `Source/DataIndexerEd/Private/` 下の Scope 内 (Views 18 + Toolkit 1 + テスト 1)。Toolkit は P-V6 が名指しする `HandleModelRowsChanged`。想定外のパスは無い。
- bd46 からの引き継ぎ: `SViewHeader::BuildColumns` が列の並べ替えでも全行の DisplayName を測る件は fold しない。9 項目の外 (毎フレームではない) で、再測定をいつ無効化するか (行名変更) の判断が要る。報告へ回す。
- critic round 1 (task-critic):
  ```
  FAIL
  1. DataIndexerViewTypes.h:63 — kind 5 — FCellEntry::FilterText は読み手 0 件になった (最後の読み手のフィルタ述語を SearchText に置き換えたため)。メンバーは消すか、使い道を書く。
  NOTE: SDataIndexerSelectionDetails.cpp — AC3 は構造上は満たす (保留タイマーとログカウンタ)。実数は GUI でしか測れない。
  NOTE: DataIndexerViewModel_Visibility.cpp — AC4 は静的に満たす (GetRootDefinitionOrder 削除、参照 0 件)。
  NOTE: AC1 (残り 2 件は RowOps.cpp:240,246 MoveRow 内) と AC2 (残る FindRow は SViewCell L974 / SViewRow L105, L337 で paint 時 attribute ではない) を確認。削除名 (IsColumnActive / ViewTextColor::IsCommentOut / GetRootDefinitionOrder) の呼び手は残っていない。fill preview を読むのは SViewCell と SFillHandleWidget だけで、両方 OnFillPreviewChanged を購読する。
  NOTE: DataIndexerViewTypes.cpp:470 — bReferred は再計算行だけ更新。Execution Log の選択 3 に記載済み。
  ```
  回答: 1 を修正。`FCellEntry::FilterText` メンバーを削除し、ctor 引数 `InFilterText` は SearchText の材料として残した。ViewTypes.h / ViewTypes.cpp:486 / Visibility.cpp:81 のコメントからメンバー名を外した。再 build 22.8s、`DataIndexerEd.View+DataIndexerEd.Repository` → Success=18 Fail=0。

### verify sweep 2026-10-28 19:34 (ALICE、合格 → done)
- 前提 t261026-bd46 は同じ sweep で合格した。
- 環境: エディタ PID 40584 (submodule 0f651e4 のビルド)、UE MCP 127.0.0.1:8001。GUI 操作はユーザー許可済み (dispatch 2026-10-28「今からやって」)。一時アセット `/Game/__Verify/DI_V1000` (DIS_Item、1000 行) を bd46 と共用し、終了時に削除した。証跡は `.agents/tasks/evidence/t261026-dd8a/`。
- AC3: `LogsToolset.SetVerbosity LogDataIndexer Verbose`。6 行をまたぐ範囲選択を実マウスでドラッグ (01 保持中、02 後) → `SSelectionDetails: ForceRefresh` は 1 行だけ (#1、6 rows)。対照: 行を 2 回クリック → #2・#3 の 2 行。計器は選択変更ごとに数えている。
- fill handle: 選択行の上にホバー → ハンドルのピクセル 48 個 (03)。別の行にホバー → 0 個 (03b)。ハンドル上の OS カーソルは IDC_SIZENS、ハンドル外は SIZENS でない (`_harness/cursor.ps1`)。ハンドルをドラッグ → 2 行に青枠のプレビューと "Fill Down" (04a)。離すとプレビューが消え、2 行に値が入った (04b)。
- CommentOut: 行を選んで Shift+T → 目のアイコンが閉じ、チェックが外れた (05b)。選択を外すと文字が暗くなった (05c)。もう一度 Shift+T で元に戻った (チェック checked)。
- Reload (ba24 から合流): Python (`py` コマンドを Cmd 欄から) で `get_all_primary_keys` を数えた。編集後 (行削除 + Insert) は 1000 件で、削除した行のキー 694B9C9F… は無い。`EditorLoadingAndSavingUtils.reload_packages(..., ASSUME_POSITIVE)` で Reload → 1000 件で、そのキーが戻った。compiled cache は Reload で捨てられ、保存済みの内容から作り直された。ensure・警告は無い。
- `stat Slate` の before/after: 旧バイナリが無いので before を撮れない。README の perf 規則に従い、構造の AC (AC1・AC2・AC4 は静的に確認済み) で代えた。
- 所要 約 6 分 (Data View の作業 19:34〜19:54 を bd46 / dd8a / ed01 で共用。按分は概算)。

## Summary
Data View の毎フレーム経路から、線形探索・EditorData の FindRow・AssetRegistry 問い合わせを外した。代わりにマップ 3 つと、FRowEntry / FColumnEntry / FCellEntry のキャッシュを引く。Sort の O(n² log n) と選択フィルタの O(sel×rows) は、1 パスで作るマップに置き換えた。fill プレビューは専用デリゲートに分け、Details はドラッグ中の再構築を保留する。自動テストは green。GUI の確認は 2026-10-28 の verify sweep で合格した。

## Changed Files
Plugins/DataIndexer (submodule):
- Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp
- Source/DataIndexerEd/Private/Tests/ViewModel/DataIndexerViewModelTests.cpp
- Source/DataIndexerEd/Private/Views/DataIndexerViewModel.cpp
- Source/DataIndexerEd/Private/Views/DataIndexerViewModel.h
- Source/DataIndexerEd/Private/Views/DataIndexerViewModel_FillHandle.cpp
- Source/DataIndexerEd/Private/Views/DataIndexerViewModel_RowOps.cpp
- Source/DataIndexerEd/Private/Views/DataIndexerViewModel_Selection.cpp
- Source/DataIndexerEd/Private/Views/DataIndexerViewModel_Visibility.cpp
- Source/DataIndexerEd/Private/Views/DataIndexerViewTypes.cpp
- Source/DataIndexerEd/Private/Views/DataIndexerViewTypes.h
- Source/DataIndexerEd/Private/Views/DataIndexerViewVisuals.cpp
- Source/DataIndexerEd/Private/Views/DataIndexerViewVisuals.h
- Source/DataIndexerEd/Private/Views/SDataIndexerSelectionDetails.cpp
- Source/DataIndexerEd/Private/Views/SDataIndexerSelectionDetails.h
- Source/DataIndexerEd/Private/Views/SDataIndexerViewCell.cpp
- Source/DataIndexerEd/Private/Views/SDataIndexerViewFillHandle.cpp
- Source/DataIndexerEd/Private/Views/SDataIndexerViewFillHandle.h
- Source/DataIndexerEd/Private/Views/SDataIndexerViewHeader.cpp
- Source/DataIndexerEd/Private/Views/SDataIndexerViewRow.cpp
- Source/DataIndexerEd/Private/Views/SDataIndexerViewRow.h

親 repo:
- Plugins/DataIndexer (submodule pointer)
- .agents/tasks/done/t261026-dd8a-view-per-frame-perf.md

## Blockers
None
