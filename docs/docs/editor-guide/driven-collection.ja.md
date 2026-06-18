# Driven Collection

`UDataIndexerDrivenCollection` は、`FDataIndexerPrimaryKey` をキーとするサブアセット — アイコン・アビリティクラス参照・その他の `UObject` ポインタなど — を管理するエディタアセット用の抽象 `UDataAsset` 基底クラスです。エントリはソースRepositoryと自動的に同期されます。

!!! note "C++ 必須"
    この機能を利用するには C++ でのサブクラス化が必要です。エントリビルダーパターンには C++ のテンプレートインスタンス化が必要なため、Blueprint サブクラス化はサポートされていません。

## コンセプト

Driven Collection は `UDataIndexerRepository` のコンパニオンアセットです。Repositoryと同じ `FDataIndexerPrimaryKey` セットをキーとする `TMap` を保持し、各値には行データの内部に置けない・置くべきでないアセットや構造体を格納します。エディタでソースRepositoryから行が追加・削除されると、コレクションは自動的にエントリを再構築します。

**主なユースケース：**

- 行データに含めるべきでない行ごとのアセット参照（例：アイコン・メッシュ・アビリティクラス）。アセット参照を分けることで、ハード参照によるロード管理を行データと切り離せます。
- 親を含めた行一覧に対してのデータ設定。

例として `CharacterClassIconCollection` は、`DI_CharacterClass` Repository の各クラス（Warrior / Mage / Rogue / Priest）にアイコンテクスチャを 1 つずつ紐付けます。キーは読み取り専用で表示名に解決され、`Source Repository` の行に追従します。

![CharacterClassIconCollection の詳細パネル。クラスごとのアイコンマップと Source Repository](../assets/images/driven-collection-details.png)

## SourceRepository

```cpp
UPROPERTY(EditDefaultsOnly, Category = Settings)
TObjectPtr<UDataIndexerRepository> SourceRepository;
```

アセットの Details パネルで設定します。エディタで `SourceRepository` が保存・ロードされると、コレクションは `EntryBuilder` の `Rebuild()` を呼び出します。

!!! warning "エディタ専用の再構築"
    再構築メカニズム（`TEntryBuilder`・`PostEditChangeProperty`・アセットロードデリゲート）はすべて `WITH_EDITOR` です。コレクションのデータは保存時にアセットに焼き込まれ、ランタイムでは通常の UPROPERTY シリアライズでアクセスされます。ランタイムにRepositoryへの依存はありません。

## C++ でのサブクラス化

`TEntryBuilder<TValue>` を実装してコンストラクタで割り当てます。

```cpp
// MyDrivenCollection.h
UCLASS()
class UMyDrivenCollection : public UDataIndexerDrivenCollection
{
    GENERATED_BODY()

public:
    UMyDrivenCollection();

    UPROPERTY(EditDefaultsOnly, EditFixedSize,
        meta = (ReadOnlyKeys, Repository = "SourceRepository"))
    TMap<FDataIndexerPrimaryKey, FMyCurveData> Entries;

#if WITH_EDITOR
private:
    class FMyEntryBuilder : public TEntryBuilder<FMyCurveData>
    {
    public:
        explicit FMyEntryBuilder(UMyDrivenCollection& Asset)
            : TEntryBuilder(Asset) {}

    private:
        virtual TMap<FDataIndexerPrimaryKey, FMyCurveData>& GetEntries() const override
        {
            return GetAsset<UMyDrivenCollection>().Entries;
        }

        virtual FMyCurveData GetDefaultValue(const FDataIndexerPrimaryKey& Key) const override
        {
            return FMyCurveData{};
        }
    };
#endif
};

// MyDrivenCollection.cpp
UMyDrivenCollection::UMyDrivenCollection()
{
#if WITH_EDITOR
    EntryBuilder = MakeShared<FMyEntryBuilder>(*this);
#endif
}
```

!!! warning "エントリ TMap に必須のメタデータ"
    エントリ `TMap` には次の指定子が**必須**です。欠けると Details パネルが正しく動作しません。

    | 指定子 | 欠けた場合の挙動 |
    | --- | --- |
    | `meta = (Repository = "SourceRepository")` | キーセルが所属Repositoryを解決できず、表示名の代わりに空欄 / `None` が表示される。 |
    | `meta = (ReadOnlyKeys)` | キーが読み取り専用ラベルではなく編集可能なセレクタになり、Repository駆動のキーセットと不整合を起こせてしまう。 |
    | `EditFixedSize` | ユーザーが手動でマップエントリを追加・削除でき、エントリセットを所有する `Rebuild()` と競合する。 |

    `Repository` の値はアセットに対して解決されるプロパティパスです。基底クラスで宣言された `SourceRepository` プロパティを指すように指定します。

## 再構築の動作

`TEntryBuilder<TValue>::Rebuild()` は安定したマージを実行します。

1. `SourceRepository` からすべてのPrimaryKeyを走査する
2. Repositoryに存在しなくなったキーのエントリを削除する
3. 新しいキーにデフォルト構築されたエントリを追加する
4. Repositoryの行順にマッチするようエントリを安定ソートする

キーが引き続き存在する既存エントリはそのまま残ります — 値は再構築後も保持されます。

## ランタイムアクセス

コレクションのエントリマップはアセットにベイクされ、他の UPROPERTY と同様にシリアライズされるため、ソース Repository なしでランタイムから読み取れます。`FDataIndexerPrimaryKey` で値を引く `BlueprintCallable` の getter を公開し、Blueprint からアセットを参照できるようクラスに `BlueprintType` を付けます。

```cpp
UCLASS(BlueprintType)
class UMyDrivenCollection : public UDataIndexerDrivenCollection
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = UI)
    TSoftObjectPtr<UTexture2D> GetIcon(const FDataIndexerPrimaryKey& Key) const;

    // ... エントリマップとエディタ用 builder は上記と同じ ...
};

// .cpp
TSoftObjectPtr<UTexture2D> UMyDrivenCollection::GetIcon(const FDataIndexerPrimaryKey& Key) const
{
    return Entries.FindRef(Key); // Key が無ければ空の soft ptr
}
```

!!! tip "ソフト参照"
    loaded な `UTexture2D*` ではなく `TSoftObjectPtr` を返すことで、ロードのタイミングを呼び出し側に委ねられます — アセット参照を行データの外に置く本来の目的です。呼び出し側は必要に応じて `LoadSynchronous()` や非同期ロードを行えます。
