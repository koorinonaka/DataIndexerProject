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

**UPROPERTY に保存する場合：** アクターやデータアセットの UPROPERTY にベアなプライマリキーを保存することもできます。その場合は `meta = (Repository = "PropertyPath")` を指定してリポジトリを特定できるようにすると便利です。

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
