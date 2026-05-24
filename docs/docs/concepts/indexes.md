# Indexes

Indexes are secondary lookup dimensions that let you retrieve a set of rows by a domain-specific attribute — category, faction, rarity, or any other grouping — without iterating the entire repository.

## Types

| Type | Role |
|------|------|
| `FDataIndexerIndex` | Holds a `FDataIndexerIndexKey` and an editor-only `DevComment`. Implicitly converts to `FDataIndexerIndexKey` |
| `FDataIndexerIndexKey` | `FGuid` wrapper (USTRUCT) used as the map key that identifies a lookup dimension |

Builder functions return a `FGuid`; the compiler converts it to `FDataIndexerIndexKey` before storing in `ReverseLookups`.

## How indexes work

At save time, the compiler calls each registered **BuildIndex** function for every row. The builder returns a `FGuid` (the index key) and optionally sets a `FText` display name for editor labelling. The result is stored in the repository's `ReverseLookups` table:

```
ReverseLookups[FDataIndexerIndexKey] → { TMap<FGuid, TArray<FDataIndexerPrimaryKey>> }
```

At runtime, `Repository.ForEachPrimaryKeys(Index, Query, Callback)` calls the builder on the query struct to derive the lookup GUID, then performs a direct map lookup — O(matches), not O(all rows).

## Defining an index

### In C++

Use `DI_DEFINE_INDEX` to declare each index on the schema class. The macro generates a static accessor function that returns a stable `FDataIndexerIndex` with a deterministic GUID derived from the class path and the name.

!!! note "Define this macro in your project"
    `DI_DEFINE_INDEX` is not provided by the DataIndexer plugin. Add it to a project header (e.g., `DataIndexerKeyHelpers.h`):

```cpp title="DataIndexerKeyHelpers.h"
#define DI_DEFINE_INDEX( VarName ) \
    static const FDataIndexerIndex& VarName() \
    { \
        static const FDataIndexerIndex Key \
        { \
            FGuid::NewDeterministicGuid( StaticClass()->GetPathName() + TEXT( "." #VarName ) ), \
            INVTEXT( #VarName ) \
        }; \
        return Key; \
    }
```

```cpp title="ItemSchema.h"
UCLASS()
class UItemSchema : public UDataIndexerSchema
{
    GENERATED_BODY()
public:
    UItemSchema();

    DI_DEFINE_INDEX(ByTypeIndex);
    DI_DEFINE_INDEX(ByRarityIndex);

protected:
    UFUNCTION()
    static FGuid BuildTypeIndex(const FInstancedStruct& RowEntity);

    UFUNCTION()
    static FGuid BuildRarityIndex(const FInstancedStruct& RowEntity);
};
```

Register the builders in the constructor:

```cpp title="ItemSchema.cpp"
UItemSchema::UItemSchema()
{
    RowStruct = FItemRow::StaticStruct();
    RegisterFunction_BuildIndex(ByTypeIndex(),   GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeIndex));
    RegisterFunction_BuildIndex(ByRarityIndex(), GET_FUNCTION_NAME_CHECKED(ThisClass, BuildRarityIndex));
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

### In Blueprint

1. Open the Schema Blueprint → Class Defaults
2. In **Build Index Functions**, add an entry:
   - **Key**: An `FDataIndexerIndex` variable (set a fixed GUID in the variable defaults)
   - **Value**: A function reference matching the `Prototype_BuildIndex` signature (`RowEntity → FGuid`)

Implementation example (index by class):

![BuildIndexByClass implementation example](../assets/images/build-index-by-class.png)

`Get Instanced Struct Value` unpacks the row; `Enum to String` → `Parse String to Guid` derives a deterministic GUID from the class enum value.

## Querying by index

**C++** — pass a partially filled row value to express "find all rows with the same field as this query":

```cpp
// All Weapon-type items
FItemRow Query;
Query.Type = EItemType::Weapon;

TArray<FDataIndexerPrimaryKey> Keys =
    FItemInterface::GetPrimaryKeys(*Repository, UItemSchema::ByTypeIndex(), Query);
```

For a reverse index (e.g., all characters whose `DefaultWeapon` points to a specific item):

```cpp
FCharacterRow Query;
Query.DefaultWeapon.PrimaryKey = WeaponKey;

TArray<FDataIndexerPrimaryKey> Characters =
    FCharacterInterface::GetPrimaryKeys(*Repository, UCharacterSchema::ByDefaultWeaponIndex(), Query);
```

**Blueprint:**

Use a `FDataIndexerKeysHandle` UPROPERTY and the **Get Rows Handle Value** function library node. The node takes the handle and a **Query** wildcard struct pin — fill in the fields that drive the index (e.g., set `Type = Weapon` for a `ByType` index).

## Index key stability

The GUID returned by a builder function must be stable across editor sessions. Use deterministic values derived from enum ordinals, string hashes, or other stable domain attributes — never `FGuid::NewGuid()` inside a builder.

`DI_DEFINE_INDEX` uses `FGuid::NewDeterministicGuid(StaticClass()->GetPathName() + "." + IndexName)` for the index identifier, so renaming the index or the schema class (which changes `GetPathName()`) would change its GUID. Treat those names as stable API.

!!! note "When ReverseLookups is rebuilt"
    In the editor, the table is rebuilt automatically when accessed. For packaged builds, it is rebuilt at cook time. No manual re-save is required.
