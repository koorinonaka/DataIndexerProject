---
status: ready
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

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし
