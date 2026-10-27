---
status: ready
order: 140
tier: standard
screen: required
agent: any
created_by: task-create
---

# K2Node GetRow / GetKeysByIndex を共通基底にまとめ、undo とリンク切断の bug を基底で直す (X4)

## Goal
`UK2Node_DataIndexerGetRow` と `UK2Node_DataIndexerGetKeysByIndex` の逐語コピー ~450 行が `UK2Node_DataIndexerHandleNodeBase` に集まり、
ピンの参照性 undo・リンク切断の片側 undo・drift したメッセージ・死んだ `GetPinMetaData` が 1 回の修正で両ノードに効く。

## Context
監査 `.agents/audit-2026-10-26.md` の X4 = R-C2, B-C3, B-C4, B-C5, B-C11, P-C7, R-C11 (K2Node 分)。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` 相対、行番号は 2026-10-26 時点。

- R-C2: `K2Node_DataIndexerGetRow.cpp:28-127` ≡ `K2Node_DataIndexerGetKeysByIndex.cpp:28-127` (`IsPinOrDescendantOf`, `IsSplitPinNameMatch`,
  `FindSubPinByLeafName`, `ResolveRepositoryFromPin`, `ResolveTypeFromLinkedPins`)。加えて ctor (`:130-147/:130-146`)、`PreloadRepositoryChain`
  (`:340-355/:321-336`)、Reallocate (`:366-380/:347-361`)、PostPlacedNewNode (`:382-398/:363-379`)、PostReconstruct/PostEditUndo (`:400-413/:381-394`)、
  Notify/PinDefaultValueChanged (`:415-444/:396-425`)、ResolveWildcardType/PropagatePinType (`:446-507/:427-488`)、Reset*/Refresh*PinRequirement
  (`:223-267/:217-261`)。差分はハンドル構造体・キーピン末尾名・ワイルドカードピン名・ライブラリ関数名だけ。
- B-C3: `GetRow.cpp:258-260`, `GetKeysByIndex.cpp:252-254` `bIsReference/bIsConst` を書いた**後**に `Pin->Modify()` → undo で参照性が戻らない。
- B-C4: `GetRow.cpp:494`, `GetKeysByIndex.cpp:475` `BreakAllPinLinks(bNotifyNodes=false)` → リンク先ノードの `Modify()` なし → undo で片側だけリンク復活、
  相手ノードが `NotifyPinConnectionListChanged` を受けず古い型を持ち越す。`Schema->BreakPinLinks(*Pin, true)` か相手ノードの `Modify()` を先に。
- B-C5: `GetRow.cpp:273` `GetPinMetaData` が `PrimaryKeyPinName` (= `"PrimaryKey"`) と比較するが split サブピン名は `Handle_PrimaryKey`
  (エンジン `EdGraphSchema_K2.cpp` SplitPin の `%s_%s`) → 一度も発火しない。発火させると返る `"Repository"` がノードの UPROPERTY に無いので
  Resolver の CDO fallback に落ちて B-C9 を悪化させる → override 削除が唯一の修正。
- B-C11: `GetKeysByIndex.cpp:279,284,286` IndexKey の default を `FDataIndexerPrimaryKey(DefaultValue)` で検証 (型違い、両方 FGuid 派生なので偶然動く)。
  メッセージのピン名が `RowsHandle.IndexKey` / `Handle.IndexKey` で不一致 (ピン名は `Handle`)。
- P-C7: `GetRow.cpp:134-145`, `GetKeysByIndex.cpp:134-144` ノードごとに `OnGraphChanged` を一生購読し全グラフ操作で `PropagatePinType`。クローングラフ由来も残る。
  `Action.Action == GRAPHACTION_Default && Action.Nodes.Contains(this)` で絞る (`SplitPin` / `RecombinePin` は default action しか出さない)。
- R-C11 (K2Node 分): `GetRow.cpp:95-101` / `GetKeysByIndex.cpp:95-101` は linked (出力) ピンの `DefaultObject` を読むが出力ピンは持たない。
  `GetRow.cpp:300` / `GetKeysByIndex.cpp:282` の `|| LinkedTo.Num() > 0` はその `else if` で常に false。
- t261026-5a97 が `DataIndexerK2NodePinKeys.h` を作っている。本タスクの `DataIndexerK2NodeUtils.h` がそれを吸収 (ファイルを統合、関数は移すだけ)。
- t261026-de66 が Resolver を `Find/Evaluate` に分けている。`ResolveRepositoryFromPin` は新 API を呼ぶ。

## Scope
- `UK2Node_DataIndexerHandleNodeBase : UK2Node` (abstract) を新設。仮想: `GetHandleStruct()` / `GetKeyPinLeafName()` / `GetWildcardPinName()` /
  `GetLibraryFunctionName()`。共通実装を基底へ移し、派生 2 クラスは仮想 4 つ + `ExpandNode` の関数名差分だけ。
- 自由関数 5 つを `DataIndexerK2NodeUtils.h/.cpp` へ。`DataIndexerK2NodePinKeys.h` の中身もここへ統合。
- B-C3 / B-C4 / B-C11 / P-C7 / R-C11 を基底側で直す。`GetPinMetaData` override を削除。

## Out of Scope
- Switch / Select (t261026-5a97 で done 済み)。
- Resolver の中身 (t261026-de66)。
- 既存 BP アセットの再保存。基底化で `UCLASS` の継承が変わるが、シリアライズ形式 (ピン・UPROPERTY) は変えないので再保存不要。変わるなら止めて報告。

## Preconditions
- t261026-5a97 と t261026-de66 が done。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. `DataIndexerK2NodeUtils.h/.cpp` を作り、自由関数 5 つと `PinKeys` の関数を移す。`DataIndexerK2NodePinKeys.h` を消す。
2. `K2Node_DataIndexerHandleNodeBase.h/.cpp` を作り、GetRow の共通部分を移す。仮想 4 つを定義。
3. GetRow / GetKeysByIndex を基底派生に書き換え、差分だけ残す。ビルド。
4. B-C3 → `Modify()` を先に。B-C4 → `Schema->BreakPinLinks(*Pin, true)`。B-C11 → `FDataIndexerIndexKey` で検証、メッセージのピン名を `Handle` に。
   P-C7 → `OnGraphChanged` のフィルタ。R-C11 → 到達不能分岐削除。B-C5 → override 削除。
5. 既存の BP (`Content/` にある GetRow / GetKeysByIndex 入り BP) を開いてコンパイル、ピンが保たれていることを確認。
6. GUI で Verification。

## Acceptance Criteria
- `wc -l K2Node_DataIndexerGetRow.cpp K2Node_DataIndexerGetKeysByIndex.cpp` の合計が現在 (567+556=1123) の半分以下。
- `rg 'IsSplitPinNameMatch|FindSubPinByLeafName' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_*.cpp` が 0 件 (現在 2 ファイル)。
- `rg 'GetPinMetaData' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerGetRow.*` が 0 件 (現在 2)。
- `rg 'BreakAllPinLinks' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` が 0 件 (現在 2)。
- Repository ピンを繋ぐ → Ctrl+Z でキーピンの参照性 (`bIsReference`) が繋ぐ前に戻る (現在: 戻らない)。
- Repository ピンを繋ぎ替えてワイルドカード型が変わる → Ctrl+Z で相手ノードのリンクも戻る (現在: 片側だけ)。
- `rg 'RowsHandle.IndexKey|Handle.IndexKey' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 2)。
- 既存 BP を開いてもピン欠落・再接続なし。

## Tests
- 自動テストなし (K2Node)。Tests 4 問: 既存テーブルなし、BP compile fixture は本番の呼び手が要らない口。
- quick: ビルド + `DataIndexerEd.Compiler`。

## Verification
- undo 2 シナリオのスクリーンショット (繋ぐ前 / 繋いだ後 / Ctrl+Z 後)。既存 BP のコンパイル結果。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
