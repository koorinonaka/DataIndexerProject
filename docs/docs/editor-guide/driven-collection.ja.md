# Driven Collection

`UDataIndexerDrivenCollection` は、`FDataIndexerPrimaryKey` をKeyとするサブAsset — アイコン・アビリティクラス参照・その他の `UObject` ポインタなど — を管理するエディタアセット用の抽象 `UDataAsset` 基底クラスです。エントリはソースRepositoryと自動的に同期されます。

!!! note "C++ 必須"
    この機能を利用するには C++ でのサブクラス化が必要です。エントリビルダーパターンには C++ のテンプレートインスタンス化が必要なため、Blueprint サブクラス化はサポートされていません。

## コンセプト

Driven Collection は `UDataIndexerRepository` のコンパニオンAssetです。Repositoryと同じ `FDataIndexerPrimaryKey` セットをKeyとする `TMap` を保持し、各値には行データの内部に置けない・置くべきでないAssetや構造体を格納します。エディタでソースRepositoryから行が追加・削除されると、コレクションは自動的にエントリを再構築します。

**主なユースケース：**

- 行データに含めるべきでない行ごとのAsset参照（例：アイコン・メッシュ・アビリティクラス）。Asset参照を分けることで、ハード参照によるロード管理を行データと切り離せます。
- 親を含めた行一覧に対してのデータ設定。

## SourceRepository

```cpp
UPROPERTY(EditDefaultsOnly, Category = Settings)
TObjectPtr<UDataIndexerRepository> SourceRepository;
```

Assetの Details パネルで設定します。エディタで `SourceRepository` が保存・ロードされると、コレクションは `EntryBuilder` の `Rebuild()` を呼び出します。

!!! warning "エディタ専用の再構築"
    再構築メカニズム（`TEntryBuilder`・`PostEditChangeProperty`・Assetロードデリゲート）はすべて `WITH_EDITOR` です。コレクションのデータは保存時にAssetに焼き込まれ、ランタイムでは通常の UPROPERTY シリアライズでアクセスされます。ランタイムにRepositoryへの依存はありません。

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

    UPROPERTY(EditDefaultsOnly)
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

## 再構築の動作

`TEntryBuilder<TValue>::Rebuild()` は安定したマージを実行します。

1. `SourceRepository` からすべてのプライマリKeyを走査する
2. Repositoryに存在しなくなったKeyのエントリを削除する
3. 新しいKeyにデフォルト構築されたエントリを追加する
4. Repositoryの行順にマッチするようエントリを安定ソートする

Keyが引き続き存在する既存エントリはそのまま残ります — 値は再構築後も保持されます。
