# Schema

`UDataIndexerSchema` は Repository とエディタ動作の間のコントラクトです。抽象 `UObject` サブクラスであり、直接インスタンス化せず、特定のデータ型を定義するために Blueprint または C++ でサブクラス化します。

## 役割

Schema は 3 つのことを担当します。

1. **行構造体の宣言** — `RowStruct` は Repository の `LocalEntries` に格納される構造体型を識別する `TObjectPtr<const UScriptStruct>` です。
2. **表示ロジックの提供** — `GetRowDisplayName` は任意の行に対して人間可読なラベルを返す `BlueprintNativeEvent` で、エディタ UI 全体で使用されます。
3. **拡張関数の登録** — 行の検索キー生成・プロパティの表示テキスト・エディタウィジェットをカスタマイズする関数を、Blueprint または C++ の名前付き関数として登録します。

## サブクラス化

=== "C++"

    ### 宣言と初期化

    `RowStruct` はコンストラクタで設定します。

    ```cpp
    UCLASS()
    class UItemSchema : public UDataIndexerSchema
    {
        GENERATED_BODY()

    public:
        UItemSchema();

        DI_DEFINE_INDEX(ByTypeIndex);
        DI_DEFINE_INDEX(ByRarityIndex);

    protected:
        virtual FText GetRowDisplayName_Implementation(
            const FDataIndexerPrimaryKey& PrimaryKey,
            const FInstancedStruct& RowEntity) const override;

        UFUNCTION()
        static FGuid BuildTypeIndex(const FInstancedStruct& RowEntity);

        UFUNCTION()
        static FGuid BuildRarityIndex(const FInstancedStruct& RowEntity);
    };
    ```

    ```cpp
    UItemSchema::UItemSchema()
    {
        RowStruct = FItemRow::StaticStruct();

        RegisterFunction_BuildIndex( ByTypeIndex(),    GET_FUNCTION_NAME_CHECKED( ThisClass, BuildTypeIndex ) );
        RegisterFunction_BuildIndex( ByRarityIndex(),  GET_FUNCTION_NAME_CHECKED( ThisClass, BuildRarityIndex ) );
    }
    ```

    `DI_DEFINE_INDEX` で宣言したIndexごとに `RegisterFunction_BuildIndex` を呼び出し、ビルダー関数を紐付けます。

    ### GetRowDisplayName_Implementation

    `GetRowDisplayName_Implementation` をオーバーライドして行の表示名を返します。見つからない場合は `Super` に委譲します。

    ```cpp
    FText UItemSchema::GetRowDisplayName_Implementation(
        const FDataIndexerPrimaryKey& PrimaryKey,
        const FInstancedStruct& RowEntity) const
    {
        if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
        {
            return Row->DisplayName;
        }
        return Super::GetRowDisplayName_Implementation( PrimaryKey, RowEntity );
    }
    ```

    ### Build Index Functions

    `DI_DEFINE_INDEX` でIndexを宣言し、対応する `static UFUNCTION` をビルダーとして実装します（[Index](indexes.md) 参照）。

    ```cpp
    FGuid UItemSchema::BuildTypeIndex(const FInstancedStruct& RowEntity)
    {
        if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
        {
            return FGuid( static_cast<uint32>( Row->Type ), 0, 0, 0 );
        }
        return {};
    }
    ```

    ### Property Text Customizations

    `PropertyTextCustomizations` マップにプロパティ名と `FText` を返す関数ポインタを登録します。Blueprint の **Property Text Customizations** マップに相当します。

    プロパティごとに `static UFUNCTION` を宣言し、コンストラクタで `RegisterFunction_PropertyTextCustomization` を呼び出します。

    ```cpp
    // クラス宣言内
    UFUNCTION()
    static FText GetTypeDisplayText(const FInstancedStruct& RowEntity);
    ```

    ```cpp
    UItemSchema::UItemSchema()
    {
        RowStruct = FItemRow::StaticStruct();

        RegisterFunction_PropertyTextCustomization(
            GET_MEMBER_NAME_CHECKED(FItemRow, Type),
            GET_FUNCTION_NAME_CHECKED(ThisClass, GetTypeDisplayText));
    }
    ```

    ```cpp
    FText UItemSchema::GetTypeDisplayText(const FInstancedStruct& RowEntity)
    {
        if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
        {
            switch (Row->Type)
            {
                case EItemType::Weapon: return NSLOCTEXT("Item", "TypeWeapon", "武器");
                case EItemType::Armor:  return NSLOCTEXT("Item", "TypeArmor",  "防具");
                default: break;
            }
        }
        return FText::GetEmpty();
    }
    ```

=== "Blueprint"

    ### Row Struct の設定

    1. 親クラスに `DataIndexerSchema` を指定して **Blueprint Class** を作成する
    2. **Class Defaults** で **Row Struct** を使用する `USTRUCT` に設定する

    ### GetRowDisplayName

    **Class Defaults** で `GetRowDisplayName` イベントをオーバーライドし、行構造体のフィールドから意味のある `FText` を返します。このラベルはエディタ UI 全体の行一覧・ピッカーで使用されます。

    ### Build Index Functions

    **Class Defaults** の **Build Index Functions** マップでIndex ビルダーを登録します。キーはIndex名（文字列）、値は `FGuid` を返す関数です（[Index](indexes.md) 参照）。

    ### Property Text Customizations

    **Class Defaults** の **Property Text Customizations** マップで、プロパティ名をキーに `FText` を返す関数を登録します。Data View グリッドでのプロパティ値の表示テキストを上書きします。

    上書きしたいプロパティごとにエントリを追加します。例えば `Type` を生の列挙値整数ではなくローカライズ済みラベルで表示する場合：

    | キー（プロパティ名） | 値（関数） |
    |---|---|
    | `Type` | `GetTypeDisplayText` |

    関数は `FInstancedStruct`（行）を受け取り `FText` を返します。デフォルト表示にフォールバックするには `FText::GetEmpty()` を返します。

## データバリデーション

!!! warning "Blueprint 未対応"
    `IsRowValid` の Blueprint オーバーライドは現在対応していません。バリデーションは C++ でのみ実装できます。

`IsRowValid` をオーバーライド（エディタ専用）して、各行のバリデーションロジックを追加します。

検証は次のタイミングで自動的に実行されます。

- Content Browser で **右クリック → Validate Data** を選択したとき
- アセット保存時（エディタ設定で **Save Validation** を有効にした場合）
- クック時

戻り値が `EDataValidationResult::Invalid` の場合、`Context` に追加したエラーメッセージがエディタに表示され、保存・クックがブロックされます。

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

![カラムごとの表示・非表示を切り替える Column Layout パネル](../assets/images/schema-column-layout.png)
