---
title: Parent Repository Hierarchy
---

# Parent Repository Hierarchy

A repository can declare one or more *parent* repositories. When iterating or querying, the hierarchy is walked recursively — rows in parents are visible through any child that includes them. This lets you build shared base tables and domain-specific overlays without duplicating rows.

## Scenario

This guide uses a classic item-shop scenario:

- `DI_AllItems` — the global item master with every item in the game
- `DI_ShopA` — the inventory for a specific NPC shop; sells a subset of items and overrides the price on some

```
DI_AllItems  (parent)
     └── DI_ShopA  (child — adds rows, overrides prices)
```

## Step 1 — Create the parent repository

Create `DI_AllItems` normally: right-click → **Miscellaneous → DataIndexer → Repository**, bind to `BP_ItemSchema`, and author all base item rows.

## Step 2 — Create the child repository and set the parent

1. Create `DI_ShopA` with the same schema (`BP_ItemSchema`)
2. Open `DI_ShopA` in the Data View
3. In the **Details** panel, find **Parent Repositories** and add `DI_AllItems`
4. Save `DI_ShopA`

The Data View now shows rows from both repositories. Parent-sourced rows are visually distinguished (lighter text or a lock icon, depending on the editor theme).

!!! note "Multiple parents"
    A repository can list more than one parent. Rows from all parents are visible, and the editor detects circular references automatically.

## Step 3 — Add child-only rows

Press **Insert** in `DI_ShopA`'s Data View to add rows that exist only in the shop. These rows are not visible from `DI_AllItems`.

## Override Mode { #override-mode }

To change a field on a parent row — for example, reduce the `ShopPrice` for a sale — edit that row in the child's Data View:

1. Open `DI_ShopA`
2. Click a parent-sourced row in the grid
3. The row enters **override mode** — editing any field creates a local copy in `DI_ShopA`'s `LocalEntries` without touching `DI_AllItems`

The overridden row now belongs to both: `DI_AllItems` retains the original, and `DI_ShopA`'s local copy holds the modified version. Querying `DI_ShopA` returns the overridden value.

To revert the override, select the row and press the **Reset to Parent** button in the toolbar.

### Locking fields that must not be overridden

If `BaseValue` and `Rarity` should be immutable, mark them `NotOverridable` in the UDStruct. When a child edits a parent row, those fields remain read-only. See [Repository Metadata → NotOverridable](repository-metadata.md#notoverridable) for setup steps.

## Selective Override { #selective-override }

If the shop should only sell *some* items from `DI_AllItems` rather than all of them, do not use the parent hierarchy for this — parent rows are always included.

Instead, maintain `DI_ShopA` as a standalone repository and use a `FDataIndexerHandle` array or a secondary index to define the shop's inventory explicitly. Use the parent hierarchy when you want *full* visibility plus optional field overrides, not when you want a subset.

## Runtime Querying { #runtime-querying }

Querying `DI_ShopA` transparently returns rows from both the child and all parents. No special handling is needed.

=== "C++"

    ```cpp
    // GetAllPrimaryKeys walks the entire hierarchy
    for (const FDataIndexerPrimaryKey& Key : FItemInterface::GetAllPrimaryKeys(ShopARepository))
    {
        if (const FItemRow* Row = FItemInterface::FindRow(ShopARepository, Key))
        {
            // Row comes from ShopA (own or override) or AllItems (parent)
        }
    }
    ```

    To query only parent-owned rows (e.g. to show original stats), call `FindRow` on the parent repository directly:

    ```cpp
    if (const FItemRow* BaseRow = FItemInterface::FindRow(AllItemsRepository, Key))
    {
        // Original value from parent
    }
    ```

=== "Blueprint"

    Pass `DI_ShopA` to **Get All Primary Keys** — the result includes all primary keys visible through the full hierarchy. Feed each key into **Get Row** as usual.

    To compare against the parent's original value, pass `DI_AllItems` to a second **Get Row** node with the same key.

## Multi-level hierarchies

The composition is recursive. `DI_RegionSale` can set `DI_ShopA` as its parent, which in turn has `DI_AllItems`:

```
DI_AllItems
    └── DI_ShopA
            └── DI_RegionSale
```

Querying `DI_RegionSale` walks the full chain. An override in `DI_ShopA` takes precedence over `DI_AllItems`; a further override in `DI_RegionSale` takes precedence over `DI_ShopA`.

The editor's cycle detection prevents infinite loops if you accidentally create a circular parent reference.

## Related

- [Repository — Parent Repository Composition](../concepts/repository.md#parent-repository-composition)
- [Repository Metadata → NotOverridable](repository-metadata.md#notoverridable)
