# Indexes

Indexes are secondary lookup dimensions that let you retrieve a set of rows by a domain-specific attribute — category, faction, rarity, or any other grouping — without iterating the entire repository.

## Types

| Type | Role |
|------|------|
| `FDataIndexerIndex` | Identifies a lookup dimension; holds a deterministic GUID and an optional `DevComment` (editor-only) |

Index keys are raw `FGuid` values computed by the builder function — there is no separate `FDataIndexerIndexKey` type.

## How indexes work

At save time, the compiler calls each registered **BuildIndex** function for every row. The builder returns a `FGuid` (the index key) and optionally sets a `FText` display name for editor labelling. The result is stored in the repository's `ReverseLookups` table:

```
ReverseLookups[FDataIndexerIndex] → { TMap<FGuid, TArray<FDataIndexerPrimaryKey>> }
```

At runtime, `Repository.ForEachPrimaryKeys(Index, Query, Callback)` calls the builder on the query struct to derive the lookup GUID, then performs a direct map lookup — O(matches), not O(all rows).

## Defining an index

### In C++

Use `DI_DEFINE_INDEX` to declare each index on the schema class. The macro generates a static accessor function that returns a stable `FDataIndexerIndex` with a deterministic GUID derived from the class path and the name:

```cpp
// ItemSchema.h
UCLASS()
class UItemSchema : public UDataIndexerSchema
{
    GENERATED_BODY()
public:
    DI_DEFINE_INDEX(ByTypeIndex);
    DI_DEFINE_INDEX(ByRarityIndex);

protected:
    virtual void PostInitProperties() override;

    UFUNCTION()
    static FGuid BuildTypeIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName);

    UFUNCTION()
    static FGuid BuildRarityIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName);
};
```

Register the builders in `PostInitProperties`. Call `Super::PostInitProperties()` **after** the CDO block:

```cpp
// ItemSchema.cpp
void UItemSchema::PostInitProperties()
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        RowStruct = FItemRow::StaticStruct();
        RegisterFunction_BuildIndex(ByTypeIndex(),   GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeIndex));
        RegisterFunction_BuildIndex(ByRarityIndex(), GET_FUNCTION_NAME_CHECKED(ThisClass, BuildRarityIndex));
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

### In Blueprint

1. Open the Schema Blueprint → Class Defaults
2. In **Build Index Functions**, add an entry:
   - **Key**: An `FDataIndexerIndex` variable (set a fixed GUID in the variable defaults)
   - **Value**: A function reference matching the `Prototype_BuildIndex` signature (`RowEntity → FGuid`)

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

Use a `FDataIndexerRowsHandle` UPROPERTY and the **Get Rows Handle Keys** function library node. The node takes the handle and a **Query** wildcard struct pin — fill in the fields that drive the index (e.g., set `Type = Weapon` for a `ByType` index).

## Index key stability

The GUID returned by a builder function must be stable across editor sessions. Use deterministic values derived from enum ordinals, string hashes, or other stable domain attributes — never `FGuid::NewGuid()` inside a builder.

`DI_DEFINE_INDEX` uses `FGuid::NewDeterministicGuid(StaticClass()->GetPathName() + "." + IndexName)` for the index identifier, so renaming the index or the schema class (which changes `GetPathName()`) would change its GUID. Treat those names as stable API.

!!! warning "Rebuild on schema change"
    If you change which index key a builder returns for existing rows, re-save the repository to rebuild the `ReverseLookups` table.
