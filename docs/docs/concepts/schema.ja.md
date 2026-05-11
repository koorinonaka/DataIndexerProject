# Schema

`UDataIndexerSchema` は Repository とエディタ動作の間のコントラクトです。抽象 `UObject` サブクラスであり、直接インスタンス化せず、特定のデータ型を定義するために Blueprint または C++ でサブクラス化します。

## 役割

Schema は 3 つのことを担当します。

1. **行構造体の宣言** — `RowStruct` は Repository の `LocalEntries` に格納される構造体型を識別する `TObjectPtr<const UScriptStruct>` です。
2. **表示ロジックの提供** — `GetRowDisplayName` は任意の行に対して人間可読なラベルを返す `BlueprintNativeEvent` で、エディタ UI 全体で使用されます。
3. **拡張関数の登録** — インデックスキービルダー・プロパティテキストカスタマイズ・プロパティウィジェットカスタマイズを名前付き Blueprint または C++ 関数として登録します。

## Blueprint サブクラス化

1. 親クラスに `DataIndexerSchema` を指定して **Blueprint Class** を作成する
2. **Class Defaults** で **Row Struct** を使用する `USTRUCT` に設定する
3. `GetRowDisplayName` をオーバーライドして行構造体フィールドから意味のある `FText` を返す
4. **Build Index Functions** マップでインデックスビルダーを登録する（[インデックス](indexes.md) 参照）
5. **Property Text Customizations** マップでプロパティごとのカスタムテキストレンダリングを登録する

## C++ サブクラス化

```cpp
UCLASS()
class UItemSchema : public UDataIndexerSchema
{
    GENERATED_BODY()

public:
    static const FDataIndexerIndex& ByTypeIndex()
    {
        static const FDataIndexerIndex Index(FGuid(1, 0, 0, 0), INVTEXT("ByType"));
        return Index;
    }

protected:
    virtual void PostInitProperties() override;

    UFUNCTION()
    static FGuid BuildTypeIndex(const FInstancedStruct& RowEntity);

public:
    virtual FText GetRowDisplayName_Implementation(
        const FDataIndexerPrimaryKey& PrimaryKey,
        const FInstancedStruct& RowEntity) const override;
};
```

```cpp
void UItemSchema::PostInitProperties()
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        RowStruct = FItemRow::StaticStruct();
        RegisterFunction_BuildIndex(ByTypeIndex(),
            GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeIndex));
    }
    Super::PostInitProperties();
}
```

## データバリデーション

`IsRowValid` をオーバーライド（エディタ専用）して、**Validate Data** 実行時の各行のバリデーションを追加します。

```cpp
#if WITH_EDITOR
EDataValidationResult UItemSchema::IsRowValid(
    FConstStructView RowEntity, FDataValidationContext& Context) const
{
    if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
    {
        if (Row->BaseValue < 0)
        {
            Context.AddError(NSLOCTEXT("Item", "BadValue", "BaseValue must be >= 0"));
            return EDataValidationResult::Invalid;
        }
    }
    return EDataValidationResult::Valid;
}
#endif
```

## カラムレイアウト（ExpandedStructEntries）

`ExpandedStructEntries` は Data View グリッドでネスト構造体のどのプロパティを個別カラムとして表示するかを制御します。デフォルトでは `RowStruct` のすべてのトップレベルプロパティがカラムとして表示されます。

C++ で `InitializeExpandedStructEntries` をオーバーライドしてプログラムから設定できます。

```cpp
void UItemSchema::InitializeExpandedStructEntries()
{
    Super::InitializeExpandedStructEntries();

    // SomeField をトップレベルから除外（インライン表示に戻す）
    if (FDataIndexerExpandedStructEntry* Entry = ExpandedStructEntries.Find(RowStruct))
    {
        *Entry -= GET_MEMBER_NAME_CHECKED(FItemRow, SomeField);
    }

    // ネスト構造体のプロパティをカラムに展開
    ExpandedStructEntries.FindOrAdd(FMyInnerStruct::StaticStruct()) += {
        GET_MEMBER_NAME_CHECKED(FMyInnerStruct, A),
        GET_MEMBER_NAME_CHECKED(FMyInnerStruct, B),
    };
}
```

Blueprint では Class Defaults パネルの **Expanded Struct Entries** マップから切り替えられます。
