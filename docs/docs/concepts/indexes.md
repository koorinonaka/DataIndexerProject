# Indexes

Indexes are secondary lookup dimensions that let you retrieve a set of rows by a domain-specific attribute — category, faction, rarity, or any other grouping — without iterating the entire repository.

## Types

| Type | Role |
|------|------|
| `FDataIndexerIndex` | Identifies a lookup dimension (extends `FGuid`) |
| `FDataIndexerIndexKey` | A computed key value for a row within an index (extends `FGuid`) |
| `FDataIndexerImmutableKey` | A stable, named identifier for an index definition — holds the `FGuid` and an optional `DevComment` |

## How indexes work

At save time, the compiler calls each registered **BuildIndexKey** function for every row. The result is a `(FDataIndexerIndexKey, FText display)` pair that is stored in the repository's `ReverseLookups` table:

```
ReverseLookups[Index][ImmutableKey] → TMultiMap<FDataIndexerImmutableKey, FDataIndexerPrimaryKey>
```

At runtime, `Repository.ForEachPrimaryKeys(Index, IndexKey, Callback)` performs a direct multimap lookup — O(matches), not O(all rows).

## Defining an index

### In C++

Declare a static `FDataIndexerImmutableKey` for each index, then register the builder in `PostInitProperties`:

```cpp
// MyItemSchema.h
static inline FDataIndexerImmutableKey CategoryIndex{
    FGuid(0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00),
    INVTEXT("Category")
};

// MyItemSchema.cpp
void UMyItemSchema::PostInitProperties()
{
    Super::PostInitProperties();
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        RegisterFunction_BuildIndexKey(
            FDataIndexerIndex(CategoryIndex.Identifier),
            GET_FUNCTION_NAME_CHECKED(UMyItemSchema, BuildCategoryKey));
    }
}

FDataIndexerIndexKey UMyItemSchema::BuildCategoryKey(
    const FInstancedStruct& RowEntity, FText& OutDisplayName)
{
    if (const FMyItemRow* Row = RowEntity.GetPtr<const FMyItemRow>())
    {
        OutDisplayName = UEnum::GetDisplayValueAsText(Row->Category);
        // Convert the enum value to a deterministic GUID
        return FDataIndexerIndexKey(FGuid(
            static_cast<uint32>(Row->Category), 0, 0, 0));
    }
    return {};
}
```

### In Blueprint

1. Open the Schema Blueprint → Class Defaults
2. In **Build Index Key Functions**, add an entry:
   - **Key**: An `FDataIndexerImmutableKey` (create a static Blueprint variable with a fixed GUID)
   - **Value**: A function reference matching the `Prototype_BuildIndexKey` signature (`RowEntity → IndexKey + OutDisplayName`)

## Querying by index

**C++:**

```cpp
Repository.ForEachPrimaryKeys(
    FDataIndexerIndex(UMyItemSchema::CategoryIndex.Identifier),
    WeaponIndexKey,
    [&](const FDataIndexerPrimaryKey& Key)
    {
        // process Key...
    });
```

**Blueprint:**

Use a `FDataIndexerRowsHandle` UPROPERTY and the **Get Rows Handle Keys** function library node to retrieve the matching primary keys.

## Index key stability

`FDataIndexerIndexKey` values must be stable across editor sessions. Use deterministic GUIDs derived from enum values, string hashes, or other stable domain attributes — never from `FGuid::NewGuid()` in a builder function.

!!! warning "Rebuild on schema change"
    If you change which index key a builder returns for existing rows, re-save the repository to rebuild the `ReverseLookups` table.
