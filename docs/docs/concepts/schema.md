# Schema

`UDataIndexerSchema` is the contract between a repository and its editor behavior. It is an abstract `UObject` subclass — you never instantiate it directly, but always subclass it (in Blueprint or C++) to define a specific data type.

## Role

A schema does three things:

1. **Declares the row struct** — `RowStruct` is a `TObjectPtr<const UScriptStruct>` that identifies the struct type stored in the repository's `LocalEntries`.
2. **Provides display logic** — `GetRowDisplayName` resolves the bound **Row Display Name Function** (or a C++ override) to return a human-readable label for any row, used throughout the editor UI.
3. **Registers extension functions** — functions that generate index keys and customize property cell widgets are bound as named Blueprint or C++ functions that take the concrete row struct directly.

## Subclassing

=== "C++"

    ### Declaration and initialization

    Set `RowStruct` in the constructor.

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
        virtual TOptional<FText> GetRowDisplayName(
            const FDataIndexerPrimaryKey& PrimaryKey,
            const FConstStructView& RowEntity) const override;

        UFUNCTION()
        static FGuid BuildTypeIndex(const FItemRow& Row);

        UFUNCTION()
        static FGuid BuildRarityIndex(const FItemRow& Row);
    };
    ```

    ```cpp
    UItemSchema::UItemSchema()
    {
        RowStruct = FItemRow::StaticStruct();

        DI_REGISTER_BUILD_INDEX( ByTypeIndex(),   FItemRow, BuildTypeIndex );
        DI_REGISTER_BUILD_INDEX( ByRarityIndex(), FItemRow, BuildRarityIndex );
    }
    ```

    Call `DI_REGISTER_BUILD_INDEX` once per `DI_DEFINE_INDEX` declaration to bind each builder function. The macro checks the builder's signature (`FGuid( const RowType& )`) at compile time.

    ### GetRowDisplayName

    Override the `GetRowDisplayName` `virtual` to return the display name for a row. `RowEntity` is a `FConstStructView`; unpack it to the concrete row struct and return the field in one line.

    ```cpp
    TOptional<FText> UItemSchema::GetRowDisplayName(
        const FDataIndexerPrimaryKey& PrimaryKey,
        const FConstStructView& RowEntity) const
    {
        return RowEntity.Get<const FItemRow>().DisplayName;
    }
    ```

    ### Build Index Functions

    Declare indexes with `DI_DEFINE_INDEX` and implement the corresponding `static UFUNCTION` as the builder. The builder receives the **concrete row struct** directly (see [Indexes](indexes.md)).

    ```cpp
    FGuid UItemSchema::BuildTypeIndex(const FItemRow& Row)
    {
        return FGuid( static_cast<uint32>( Row.Type ), 0, 0, 0 );
    }
    ```

=== "Blueprint"

    ### Row Struct

    1. Create a **Blueprint Class** with parent `DataIndexerSchema`
    2. In **Class Defaults**, assign **Row Struct** to your `USTRUCT`

    ### GetRowDisplayName

    In **Class Defaults**, bind **Row Display Name Function** to a function returning a meaningful `FText` from the row struct fields. The function receives the **concrete row struct** directly — no `Get Instanced Struct Value` node needed. This label is used throughout the editor UI in row lists and pickers.

    ### Build Index Functions

    Use the **Build Index Functions** map in **Class Defaults** to register index builders. The key is the index, the value is a function taking the **concrete row struct** and returning `FGuid`. The picker filters to matching functions and "Create matching function" generates a stub with the concrete row parameter (see [Indexes](indexes.md)).

    ### Property Cell Widget Customizations

    Use the **Property Widget Customizations** map in **Class Defaults** to register per-property widget rendering. The key is the property name, the value is a function returning `UUserWidget*`.

    Return `nullptr` from the customization function to fall back to the default cell display.

## Data validation

!!! warning "Blueprint not supported"
    Blueprint override of `IsRowValid` is not currently supported. Validation must be implemented in C++.

Override `IsRowValid` (editor-only) to add per-row validation logic.

Validation runs automatically at the following points:

- When selecting **Validate Data** via right-click in the Content Browser
- On asset save (if **Save Validation** is enabled in editor settings)
- During cook

A return value of `EDataValidationResult::Invalid` surfaces the errors added to `Context` in the editor and blocks save and cook.

```cpp
#if WITH_EDITOR
EDataValidationResult UItemSchema::IsRowValid(
    FConstStructView RowEntity, FDataValidationContext& Context) const
{
    if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
    {
        if (Row->MaxStack <= 0)
        {
            Context.AddError(NSLOCTEXT("MyItem", "BadStack", "MaxStack must be > 0"));
            return EDataValidationResult::Invalid;
        }
    }
    return EDataValidationResult::Valid;
}
#endif
```

## Column layout (ExpandedStructEntries)

`ExpandedStructEntries` controls which nested struct properties appear as individual columns in the Data View grid. By default all top-level properties of `RowStruct` are shown as columns.

Override `InitializeExpandedStructEntries` in C++ to configure this programmatically:

```cpp
void UItemSchema::InitializeExpandedStructEntries()
{
    Super::InitializeExpandedStructEntries();

    // Collapse SomeField from the top-level struct (keep it inline)
    if (FDataIndexerExpandedStructEntry* Entry = ExpandedStructEntries.Find(RowStruct))
    {
        *Entry -= GET_MEMBER_NAME_CHECKED(FItemRow, SomeField);
    }

    // Expand a nested struct's properties into columns
    ExpandedStructEntries.FindOrAdd(FMyInnerStruct::StaticStruct()) += {
        GET_MEMBER_NAME_CHECKED(FMyInnerStruct, A),
        GET_MEMBER_NAME_CHECKED(FMyInnerStruct, B),
    };
}
```

In Blueprint, toggle the **Expanded Struct Entries** map from the Class Defaults panel.

![Column Layout panel showing per-property column visibility checkboxes](../assets/images/schema-column-layout.png)
