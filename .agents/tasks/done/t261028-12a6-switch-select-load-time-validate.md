---
status: done
order: 230
tier: standard
screen: required
agent: any
created_by: task-create
---

# Switch / Select PrimaryKey: エディタ再起動後の読み込み時コンパイルで出る偽の「PrimaryKey … is not in <Repository>」を直す

## Goal
エディタ再起動後に K2Node Switch / Select PrimaryKey 入りの BP を開いても、読み込み時のコンパイルが偽のエラーを出さない。
case ピンの表示名は GUID でなく行の表示名になる。

## Context
- ユーザー決定 2026-10-28: t261026-5a97 の verify sweep で見つかった回帰の follow-up として起票を承認 (この回答が承認、`ready/` 直行)。
- 重複チェック通過 (25件対象)。最も近いのは `done/t261026-5a97-k2node-switch-select-validate.md` (dup 0.40)。5a97 は検証を足したタスクで、本タスクはその検証が読み込み中に誤って走る件。
- 証跡: `.agents/tasks/evidence/t261026-5a97/followup-02b-load-time-false-compile-error.txt`、`followup-after-restart-pins-show-guid.png`。
  - 再起動後の初回読み込み (DI_VItemRarity は BP の import として読み込まれた) で `PrimaryKey 2B8F0001-… is not in DI_VItemRarity in Switch on Primary Key` が 2 件出た。
  - キーは DI_VItemRarity にある。数秒後の `compile_blueprint` はエラーを出さない。
  - 同じ BP の Select ノード (起動時に読み込み済みの DI_ItemRarity を参照) はエラーを出さない。
  - case ピンの表示名は GUID のまま残り、再コンパイル後も戻らない。
  - 一時アセット `/Game/__Verify/BP_VSwitch` / `DI_VItemRarity` は sweep の終わりに消した可能性がある。無ければ作り直す。
- コード (2026-10-28 時点。ずれていたら関数名で探す。パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` 相対):
  - `DataIndexerK2NodeUtils.cpp` `DataIndexer::K2NodePinKeys::ValidatePinKeys` が `Repository->FindRowEntity( PinKey ).IsValid()` で存在を確かめる。読み込み途中の Repository では行が無く、偽のエラーになる (疑い)。
  - `K2Node_DataIndexerSwitchPrimaryKey.cpp` `CreateCasePins` / `AddPinToSwitchNode` が `PinFriendlyName = GetPinNameOverride( … )` を 1 回だけ書く。`GetPinNameOverride` は `Repository->GetDisplayName( PrimaryKey )`。Repository の読み込みが済んだ後に作り直す経路が無い。Select も同じ Repository UPROPERTY と `ValidatePinKeys` を持つ。
  - 前例: `K2Node_DataIndexerHandleNodeBase.cpp` の `PreloadRequiredAssets` → `PreloadRepositoryChain` が Repository / EditorData / RowStruct を `PreloadObject` する。コメントに同じ類の症状 (再起動直後だけコンパイルエラー、手動コンパイルで解消) が書いてある。Switch / Select には `PreloadRequiredAssets` override が無い (`rg PreloadRequiredAssets` は HandleNodeBase だけ)。
  - 消えた防御: t261026-91cd が消した `FDataIndexerPrimaryKey::OwnerRepository` の HACK には「load 中 (RF_NeedLoad / RF_NeedPostLoad) は repository getter を呼ばない」ガードがあった (`done/t261026-91cd-primarykey-owner-hack.md` の critic NOTE)。この類の問題を避けていた。
- 原因はまだ疑い。Step 1 で確かめてから直す。

## Scope
- Switch / Select の読み込み時に Repository を先に読み込ませる (HandleNodeBase の `PreloadRepositoryChain` を共有するのが第一候補。`DataIndexerK2NodeUtils` へ移す)。
- それでも読み込み途中の Repository に当たる経路が残るなら、`ValidatePinKeys` は読み込み途中 (`RF_NeedLoad | RF_NeedPostLoad`) の Repository で「無い」と言わない。
- Repository の読み込みが済んだ後に case ピンの `PinFriendlyName` を作り直す。

## Out of Scope
- GetRow / GetKeysByIndex (HandleNodeBase) の挙動。共有化で関数を移すだけ。
- `FDataIndexerPrimaryKey::OwnerRepository` の HACK の復活。

## Preconditions
- t261026-5a97 / t261026-dfba / t261026-91cd が done (済)。
- 作業ブランチ `fix/audit-2026-10` (両 repo)。

## Steps
1. 確認: `ValidatePinKeys` の冒頭で `Repository->HasAnyFlags( RF_NeedLoad | RF_NeedPostLoad )` と `FindRowEntity` の結果を一時ログに出す。エディタを再起動し、Repository が未読み込みの状態で Switch 入り BP を開いて、偽エラーの時点でフラグが立っているかを見る。一時ログは消す。結果を Execution Log に書く。
2. Step 1 で読み込み途中と分かったら: `PreloadRepositoryChain` 相当を `DataIndexerK2NodeUtils` に移し、HandleNodeBase と Switch / Select の `PreloadRequiredAssets` から呼ぶ。Switch / Select は UPROPERTY `Repository` を渡す。
3. 再起動して同じ手順で偽エラーが消えたかを見る。消えなければ `ValidatePinKeys` に読み込み途中のガードを足す。
4. case ピンの表示名: Repository の読み込みが済んだ後 (`PostReconstructNode` か、Step 2 の preload の後) に `PinFriendlyName` を作り直す。
5. Step 1 の結果が疑いと違ったら、止めずに実測の原因を Execution Log に書き、同じ Goal を満たす直し方を選ぶ。

## Acceptance Criteria
- `rg 'PreloadRequiredAssets' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexer{Switch,Select}PrimaryKey.*` が 1 件以上 (現在 0)。Step 2 以外の直し方を選んだときは、選んだ機構を Execution Log に書き、この行を言い直す。
- エディタ再起動後、Repository が未読み込みの状態で Switch 入り BP を開いても、ログに `is not in` のコンパイルエラーが出ない (現在: 2 件出る)。
- 同じ条件で case ピンの表示名が行の表示名 (例 VCommon / VUncommon) になる (現在: GUID のまま)。
- 実在しないキーを入れた PinKeys は、再コンパイルで今までどおり `is not in` を出す (5a97 の検証を壊さない)。

## Tests
- 自動テストなし。Tests 4 問: 守るのは「読み込み途中の Repository に対して検証しない」。落ちる回帰はエディタ再起動 + import 経由の読み込みでしか再現しない。既存の `DataIndexerEd.Compiler` は読み込み済みの Repository しか使わない。テスト用に読み込み順を操る口は本番の呼び手が要らない口なので足さない。
- quick: ビルド + `DataIndexerEd.Compiler`。

## Verification
- `KNOWLEDGE.md`「UE MCP GUI ハーネス」の手順で行う。K2Node の検証は BeginPlay から exec に繋ぐ (孤立ノードは刈られる)。
- 1. 一時 BP (Switch と Select、Repository は起動時に読み込まれない一時 Repository) を作って保存する。
- 2. エディタを再起動し、何も開かずに BP を開く。ログの `LogBlueprint` に `is not in` が無いことを `GetLogEntries` で確かめる。
- 3. case ピンのスクリーンショット (表示名が行の名前)。
- 4. 実在しないキーを足して再コンパイル → `is not in` が出ることを確かめる。
- 一時アセットは終わりに消す。証跡は `.agents/tasks/evidence/t261028-12a6/`。
- 済 (2026-10-29 verify sweep): 1〜4 すべて。

## Execution Log
2026-10-28 (ALICE、dispatch 1/3、エディタ PID 40584 は run notes に従い Stop-Process で閉じた。ユーザーが PC 使用中のため GUI 検証はしない)

- Step 1 (確認): GUI でなく headless で再現した (`evidence/t261028-12a6/setup.py` / `repro.py`)。`DI_ItemRarity` は起動時に `VM_InventoryRowCard` 経由で読み込まれるため、複製 `/Game/__Verify/DI_VItemRarity` と、Select ノードをそれへ向けた `VM_InventoryRowCard` の複製 `/Game/__Verify/BP_VSelect` を作った。新しい `UnrealEditor-Cmd -nullrhi` で BP を `load_asset` すると、HEAD のバイナリで `PrimaryKey 2B8F000n-… is not in DI_VItemRarity in Select by Primary Key` が 4 件出た (2/2 回同じ)。
- 一時ログ (`ValidatePinKeys` 冒頭でフラグ・`FindRowEntity`・case ピンの `PinFriendlyName`。新しい preload は環境変数で外せるようにした) で、preload なし / あり を交互に 2 回ずつ流した:
  - preload なし: 偽エラー時点の Repository は `RF_NeedLoad=1`、`EditorData=null` (未シリアライズ)。`FindRowEntity` は全キー found=0。case ピンの `PinFriendlyName` は GUID 文字列 (`GetDisplayName` が EditorData 無しで runtime 経路へ落ち、`Describe(GUID)` を返す)。疑いどおり「読み込み途中」。2/2。
  - preload あり: 同じ時点で `RF_NeedLoad=0`、`EditorData` あり (`RF_NeedPostLoad` は残るが行は引ける)。全キー found=1、エラー 0 件、`PinFriendlyName` は Common / Uncommon / Rare / Epic。2/2。
  - 一時ログと環境変数の分岐は消して再ビルドした (`rg 'DIAG12|TEMP12' Source` 0 件)。
- Step 2: `PreloadRepositoryChain` を `DataIndexer::K2NodeUtils::PreloadRepositoryChain( const UDataIndexerRepository* )` へ移した (`UK2Node::PreloadObject` と同じ `ConditionalPreload` を Repository → EditorData → RowStruct の順に呼ぶ)。HandleNodeBase は `ResolveRepositoryFromPin(...)` を渡して呼ぶだけに変えた (挙動は不変、使わなくなった `DataIndexerEditorInterface.h` の include を消した)。Switch / Select に `PreloadRequiredAssets` override を足し、UPROPERTY `Repository` を渡す。
- Step 3: preload で偽エラーは消えた (最終ビルドで after1/after2 とも `is not in DI_` 0 件)。`ValidatePinKeys` に読み込み途中のガードは足していない (Scope の「それでも残るなら」に当たらない)。
- Step 4: 読み込み時の再構築の時点で行が引けるようになり、`PinFriendlyName` は読み込み直後から行の表示名 (上の preload ありの実測)。読み込み後に作り直す経路は足していない —— 作り直す元の値が読み込み時点で正しくなったため。
- AC4: 最終ビルドで BP_VSelect の Select の Repository を `DI_ItemType` (Rarity のキーを持たない) へ変えて `compile_blueprint` → `is not in DI_ItemType` が 4 件出た。5a97 の検証は生きている。
- Switch: Python から K2Node を新規に置く手段が無く、既存アセットにも Switch on Primary Key が無いので headless では走らせていない。同じ `PreloadRequiredAssets` → `PreloadRepositoryChain` の経路。GUI 検証 (Verification 1–4) に残す。
- 一時アセット `Content/__Verify/` は消した (未追跡のまま、commit なし)。
- quick: `Build.bat DataIndexerProjectEditor Win64 Development` 成功 (診断入り 33.6 s、最終 29.6 s、ICE なし)。`Automation RunTests DataIndexerEd` (全群) → 77 件 `Result={Success}`、失敗 0 (うち Compiler 6 件)。full は run 終端。Game ターゲットは触っていない (editor モジュールだけ、runtime 構造体に触れない)。
- 5.7: 使ったエンジン API は `UK2Node::PreloadRequiredAssets` (5.7 `K2Node.h:384`) と `UObject::ConditionalPreload` だけで、5.7 と同じ。
- 自己レビュー (stat): submodule 8 ファイル +48/-22、全て Scope の Customizations 内 (utils / HandleNodeBase / Switch / Select)。親 repo はタスクファイル・evidence・KNOWLEDGE.md。
- 畳んだ 軽微 (ソース変更なし): `KNOWLEDGE.md`「UE MCP GUI ハーネス」へ headless 再現の手順を 1 行足した (1 ファイル +1 行)。この再現法は GUI 無しで読み込み時コンパイルを見る唯一の確かめ方で、次の dispatch が作り直さないよう残す。
- Tests 4 問: 自動テストは足さない (タスクの Tests のとおり。読み込み順を操る口が要る)。headless スクリプトは evidence に残した。
- critic r1 (task-critic、原文):
  ```
  PASS
  NOTE: …/K2Node_DataIndexerSwitchPrimaryKey.cpp:104 — AC2 と AC3 は Switch で一度も実行されていない。headless 再現が使ったのは Select だけ (task Execution Log)。どちらの AC も画面と再起動でしか確かめられないので、FAIL にはしない。task が verify_pending なのは正しい。Verification 1–4 は Switch で行う必要がある。
  NOTE: …/DataIndexerK2NodeUtils.cpp:89 — HandleNodeBase の preload は `UK2Node::PreloadObject` から `UObject::ConditionalPreload` に変わった。Out of Scope は GetRow/GetKeysByIndex の挙動を変えないことを求めている。2 つの関数が同じ動きをするとは、engine source が手元に無いため確かめられなかった。根拠は author の主張と、build 成功・DataIndexerEd 77 tests 成功の結果だけ。動きが違えば `RF_NeedLoad` のガードの扱いだけが変わりうる。
  NOTE: …/K2Node_DataIndexerHandleNodeBase.cpp:7 — 削除された `DataIndexerEditorInterface.h` の include を確かめた。HandleNodeBase.cpp に `IDataIndexerInterface_EditorData` / `GetEditorData` への参照は残っていない。消えたメンバ `PreloadRepositoryChain(const UEdGraphPin*)` の呼び出し 2 か所 (193, 205) は、新しい free 関数への呼び出しに置き換わっている。新しい関数の参照は 4 件ある。kind 3 と kind 5 に当たるものはない。
  ```
  2 つ目の NOTE への答え: UE 5.8 `K2Node.h:558` の `PreloadObject` は `if (ReferencedObject) ReferencedObject->ConditionalPreload();` だけ (5.7 も `K2Node.h:553` で同じ)。挙動は同一。

### verify sweep 2026-10-29 05:12 (ALICE、合格 → done)
- 環境: エディタは HEAD のビルド (UnrealEditor-DataIndexerEd.dll 2026-10-28 21:29 = submodule c9ca5c7 の木)。UE MCP 127.0.0.1:8001 を `evidence/_harness/mcp.py` で HTTP から叩いた。ユーザーは不在で、GUI 操作は dispatch で許可済み。証跡 `evidence/t261028-12a6/gui-restart-verify.txt` と `gui-after-restart-case-pins.png`。
- V1: `/Game/__Verify/DI_VItemRarity` (DI_ItemRarity の複製、行名を VCommon… に変更) と `/Game/__Verify/BP_VSwitch` を作った。BP には Switch on Primary Key (add_node_pin ×2 → 2B8F0001 / 0002) と Select by Primary Key (同じキー) を置き、BeginPlay → Switch → PrintString、Select → PrintString.InString と繋いだ。コンパイルして保存した。
- V2 / AC2: エディタを再起動した (PID 27548 → 34332)。開く前に `find_object` で DI_VItemRarity と BP_VSwitch が未読み込みであることを確かめた (DI_ItemRarity は読み込み済み)。`OpenEditorForAsset` で BP を開いた。`GetLogEntries(LogBlueprint, "is not in")` は 0 件。修正前は同じ手順で 2 件出ていた (5a97 の followup-02b)。
- V3 / AC3: 再起動後の初回表示で、Switch の case ピンは VCommon / VUncommon、Select のピンも VCommon / VUncommon (gui-after-restart-case-pins.png)。修正前は GUID だった。
- V4 / AC4: Switch の PinKeys に AAAAAAAA-… を足して `compile_blueprint` → `PrimaryKey AAAAAAAA-… is not in DI_VItemRarity in Switch on Primary Key` が 1 件出た。同じ `GetLogEntries` の問い合わせがこの行を返した。V2 の 0 件の対照になる。
- AC1: `rg PreloadRequiredAssets` は Switch / Select の .h / .cpp で 4 件 (実装時に確認済み)。
- 一時アセットは消した (`exists` false)。

## Summary
Switch / Select PrimaryKey の `PreloadRequiredAssets` で Repository → EditorData → RowStruct を先にシリアライズさせた (HandleNodeBase の `PreloadRepositoryChain` を `DataIndexerK2NodeUtils` へ移して共有)。原因は実測で確定: 読み込み時のコンパイルの時点で import の Repository が `RF_NeedLoad`・EditorData なしだった。headless の再現 (Select) で偽エラー 4 件 → 0 件、case ピン名 GUID → 行の表示名。実在しないキーの検出は残る。2026-10-29 の verify sweep で Switch も GUI で確認した (再起動後の読み込みで `is not in` 0 件、case ピンは行の表示名)。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerK2NodeUtils.h` / `.cpp` — `PreloadRepositoryChain( const UDataIndexerRepository* )` を追加
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerHandleNodeBase.h` / `.cpp` — private メンバを消し、共有関数を呼ぶ
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSwitchPrimaryKey.h` / `.cpp` — `PreloadRequiredAssets` override
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSelectPrimaryKey.h` / `.cpp` — `PreloadRequiredAssets` override
- `.agents/tasks/evidence/t261028-12a6/` — headless 再現スクリプトとログ抜粋 (`headless-repro.txt`)
- `.agents/tasks/KNOWLEDGE.md` — 畳んだ 軽微 (headless 再現の手順 1 行)
- `.agents/tasks/evidence/t261028-12a6/gui-restart-verify.txt` / `gui-after-restart-case-pins.png` — verify sweep の証跡

## Blockers
None
