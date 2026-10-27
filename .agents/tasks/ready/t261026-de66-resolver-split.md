---
status: ready
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
- `rg 'FEditorScriptExecutionGuard' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` の呼び出し元が `EvaluateRepositoryPath` 1 箇所 (現在: Resolver 内 1 箇所だが Tick から到達)。
- `SDataIndexerGraphPin_PrimaryKey::Tick` から `EvaluateRepositoryPath` への経路が、兄弟ピン変化時以外に無い (ログカウンタ: グラフを開いて静止で 0 回/秒。現在: 3 回/フレーム)。
- `Repository` 未設定の Select ノードの PrimaryKey ピッカーが、BP 変数 `Repository` の行を出さない (現在: 出す)。
- `rg 'GraphPinObj->' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerGraphPin_*Key.cpp` の各行が null ガード後 (読みで確認)。
- `rg 'ResolveSiblingRepository'` が 1 定義 + 2 呼び手。

## Tests
- 自動テストなし (Slate/GraphPin は BP エディタ経由)。Tests 4 問: 既存テーブルなし、Slate 起動 fixture は本番の呼び手が要らない口。
- quick: ビルド + `DataIndexerEd.Compiler` (回帰確認のみ)。

## Verification
- GetRow ノード入り BP グラフを開いて静止 → log のカウンタ 0。Repository ピンを繋ぎ替え → キー ピッカーが更新されるスクリーンショット。
- Select ノード `Repository` 未設定でのピッカー (空) のスクリーンショット。
- Data View の Repository セル・TOptional プロパティ・エディタ起動直後に開く Repository の 3 シナリオで警告/クラッシュなし (log)。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
