# Schema

`UDataIndexerSchema` is the contract between a repository and its editor behavior. It is an abstract `UObject` subclass — you never instantiate it directly, but always subclass it (in Blueprint or C++) to define a specific data type.

## Role

A schema does three things:

1. **Declares the row struct** — `RowStruct` is a `TObjectPtr<const UScriptStruct>` that identifies the struct type stored in the repository's `LocalEntries`.
2. **Provides display logic** — `GetRowDisplayName` is a `BlueprintNativeEvent` that returns a human-readable label for any row, used throughout the editor UI.
3. **Registers extension functions** — index key builders, property text customizations, and property widget customizations are registered as named Blueprint or C++ functions.

## Blueprint subclassing

1. Create a **Blueprint Class** with parent `DataIndexerSchema`
2. In **Class Defaults**, assign **Row Struct** to your `USTRUCT`
3. Override `GetRowDisplayName` to return a meaningful `FText` from the row struct fields
4. Use the **Build Index Functions** map to register index builders (see [Indexes](indexes.md))
5. Use the **Property Text Customizations** map to register custom text rendering per property

## C++ subclassing

```cpp
UCLASS()
class UMyItemSchema : public UDataIndexerSchema
{
    GENERATED_BODY()

public:
    DI_DEFINE_INDEX(ByCategoryIndex);

protected:
    virtual void PostInitProperties() override;

    UFUNCTION()
    static FGuid BuildCategoryIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName);

public:
    virtual FText GetRowDisplayName_Implementation(
        const FDataIndexerPrimaryKey& PrimaryKey,
        const FInstancedStruct& RowEntity) const override;
};
```

```cpp
void UMyItemSchema::PostInitProperties()
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        RowStruct = FMyItemRow::StaticStruct();
        RegisterFunction_BuildIndex(
            ByCategoryIndex(),
            GET_FUNCTION_NAME_CHECKED(ThisClass, BuildCategoryIndex));
    }
    Super::PostInitProperties();
}
```

## Data validation

Override `IsRowValid` (editor-only) to perform per-row validation during **Validate Data**:

```cpp
#if WITH_EDITOR
EDataValidationResult UMyItemSchema::IsRowValid(
    FConstStructView RowEntity, FDataValidationContext& Context) const
{
    if (const FMyItemRow* Row = RowEntity.GetPtr<const FMyItemRow>())
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
void UMyItemSchema::InitializeExpandedStructEntries()
{
    Super::InitializeExpandedStructEntries();

    // Collapse SomeField from the top-level struct (keep it inline)
    if (FDataIndexerExpandedStructEntry* Entry = ExpandedStructEntries.Find(RowStruct))
    {
        *Entry -= GET_MEMBER_NAME_CHECKED(FMyItemRow, SomeField);
    }

    // Expand a nested struct's properties into columns
    ExpandedStructEntries.FindOrAdd(FMyInnerStruct::StaticStruct()) += {
        GET_MEMBER_NAME_CHECKED(FMyInnerStruct, A),
        GET_MEMBER_NAME_CHECKED(FMyInnerStruct, B),
    };
}
```

In Blueprint, toggle the **Expanded Struct Entries** map from the Class Defaults panel.
