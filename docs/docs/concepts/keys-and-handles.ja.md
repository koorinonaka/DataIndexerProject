# Keys & Handles

DataIndexer は行を特定するために 3 種類のアドレス型を使用します。それぞれ異なるコンテキストに最適化されています。

## FDataIndexerPrimaryKey

`FDataIndexerPrimaryKey` は `FGuid` を継承し、リポジトリ内の単一行を安定・恒久的に識別します。

```cpp
USTRUCT(BlueprintType)
struct DATAINDEXER_API FDataIndexerPrimaryKey : public FGuid { ... };
```

**主な特性：**

- エディタで行を作成したときに 1 度だけ生成され、以後変更されない。
- アセットの移動・リネーム・リポジトリのリファクタリングを経ても維持される。
- リポジトリの `LocalEntries` のキーとして使用される。
- メタデータから所有リポジトリを特定・保存し、追跡できるようになる。

**使うべき場面：** 低レベル C++ での走査（`ForEachPrimaryKeys`）、ゲームシステムでのマップキー、インデックス検索テーブルの構築。

### `Repository` メタデータ

アクター・データアセット・構造体の `UPROPERTY` にベアなプライマリキーを保存できます。エディタが行ピッカーを表示するリポジトリを特定できるよう `meta = (Repository = "...")` を指定してください。

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

    このキーは `UPROPERTY` 指定子では設定しません。エディタカスタマイズが Blueprint 変数メタデータ API で管理します。

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

    Blueprint 変数の **Details** パネルを開きます。`FDataIndexerPrimaryKey` 変数を選択し、**Read Only Keys** チェックボックスをオンにします。オンにすると行ピッカーが非表示になり、格納されているキー値がロックされます。

## FDataIndexerRowHandle

`FDataIndexerRowHandle` はリポジトリ参照とプライマリキーをペアにします。Blueprint 変数や単一行へのアセット参照に推奨される UPROPERTY 型です。

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
- `GetRowDisplayName()` — スキーマ駆動の表示ラベルを `TOptional<FText>` で返す。ハンドルが無効な場合は empty。
- エディタはこれを名前付き行ピッカーとしてレンダリングし、スキーマからの表示名を表示する。

**使うべき場面：** 特定の行を指したいアクター・データアセット・セーブゲーム構造体の UPROPERTY。行参照用の Blueprint 変数。

## FDataIndexerKeysHandle

`FDataIndexerKeysHandle` はセカンダリインデックスを使って行のセットをアドレスします。リポジトリとインデックス識別子を格納し、マッチする行のセットはクエリ時に部分的に埋めた行構造体を渡して決定します。

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
- Blueprint 関数ライブラリの `GetRowsHandleKeys(Handle, Query)` を使って `TArray<FDataIndexerPrimaryKey>` を取得できる。Blueprint では `Query` はワイルドカード構造体ピン。

**使うべき場面：** Blueprint やアセットがインデックスを指定しておき、フィルター値（クエリ構造体）は呼び出し時に決める場合。正確なセットはランタイムにリポジトリの逆引きテーブルから解決される。

## まとめ

| 型 | アドレス対象 | Blueprint 対応 | リポジトリを保持 | クエリタイミング |
|----|------------|---------------|----------------|----------------|
| `FDataIndexerPrimaryKey` | 1行 | Yes (BlueprintType) | No | n/a |
| `FDataIndexerRowHandle` | 1行 | Yes (BlueprintType) | Yes | n/a |
| `FDataIndexerKeysHandle` | N行（インデックス経由） | Yes (BlueprintType) | Yes | クエリ構造体を呼び出し時に渡す |
