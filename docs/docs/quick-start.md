# Quick Start

This guide walks through the minimal path from zero to querying rows from C++ and Blueprint.

## 1. Create a Row Struct

Define the struct that holds your row data. In C++:

```cpp
// MyItemRow.h
#pragma once
#include "CoreMinimal.h"
#include "MyItemRow.generated.h"

USTRUCT(BlueprintType)
struct FMyItemRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxStack = 1;
};
```

Or define it as a Blueprint struct in the Content Browser (**Miscellaneous → Structure**).

## 2. Create a Schema

A schema binds your row struct to DataIndexer and controls how the editor presents the data.

1. Right-click in the Content Browser → **Blueprint Class**
2. Search for `DataIndexerSchema` and select it as the parent class
3. Name it `BP_MyItemSchema` and open it
4. In the **Class Defaults**, set **Row Struct** to `MyItemRow`

!!! tip "C++ schema"
    For production use, subclass `UDataIndexerSchema` in C++ and call `RegisterFunction_BuildIndexKey` in `PostInitProperties` to register typed index key builders.

## 3. Create a Repository

1. Right-click in the Content Browser → **Miscellaneous → Data Asset**
2. Select `DataIndexerRepository` as the asset class
3. Name it `DA_MyItemRepository` and open it
4. In the details panel, set **Schema Class** to `BP_MyItemSchema`

## 4. Author Rows

Double-click `DA_MyItemRepository` to open the custom editor.

- Click **Add Row** in the toolbar to create a new entry — a `FDataIndexerPrimaryKey` (GUID) is generated automatically
- Edit properties directly in the Data View grid
- The **Schema** controls display names and which columns appear

## 5. Query from Blueprint

Use the **Get Row** node (from the `DataIndexer` category) to retrieve a typed row:

```
[Repository Reference] → K2Node_DataIndexerGetRow → [FMyItemRow pin]
```

Or use `FDataIndexerRowHandle` as a UPROPERTY in your actor to hold a reference to a specific row, then call **Get Row Handle Value** to extract the struct.

## 6. Query from C++

```cpp
#include "DataIndexerSchemaInterface.h"

using FItemInterface = DataIndexer::TNativeSchemaInterface<FMyItemRow>;

// Find a single row by primary key
TConstStructView<FMyItemRow> Row = FItemInterface::FindRow(Repository, PrimaryKey);
if (Row.IsValid())
{
    UE_LOG(LogTemp, Log, TEXT("Item: %s"), *Row.Get().DisplayName.ToString());
}

// Iterate all rows
FItemInterface::ForEachPrimaryKeys(Repository, [&](const FDataIndexerPrimaryKey& Key)
{
    TConstStructView<FMyItemRow> RowView = FItemInterface::FindRow(Repository, Key);
    // ...
});
```

---

## What's Next

- [Core Concepts](concepts/index.md) — understand how Repository, Schema, Keys, and Indexes fit together
- [Blueprint API](blueprint-api/index.md) — full reference for Blueprint nodes and the function library
- [C++ API](cpp-api/index.md) — typed C++ access patterns
