---
title: Quick Start
---

# Quick Start

A complete walkthrough — from creating a schema and repository to authoring rows and querying them at runtime.

!!! tip "Prerequisite"
    This guide assumes [Installation](installation.md) is complete. If you haven't set up the plugin yet, start there.

---

## 1. Create a Struct and Schema

=== ":material-puzzle: Editor & Blueprint"

    **Create a Blueprint Struct**

    Define the shape of a single row as a Blueprint struct in the Content Browser.

    ![Creating a Blueprint struct for row data](assets/images/bp-struct-creation.png)

    1. Right-click in the **Content Browser** → **Blueprints → Structure**
    2. Name it (e.g., `S_ItemRow`) and double-click to open
    3. Add a variable for each data field — `DisplayName` (Text), `Type` (Enum), `BaseValue` (Integer), etc.

    **Create a Schema Blueprint**

    A Schema Blueprint links your struct to a repository and controls editor behavior.

    1. Right-click → **Blueprint Class**, search for `DataIndexerSchema`, select it
    2. Name it (e.g., `BP_ItemSchema`) and open it
    3. In **Class Defaults → Row Struct**, select your struct (`S_ItemRow`)

    ![Selecting Row Struct in a Schema Blueprint](assets/images/schema-blueprint-creation.png)

    Optionally implement **Get Row Display Name** to show a readable label per row in the Data View:

    ![GetRowDisplayName implementation](assets/images/schema-get-row-display-name.png)

=== ":material-language-cpp: C++"

    **Define a Row Struct**

    Declare the struct that holds row data and a type alias for the native schema interface:

    ```cpp title="ItemTypes.h"
    #pragma once
    #include "DataIndexerSchemaInterface.h"
    #include "ItemTypes.generated.h"

    UENUM(BlueprintType)
    enum class EItemType : uint8
    {
        Weapon, Armor, Accessory, Material,
    };

    UENUM(BlueprintType)
    enum class EItemRarity : uint8
    {
        Common, Uncommon, Rare, Epic, Legendary,
    };

    USTRUCT(BlueprintType)
    struct FItemRow
    {
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FText DisplayName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        EItemType Type = EItemType::Weapon;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        EItemRarity Rarity = EItemRarity::Common;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int32 BaseValue = 0;
    };

    using FItemInterface = DataIndexer::TNativeSchemaInterface<FItemRow>;
    ```

    **Implement a C++ Schema**

    Declare indexes with `DI_DEFINE_INDEX` and register build functions in the constructor:

    ```cpp title="ItemSchema.h"
    #pragma once
    #include "DataIndexerKeyHelpers.h"
    #include "DataIndexerSchema.h"
    #include "ItemSchema.generated.h"

    UCLASS()
    class UItemSchema : public UDataIndexerSchema
    {
        GENERATED_BODY()

    public:
        UItemSchema();

        DI_DEFINE_INDEX(ByTypeIndex);
        DI_DEFINE_INDEX(ByRarityIndex);
        DI_DEFINE_INDEX(ByTypeAndRarityIndex);

    protected:
        virtual FText GetRowDisplayName_Implementation(
            const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity) const override;

        UFUNCTION()
        static FGuid BuildTypeIndex(const FInstancedStruct& RowEntity);

        UFUNCTION()
        static FGuid BuildRarityIndex(const FInstancedStruct& RowEntity);

        UFUNCTION()
        static FGuid BuildTypeAndRarityIndex(const FInstancedStruct& RowEntity);
    };
    ```

    ```cpp title="ItemSchema.cpp"
    #include "ItemSchema.h"
    #include "ItemTypes.h"

    UItemSchema::UItemSchema()
    {
        RowStruct = FItemRow::StaticStruct();

        RegisterFunction_BuildIndex(ByTypeIndex(),          GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeIndex));
        RegisterFunction_BuildIndex(ByRarityIndex(),        GET_FUNCTION_NAME_CHECKED(ThisClass, BuildRarityIndex));
        RegisterFunction_BuildIndex(ByTypeAndRarityIndex(), GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeAndRarityIndex));
    }

    FGuid UItemSchema::BuildTypeIndex(const FInstancedStruct& RowEntity)
    {
        if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
        {
            return FGuid(static_cast<uint32>(Row->Type), 0, 0, 0);
        }
        return {};
    }
    ```

    !!! tip "Blueprint schema"
        For quick prototyping you can subclass `UDataIndexerSchema` in Blueprint and register index functions from **Class Defaults → Build Index Functions**. Switch to a C++ schema when you need `DI_DEFINE_INDEX` for compile-time access in subsystems.

---

## 2. Create a Repository and Author Rows

**Create a Repository**

1. Right-click → **Miscellaneous → Data Asset**, select `DataIndexerRepository`
2. Name it (e.g., `DI_Items`) and open it
3. In the **Details** panel, set **Schema Class** to your schema (e.g., `BP_ItemSchema` or `ItemSchema`)

![Repository with schema bound in the Details panel](assets/images/repository-schema-binding.png)

!!! warning "Schema binding is permanent"
    Set **Schema Class** before adding any rows. Changing it later will invalidate all existing row data.

**Author Rows**

Double-click the repository asset to open the Data View.

![Adding a row with the Insert key](assets/images/data-view-add-row.png)

- Press **Insert** to add a row — a GUID primary key is generated automatically
- Edit values inline in the grid, or select a row to use the **Selection Details** panel on the right
- **Save** — reverse lookup tables rebuild automatically

For a full breakdown of the editor, see the [Editor Guide](editor-guide/index.md).

---

## 3. Query Examples

=== ":material-puzzle: Editor & Blueprint"

    **Reference a specific row**

    Declare a `FDataIndexerRowHandle` variable on an Actor or component. In the Details panel, set it to a specific row from the repository.

    In a Blueprint event graph, use the **Get Row** node:

    1. Drag from the Row Handle variable and search for **Get Row**
    2. In the node's Details panel, set **Schema Class** to your schema
    3. The `Row` output pin resolves to your struct type — wire it into your logic

    **Iterate all rows**

    Use **Get All Primary Keys** (DataIndexer function library) with a repository reference to get every primary key, then call **Get Row** for each key.

    For index-based filtering, see [Blueprint API → Function Library](api-reference/function-library.md).

=== ":material-language-cpp: C++"

    Wrap repository access in a `UEngineSubsystem` (or any class that can reach the singleton):

    ```cpp title="GameDataSubsystem.cpp"
    // Direct row lookup
    const FItemRow* UGameDataSubsystem::GetItem(const FDataIndexerPrimaryKey& Key) const
    {
        const UGameDataSettings* Settings = UGameDataSettings::Get();
        if (!Settings || !Settings->ItemRepository) return nullptr;

        return FItemInterface::FindRow(*Settings->ItemRepository, Key);
    }

    // Forward index lookup — all items of a given type
    TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByType(EItemType Type) const
    {
        const UGameDataSettings* Settings = UGameDataSettings::Get();
        if (!Settings || !Settings->ItemRepository) return {};

        FItemRow Query;
        Query.Type = Type;
        return FItemInterface::GetPrimaryKeys(*Settings->ItemRepository, UItemSchema::ByTypeIndex(), Query);
    }

    // Compound index — type + rarity simultaneously
    TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByTypeAndRarity(EItemType Type, EItemRarity Rarity) const
    {
        const UGameDataSettings* Settings = UGameDataSettings::Get();
        if (!Settings || !Settings->ItemRepository) return {};

        FItemRow Query;
        Query.Type   = Type;
        Query.Rarity = Rarity;
        return FItemInterface::GetPrimaryKeys(*Settings->ItemRepository, UItemSchema::ByTypeAndRarityIndex(), Query);
    }
    ```

---

## Next steps

<div class="grid cards" markdown>

- :material-graph-outline:{ .lg .middle } &nbsp; **[Core Concepts](concepts/index.md)**

    ---

    Understand the relationship between Repository, Schema, Keys, and Indexes.

- :material-puzzle-outline:{ .lg .middle } &nbsp; **[Blueprint API](api-reference/index.md)**

    ---

    Full reference for Blueprint nodes, the function library, and Driven Collection.

- :material-language-cpp:{ .lg .middle } &nbsp; **[C++ API](api-reference/index.md)**

    ---

    Type-safe query patterns and the Native Schema Interface.

- :material-table-edit:{ .lg .middle } &nbsp; **[Editor Guide](editor-guide/index.md)**

    ---

    Data View, JSON import/export, and custom widgets.

</div>
