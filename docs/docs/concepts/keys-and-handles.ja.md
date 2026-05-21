# Keys & Handles

DataIndexer は行を特定するために 3 種類のアドレス型を使用します。それぞれ異なるコンテキストに最適化されています。

## FDataIndexerPrimaryKey

`FDataIndexerPrimaryKey` は `FGuid` を継承し、Repository内の単一行を安定・恒久的に識別します。

```cpp
USTRUCT(BlueprintType)
struct DATAINDEXER_API FDataIndexerPrimaryKey : public FGuid { ... };
```

**主な特性：**

- エディタで行を作成したときに 1 度だけ生成され、以後変更されない。
- Assetの移動・リネーム・Repositoryのリファクタリングを経ても維持される。
- Repositoryの `LocalEntries` のKeyとして使用される。
- メタデータから所有Repositoryを特定・保存し、追跡できるようになる。

**使うべき場面：** 低レベル C++ での走査（`ForEachPrimaryKeys`）、ゲームシステムでのマップKey、Index検索テーブルの構築。

### `Repository` メタデータ

アクター・データAsset・構造体の `UPROPERTY` にベアなプライマリKeyを保存できます。エディタが行ピッカーを表示するRepositoryを特定できるよう `meta = (Repository = "...")` を指定してください。

値には同じクラス上の `UDataIndexerRepository*` を返す**プロパティ名または関数名**を指定します。引数なしの `UFUNCTION` も使用できます。

=== "C++"

    ```cpp
    // プロパティ参照 — 同じオブジェクト上の UPROPERTY を指定
    UPROPERTY(EditAnywhere, meta = (Repository = "Repository"))
    FDataIndexerPrimaryKey SingleKey;

    UPROPERTY(EditAnywhere, meta = (Repository = "Repository"))
    TArray<FDataIndexerPrimaryKey> MultipleKeys;

    // 関数参照 — UDataIndexerRepository* を返す引数なし UFUNCTION
    UPROPERTY(EditAnywhere, meta = (Repository = "GetRepository"))
    FDataIndexerPrimaryKey KeyFromFunction;
    ```


=== "Blueprint"

    Blueprint 変数の **Details** パネルを開きます。`FDataIndexerPrimaryKey` 変数を選択すると **Repository** ドロップダウンが表示されます。行ピッカーに使用するプロパティ名または関数名を選択してください。
### `ReadOnlyKeys` メタデータ

`FDataIndexerPrimaryKey` Blueprint 変数を読み取り専用にします。行ピッカーが非表示になり、値はプログラムからのみ設定できます。

=== "C++"

    ```cpp
    UPROPERTY(EditAnywhere, meta = (ReadOnlyKeys))
    FDataIndexerPrimaryKey ReadOnlyKey;
    ```

    格納値は Blueprint 変数メタデータ API からプログラムで制御することもできます。

    ```cpp
    // ロック — 行ピッカーを非表示
    FBlueprintEditorUtils::SetBlueprintVariableMetaData(
        Blueprint, VarName, nullptr,
        DataIndexer::MetaDataKeys::ReadOnlyKeys, TEXT("true"));

    // ロック解除 — 行ピッカーを再表示
    FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(
        Blueprint, VarName, nullptr,
        DataIndexer::MetaDataKeys::ReadOnlyKeys);
    ```


=== "Blueprint"

    Blueprint 変数の **Details** パネルを開きます。`FDataIndexerPrimaryKey` 変数を選択し、**Read Only Keys** チェックボックスをオンにします。オンにすると行ピッカーが非表示になり、格納されているKey値がロックされます。
## FDataIndexerRowHandle

`FDataIndexerRowHandle` はRepository参照とプライマリKeyをペアにします。Blueprint 変数や単一行へのAsset参照に推奨される UPROPERTY 型です。

```cpp
USTRUCT(BlueprintType, meta = (PresentAsType = "NamedStruct"))
struct DATAINDEXER_API FDataIndexerRowHandle
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UDataIndexerRepository> Repository;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDataIndexerPrimaryKey PrimaryKey;
    ...
};
```

**主な特性：**

- `IsValid()` — `Repository` と `PrimaryKey` の両方が null/ゼロでない場合に true を返す。
- `FindRowEntity()` — `Repository->FindRowEntity(PrimaryKey)` に委譲する。
- `GetRowDisplayName()` — Schema駆動の表示ラベルを `TOptional<FText>` で返す。ハンドルが無効な場合は empty。
- エディタはこれを名前付き行ピッカーとしてレンダリングし、Schemaからの表示名を表示する。

**使うべき場面：** 特定の行を指したいアクター・データAsset・セーブゲーム構造体の UPROPERTY。行参照用の Blueprint 変数。

### `Repository` メタデータ

ハンドルのRepositoryピッカーを特定のRepositoryに制限します。エディタがRepositoryを事前選択または絞り込みできるよう `meta = (Repository = "...")` を指定してください。

値には同じクラス上の `UDataIndexerRepository*` を返す**プロパティ名または関数名**を指定します。引数なしの `UFUNCTION` も使用できます。

=== "C++"

    ```cpp
    // プロパティ参照
    UPROPERTY(EditAnywhere, meta = (Repository = "Repository"))
    FDataIndexerRowHandle RowHandle;

    // 関数参照 — UDataIndexerRepository* を返す引数なし UFUNCTION
    UPROPERTY(EditAnywhere, meta = (Repository = "GetRepository"))
    FDataIndexerRowHandle RowHandleFromFunction;
    ```


=== "Blueprint"

    Blueprint 変数の **Details** パネルを開きます。`FDataIndexerRowHandle` 変数を選択すると **Repository** ドロップダウンが表示されます。Repositoryを提供するプロパティ名または関数名を選択してください。
### `Schema` メタデータ

ハンドルのRepositoryピッカーを特定のSchemaのRepositoryに絞り込みます。`meta = (Schema = "AssetPath")` を追加してください。エディタはパスを解決し、Assetピッカーを一致するRepositoryのみに絞り込みます。

=== "C++"

    ```cpp
    UPROPERTY(EditAnywhere,
        meta = (Schema = "/Game/DataIndexer/DA_MySchema.DA_MySchema"))
    FDataIndexerRowHandle RowHandle;
    ```


=== "Blueprint"

    Blueprint 変数の **Details** パネルを開きます。`FDataIndexerRowHandle` 変数を選択すると **Schema** ピッカーが表示されます。SchemaAssetを選択するとRepositoryピッカーが一致するRepositoryのみに絞り込まれます。ピッカーをクリアするとすべてのRepositoryが表示されます。
## FDataIndexerKeysHandle

`FDataIndexerKeysHandle` はセカンダリIndexを使って行のセットをアドレスします。RepositoryとIndex識別子を格納し、マッチする行のセットはQuery時に部分的に埋めた行構造体を渡して決定します。

```cpp
USTRUCT(BlueprintType)
struct DATAINDEXER_API FDataIndexerKeysHandle
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UDataIndexerRepository> Repository;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDataIndexerIndex Index;
    ...
};
```

**主な特性：**

- `IsValid()` — Repository と Index が null/ゼロでないことを確認する。
- `ForEachPrimaryKeys(Query, Callback)` — `Repository->ForEachPrimaryKeys(Index, Query, Callback)` に委譲する。`Query` は部分的に埋めた行構造体の `FConstStructView`。
- Blueprint では `GetKeysByIndex` カスタム K2Node を使って `TArray<FDataIndexerPrimaryKey>` を取得できる。

**使うべき場面：** Blueprint やAssetがIndexを指定しておき、Filterー値（Query構造体）は呼び出し時に決める場合。正確なセットはランタイムにRepositoryの逆引きテーブルから解決される。

## まとめ

| 型 | アドレス対象 | Blueprint 対応 | Repositoryを保持 | Queryタイミング |
|----|------------|---------------|----------------|----------------|
| `FDataIndexerPrimaryKey` | 1行 | Yes (BlueprintType) | No | n/a |
| `FDataIndexerRowHandle` | 1行 | Yes (BlueprintType) | Yes | n/a |
| `FDataIndexerKeysHandle` | N行（Index経由） | Yes (BlueprintType) | Yes | Query構造体を呼び出し時に渡す |
