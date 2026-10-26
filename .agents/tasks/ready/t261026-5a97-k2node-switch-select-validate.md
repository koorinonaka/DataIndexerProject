---
status: ready
order: 50
tier: standard
screen: required
agent: any
created_by: task-create
---

# K2Node Switch/Select PrimaryKey の null deref と PinKeys 検証

## Goal
`UK2Node_DataIndexerSwitchPrimaryKey` / `UK2Node_DataIndexerSelectPrimaryKey` が、未コンパイル BP への配置で落ちず、無効・重複・
Repository に無いキーをコンパイルエラーで報告し、Repository 変更後にピン表示名が追随する。

## Context
監査 `.agents/audit-2026-10-26.md` の B-C1, B-C2, B-C8, R-C10。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` 相対、行番号は 2026-10-26 時点。

- B-C2: `K2Node_DataIndexerSwitchPrimaryKey.cpp:141-142,148` `FindUField<UFunction>` の結果を null 未確認で `Function->HasAllFunctionFlags`、
  `BP->SkeletonGeneratedClass->IsChildOf` も null 未確認。エンジン 5.8 `K2Node_Switch.cpp:314-335` の同関数は両方ガードする。
  この override はエンジンのコピーからガードを抜いたもの → override 削除でエンジン実装に戻す。
- R-C10: `:23-31` `PostLoad` が関数ピンの `DefaultObject` を再設定する。5.8 の `UK2Node_Switch::CreateFunctionPin` は
  `SkeletonGeneratedClass` null 時も `DefaultObject` を設定する。ピンはシリアライズ済みなのでロード時に `AllocateDefaultPins` は走らない。
  実行者は 5.8 でロード時ケース (保存→再読込→関数ピンの DefaultObject) を確認し、不要なら削除する。
- B-C1: Switch に `ValidateNodeDuringCompilation` が無い。`CreateCasePins` (`:124-131`) は生の `PinKeys` を使う (無効/重複キー → 同名 exec ピン 2 本)。
  `AddPinToSwitchNode` (`:158`) はクリックごとに無効キーを追加 (2 クリック目で同名ピン)。`RemovePin` (`:165`) は重複を全部消す。
  Select (`K2Node_DataIndexerSelectPrimaryKey.cpp:120-131, 198-232`) は `PinKeys` が `Repository` に存在するか検証せず、`Repository` 変更後も古いキーを残す。
  共通の `ValidatePinKeys(FCompilerResultsLog&)` (Repository null / key ∉ Repository / 重複) を足し、Switch も `GetValidUniquePinKeys` を使う。
  advisor 判断: 共通関数は今は小さい専用ヘッダ `DataIndexerK2NodePinKeys.h` に置く。t261026-dfba の utils ヘッダが後で吸収する。
- B-C8: Switch `:33-41` は `PinKeys` 変更時だけ `ReconstructNode`。Select は `Repository` / `PinKeys` / `bHasDefaultPin` で再構築。
  Switch で `Repository` を変えると `CreateCasePins:129` で 1 回だけ設定した `PinFriendlyName` が古いまま。

## Scope
- Switch: `CreateFunctionPin` override 削除、`PostLoad` の要否確認と削除、`ValidateNodeDuringCompilation` 追加、
  `CreateCasePins` / `AddPinToSwitchNode` を有効一意キーに限定、`PostEditChangeProperty` に `Repository` 条件を追加。
- Select: `ValidateNodeDuringCompilation` から `ValidatePinKeys` を呼ぶ。
- 新規 `DataIndexerK2NodePinKeys.h/.cpp` に `ValidatePinKeys` と `GetValidUniquePinKeys` (Select 側に既にあるなら移動)。

## Out of Scope
- GetRow / GetKeysByIndex の基底クラス化 (t261026-dfba)。
- Resolver の分割 (t261026-de66)。Switch/Select の Repository は自ノードの UPROPERTY で Resolver を通らない。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. `D:\Apps\Epic Games\UE_5.7\Engine\Source\Editor\BlueprintGraph\Private\K2Node_Switch.cpp` の `UK2Node_Switch::CreateFunctionPin` を読み、
   5.8 と同じ null ガード 2 つがあるか確認する。あれば override を削除。無ければ override を残し、ガード 2 つを足す (5.7 archive で
   クラッシュを再導入しないため。README 品質基準の 5.7 規則)。その後ビルドし、未コンパイル BP (新規 BP を作って保存せず) に
   Switch を配置して落ちないことを確認。
2. `PostLoad` を一時的に無効化し、Switch 入り BP を保存 → エディタ再起動 → 関数ピンの `DefaultObject` が入っていることを確認。入っていれば削除。
3. `DataIndexerK2NodePinKeys.h/.cpp` を新設し、Select の重複排除ロジックを移して `ValidatePinKeys` を書く。
4. Switch の `CreateCasePins` / `AddPinToSwitchNode` / `RemovePin` を有効一意キー前提に直す。`ValidateNodeDuringCompilation` を override。
5. Switch の `PostEditChangeProperty` に `Repository` を足す。
6. Select の `ValidateNodeDuringCompilation` から `ValidatePinKeys` を呼ぶ。
7. BP でコンパイルし、エラーメッセージを確認。

## Acceptance Criteria
- `rg 'CreateFunctionPin' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSwitchPrimaryKey.*` が 0 件 (現在 2)、
  または 5.7 にガードが無い場合は override 内に null ガード 2 つ (`rg 'if \( !Function|if \( !.*SkeletonGeneratedClass'` 2 件、現在 0)。
- Switch の `PinKeys` に同じキーを 2 つ入れてコンパイル → エラー 1 件、exec ピンは 1 本 (現在: ピン 2 本、エラーなし)。
- Select の `PinKeys` に Repository に無いキー → コンパイルエラー (現在: 通る)。
- Switch の `Repository` を別 Repository に変えると case ピンの表示名が新 Repository の行名になる (現在: 古い名前)。

## Tests
- 自動テストなし (K2Node は BP コンパイル経由でしか検証できない。既存テストにも K2Node 群は無い)。Tests 4 問: 既存テーブルが無く、
  BP compile を起動する fixture は本番の呼び手が要らない口になるので足さない。
- quick: ビルド + `DataIndexerEd.Compiler` (回帰確認のみ)。

## Verification
- AC 3 項目のスクリーンショット (コンパイル結果パネル、ノードのピン表示)。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
