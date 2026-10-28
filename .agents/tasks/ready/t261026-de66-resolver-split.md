---
status: verify_pending
order: 130
tier: standard
screen: required
agent: any
created_by: task-create
---

# Repository 解決を「パス探索」と「評価」に分け、ウィジェットが 1 回だけ解決して保持する (X3)

## Goal
PrimaryKey / IndexKey の GraphPin と PropertyTypeCustomization が、Tick / TAttribute から毎フレーム Repository 解決
(スクリプト実行つき) を呼ばず、Construct / CustomizeHeader で 1 回解決して `TWeakObjectPtr` を保持し、明示の無効化でだけ再解決する。
CDO fallback が無関係な Repository を拾わない。

## Context
監査 `.agents/audit-2026-10-26.md` の X3 = R-C1, P-C1, P-C2, B-C9, B-C10, R-C5。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` 相対、行番号は 2026-10-26 時点。

- 根: `DataIndexerPrimaryKeyRepositoryResolver.cpp:23-36` が `PropertyPathHelpers::GetPropertyValue` + `FEditorScriptExecutionGuard` で
  **スクリプトを実行する**解決を、`DataIndexerGraphPin_PrimaryKey.cpp:53` (Tick) / `:100,115` (attribute)、`DataIndexerGraphPin_IndexKey.cpp:75,117`、
  `DataIndexerPropertyTypeCustomization_PrimaryKey.cpp:99,134` から毎フレーム呼ぶ。直近 3 fix (thumbnail pool fallback `36a28c8`、TOptional 走査
  `03a2482`、PostLoad guard `6032354`) は全部「今解決してはいけない場所」が増えた症状。
- P-C1: PrimaryKey ピン 1 本につき毎フレーム 3 回フル解決 (`FindPinByPredicate`、`GetPinMetaData` 文字列処理、`FindObjectSafe<UFunction>`
  フルパス検索、`PropertyPathHelpers` + BP 呼び出し)。
- P-C2: `DataIndexerGraphPin_PrimaryKey.cpp:119`, `DataIndexerPropertyTypeCustomization_PrimaryKey.cpp:136` `GetDisplayName` を毎フレーム →
  階層 `FindRepository` + `EditorRows.IndexOfByPredicate` + Schema の `RowDisplayNameFunction` BP 呼び出し。FText をキャッシュし値変更で無効化。
- B-C9: `DataIndexerPrimaryKeyRepositoryResolver.cpp:139-146` ノード自身で解決失敗 → meta パスを Blueprint CDO で評価。Select / Switch のパスは
  `"Repository"` (ノード自身の UPROPERTY) なので、`Repository` 未設定ノードが BP 変数 `Repository` を拾う。CDO fallback をノードプロパティでないパスに限定。
- B-C10: `DataIndexerGraphPin_PrimaryKey.cpp:100,105,115,139`, `DataIndexerGraphPin_IndexKey.cpp:75,117,141` `*GraphPinObj` を null 未確認で deref
  (`ShouldUseComboBox:36` は確認している)。再構築中に `SGraphPin` が `UEdGraphPin` より長生きし attribute は poll され続ける。
- R-C5: `DataIndexerPropertyTypeCustomization_PrimaryKey.cpp:73-83` ≡ `DataIndexerPropertyTypeCustomization_IndexKey.cpp:53-64` 兄弟 `Repository` 参照。
  `DataIndexerGraphPin_IndexKey.cpp:20-36` の case 1 は `Resolver.cpp:108-118` (split サブピン含む全入力ピンから Repository default を探す) と冗長。
- advisor / 監査の設計: Resolver を `FindRepositoryMetaPath(FProperty|UEdGraphPin)` (純粋、安い、キャッシュ可) と
  `EvaluateRepositoryPath(UObject&, Path)` (スクリプト実行あり) に分ける。無効化: GraphPin 側は Tick で兄弟 Repository ピンの `DefaultObject` ポインタ +
  `LinkedTo.Num()` の同一性だけ見る (安い)。Details 側は親ハンドルの `SetOnPropertyValueChanged`。

## Scope
- Resolver の 2 分割。CDO fallback の条件。
- 4 ウィジェット (GraphPin_PrimaryKey / GraphPin_IndexKey / PTC_PrimaryKey / PTC_IndexKey) が `TWeakObjectPtr<UDataIndexerRepository> CachedRepository`
  と `FText CachedDisplayName` を持ち、無効化条件でだけ再解決。
- `GraphPinObj` の null ガード。
- `ResolveSiblingRepository(IPropertyHandle&, UScriptStruct* HandleStruct)` の抽出と GraphPin case 1 の削除。

## Out of Scope
- GetRow / GetKeysByIndex の基底化 (t261026-dfba)。`ResolveRepositoryFromPin` は新 API を呼ぶだけに変える。
- コンボ選択肢・表示テキストの重複排除 (t261026-e5c0)。
- Data View セルの Repository ピッカー (`36a28c8` の fallback はそのまま)。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. `DataIndexerPrimaryKeyRepositoryResolver.h/.cpp` に `FindRepositoryMetaPath` / `EvaluateRepositoryPath` を切り、既存の公開関数はその合成に。
   CDO fallback を「パスがノードの UPROPERTY 名でない」ときに限定。
2. `SDataIndexerGraphPin_PrimaryKey` / `_IndexKey`: Construct で解決してキャッシュ。Tick は兄弟 Repository ピンの `DefaultObject` + `LinkedTo.Num()`
   を前回値と比べ、変化時だけ再解決。attribute は キャッシュを返す。`GraphPinObj` null ガード。
3. `PTC_PrimaryKey` / `_IndexKey`: CustomizeHeader で解決、親ハンドルの `SetOnPropertyValueChanged` で再解決。`ResolveSiblingRepository` に共通化。
4. `GetDisplayName` の FText キャッシュを値変更 (ピンの `DefaultValue` / ハンドルの値) で無効化。
5. GraphPin_IndexKey の case 1 を削除。
6. GUI で Verification。既存 3 fix のシナリオ (Data View セルの Repository ピッカー、TOptional プロパティ、PostLoad 中) が回帰しないことを確認。

## Acceptance Criteria
- Repository 解決での `FEditorScriptExecutionGuard` は `EvaluateRepositoryPath` 1 箇所 (`rg 'FEditorScriptExecutionGuard' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` の他の hit は Repository 解決ではないので数えない。`rg ResolveFromObject` = 0 件)。
- `SDataIndexerGraphPin_PrimaryKey::Tick` から `EvaluateRepositoryPath` への経路が、兄弟ピン変化時以外に無い (ログカウンタ: グラフを開いて静止で 0 回/秒。現在: 3 回/フレーム)。
- `Repository` 未設定の Select ノードの PrimaryKey ピッカーが、BP 変数 `Repository` の行を出さない (現在: 出す)。
- `rg 'GraphPinObj->' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_*Key.cpp` の各行が null ガード後 (読みで確認)。
- `rg 'ResolveSiblingRepository'` が定義 1 + 宣言 1 + 呼び手 2。

## Tests
- 自動テストなし (Slate/GraphPin は BP エディタ経由)。Tests 4 問: 既存テーブルなし、Slate 起動 fixture は本番の呼び手が要らない口。
- quick: ビルド + `DataIndexerEd.Compiler` (回帰確認のみ)。

## Verification
- GetRow ノード入り BP グラフを開いて静止 → log のカウンタ 0。Repository ピンを繋ぎ替え → キー ピッカーが更新されるスクリーンショット。
- Select ノード `Repository` 未設定でのピッカー (空) のスクリーンショット。
- Data View の Repository セル・TOptional プロパティ・エディタ起動直後に開く Repository の 3 シナリオで警告/クラッシュなし (log)。

## Execution Log
2026-10-28 (dispatch 14/18, branch `fix/audit-2026-10`)。前提 `release/v1.0.2` の main マージは run 前提として満たされている (orchestrator 確認済み)。

### 実装
1. Resolver 分割 (`DataIndexerPrimaryKeyRepositoryResolver.h/.cpp`)
   - 純粋側: `FindRepositoryMetaPath(const FProperty&)` (旧 `FindPropertyPathMetaData`)、`FindRepositoryMetaPath(const UEdGraphPin&)`
     (ノードの `GetPinMetaData`)、`FindSiblingRepositoryPin(const UEdGraphNode&)` (旧 :90-100 の predicate)。
   - 評価側: `EvaluateRepositoryPath(UObject&, const FString&)` (旧 `ResolveFromObject`)。`IsRoutingPostLoad` の早期 return と
     `FEditorScriptExecutionGuard` をこの関数だけが持つ。`UE_LOG(LogDataIndexer, Verbose, ...)` を 1 行入れた (Verification のカウンタ)。
   - 既存公開関数 (`FindOwnerRepository` / `ResolveFromGraphPin` / `ResolveFromEditorObject` / `ResolveFromStaticFunctionPath`) は上の合成。評価順は不変。
   - B-C9: Blueprint CDO fallback は、パスの先頭セグメントがノードクラスの UPROPERTY 名でないときだけ走る
     (`OwningNode.GetClass()->FindPropertyByName`)。Select / Switch の `"Repository"` は CDO に行かない。
   - `FGraphPinRepositoryCache` (同ヘッダ): 兄弟 Repository ピンの `(DefaultObject, LinkedTo.Num())` を前回値と比べ、変化・初回・weak ptr stale のときだけ
     `ResolveFromGraphPin` を呼ぶ。PostLoad 中の解決は答えとして latch しない (次の Update で再解決)。
2. GraphPin_PrimaryKey / GraphPin_IndexKey: Construct と Tick で `RepositoryCache.Update`。attribute (`HandleVisibleText` / `CanBrowseTo` / `ShouldUseComboBox`)
   はキャッシュだけを読む。表示名は `GetDefaultAsString()` を前回キーと文字列比較し、変化時だけ引き直す。コンボを開く・Browse は利用者の明示操作なので
   `Invalidate()` してから再解決する。IndexKey ピンには Tick が無かったので追加。
3. PTC_PrimaryKey / PTC_IndexKey: `CustomizeHeader` で 1 回解決し `TWeakObjectPtr` に保持。`GetOwnerRepository()` はキャッシュを返す (サブクラス
   `PrimaryKeyOptions` / `PrimaryKeyReference` は無変更)。兄弟 `Repository` 参照は `ResolveSiblingRepository(IPropertyHandle&, const UScriptStruct*)` に共通化
   (親ハンドルの `GetChildHandle("Repository")->GetValue`、構造体レイアウトに依存しない)。
4. 表示名 FText キャッシュ: 4 ウィジェットとも値 (ピン DefaultValue / ハンドル値) の比較で無効化。PrimaryKey の 2 ウィジェットは加えて
   `Delegates::OnEditorRowsUpdated` (行編集時に `UDataIndexerEditorData::NotifyRowsChanged` が broadcast) で同じ Repository なら表示名を捨てる。
5. GraphPin_IndexKey の case 1 (ParentPin->SubPins 探索) を削除。split された KeysHandle の Repository サブピンはノードの Pins に入るので、
   `FindSiblingRepositoryPin` が拾う。
6. `GraphPinObj` null ガード: 両 GraphPin に `GetLivePin()` (`!bGraphDataInvalid && GraphPinObj && !GraphPinObj->bWasTrashed`、エンジンの
   `SGraphPin::GetPinObj` と同条件で ensure なし)。`GraphPinObj->` の残りはこの 1 行だけ。

### タスク本文との差 (記録)
- Step 3 の「親ハンドルの `SetOnPropertyValueChanged`」は `SetOnChildPropertyValueChanged` にした。根拠: `PropertyNode.cpp:3547-3561`
  `BroadcastPropertyChangedDelegates` は子の変更で祖先ノードの `OnChildPropertyValueChanged` を鳴らし、親自身の `PropertyValueChangedEvent` は鳴らさない。
  祖先を全部たどるので、トップレベルのプロパティ (親 = オブジェクトノード) でも同じオブジェクトの別プロパティ変更 (meta パスの参照先) を拾う。意図は同じ。
- 既定: `ResolveSiblingRepository` が null (RowHandle の Repository 未設定) のとき、PTC_PrimaryKey は旧実装と違い `FindOwnerRepository` へ落ちる。
  `FDataIndexerRowHandle` / `FDataIndexerKeysHandle` に Repository meta は無いので実質 null のまま。戻すなら owner struct 判定を PTC 側に 1 つ足す。
- 既定: IndexKey の表示名 (Schema の Index 名) は Index 名の変更では無効化しない。値変更・Repository 変更・ウィジェット再生成で更新。
  ライブにするなら Schema 側の変更通知が要る (今は無い)。
- 既定: Blueprint 変数のデフォルト値 (CDO fallback の参照先) を変えた場合、グラフピンは兄弟ピンの変化として検出しない。
  コンボを開く・Browse・ノード再構築 (BP コンパイル) で再解決する。

### Acceptance Criteria の確認
- AC1 `rg FEditorScriptExecutionGuard Customizations/`: 実際は 4 件 (Resolver:29 `EvaluateRepositoryPath`、`PTC_PrimaryKeyOptions.cpp:35`
  filter 関数、`PTC_Repository.cpp:78`、`PTC_Schema.cpp:37`)。後ろ 3 件は Repository 解決ではなく Scope 外 (起票時の「現在 1 箇所」も同じく不正確)。
  強い同等条件: Repository 解決でスクリプトを走らせる経路は `EvaluateRepositoryPath` 1 箇所 (旧 `ResolveFromObject` 3 呼び出しはすべて置換、`ResolveFromObject` 0 件)。
- AC2 (構造): `SPrimaryKeyGraphPin::Tick` → `UpdateRepository` → `FGraphPinRepositoryCache::Update` は兄弟ピン状態が前回と同じなら `ResolveFromGraphPin` を
  呼ばずに return。`ShouldUseComboBox` / `HandleVisibleText` / `CanBrowseTo` は Resolver を呼ばない (読みで確認)。ログ計測は Verification (GUI) 側に残る。
- AC3: コード上 CDO 分岐は node UPROPERTY 名のパスで走らない。GUI 確認は Verification に残る。
- AC4: `rg 'GraphPinObj->' DataIndexerGraphPin_*Key.cpp` = 2 行、どちらも `GetLivePin()` 内のガード式自身。
- AC5: `rg ResolveSiblingRepository` = 定義 1 (Resolver.cpp) + 宣言 1 (.h) + 呼び手 2 (PTC_PrimaryKey / PTC_IndexKey)。

### quick
- `Build.bat DataIndexerProjectEditor Win64 Development`: 1 回目 `SparseArray.h(990) fatal error C1001` (run notes の transient)。
  再実行 `Result: Succeeded` (21.1 s)。`UnrealEditor-DataIndexerEd.dll` 13:51:32 更新。
- `Automation RunTests DataIndexerEd.Compiler+DataIndexerEd.RowHandle`: Found 14 tests、Success 14 / Fail 0 (Compiler 5, RowHandle 9)。
  RowHandle 群は Resolver を delegate 経由で叩く (TOptional / static function path / graph pin / PostLoad の既存 3 fix の行を含む)。
- Game target: runtime 構造体・`WITH_EDITORONLY_DATA` に触れていない (DataIndexerEd のみ) のでビルド対象外。full は run 終端。

### Verification が未実施の理由
- UE MCP server (127.0.0.1:8001) がこの run では停止 (run notes)。GUI 操作の Verification は全部残る。

### Critic round 1
```
PASS
NOTE: DataIndexerPropertyTypeCustomization_PrimaryKeyOptions.cpp:35 — AC1 を文字どおり読むと満たされない。`rg FEditorScriptExecutionGuard Customizations/` は 4 件ある。残り 3 件 (`PTC_PrimaryKeyOptions.cpp:35`、`PTC_Repository.cpp:78`、`PTC_Schema.cpp:37`) は diff の外にあり、元からあるコード。どれも Repository 解決ではない。AC の「現在: Resolver 内 1 箇所」という基準値がそもそも不正確。Repository 解決側は `DataIndexerPrimaryKeyRepositoryResolver.cpp:29` の 1 箇所だけになっていて、AC の意図は満たしている。そのため FAIL にはしない。タスク作成者が AC の文言を直すべき。
NOTE: DataIndexerPrimaryKeyRepositoryResolver.h:23 — AC5 の「1 定義 + 2 呼び手」について。rg で数えると、ヘッダの宣言が 1 件増えて計 4 件になる。中身は定義 1 (cpp:164)、宣言 1、呼び手 2 (`PTC_PrimaryKey.cpp:100`、`PTC_IndexKey.cpp:55`) で、意図どおり。
NOTE: DataIndexerPropertyTypeCustomization_PrimaryKey.cpp:99 — 挙動が変わっている。RowHandle の Repository が未設定のとき、旧実装は null を返した。新実装は `FindOwnerRepository` へ落ちる。作者は「meta が無いので実質 null」と主張している。外側のプロパティに Repository meta が付いているケースでも null になるかは、diff だけでは確定できない。
NOTE: DataIndexerPrimaryKeyRepositoryResolver.cpp:260 — AC2 (静止時のログカウンタが 0 回/秒) と AC3 (Select ノードのピッカーが空) は GUI でしか確かめられない。コードを読む限り、`FGraphPinRepositoryCache::Update` の早期 return と、CDO 分岐の `FindPropertyByName` ガード (cpp:~107) は条件を満たしている。画面での確認は Verification に残っている。
NOTE: K2Node_DataIndexerGetRow.cpp:83 — Out of Scope には「`ResolveRepositoryFromPin` は新 API を呼ぶだけに変える」とある。今回の diff はこの関数に触れていない。範囲違反ではない。
```
(3 つ目の NOTE は「既定」に記録済み。外側プロパティに Repository meta が付いた RowHandle では Repository 未設定時にその meta 先へ落ちうる。)

### ユーザー決定 2026-10-28 (AC 文言の修正)
- AC1・AC5 を実行者の案 (a) で書き直した。status は変えない。
- AC1 旧文言: 「`rg 'FEditorScriptExecutionGuard' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` の呼び出し元が `EvaluateRepositoryPath` 1 箇所 (現在: Resolver 内 1 箇所だが Tick から到達)。」
- AC1 新文言: Repository 解決での guard は `EvaluateRepositoryPath` 1 箇所。
- AC5 旧文言: 「`rg 'ResolveSiblingRepository'` が 1 定義 + 2 呼び手。」
- AC5 新文言: 定義 1 + 宣言 1 + 呼び手 2。

## Summary
Repository 解決を純粋なメタパス探索 (`FindRepositoryMetaPath` / `FindSiblingRepositoryPin`) とスクリプト実行つき評価 (`EvaluateRepositoryPath`) に分けた。
グラフピン 2 種は `FGraphPinRepositoryCache` で兄弟 Repository ピン変化時だけ再解決し、PTC 2 種は `CustomizeHeader` で 1 回解決して
子プロパティ変更で再解決する。表示名は FText をキャッシュし値変更・行更新で捨てる。CDO fallback はノード UPROPERTY 名のパスで走らない。
ビルドと Compiler / RowHandle の自動テストは通過。GUI の Verification は MCP 停止のため検証待ち。

## Tests
- 自動テストの追加なし (タスク本文どおり)。
- quick: Editor ビルド Succeeded、`DataIndexerEd.Compiler+DataIndexerEd.RowHandle` 14/14 Success。full は run 終端。

## Verification
残り (全部 GUI、UE MCP 必要):
- GetRow ノード入り BP グラフを開いて静止 → `log LogDataIndexer Verbose` で `EvaluateRepositoryPath` 行が出ない。Repository ピンを繋ぎ替え →
  キー ピッカーが更新されるスクリーンショット。
- `Repository` 未設定の Select ノードで、BP 変数 `Repository` がある BP のピッカーが空のスクリーンショット。
- Data View の Repository セル・TOptional プロパティ・エディタ起動直後に開く Repository の 3 シナリオで警告/クラッシュなし (log)。
- Data View で行名を変えたとき、開いている BP の PrimaryKey ピン表示が追従する (OnEditorRowsUpdated)。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPrimaryKeyRepositoryResolver.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPrimaryKeyRepositoryResolver.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_PrimaryKey.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_PrimaryKey.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_IndexKey.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_IndexKey.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_PrimaryKey.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_PrimaryKey.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_IndexKey.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerPropertyTypeCustomization_IndexKey.cpp`
- `.agents/tasks/ready/t261026-de66-resolver-split.md`

## Blockers
検証待ち: GUI の Verification (上記 4 項目) が残る。UE MCP server (127.0.0.1:8001) 停止中のため未実施。実装は残っていない。
