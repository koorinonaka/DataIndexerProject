---
status: done
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
- `rg 'IsSplitPinNameMatch|FindSubPinByLeafName'` が GetRow/GetKeysByIndex の .cpp (`K2Node_DataIndexerGetRow.cpp` / `K2Node_DataIndexerGetKeysByIndex.cpp`) で 0 件、定義は `DataIndexerK2NodeUtils.cpp` だけ (現在 2 ファイル)。
- `rg 'GetPinMetaData' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerGetRow.*` が 0 件 (現在 2)。
- `rg 'BreakAllPinLinks' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` が 0 件 (現在 2)。
- Repository ピンを繋ぐ → Ctrl+Z でキーピンの参照性 (`bIsReference`) が繋ぐ前に戻る (現在: 戻らない)。
- Repository ピンを繋ぎ替えてワイルドカード型が変わっても、リンクは両側とも切れない (リンクを保つ分岐は dfba 以前から在る)。Ctrl+Z で型が戻り、リンクは両側とも残る。リンクが切れるのは PinSubCategoryObject が無効なときだけ (ユーザー決定 2026-10-28 で書き直し、旧文言は Execution Log)。
- `rg 'RowsHandle.IndexKey|Handle.IndexKey' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 2)。
- 既存 BP を開いてもピン欠落・再接続なし。

## Tests
- 自動テストなし (K2Node)。Tests 4 問: 既存テーブルなし、BP compile fixture は本番の呼び手が要らない口。
- quick: ビルド + `DataIndexerEd.Compiler`。

## Verification
- undo 2 シナリオのスクリーンショット (繋ぐ前 / 繋いだ後 / Ctrl+Z 後)。既存 BP のコンパイル結果。
- 残り (2026-10-28): (1) Repository ピンを繋ぐ → Ctrl+Z でキーピンの参照性が戻る。(2) Repository を繋ぎ替えて型が変わる → リンクは両側とも残り、Ctrl+Z で型が戻ってもリンクは両側とも残る。(3) Boot Level BP をエディタで開き、ピン欠落・再接続がないこと (ヘッドレスのコンパイルは済)。(4) Handle の Split / Recombine で出力型が追従すること (P-C7 のフィルタ変更の確認)。
- 済 (2026-10-28 verify sweep、ALICE): (1)〜(4)。(2) は型が変わってもリンクが切れない経路だった (Execution Log)。証跡 `.agents/tasks/evidence/t261026-dfba/` (ピンの状態はクリップボード T3D を `_harness/t3dpins.ps1` で読んだ数値)。

## Execution Log
- 2026-10-28 前提: t261026-5a97 / t261026-de66 はどちらも `verify_pending` (検証未完)。その上に構築した。
- Step 1: `DataIndexerK2NodePinKeys.h/.cpp` を `DataIndexerK2NodeUtils.h/.cpp` に統合した。PinKeys の関数は namespace `DataIndexer::K2NodePinKeys` のまま移した (Switch/Select の呼び出しは無変更、`#include` 1 行ずつだけ変更)。自由関数は `DataIndexer::K2NodeUtils` に置いた。`IsSplitPinNameMatch` は `FindSubPinByLeafName` だけが使うので .cpp 内の無名 namespace に閉じた。
- Step 2-3: `UK2Node_DataIndexerHandleNodeBase` (`UCLASS(Abstract)`) を新設した。派生 2 クラスは UPROPERTY なし・ピン名とピンの向き/順は不変 (入力 Execute, Handle[, Query] / 出力 Then, Missing, ReturnValue)。シリアライズ形式は変えていない。
- 既定 (仮想の数): Scope は「仮想 4 つ + ExpandNode の関数名差分だけ」と書くが、2 クラスはピン構成 (GetKeysByIndex は入力 `Query` と配列出力 `Out Keys`)、ExpandNode の結線先 (`OutValue` / `Query`+`OutKeys`)、タイトルの NSLOCTEXT キーも違う。そこで仮想を 6 つにした: 4 つ + `IsKeyDefaultValueValid` (B-C11 で型ごとに検証するため) + `ExpandValuePins`。派生は `AllocateDefaultPins` (Super の後に固有ピン)、`GetNodeTitle` (基底の `GetResolvedNodeTitle` を呼ぶ)、`GetTooltipText` も持つ。変える費用: 仮想を減らすなら型と結線を表で渡す形への書き換え (派生 2 ファイル)。
- 既定 (カテゴリ文字列): `GetMenuCategory` は基底 1 つにした。GetKeysByIndex の LOCTEXT キー `DataIndexerGetKeysByIndex_Category` は消え、`DataIndexerGetRow_Category` (同じ "DataIndexer") に揃う。翻訳ファイルは repo に無い。
- 既定 (Context の ResolveRepositoryFromPin): Context は「新 API を呼ぶ」とするが、元の関数は Resolver を呼んでいない。Repository サブピンの `DefaultObject` を Cast するだけで、de66 の `FindRepositoryMetaPath` / `EvaluateRepositoryPath` / `ResolveFromGraphPin` はキーピンの meta から Repository を探す API で、Repository ピンそのものには当てはまらない。R-C11 で死んだ LinkedTo ループを消し、Cast 1 行にした。
- B-C3: `RefreshKeyPinRequirement` で `KeyPin->Modify()` を `bIsReference/bIsConst` の書き込みより前へ移した。
- B-C4: `PropagatePinType` の wildcard 戻しで `BreakAllPinLinks()` を `GetSchema()->BreakPinLinks( *Pin, true )` に替えた。エンジンを読んだ結果: `UEdGraphPin::BreakLinkTo` は既に相手ピンの `Modify()` (= 相手ノードの Modify) を呼ぶ (UE5.8 `EdGraphPin.cpp:668-673, 998-1003`)。足りなかったのは相手ノードへの `PinConnectionListChanged` 通知だった。Schema 版は相手ノードと自ノードに通知する (`EdGraphSchema.cpp:672-700`)。自ノードへの通知は `UK2Node::PinConnectionListChanged` → `NotifyPinConnectionListChanged` (`K2Node.cpp:600`) → `PropagatePinType` へ再入する。ピンがまだ `PC_Struct` だと再び break に入り無限再帰になるので、型を Wildcard に戻してから break する順にした。break はリンクが残っているときだけ呼ぶ。K2 版 `BreakPinLinks` は `FScopedTransaction(…, ShouldActuallyTransact())` を開く。`ShouldActuallyTransact` は `!IsInAsyncLoadingThread()` (`EdGraphSchema_K2.cpp:664-667`)。UE 5.7 にも `BreakPinLinks(UEdGraphPin&, bool)` と `GRAPHACTION_Default` がある (`EdGraphSchema.h:1040`, `GraphEditAction.h:11`) ので分岐は不要。
- B-C5: `GetPinMetaData` override と `DataIndexerMetaDataKeys.h` の include を消した。
- B-C11: 検証を派生の `IsKeyDefaultValueValid` へ移した。GetKeysByIndex は `FDataIndexerIndexKey( DefaultValue )`、GetRow は `FDataIndexerPrimaryKey( DefaultValue )`。メッセージは基底で `Handle.%s` + キーの葉名で組む (GetKeysByIndex の旧 `RowsHandle.IndexKey` / `Handle.IndexKey` 不一致は消えた)。
- P-C7 (Context と違う修正): Context の `Action.Nodes.Contains(this)` は使わなかった。`UEdGraphSchema_K2::SplitPin` / `RecombinePin` は引数なしの `Graph->NotifyGraphChanged()` (UE5.8 `EdGraphSchema_K2.cpp:7529, 7625`) を呼び、これは既定構築の `FEdGraphEditAction` を送る (`EdGraph.cpp:283-287`) ので `Nodes` は空。Contains で絞ると購読の目的 (Split/Recombine の追従) が黙って死ぬ。`Action.Action == GRAPHACTION_Default` だけで絞った (AddNode / RemoveNode / SelectNode / EditNode は来なくなる)。ノード寿命の購読とクローングラフ由来の購読は残る (CreateWeakLambda なので安全)。
- R-C11: `ResolveRepositoryFromPin` の LinkedTo ループと、EarlyValidation の `else if` の `|| LinkedTo.Num() > 0` を消した。
- Step 4 の型ミスマッチ文言: 基底で `"%s type mismatch in @@."` + ワイルドカードピンの `GetDisplayName()` にした (GetRow は "Out Row"、GetKeysByIndex は "Query")。
- ビルド: `Build.bat DataIndexerProjectEditor Win64 Development` → `Result: Succeeded` (23 秒、1 回)。DataIndexerEd は Editor 専用 module なので Game ターゲットのビルドは不要。
- quick: `Automation RunTests DataIndexerEd.Compiler` → Found 5 tests、5 Success (`TEST COMPLETE. EXIT CODE: 0`)。full は run 終端。
- Step 5 (既存 BP、ヘッドレス): `Content/Boot.umap` の Level BP が GetRow ×2 (`K2Node_DataIndexerGetRow_0/_2`) と GetKeysByIndex ×1 (`K2Node_DataIndexerGetKeysByIndex_0`) を持つ。`UnrealEditor-Cmd -ExecutePythonScript` で `/Game/Boot` を読み込み、`BlueprintEditorLibrary.compile_blueprint` → `BS_UP_TO_DATE` (警告なし。ピン欠落の "no longer exists" 警告は 0 件)。ノードは新クラスのまま読めた。保存はしていない。
- 受け入れ条件:
  - 行数: `wc -l` GetRow.cpp + GetKeysByIndex.cpp = 63 + 80 = 143 (基準 1123 の半分 561 以下)。
  - `IsSplitPinNameMatch|FindSubPinByLeafName` in `K2Node_*.cpp`: GetRow/GetKeysByIndex は 0 件。新設の `K2Node_DataIndexerHandleNodeBase.cpp` が glob に当たり、`FindSubPinByLeafName` の呼び出し 2 か所が残る。定義は `DataIndexerK2NodeUtils.cpp` の 1 か所だけ。条件の意図 (逐語コピーの解消) は満たすが、文字どおりの 0 件ではない —— 基底ファイル名が glob に入ることを起票時に想定していなかった。
  - `GetPinMetaData` in GetRow.*: 0 件。
  - `BreakAllPinLinks` in Customizations/: 0 件。
  - `RowsHandle.IndexKey|Handle.IndexKey` in DataIndexerEd/: 0 件。
  - undo 2 シナリオと既存 BP の GUI 確認: UE MCP (127.0.0.1:8001) が落ちており未実施 → verify_pending。B-C4 の undo シナリオは、上の B-C4 の調査どおり変更前から相手ノードの Modify は記録されていた可能性がある。GUI で「変更前も片側だけだったか」を対照として確かめる。
- Self review (stat): submodule 11 ファイル、+723 / −1162。全て `Private/Customizations/` (Scope 内)。Switch/Select は `#include` 1 行ずつ (PinKeys.h 削除に伴う、Step 1 の範囲)。
- Critic round 1 (task-critic):
  ```
  FAIL
  1. K2Node_DataIndexerHandleNodeBase.cpp:85, :222 — kind 1 — Acceptance Criterion `rg 'IsSplitPinNameMatch|FindSubPinByLeafName' .../Customizations/K2Node_*.cpp` が 0 件でない。新設の基底ファイルが glob に当たり、`FindSubPinByLeafName` の呼び出しが 2 件残る。(a) 呼び出しを Utils 側の helper の後ろへ隠す、(b) task の条件文を GetRow/GetKeysByIndex に絞る形へ改訂する。
  NOTE: Switch/Select の #include 1 行ずつは Step 1 が強制する変更なので FAIL にしない。
  NOTE: GetMenuCategory の LOCTEXT キーは DataIndexerGetRow_Category に統一。旧キーの参照 0 件。
  NOTE: 旧 GetRow AllocateDefaultPins 末尾の RefreshPrimaryKeyPinRequirement() 削除は実害なし (サブピン未生成で no-op)。
  NOTE: 変更したエラーメッセージの参照はテスト・docs に 0 件。
  NOTE: 新しい定義は全て参照あり。
  NOTE: undo 2 シナリオと既存 BP のピン欠落は GUI でしか確かめられない。
  ```
  回答: コードは変えない。(a) は grep を避けるための改名で、条件を満たしたことにならない。条件の基準「現在 2 ファイル」は逐語コピー 2 つを指し、Scope 自体が基底 .cpp の新設を求めている。成り立つ強い同等条件: GetRow/GetKeysByIndex .cpp で 0 件、定義は `DataIndexerK2NodeUtils.cpp` だけ、`IsSplitPinNameMatch` は `DataIndexerK2NodeUtils.cpp` 以外に 0 件。条件文の改訂は 要ユーザー判断 として run 終端へ送る。
- Critic round 2 (task-critic):
  ```
  FAIL
  1. K2Node_DataIndexerHandleNodeBase.cpp:85, :222 — kind 1 — 条件は依然 2 件。author の解釈 (意図は満たす) は妥当だが文字どおりは 0 件を求めている。解消はユーザーが条件の言い直しを受け入れるか、コードを変えるか。
  NOTE: 削除した名前 (DataIndexerK2NodePinKeys ほか) のコード参照は 0 件。
  NOTE: 削除した文言・LOCTEXT キーの参照は 0 件。
  NOTE: 追加した定義は全て呼び出し元あり。
  NOTE: Switch/Select は #include 1 行ずつ、Out of Scope の振る舞いには触れない。
  NOTE: undo シナリオと既存 BP は GUI 確認待ち (verify_pending)。
  NOTE: P-C7 は GRAPHACTION_Default だけで絞った。理由は Execution Log。Verification (4) で確認。
  ```
  2 round で残った 1 件は Follow-ups の 要ユーザー判断 (条件文の言い直しを受け入れるか、改名するか) としてコミットを進める。

### ユーザー決定 2026-10-28 (AC 文言の修正)
- 条件文の言い直しを受け入れた。コードは変えない。status は変えない。
- 旧文言: 「`rg 'IsSplitPinNameMatch|FindSubPinByLeafName' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_*.cpp` が 0 件 (現在 2 ファイル)。」
- 新文言: GetRow/GetKeysByIndex の .cpp で 0 件、定義は `DataIndexerK2NodeUtils.cpp` だけ。

### verify sweep 2026-10-28 20:00 (ALICE、合格 → done)
- 前提 t261026-de66 は同じ sweep で合格した。
- 環境: エディタ PID 40584 (submodule 0f651e4 のビルド)、UE MCP 127.0.0.1:8001。GUI 操作はユーザー許可済み (dispatch 2026-10-28「今からやって」)。前回 sweep の残り `/Game/__Verify/BP_VGetRow_Dup` (GetRow、Handle 分割済み) に変数 RepoVar と Break ItemRow ノードを足して使い、終了時に削除した。証跡は `.agents/tasks/evidence/t261026-dfba/`。
- ハーネスの落とし穴: 最初の `TRANSACTION UNDO` は何もしなかった。`py unreal.SystemLibrary.end_transaction()` が 1 を返した (開いたままのトランザクションが残っていた。どの MCP ツール由来かは不明)。閉じて undo の塊を 1 回戻し、`end_transaction()` が 0 を返すのを確かめてから測り直した。下の数値はその測り直し。
- (1) RepoVar の Get ノードを実マウスで Handle Repository に繋いだ。T3D の Handle_PrimaryKey: 繋ぐ前 `bIsReference=False bIsConst=False` (01a) → 繋いだ後 `True True` (01b) → `TRANSACTION UNDO` 後 `False False`、Handle_Repository の LinkedTo も 0 (01c)。
- (2) Out Row を Break ItemRow に繋いだ状態で、Handle Repository を DI_V1000 → DI_ItemRarity にピンのコンボで変えた → Out Row は ItemRarityRow 型、リンクは両側とも残った (02b)。Undo → Out Row は ItemRow 型に戻り、リンクは両側とも 1 本 (02c)。AC の「現在: 片側だけ」は、この経路では起きない。`PropagatePinType` は、型が解決できずリンクがある struct ピンなら型を保つ (この分岐は dfba 以前の `K2Node_DataIndexerGetRow.cpp` にもある、`e7c1f91^` で確認)。リンクを切る分岐に入るのは PinSubCategoryObject が無効なときだけ。AC 文言の扱いは run 終端の 要ユーザー判断。
- (3) Boot の Level BP をエディタで開いた (03)。GetRow ×2 と GetKeysByIndex ×1 のピンと結線はそろっていた (`get_node_infos`)。開いた後のログに Warning / Error は無い。保存はしていない。
- (4) Handle を右クリックで「構造体ピンを再結合」→ Handle 1 本、Out Row は ItemRow 型、リンクは残った (04a)。「構造体ピンを分割」→ Handle_Repository / Handle_PrimaryKey、型とリンクは同じ (04b)。限界: この経路では型が変わる理由が無いので、「追従しない」場合との区別はつかない。
- 所要 約 6 分 (20:00〜20:06)。

### ユーザー決定 2026-10-28 (AC / Verification の文言を実挙動に合わせる)
- verify sweep (c51d89a) の 要ユーザー判断 への回答。文言を実挙動に合わせた。コードは変えない。status は変えない。
- 旧 AC: 「Repository ピンを繋ぎ替えてワイルドカード型が変わる → Ctrl+Z で相手ノードのリンクも戻る (現在: 片側だけ)。」
- 旧 Verification 残り (2): 「Repository を繋ぎ替えて型が変わる → Ctrl+Z で相手ノードのリンクも戻る。」
- 旧文言の前提 (繋ぎ替え → 型変化でリンクが切れ、Ctrl+Z で片側だけ戻る) は実挙動と違う。
  - 型が変わってもリンクは切れない。リンクを保つ分岐は dfba 以前から在る。
  - リンクが切れるのは PinSubCategoryObject が無効なときだけ。
  - Undo で型が戻り、リンクは両側とも残る (verify sweep c51d89a で確認)。
- 新文言は Acceptance Criteria と Verification の該当行。

## Summary
GetRow / GetKeysByIndex の共通部分を `UK2Node_DataIndexerHandleNodeBase` (abstract) に集め、自由関数と PinKeys を `DataIndexerK2NodeUtils` に統合した。B-C3 / B-C4 / B-C5 / B-C11 / P-C7 / R-C11 を基底で直した。ビルドと `DataIndexerEd.Compiler` 5/5 は通過。既存 Boot Level BP はヘッドレスで警告なしにコンパイルできた。undo などの GUI 確認は 2026-10-28 の verify sweep で合格した。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerHandleNodeBase.h` (新規)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerHandleNodeBase.cpp` (新規)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerK2NodeUtils.h` (旧 `DataIndexerK2NodePinKeys.h`)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerK2NodeUtils.cpp` (旧 `DataIndexerK2NodePinKeys.cpp`)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerGetRow.h/.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerGetKeysByIndex.h/.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSwitchPrimaryKey.cpp` (`#include` のみ)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSelectPrimaryKey.cpp` (`#include` のみ)
- `.agents/tasks/done/t261026-dfba-k2node-handle-base.md`

## Blockers
None
