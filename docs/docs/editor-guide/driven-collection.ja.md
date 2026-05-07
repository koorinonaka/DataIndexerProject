# Driven Collection

`UDataIndexerDrivenCollection` は、`FDataIndexerPrimaryKey` をキーとするサブアセット — アイコン・アビリティクラス参照・その他の `UObject` ポインタなど — を管理するエディタアセット用の抽象 `UDataAsset` 基底クラスです。エントリはソースリポジトリと自動的に同期されます。

## コンセプト

Driven Collection は `UDataIndexerRepository` のコンパニオンアセットです。リポジトリと同じ `FDataIndexerPrimaryKey` セットをキーとする `TMap` を保持し、各値には行データの内部に置けない・置くべきでないアセットや構造体を格納します。エディタでソースリポジトリから行が追加・削除されると、コレクションは自動的にエントリを再構築します。

**主なユースケース：**

- 行データに含めるべきでない行ごとのアセット参照（例：アイコン・メッシュ・アビリティクラス）
- 行ごとのエディタ設定（例：カーブアセット、スポーンデータのオーバーライド）
- リポジトリの行構造体とは異なる値型が必要な派生ルックアップテーブル

## SourceRepository

```cpp
UPROPERTY(EditDefaultsOnly, Category = Settings)
TObjectPtr<UDataIndexerRepository> SourceRepository;
```

アセットの Details パネルで設定します。エディタで `SourceRepository` が保存・ロードされると、コレクションは `EntryBuilder` の `Rebuild()` を呼び出します。

!!! warning "エディタ専用の再構築"
    再構築メカニズム（`TEntryBuilder`・`PostEditChangeProperty`・アセットロードデリゲート）はすべて `WITH_EDITOR` です。コレクションのデータは保存時にアセットに焼き込まれ、ランタイムでは通常の UPROPERTY シリアライズでアクセスされます。ランタイムにリポジトリへの依存はありません。

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

1. `SourceRepository` からすべてのプライマリキーを走査する
2. リポジトリに存在しなくなったキーのエントリを削除する
3. 新しいキーにデフォルト構築されたエントリを追加する
4. リポジトリの行順にマッチするようエントリを安定ソートする

キーが引き続き存在する既存エントリはそのまま残ります — 値は再構築後も保持されます。

## Blueprint でのサブクラス化

`UDataIndexerDrivenCollection` の Blueprint サブクラス化はサポートされていません。エントリビルダーパターンには C++ のテンプレートインスタンス化が必要です。
