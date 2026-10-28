---
status: verify_pending
order: 190
tier: standard
screen: required
agent: any
created_by: task-create
---

# Schema の Column Layout ツリーを、RowStruct が内包する UDS の編集にも追随させる

## Goal
Schema asset editor の Column Layout パネルを開いたまま、RowStruct のメンバとしてネストした別の UDS を編集すると、ツリーが即座に作り直される。

## Context
- 由来: root/t261026-72fc (B-C7、verify_pending) の Execution Log「範囲外の観察」。72fc は RowStruct 自身の UDS 変更だけを購読した。
- 現状の機構 (2026-10-28 時点。行番号がずれていたら関数名で探す):
  - `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_Schema.h`
    `FSchemaDetails` は `FStructureEditorUtils::INotifyOnStructChanged` を継承 (登録・解除は基底の ctor/dtor)。
    メンバ `TWeakObjectPtr<const UUserDefinedStruct> WatchedStruct`。
  - `DataIndexerDetailCustomization_Schema.cpp`
    - `CustomizeDetails` (520 行付近): `WatchedStruct = Cast<UUserDefinedStruct>( RowStruct )`。
    - `PostChange` (631 行付近): `Changed == WatchedStruct.Get()` のときだけ `RequestForceRefresh()`。
    - `BuildNodesRecursive` (52 行付近): `FStructProperty` だけを辿ってノードを作る。`VisitedStructs` は循環止めで、戻るときに `Remove` する。
      各 struct ノードは `ChildStructType` を持つ。
- 不具合: RowStruct `S_Row` が `S_Inner` (UDS) をメンバに持つとき、`S_Inner` にフィールドを足しても `PostChange` の `Changed` は
  `S_Inner` なので更新されない。ツリーには `S_Inner` の子が展開されているので古いまま残る。
- 判断 (Advisor 判定、2026-10-28):
  - `WatchedStruct` を `TSet<TWeakObjectPtr<const UUserDefinedStruct>> WatchedStructs` に置き換える。`CustomizeDetails` で、
    構築したノード木の `ChildStructType` (と RowStruct 自身) のうち UDS のものをすべて入れる。
  - `VisitedStructs` を流用しない (循環止めで、抜けるときに消える)。
  - `RequestForceRefresh` で `CustomizeDetails` が再実行され集合も作り直されるので、新しくネストした UDS は自動で監視対象になる。
    別の仕組みは足さない。
- 範囲外: array/map/set の中の UDS。Column Layout は FStructProperty のネストだけを展開する設計 (memory「ExpandedStruct Array非対応」)。
  ツリーに出ないものは監視しなくてよい。
- 重複チェック通過 (20件対象)。

## Scope
- `FSchemaDetails` の監視対象を「RowStruct から FStructProperty で到達できる UDS すべて」にする。

## Out of Scope
- array/map/set 内の struct の展開・監視。
- `PreChange` (空のまま)。
- `RequestForceRefresh` を `ForceRefresh` に戻すこと (72fc の判断を維持)。

## Preconditions
- 木に `FSchemaDetails::PostChange` と `WatchedStruct` があること (72fc の実装。verify_pending のままでよい)。

## Steps
1. `.h`: `WatchedStruct` を `TSet<TWeakObjectPtr<const UUserDefinedStruct>> WatchedStructs` に置き換え、ヘッダのコメント
   (「RowStruct の」) を「RowStruct とそれが FStructProperty で内包する UDS の」に直す。
2. `.cpp` の `CustomizeDetails`: `WatchedStructs.Reset()`。RowStruct が UDS なら追加。Column Layout 用に構築したノード木
   (`BuildNodes` の戻り) を再帰で走り、`bIsStruct` のノードの `ChildStructType` が UDS なら追加する (匿名 namespace に小さな再帰関数を1つ)。
   ノード木を作らない分岐 (RowStruct にプロパティが無い等) があれば、RowStruct 自身だけを入れる。
3. `.cpp` の `PostChange`: 条件を `Changed && WatchedStructs.Contains( Changed )` にする。

## Acceptance Criteria
- `rg 'WatchedStructs' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` が 3 件以上 (宣言・収集・PostChange。現在 0)。
- `rg '\bWatchedStruct\b' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` が 0 件 (現在 3)。
- GUI: RowStruct `S_Row` が UDS `S_Inner` をメンバに持つ Schema を開き Column Layout を表示したまま、`S_Inner` にフィールドを足すと、
  ツリーの `S_Inner` ノードの下に新フィールドが出る (現在: 開き直すまで出ない)。

## Tests
- `FSchemaDetails` は詳細パネルを立てないと構築できず、`PostChange` は protected。自動テストは足さない
  (Tests 4 問の 4 で、テスト専用の口が要るため)。
- quick: Editor ターゲットのビルド + `Automation RunTests DataIndexerEd.Schema` と `DataIndexerEd.SchemaLayout` と `DataIndexerEd.NotOverridable`。

## Verification
- 残り (未実施): 上の GUI 項目を UE MCP か `windows-screenshot` で撮る (フィールド追加前後のツリー各 1 枚)。

## Execution Log
- 2026-10-28 実装 (fix/audit-2026-10、executor 20/21)。
  - `.h`: `WatchedStruct` → `TSet<TWeakObjectPtr<const UUserDefinedStruct>> WatchedStructs`。クラスコメントを「RowStruct とそれが FStructProperty で内包する UDS」に更新。
  - `.cpp`: 匿名 namespace に `CollectNestedUserDefinedStructs` (ノード木を再帰で走り `bIsStruct` かつ `ChildStructType` が UDS なら追加)。
    `CustomizeDetails` 冒頭で `WatchedStructs.Reset()` + RowStruct が UDS なら追加 (ノード木を作らない早期 return 分岐でも RowStruct だけは入る)。
    `BuildNodes` 後、ツリー作成前に収集。`PostChange` は `Changed && WatchedStructs.Contains( Changed )`。
  - 循環で子を展開しなかった struct ノードも `ChildStructType` は入るので監視対象に含む (害なし)。
- quick:
  - `Build.bat DataIndexerProjectEditor Win64 Development` → `Result: Succeeded` (22 秒、1 回。C1001 なし)。`UnrealEditor-DataIndexerEd.dll` 更新時刻 14:58:55 > cpp 編集 14:58:28。
  - `Automation RunTests DataIndexerEd.Schema+DataIndexerEd.SchemaLayout+DataIndexerEd.NotOverridable` → Found 16、`Result={Success}` 16 / 失敗 0。full は run 終端。
- Acceptance grep: `rg 'WatchedStructs'` → .h 1 + .cpp 4 = 5 件 (≥3)。`rg '\bWatchedStruct\b'` → 0 件。
- 自己レビュー (stat): Plugin 側 2 ファイル (+29/-4)。Scope 内のみ。
- Critic r1 (fallback 経由、verbatim):
  ```
  PASS
  NOTE: E:\workspace\DataIndexerProject\Plugins\DataIndexer\Source\DataIndexerEd\Private\Customizations\DataIndexerDetailCustomization_Schema.cpp:642 — GUI 受入項目 (S_Inner にフィールドを足すと、ツリーにすぐ出る) は diff だけでは確かめられない。画面での検証待ち。コードの流れは要件を満たす。RowStruct は 561-564 行で常に集合へ入り、ネストした UDS は 642 行で入り、657 行で Contains 判定する。
  NOTE: E:\workspace\DataIndexerProject\Plugins\DataIndexer\Source\DataIndexerEd\Private\Customizations\DataIndexerDetailCustomization_Schema.cpp:657 — 削除名 `WatchedStruct` を repo 全体で grep した。残りは 0 件。追加した `CollectNestedUserDefinedStructs` の参照は 642 行と 112 行 (再帰) の 2 件。`WatchedStructs` は .h に 1 件、.cpp に 4 件ある。Acceptance の grep 条件は 2 つとも満たす。
  ```
- GUI 検証は未実施: UE MCP (127.0.0.1:8001) が本 run で停止中 (ECONNREFUSED)。→ verify_pending。


## Summary
Column Layout の監視対象を RowStruct + FStructProperty で到達できる全 UDS の集合にした。ネストした UDS の編集でもツリーが RequestForceRefresh で作り直される。GUI 確認待ち。

## Changed Files
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_Schema.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_Schema.cpp

## Blockers
検証待ち: GUI 項目 (S_Row が S_Inner をネストする Schema で、S_Inner にフィールド追加後ツリーに出る) の前後スクリーンショット。UE MCP 停止中のため未実施。
