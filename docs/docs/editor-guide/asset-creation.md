# Asset Creation

## Creating a Schema Blueprint

A schema Blueprint defines the row struct and editor behavior for a repository.

1. In the **Content Browser**, right-click → **Blueprint Class**
2. In the class picker, search for `DataIndexerSchema`
3. Select it and click **Select**
4. Name the asset (e.g., `BP_ItemSchema`) and double-click to open it
5. In the **Class Defaults** panel:
   - Set **Row Struct** to the `USTRUCT` type that defines your row data
   - Optionally override **Get Row Display Name** (Blueprint event) to return a human-readable label

!!! tip "Name convention"
    Prefix schema Blueprints with `BP_` and suffix with `Schema` — e.g., `BP_ItemSchema`, `BP_QuestSchema`.

## Creating a Repository Asset

1. In the **Content Browser**, right-click → **Miscellaneous → Data Asset**
2. In the asset class picker, search for `DataIndexerRepository`
3. Select it and click **Select**
4. Name the asset (e.g., `DA_ItemRepository`)

## Binding Schema to Repository

Before you can author rows, the repository must know which schema (and therefore which row struct) to use.

1. Open the repository asset (single-click to open the **Details** panel, or double-click to open the full editor)
2. In the **Details** panel, locate **Schema Class**
3. Set it to your schema Blueprint (e.g., `BP_ItemSchema`)

Once a schema is bound, the repository's row struct is locked. **Changing the schema class after rows exist will invalidate all row data.**

!!! warning "Schema binding is permanent"
    Set the schema class before adding any rows. Changing it later is a destructive operation that will lose all existing row data.

## Setting Parent Repositories

To inherit rows from another repository:

1. Open the repository asset
2. In **Details → Parent Repositories**, click the **+** button
3. Select the parent repository asset

The parent's rows become visible in the Data View (shown with a different background to indicate inheritance). Parent rows cannot be edited in the child repository — open the parent to modify them.

## Verifying the setup

Double-click the repository asset. The custom editor should open with:

- **Asset Details** panel (left) showing Schema Class and Parent Repositories
- **Selection Details** panel (right, empty until a row is selected)
- **Data View** (center) showing the column headers from your row struct
