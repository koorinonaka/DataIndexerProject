# Native Schema Interface

`DataIndexer::TNativeSchemaInterface<TRowType>` (`DataIndexerSchemaInterface.h`) is a typed C++ façade over `UDataIndexerRepository`. It provides compile-time type safety for row queries without requiring any virtual dispatch or casting in calling code.

## Setup

Define a project-level type alias in a shared header:

```cpp
// MyGameDataTypes.h
#pragma once

#include "DataIndexerSchemaInterface.h"
#include "MyItemRow.h"

using FItemInterface = DataIndexer::TNativeSchemaInterface<FMyItemRow>;
```

Then call through the alias anywhere you query the item repository:

```cpp
#include "MyGameDataTypes.h"

void UMySystem::InitItems(const UDataIndexerRepository* Repository)
{
    FItemInterface::ForEachPrimaryKeys(*Repository, [&](const FDataIndexerPrimaryKey& Key)
    {
        TConstStructView<FMyItemRow> Row = FItemInterface::FindRow(*Repository, Key);
        if (Row.IsValid())
        {
            // typed access: Row.Get().DisplayName
        }
    });
}
```

---

## FindRow (by PrimaryKey)

```cpp
static TConstStructView<TRowType> FindRow(
    const UDataIndexerRepository& Repository,
    const FDataIndexerPrimaryKey& PrimaryKey);
```

Delegates to `Repository.FindRowEntity(PrimaryKey)`, then casts to `TConstStructView<TRowType>`. Returns an empty (invalid) view if the key is not found or the struct type does not match.

```cpp
TConstStructView<FMyItemRow> Row = FItemInterface::FindRow(*Repository, Key);
if (Row.IsValid())
{
    int32 MaxStack = Row.Get().MaxStack;
}
```

---

## FindRow (by RowHandle)

```cpp
static TConstStructView<TRowType> FindRow(
    const FDataIndexerRowHandle& RowHandle);
```

Convenience overload that validates the handle and delegates to `FindRow(*RowHandle.Repository, RowHandle.PrimaryKey)`. Returns an empty view if the handle is invalid.

---

## ForEachPrimaryKeys (all rows)

```cpp
static void ForEachPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback);
```

Iterates all primary keys in the repository (including parents). Thin delegate to `Repository.ForEachPrimaryKeys(Callback)`.

---

## ForEachPrimaryKeys (by index using a row value)

```cpp
static void ForEachPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const FDataIndexerIndex& Index,
    const TRowType& Row,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback);
```

Computes the index key from `Row` by calling the schema's registered builder for `Index`, then delegates to `Repository.ForEachPrimaryKeys(Index, IndexKey, Callback)`.

This is the typed secondary lookup path — pass a row value to express "find all rows with the same category as this row":

```cpp
FMyItemRow QueryRow;
QueryRow.Category = EItemCategory::Weapon;

FItemInterface::ForEachPrimaryKeys(
    *Repository,
    FDataIndexerIndex(UMyItemSchema::CategoryIndex.Identifier),
    QueryRow,
    [&](const FDataIndexerPrimaryKey& Key)
    {
        // all weapon rows
    });
```

---

## GetPrimaryKeys

```cpp
static TArray<FDataIndexerPrimaryKey> GetPrimaryKeys(
    const UDataIndexerRepository& Repository);

static TArray<FDataIndexerPrimaryKey> GetPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const FDataIndexerIndex& Index,
    const TRowType& Row);
```

Convenience wrappers over `ForEachPrimaryKeys` that collect results into a `TArray`.

---

## GetDisplayName

```cpp
static FText GetDisplayName(
    const UDataIndexerRepository& Repository,
    const FDataIndexerPrimaryKey& PrimaryKey);
```

Calls the schema's `GetRowDisplayName` with the row entity fetched from the repository. Returns `FText::FromName(NAME_None)` if the schema is null.
