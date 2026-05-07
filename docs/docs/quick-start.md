# Quick Start

A complete walkthrough — from creating a schema and repository to authoring rows and querying them at runtime.

## Walkthrough

=== "Editor & Blueprint"

    **1. Create a Blueprint Struct**

    Define the shape of a single row as a Blueprint struct in the Content Browser.

    ![Creating a Blueprint struct for row data](assets/images/bp-struct-creation.png)

    1. Right-click in the **Content Browser** → **Blueprints → Structure**
    2. Name it (e.g., `S_ItemRow`) and double-click to open
    3. Add a variable for each data field — `DisplayName` (Text), `Type` (Enum), `BaseValue` (Integer), etc.

    **2. Create a Schema Blueprint**

    A Schema Blueprint links your struct to a repository and controls editor behavior.

    1. Right-click → **Blueprint Class**, search for `DataIndexerSchema`, select it
    2. Name it (e.g., `BP_ItemSchema`) and open it
    3. In **Class Defaults → Row Struct**, select your struct (`S_ItemRow`)

    ![Selecting Row Struct in a Schema Blueprint](assets/images/schema-blueprint-creation.png)

    Optionally implement **Get Row Display Name** to show a readable label per row in the Data View:

    ![GetRowDisplayName implementation](assets/images/schema-get-row-display-name.png)

    **3. Create a Repository**

    1. Right-click → **Miscellaneous → Data Asset**, select `DataIndexerRepository`
    2. Name it (e.g., `DI_Items`) and open it
    3. In the **Details** panel, set **Schema Class** to your schema (e.g., `BP_ItemSchema`)

    ![Repository with schema bound in the Details panel](assets/images/repository-schema-binding.png)

    !!! warning "Schema binding is permanent"
        Set **Schema Class** before adding any rows. Changing it later will invalidate all existing row data.

    **4. Author Rows**

    Double-click the repository asset to open the Data View.

    ![Adding a row with the Insert key](assets/images/data-view-add-row.png)

    - Press **Insert** to add a row — a GUID primary key is generated automatically
    - Edit values inline in the grid, or select a row to use the **Selection Details** panel on the right
    - **Save** — reverse lookup tables rebuild automatically

    For a full breakdown of the editor, see the [Editor Guide](editor-guide/index.md).

    **5. Query from Blueprint**

    **Reference a specific row**

    Declare a `FDataIndexerRowHandle` variable on an Actor or component. In the Details panel, set it to a specific row from the repository.

    In a Blueprint event graph, use the **Get Row** node:

    1. Drag from the Row Handle variable and search for **Get Row**
    2. In the node's Details panel, set **Schema Class** to your schema
    3. The `Row` output pin resolves to your struct type — wire it into your logic

    **Iterate all rows**

    Use **Get All Primary Keys** (DataIndexer function library) with a repository reference to get every primary key, then call **Get Row** for each key.

    For index-based filtering, see [Blueprint API → Function Library](blueprint-api/function-library.md).

=== "C++"

    **1. Define Row Structs**

    Declare the structs that hold row data, plus a type alias for the native schema interface:

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

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FText DisplayName;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        EItemType Type = EItemType::Weapon;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        EItemRarity Rarity = EItemRarity::Common;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        int32 BaseValue = 0;
    };

    using FItemInterface = DataIndexer::TNativeSchemaInterface<FItemRow>;
    ```

    A second struct can reference a row in another repository via `FDataIndexerRowHandle`:

    ```cpp title="CharacterTypes.h"
    USTRUCT(BlueprintType)
    struct FCharacterRow
    {
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        FText DisplayName;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        ECharacterClass Class = ECharacterClass::Warrior;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        int32 MaxHP = 100;

        // Cross-repository reference to ItemRepository
        UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Schema = "/Script/YourGame.ItemSchema"))
        FDataIndexerRowHandle DefaultWeapon;
    };

    using FCharacterInterface = DataIndexer::TNativeSchemaInterface<FCharacterRow>;
    ```

    **2. Implement a C++ Schema**

    A schema binds the row struct to a repository and registers index builders. Use `DI_DEFINE_INDEX` to declare each index (generates a deterministic GUID from the class path):

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
        DI_DEFINE_INDEX(ByTypeIndex);
        DI_DEFINE_INDEX(ByRarityIndex);
        DI_DEFINE_INDEX(ByTypeAndRarityIndex);

    protected:
        virtual void PostInitProperties() override;

        virtual FText GetRowDisplayName_Implementation(
            const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity) const override;

        UFUNCTION()
        static FGuid BuildTypeIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName);

        UFUNCTION()
        static FGuid BuildRarityIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName);

        UFUNCTION()
        static FGuid BuildTypeAndRarityIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName);
    };
    ```

    Register the builders in `PostInitProperties`:

    ```cpp title="ItemSchema.cpp"
    #include "ItemSchema.h"
    #include "ItemTypes.h"

    void UItemSchema::PostInitProperties()
    {
        if (HasAnyFlags(RF_ClassDefaultObject))
        {
            RowStruct = FItemRow::StaticStruct();

            RegisterFunction_BuildIndex(ByTypeIndex(),          GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeIndex));
            RegisterFunction_BuildIndex(ByRarityIndex(),        GET_FUNCTION_NAME_CHECKED(ThisClass, BuildRarityIndex));
            RegisterFunction_BuildIndex(ByTypeAndRarityIndex(), GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeAndRarityIndex));
        }
        Super::PostInitProperties();
    }

    FGuid UItemSchema::BuildTypeIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName)
    {
        if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
        {
            OutDisplayName = UEnum::GetDisplayValueAsText(Row->Type);
            return FGuid(static_cast<uint32>(Row->Type), 0, 0, 0);
        }
        return {};
    }
    ```

    !!! tip "Blueprint schema"
        For quick prototyping you can subclass `UDataIndexerSchema` in Blueprint and register index functions from **Class Defaults → Build Index Functions**. Switch to a C++ schema when you need `DI_DEFINE_INDEX` for compile-time access in subsystems.

    **3. Set Up the Game Singleton**

    The recommended pattern exposes repository references through a `UObject` singleton configured in `DefaultEngine.ini`:

    ```cpp title="GameDataSettings.h"
    UCLASS()
    class UGameDataSettings : public UObject
    {
        GENERATED_BODY()
    public:
        static const UGameDataSettings* Get();

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameData,
            meta = (Schema = "/Script/YourGame.ItemSchema"))
        TObjectPtr<UDataIndexerRepository> ItemRepository;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameData,
            meta = (Schema = "/Script/YourGame.CharacterSchema"))
        TObjectPtr<UDataIndexerRepository> CharacterRepository;
    };
    ```

    ```ini title="Config/DefaultEngine.ini"
    [/Script/Engine.Engine]
    GameSingletonClassName=/Script/YourGame.GameDataSettings
    ```

    Open the `GameDataSettings` class defaults and assign the two repository assets you'll create in the next step.

    **4. Create Repository Assets**

    1. Right-click in the Content Browser → **Miscellaneous → Data Asset**
    2. Select `DataIndexerRepository` as the asset class
    3. Name the asset (e.g., `DI_Items`) and open it
    4. In the Details panel, set **Schema Class** to your schema (e.g., `ItemSchema`)

    For the full editor workflow, see the [Editor Guide](editor-guide/index.md).

    **5. Author Rows**

    Double-click a repository asset to open the custom editor.

    - Press **Insert** — a `FDataIndexerPrimaryKey` (GUID) is generated automatically
    - Edit properties directly in the Data View grid
    - The **Schema** controls display names and which columns appear

    **6. Query from C++**

    Wrap repository access in a `UEngineSubsystem` (or any class that can reach the singleton):

    ```cpp
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
        FItemRow Query;
        Query.Type   = Type;
        Query.Rarity = Rarity;
        return FItemInterface::GetPrimaryKeys(*Settings->ItemRepository, UItemSchema::ByTypeAndRarityIndex(), Query);
    }

    // Reverse index lookup — characters whose DefaultWeapon points to a specific item
    TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetCharactersUsingWeapon(const FDataIndexerPrimaryKey& WeaponKey) const
    {
        FCharacterRow Query;
        Query.DefaultWeapon.PrimaryKey = WeaponKey;
        return FCharacterInterface::GetPrimaryKeys(*Settings->CharacterRepository,
            UCharacterSchema::ByDefaultWeaponIndex(), Query);
    }

    // A→B relation — resolve a character's DefaultWeapon to the actual FItemRow
    TConstStructView<FItemRow> UGameDataSubsystem::GetDefaultWeapon(const FDataIndexerPrimaryKey& CharacterKey) const
    {
        TConstStructView<FCharacterRow> Character =
            FCharacterInterface::FindRow(*Settings->CharacterRepository, CharacterKey);
        if (!Character.IsValid()) return {};

        return FItemInterface::FindRow(*Settings->ItemRepository, Character.Get().DefaultWeapon.PrimaryKey);
    }
    ```

---

## What's Next

- [Core Concepts](concepts/index.md) — understand how Repository, Schema, Keys, and Indexes fit together
- [Editor Guide](editor-guide/index.md) — full editor workflow reference
- [Blueprint API](blueprint-api/index.md) — full reference for Blueprint nodes and the function library
- [C++ API](cpp-api/index.md) — typed C++ access patterns
