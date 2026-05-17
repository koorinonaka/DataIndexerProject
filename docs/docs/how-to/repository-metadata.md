---
title: Repository Metadata
---

# Repository Metadata

DataIndexer exposes several metadata keys that control how repository properties behave in the editor. This page covers the two most commonly used: **schema filtering** on UPROPERTY pickers, and **NotOverridable** field locking in parent-child hierarchies.

## Schema Filtering { #schema-filtering }

When a class exposes a `UDataIndexerRepository` UPROPERTY, by default the asset picker shows every repository in the project regardless of schema. Add `meta = (Schema = "...")` to restrict it to only repositories that use a specific schema.

This prevents accidental assignment of a quest repository to an item slot, or a character repository to a shop.

### C++

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataIndexer",
    meta = (Schema = "/Game/DataIndexer/BP_ItemSchema.BP_ItemSchema"))
TObjectPtr<UDataIndexerRepository> ItemRepository;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataIndexer",
    meta = (Schema = "/Game/DataIndexer/BP_QuestSchema.BP_QuestSchema"))
TObjectPtr<UDataIndexerRepository> QuestRepository;
```

The meta value is the asset path of the **Schema Blueprint** (not the repository). Use the same format returned by right-clicking the asset and selecting **Copy Reference**.

!!! note "Relative paths"
    The path must be an absolute content path starting with `/Game/`. Plugin content uses `/PluginName/`.

### Blueprint

1. Open the Blueprint that owns the variable
2. Select the `UDataIndexerRepository` variable in the **My Blueprint** panel
3. In the **Details** panel, find the **Schema** picker under **Variable**
4. Select your schema asset — the picker is now filtered to matching repositories

Clear the picker to restore the unfiltered view.

### GameMode example

A typical setup with multiple data domains on a single GameMode:

```cpp
UCLASS()
class AMyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Data",
        meta = (Schema = "/Game/DataIndexer/BP_ItemSchema.BP_ItemSchema"))
    TObjectPtr<UDataIndexerRepository> ItemRepository;

    UPROPERTY(EditDefaultsOnly, Category = "Data",
        meta = (Schema = "/Game/DataIndexer/BP_CharacterSchema.BP_CharacterSchema"))
    TObjectPtr<UDataIndexerRepository> CharacterRepository;

    UPROPERTY(EditDefaultsOnly, Category = "Data",
        meta = (Schema = "/Game/DataIndexer/BP_QuestSchema.BP_QuestSchema"))
    TObjectPtr<UDataIndexerRepository> QuestRepository;
};
```

Each UPROPERTY's picker only shows repositories of the matching schema. Designers cannot assign the wrong type.

---

## NotOverridable { #notoverridable }

When a child repository overrides a parent row, *all* fields are editable by default in the child's Data View. Add the `NotOverridable` metadata to a struct field to lock it — the field becomes read-only whenever a child edits a parent row.

This is useful for base stats that the content team should never change per-region or per-shop: base cost, attack power, rarity tier.

### Blueprint (User Defined Struct)

1. Open the **User Defined Struct** editor for your row struct
2. Select the variable you want to lock
3. In the **Details** panel, enable **Not Overridable**

The field is now read-only in any child repository that overrides this row.

### C++ (native structs)

For native structs, use `FStructureEditorUtils::SetMetaData` at editor-init time. The canonical place is a `UDataIndexerSchema` subclass or a dedicated editor subsystem:

```cpp
#if WITH_EDITOR
void UMyItemSchema::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

// In your EditorModule StartupModule, or a schema subclass:
void LockBaseStatFields(UUserDefinedStruct* ItemStruct)
{
    for (FGuid VariableGuid : FStructureEditorUtils::GetVarGuids(ItemStruct))
    {
        FName VarName = FStructureEditorUtils::GetVariableFriendlyNameForProperty(
            ItemStruct, FStructureEditorUtils::GetVarDescription(ItemStruct, VariableGuid));

        if (VarName == GET_MEMBER_NAME_CHECKED(FItemRow, BaseValue) ||
            VarName == GET_MEMBER_NAME_CHECKED(FItemRow, Rarity))
        {
            FStructureEditorUtils::SetMetaData(
                ItemStruct, VariableGuid,
                DataIndexer::MetaDataKeys::NotOverridable, TEXT("true"));
        }
    }
}
#endif
```

### Checking the flag in a property customization

If you write a custom `IPropertyTypeCustomization`, you can read the flag to enforce read-only rendering:

```cpp
void FMyRowCustomization::CustomizeChildren(
    TSharedRef<IPropertyHandle> StructHandle,
    IDetailChildrenBuilder& Builder,
    IPropertyTypeCustomizationUtils& Utils)
{
    uint32 NumChildren = 0;
    StructHandle->GetNumChildren(NumChildren);

    for (uint32 i = 0; i < NumChildren; ++i)
    {
        TSharedPtr<IPropertyHandle> Child = StructHandle->GetChildHandle(i);
        if (Child->GetProperty()->HasMetaData(DataIndexer::MetaDataKeys::NotOverridable))
        {
            Child->MarkHiddenByCustomization(); // or set SNew(STextBlock) as read-only
        }
        else
        {
            Builder.AddProperty(Child.ToSharedRef());
        }
    }
}
```

### Example: item repository with locked base stats

Scenario: a global item repository defines `BaseValue` and `Rarity`. Regional shop repositories may override items (e.g. to change a discount flag) but must never alter the base stats.

1. Add `NotOverridable` to `BaseValue` and `Rarity` in the `FItemRow` UDStruct
2. Create `DI_AllItems` as the global repository
3. Create `DI_ShopA` with `DI_AllItems` as a parent (see [Parent Repository Hierarchy](parent-hierarchy.md))
4. In `DI_ShopA`, open any parent-sourced row — `BaseValue` and `Rarity` are greyed out

See [Parent Repository Hierarchy](parent-hierarchy.md) for the full hierarchy setup.
