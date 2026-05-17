---
title: Migrate from DataTable
---

# Migrate from DataTable

This guide walks through migrating an existing `UDataTable` to a DataIndexer repository. The struct definition carries over as-is; the main work is converting the JSON format and updating runtime query code.

## Key differences

| | DataTable | DataIndexer |
|-|-----------|-------------|
| Row identifier | `FName` (author-chosen string) | `FDataIndexerPrimaryKey` (auto-generated GUID) |
| Asset type | `UDataTable` | `UDataIndexerRepository` |
| Schema | Implicit — struct is embedded | Explicit — separate `UDataIndexerSchema` asset |
| Import format | CSV or JSON with flat row objects | JSON with `PrimaryKey` + `Row` wrapper |
| Secondary indexes | Manual code | Declarative `BuildIndex` functions |
| Hierarchy / inheritance | Not supported | Parent repository composition |

## Step 1 — Export DataTable rows as JSON { #export-datatable-rows-as-json }

1. Right-click your DataTable asset in the Content Browser
2. Select **Asset Actions → Export**
3. Choose **JSON** format and save

The exported file looks like this:

```json
[
  {
    "---rowName---": "IronSword",
    "DisplayName": "Iron Sword",
    "MaxStack": 1,
    "Category": "Weapon"
  },
  {
    "---rowName---": "HealthPotion",
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

## Step 3 — Transform the JSON { #transform-the-json }

DataIndexer's import format wraps each row in a `{ "PrimaryKey": "...", "Row": { ... } }` envelope and uses a GUID as the key instead of a string name.

The script below converts the DataTable export to the DataIndexer import format:

```python title="dt_to_di.py"
import json
import uuid

with open("datatable_export.json", encoding="utf-8") as f:
    rows = json.load(f)

out = []
for row in rows:
    row.pop("---rowName---", None)
    out.append({
        "PrimaryKey": str(uuid.uuid4()),
        "Row": row
    })

with open("repository_import.json", "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, ensure_ascii=False)
```

Run it:

```
python dt_to_di.py
```

The output `repository_import.json` is ready for import.

!!! note "Row name as display field"
    DataTable row names often double as human-readable labels. After migration, implement `GetRowDisplayName` in your schema to return a meaningful `FText` from an actual row field (e.g. `DisplayName`). DataIndexer's editor and Blueprint nodes will use this everywhere row names appeared before.

## Step 4 — Import

1. Right-click the repository asset → **Import JSON**
2. Select `repository_import.json`

The import is a **full replacement** — all existing rows are deleted and replaced. After save, the editor rebuilds secondary indexes automatically.

## Step 5 — Update runtime references

### Replace `FindRow` calls

=== "Before (DataTable)"

    ```cpp
    if (const FItemRow* Row = ItemTable->FindRow<FItemRow>(FName("IronSword"), TEXT("")))
    {
        // use Row
    }
    ```

=== "After (DataIndexer C++)"

    DataIndexer rows are retrieved by `FDataIndexerPrimaryKey`. Store the key as a handle property and query at runtime:

    ```cpp
    // Store a handle on the component or asset
    UPROPERTY(EditAnywhere)
    FDataIndexerHandle ItemHandle;

    // Query
    if (const FItemRow* Row = FItemInterface::FindRow(Repository, ItemHandle.PrimaryKey))
    {
        // use Row
    }
    ```

=== "After (Blueprint)"

    Use a **DataIndexer Handle** variable to hold the row reference. In the graph, drag from the handle and call **Get Row**. The combo box in the node's details lets designers pick a specific row by its display name.

### Replace full-table iteration

=== "Before (DataTable)"

    ```cpp
    TArray<FItemRow*> AllRows;
    ItemTable->GetAllRows(TEXT(""), AllRows);
    for (const FItemRow* Row : AllRows) { /* ... */ }
    ```

=== "After (DataIndexer)"

    ```cpp
    for (const FDataIndexerPrimaryKey& Key : FItemInterface::GetAllPrimaryKeys(Repository))
    {
        if (const FItemRow* Row = FItemInterface::FindRow(Repository, Key))
        {
            // use Row
        }
    }
    ```

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

Once migrated, use the [JSON Support](../editor-guide/json-support.md) page to manage future exports, reimports, and VCS diffs. The `dt_to_di.py` script is only needed for the initial migration — subsequent round-trips stay within DataIndexer's native JSON format.
