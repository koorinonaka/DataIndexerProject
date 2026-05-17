---
title: リポジトリ
---

# リポジトリ

`UDataIndexerRepository` は `UDataAsset` のサブクラスで、型付きの行コレクションを格納します。DataIndexer ワークフローの主要な成果物です。デザイナーはここで行をオーサリングし、ランタイムコードはここから行をクエリします。

## 内部ストレージ

| フィールド | 型 | 役割 |
|-----------|---|------|
| `SchemaClass` | `TSubclassOf<UDataIndexerSchema>` | 行構造体とエディタ動作を決定する |
| `LocalEntries` | `TMap<FDataIndexerPrimaryKey, FInstancedStruct>` | このリポジトリが直接所有する行 |
| `EntryOwners` | `TMap<FDataIndexerPrimaryKey, TObjectPtr<UDataIndexerRepository>>` | 階層内でどのリポジトリが各キーを所有するかを追跡する |
| `ReverseLookups` | `TMap<FDataIndexerIndex, FLookupIndex>` | 保存時に構築されるセカンダリインデックステーブル |
| `ParentRepositories` | `TArray<TObjectPtr<UDataIndexerRepository>>` | 継承元リポジトリ（エディタ専用のオーサリング機能） |

## 親 Repository の合成

リポジトリは 1 つ以上の親リポジトリを参照できます。走査・クエリ時には階層が再帰的に辿られ、親の行は子を通して見えるようになります。

これにより、共有ベーステーブル（例：グローバルアイテムリポジトリ）とドメイン固有のオーバーレイ（例：ショップ用サブセット）を行を複製せずに構築できます。

!!! note "循環検出"
    `IncludesRepository(Repository)` で親チェーンの循環を検出します。エディタは循環参照を防ぎます。

### オーバーライドモードと `NotOverridable`

行が親リポジトリに由来し、子の Data View がその行を表示する場合、その行は*オーバーライドモード*に入ります。子で編集するとローカルオーバーライドが作成され、親のデータは変更されません。対応する **User Defined Struct** 変数に `NotOverridable` メタデータを付加することで、このモードで特定フィールドをロックできます。

=== "C++"

    ネイティブ C++ 構造体の場合は `const` またはアクセス制御を使用してください。UDStruct フィールドの場合、カスタマイズレイヤーがこのキーを管理します。

    ```cpp
    // オーバーライドモードで UDStruct 変数をロック
    FStructureEditorUtils::SetMetaData(
        UserDefinedStruct, VariableGuid,
        DataIndexer::MetaDataKeys::NotOverridable, TEXT("true"));

    // プロパティカスタマイズ内で確認
    if (PropertyAndParent.Property.HasMetaData(DataIndexer::MetaDataKeys::NotOverridable))
    {
        // プロパティを読み取り専用でレンダリング
    }
    ```

=== "Blueprint"

    **User Defined Struct** エディタを開きます。変数を選択し、**Details** パネルで **Not Overridable** をオンにします。有効にすると、親行をオーバーライドした子リポジトリではこのフィールドを編集できなくなります。

## パブリック API

### `GetSchema()`

```cpp
const UDataIndexerSchema* GetSchema() const;
```

このリポジトリにバインドされたスキーマ CDO を返します。スキーマクラスが設定されていない場合は `nullptr` を返すことがあります。

### `FindRowEntity(Key)`

```cpp
FConstStructView FindRowEntity(const FDataIndexerPrimaryKey& Key) const;
```

プライマリキーで行を検索し、ローカルマップを参照した後、親リポジトリを再帰的に検索します。見つからない場合は空の `FConstStructView` を返します。`FConstStructView::GetPtr<const TMyRow>()` でキャストするか、`TNativeSchemaInterface<T>::FindRow` を使用してください。

### `ForEachPrimaryKeys(Callback)`

```cpp
void ForEachPrimaryKeys(const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

このリポジトリを通して見えるすべてのプライマリキーを走査します（親を含む）。ローカル行は `LocalEntries` の挿入順、親行はその後に続きます。

### `ForEachPrimaryKeys(Index, Query, Callback)`

```cpp
void ForEachPrimaryKeys(
    const FDataIndexerIndex& Index,
    const FConstStructView Query,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

セカンダリインデックス検索にマッチするプライマリキーを走査します。`Query` は部分的に埋めた行構造体で、スキーマのビルダー関数がこれを元にルックアップ GUID を算出し、`ReverseLookups` を直接参照します。全行の走査は行いません。

### `GetDisplayName(PrimaryKey)`

```cpp
FText GetDisplayName(const FDataIndexerPrimaryKey& PrimaryKey) const;
```

スキーマの `GetRowDisplayName` に委譲します。エディタと Blueprint ノードが人間可読なラベルを表示するために使用します。

## スキーマによるピッカーの絞り込み

クラスが `UDataIndexerRepository` UPROPERTY を持ち、特定のスキーマのリポジトリのみに制限したい場合は `meta = (Schema = "AssetPath")` を追加します。エディタはパスを解決し、アセットピッカーを対応するリポジトリのみに絞り込みます。

=== "C++"

    ```cpp
    UPROPERTY(EditDefaultsOnly, Category = DataIndexer,
        meta = (Schema = "/Game/DataIndexer/DA_MySchema.DA_MySchema"))
    TObjectPtr<UDataIndexerRepository> MyRepository;
    ```

=== "Blueprint"

    Blueprint 変数の **Details** パネルを開きます。`UDataIndexerRepository` 変数を選択すると **Schema** ピッカーが表示されます。スキーマアセットを選択すると、アセットピッカーが一致するリポジトリのみに絞り込まれます。ピッカーをクリアするとすべてのリポジトリが表示されます。

## シリアライズ

バイナリアセット形式はカスタムの `Serialize` オーバーライドで `LocalEntries`・`EntryOwners`・`ReverseLookups` をシリアライズします。`ReverseLookups` は保存時にスキーマの `BuildIndexFunctions` から再構築されます。

JSON エクスポートはリポジトリの内容を人間可読かつ差分確認に適した形式で出力します。[JSON サポート](../editor-guide/json-support.md) を参照してください。
