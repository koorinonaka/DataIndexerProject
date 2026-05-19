# Native Schema Interface

`DataIndexer::TNativeSchemaInterface<TRowType>` (`DataIndexerSchemaInterface.h`) is a typed C++ façade over `UDataIndexerRepository`. It provides compile-time type safety for row queries without requiring any virtual dispatch or casting in calling code.

## Setup

Define a project-level type alias alongside the row struct. In the sample project, the alias is declared at the bottom of the same header:

```cpp title="ItemTypes.h"
#pragma once
#include "DataIndexerSchemaInterface.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8 { Weapon, Armor, Accessory, Material };

UENUM(BlueprintType)
enum class EItemRarity : uint8 { Common, Uncommon, Rare, Epic, Legendary };

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

Include the header wherever you query the item repository — no separate setup file needed.

---

## GetDisplayName

```cpp
static FText GetDisplayName(
    const UDataIndexerRepository& Repository,
    const FDataIndexerPrimaryKey& PrimaryKey);
```

Calls the schema's `GetRowDisplayName` with the row entity fetched from the repository. Returns `FText::FromName(NAME_None)` if the schema is null.

```cpp
FText Name = FItemInterface::GetDisplayName(Repository, PrimaryKey);
```

---

## FindRow (by PrimaryKey)

```cpp
static const TRowType* FindRow(
    const UDataIndexerRepository& Repository,
    const FDataIndexerPrimaryKey& PrimaryKey);
```

Delegates to `Repository.FindRowEntity(PrimaryKey)`, then returns a typed pointer. Returns `nullptr` if the key is not found or the struct type does not match.

```cpp
if (const FItemRow* Row = FItemInterface::FindRow(Repository, Key))
{
    UE_LOG(LogTemp, Log, TEXT("Item: %s  BaseValue: %d"),
        *Row->DisplayName.ToString(), Row->BaseValue);
}
```

---

## FindRow (by RowHandle)

```cpp
static const TRowType* FindRow(
    const FDataIndexerRowHandle& RowHandle);
```

Convenience overload that validates the handle and delegates to `FindRow(*RowHandle.Repository, RowHandle.PrimaryKey)`. Returns `nullptr` if the handle is invalid.

Typical use: resolving a cross-repository relation stored in a row. `FCharacterRow::DefaultWeapon` is an `FDataIndexerRowHandle` pointing at the item repository:

```cpp
// CharacterTypes.h
UPROPERTY(EditAnywhere, BlueprintReadOnly,
    meta = (Schema = "/Script/DataIndexerProject.ItemSchema"))
FDataIndexerRowHandle DefaultWeapon;
```

```cpp
if (const FItemRow* Weapon = FItemInterface::FindRow(Character->DefaultWeapon))
{
    // Weapon->DisplayName, Weapon->BaseValue, ...
}
```

---

## ForEachPrimaryKeys (all rows)

```cpp
static void ForEachPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback);
```

Iterates all primary keys in the repository (including parents). Thin delegate to `Repository.ForEachPrimaryKeys(Callback)`.

```cpp
FItemInterface::ForEachPrimaryKeys(Repository, [](const FDataIndexerPrimaryKey& Key)
{
    // ...
});
```

---

## ForEachPrimaryKeys (by index)

```cpp
static void ForEachPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const FDataIndexerIndexKey& IndexKey,
    const TRowType& Query,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback);
```

Iterates only the primary keys whose index value matches `Query`. Pass a partially filled row to express "find all rows with the same field as this query":

```cpp
FItemRow Query;
Query.Type = EItemType::Weapon;

FItemInterface::ForEachPrimaryKeys(Repository, UItemSchema::ByTypeIndex().IndexKey, Query,
    [](const FDataIndexerPrimaryKey& Key)
    {
        // ...
    });
```

---

## GetPrimaryKeys

```cpp
static TArray<FDataIndexerPrimaryKey> GetPrimaryKeys(
    const UDataIndexerRepository& Repository);

static TArray<FDataIndexerPrimaryKey> GetPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const FDataIndexerIndexKey& IndexKey,
    const TRowType& Query);
```

Convenience wrappers over `ForEachPrimaryKeys` that collect results into a `TArray`.

```cpp
// All rows
TArray<FDataIndexerPrimaryKey> Keys = FItemInterface::GetPrimaryKeys(Repository);

// Filter by Type
FItemRow QueryByType;
QueryByType.Type = EItemType::Weapon;
TArray<FDataIndexerPrimaryKey> WeaponKeys =
    FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByTypeIndex(), QueryByType);

// Filter by Rarity
FItemRow QueryByRarity;
QueryByRarity.Rarity = EItemRarity::Rare;
TArray<FDataIndexerPrimaryKey> RareKeys =
    FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByRarityIndex(), QueryByRarity);

// Composite index: Type x Rarity
FItemRow QueryByTypeAndRarity;
QueryByTypeAndRarity.Type   = EItemType::Weapon;
QueryByTypeAndRarity.Rarity = EItemRarity::Rare;
TArray<FDataIndexerPrimaryKey> RareWeaponKeys =
    FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByTypeAndRarityIndex(), QueryByTypeAndRarity);
```
