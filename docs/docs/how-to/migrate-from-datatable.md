---
title: Migrate from DataTable
---

# Migrate from DataTable

This guide walks through migrating an existing `UDataTable` to a DataIndexer repository. The struct definition carries over as-is, and no conversion script is required — DataIndexer imports the JSON you export from a DataTable **directly**, auto-detecting the format.

## Key differences

| | DataTable | DataIndexer |
|-|-----------|-------------|
| Row identifier | `FName` (author-chosen string) | `FDataIndexerPrimaryKey` (auto-generated GUID) |
| Asset type | `UDataTable` | `UDataIndexerRepository` |
| Schema | Implicit — struct is embedded | Explicit — separate `UDataIndexerSchema` asset |
| Import format | CSV or JSON with flat row objects | Imports DataTable JSON directly; native format uses a `PrimaryKey` + `RowEntity` wrapper |
| Secondary indexes | Manual code | Declarative `BuildIndex` functions |
| Hierarchy / inheritance | Not supported | Parent repository composition |

## Step 1 — Export DataTable rows as JSON { #export-datatable-rows-as-json }

1. Right-click your DataTable asset in the Content Browser
2. Select **Asset Actions → Export**
3. Choose **JSON** format and save

The exported file looks like this. The `Name` on each row is the DataTable row name:

```json
[
  {
    "Name": "IronSword",
    "DisplayName": "Iron Sword",
    "MaxStack": 1,
    "Category": "Weapon"
  },
  {
    "Name": "HealthPotion",
    "DisplayName": "Health Potion",
    "MaxStack": 10,
    "Category": "Consumable"
  }
]
```

## Step 2 — Create a Schema and Repository

If you do not yet have a schema for this data type, create one now. This step is identical to the [Quick Start](../quick-start.md) workflow.

1. Create a **Schema Blueprint** (or C++ subclass) with the same `RowStruct` as your DataTable
2. Create a **DataIndexer Repository** asset and bind it to the schema

The row struct itself does **not** need to change.

## Step 3 — Import { #import }

DataIndexer imports DataTable-format JSON directly — no conversion script needed.

1. Right-click the repository asset → **Import JSON**
2. Select the JSON you exported in Step 1

The format is detected automatically based on whether the first element has a `RowEntity` field. If it does not, the file is treated as DataTable format and a **dialog asks which property to bind the row name to**:

- The combo box lists the row struct's `FName` / `FString` / `FText` properties.
- Pick a property if you want to preserve the DataTable `Name` (row name).
- Choose **(Ignore)** if you don't need it. Rows become anonymous, identified only by `PrimaryKey` (GUID).

A `PrimaryKey` is auto-generated per row (if the JSON contains a `PrimaryKey` field, that GUID is used instead). The import is a **full replacement** — all existing rows are deleted and replaced. After save, the editor rebuilds secondary indexes automatically.

!!! note "Row name as display field"
    DataTable row names often double as human-readable labels. In most cases, choose **(Ignore)** to drop the row name and instead bind the schema's **Row Display Name Function** (or override `GetRowDisplayName` in C++) to return a meaningful `FText` from an actual row field (e.g. `DisplayName`). DataIndexer's editor and Blueprint nodes use this everywhere row names appeared before.

## Step 4 — Update runtime references

### Replace `FindRow` calls

=== "Before (DataTable)"

    ```cpp
    if (const FItemRow* Row = ItemTable->FindRow<FItemRow>(FName("IronSword"), TEXT("")))
    {
        // use Row
    }
    ```

=== "After (DataIndexer C++)"

    DataIndexer rows are retrieved by `FDataIndexerPrimaryKey`. Store an `FDataIndexerRowHandle` — which bundles the repository and key together — as a property and query at runtime:

    ```cpp
    // Store a handle on the component or asset
    UPROPERTY(EditAnywhere)
    FDataIndexerRowHandle ItemHandle;

    // Query — the handle carries both repository and primary key
    if (const FItemRow* Row = FItemInterface::FindRow(ItemHandle))
    {
        // use Row
    }
    ```

=== "After (Blueprint)"

    Use a **DataIndexer Row Handle** variable to hold the row reference. In the graph, drag from the handle and call **Get Row**. The combo box in the node's details lets designers pick a specific row by its display name.

### Replace full-table iteration

=== "Before (DataTable)"

    ```cpp
    TArray<FItemRow*> AllRows;
    ItemTable->GetAllRows(TEXT(""), AllRows);
    for (const FItemRow* Row : AllRows) { /* ... */ }
    ```

=== "After (DataIndexer)"

    ```cpp
    for (const FDataIndexerPrimaryKey& Key : FItemInterface::GetPrimaryKeys(*Repository))
    {
        if (const FItemRow* Row = FItemInterface::FindRow(*Repository, Key))
        {
            // use Row
        }
    }
    ```

=== "After (Blueprint)"

    Pass a repository to **Get All Primary Keys** to retrieve all primary keys, then loop with **For Each Loop** and call **Get Row** for each key.

### Replace name-based lookup with an index

If existing code was looking up rows by a string identifier (e.g. an item ID), migrate to a secondary index rather than iterating. See [Indexes](../concepts/indexes.md) for the full setup.

```cpp
// Find all Weapon-type items
FItemRow Query;
Query.Type = EItemType::Weapon;

TArray<FDataIndexerPrimaryKey> Keys =
    FItemInterface::GetPrimaryKeys(*Repository, UItemSchema::ByTypeIndex(), Query);
```

## Ongoing workflow

Once migrated, use the [JSON Support](../editor-guide/json-support.md) page to manage future exports, reimports, and VCS diffs. Subsequent round-trips stay within DataIndexer's native JSON format (`PrimaryKey` + `RowEntity`), and the row-name binding dialog no longer appears on reimport.
