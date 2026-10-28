---
status: verify_pending
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
- 結果 (2026-10-28): `Build.bat DataIndexerProjectEditor Win64 Development` → `Result: Succeeded` (24 s、DataIndexerEd と DataIndexerToolset を再リンク)。
  `Automation RunTests DataIndexerEd.Compiler` → Found 4、4/4 `Result={Success}`、`EXIT CODE: 0`。full は run 終端。

## Verification
残り (GUI、UE MCP 停止中のため未実施):
1. 未コンパイル BP (新規 BP を作って保存せず) に Switch on Primary Key を配置して落ちないこと。
2. Switch 入り BP を保存 → エディタ再起動 → 関数ピン (`NotEqual_PrimaryKeyPrimaryKey`) の `DefaultObject` が入っていること (PostLoad 削除の裏付け)。
3. Switch の `PinKeys` に同じキーを 2 つ → コンパイルエラー 1 件 (`duplicate PrimaryKey ...`)、exec ピン 1 本。スクリーンショット。
4. Select の `PinKeys` に Repository に無いキー → コンパイルエラー (`PrimaryKey ... is not in ...`)。スクリーンショット。
5. Switch の `Repository` を別 Repository に変える → case ピン表示名が新 Repository の行名。スクリーンショット。
6. Switch の Add pin ボタン: Repository 未使用キーが 1 本ずつ増える。Repository 未設定なら何も増えない。

## Execution Log
- 2026-10-28 11:52 着手 (branch `fix/audit-2026-10`、前提 release/v1.0.2 マージ済みは run 開始時に orchestrator が確認)。
- Step 1: 5.7 `K2Node_Switch.cpp:314-335` の `UK2Node_Switch::CreateFunctionPin` は 5.8 と同一で、`Function ? ... : false` と
  `!BP->SkeletonGeneratedClass ||` の 2 ガードを持つ → override を削除しエンジン実装に戻した。AC1 `rg 'CreateFunctionPin' K2Node_DataIndexerSwitchPrimaryKey.*` = 0 件 (grep -c で .h/.cpp とも 0)。
- Step 2 (PostLoad): 削除。根拠はコード読解: 5.8 `EdGraphPin.cpp:1875` `Ar << DefaultObject` でピンの DefaultObject はバイナリ保存される。
  さらに `BlueprintCompilationManager.cpp:1334` の `ReconstructAllNodes` で再構築時に `CreateFunctionPin` が DefaultObject を再設定する。
  エンジンの他の Switch 派生も PostLoad を持たない。保存→再起動の実機確認は MCP 停止のため Verification 2 に残した。
- Step 3: 新規 `DataIndexerK2NodePinKeys.h/.cpp` (namespace `DataIndexer::K2NodePinKeys`)。`GetValidUniquePinKeys` は Select の private 実装を移した。
  `ValidatePinKeys` は Repository null・無効キー (`PinKeys[i]`)・Repository に無いキー (`FindRowEntity` で include 先も含む)・重複 (キーごとに 1 件) を Error で出す。
- Step 4: Switch `CreateCasePins` を有効一意キーだけに限定。`ValidateNodeDuringCompilation` を override。`RemovePin` は一意ピン前提で「そのキーの全エントリを消す」が正しいため挙動はそのまま、コメントだけ足した。
- Step 5: Switch `PostEditChangeProperty` に `Repository` 条件を追加。
- Step 6: Select の Repository null エラーと重複 Warning ループを `ValidatePinKeys` 呼び出しに置換 (重複は Warning → Error。AC2 の「エラー 1 件」に合わせた)。`Algo/Find.h` の include を削除。
- 既定 (要ユーザー確認可): Switch `AddPinToSwitchNode` は Repository の `ForEachPrimaryKeys` 順で未使用の最初のキーを足してピンを作る。Repository 未設定・全キー使用済みなら何もしない。
  旧挙動 (無効キーを足して同名ピン) は AC の「有効一意キーに限定」に反する。代案「無効キーを PinKeys に足しピンは作らない」へ戻す費用は関数 1 つ (~15 行)。
- 既定: Switch も Repository 未設定をコンパイルエラーにした (タスク Context の ValidatePinKeys 定義どおり)。既存 BP で Repository 未設定の Switch はコンパイルエラーになる。
- quick: 結果は Tests 節。ビルド 1 回。
- 検証待ち: UE MCP (127.0.0.1:8001) が本 run で停止しており、GUI の Verification (screen: required) を実施できない。
- 自己レビュー (stat): submodule は Switch .h/.cpp、Select .cpp、新規 PinKeys .h/.cpp の 5 ファイルのみ。Scope どおり。
- critic r1 (task-critic):
  ```
  PASS
  NOTE: K2Node_DataIndexerSwitchPrimaryKey.cpp:24 — PostLoad は削除済み。Step 2 は実機確認後の削除を求める。根拠はコード読解だけ (EdGraphPin.cpp:1875 と BlueprintCompilationManager.cpp:1334)。実機確認は Verification 2 に残っている。
  NOTE: K2Node_DataIndexerSwitchPrimaryKey.cpp:120 — GetPinNameGivenIndex は今も生の PinKeys[Index] を使う。重複・無効キーがあると index とピンの対応がずれる。repo 内に呼び手は無く、エンジン基底が使う場合だけ影響する。既存コード、diff 外。
  NOTE: DataIndexerK2NodePinKeys.cpp:33 — Switch で Repository 未設定がコンパイルエラーになった。Context の定義どおりなので FAIL にはしない。
  NOTE: K2Node_DataIndexerSelectPrimaryKey.cpp:116 — Select の重複報告は Warning から Error に変わった。消えた Algo/Find.h は使用箇所が無い。
  NOTE: AC2-4 は画面でしか確かめられない。コードの経路は合っている。Verification 1-6 は未実施 (screen: required)。
  ```

## Summary
Switch の `CreateFunctionPin` / `PostLoad` override を削除しエンジン実装 (5.7/5.8 とも null ガード有り) に戻した。
共通 `DataIndexerK2NodePinKeys` に `GetValidUniquePinKeys` / `ValidatePinKeys` を置き、Switch と Select のコンパイル時検証で使う。
Switch は有効一意キーだけでピンを作り、Repository 変更でも再構築する。ビルドと Compiler テストは通過、GUI 検証待ち。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerK2NodePinKeys.h` (新規)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerK2NodePinKeys.cpp` (新規)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSwitchPrimaryKey.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSwitchPrimaryKey.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/K2Node_DataIndexerSelectPrimaryKey.cpp`

## Blockers
検証待ち: UE MCP 停止のため GUI の Verification 1-6 が未実施。実装は残っていない。
