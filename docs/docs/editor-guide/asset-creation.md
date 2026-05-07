# Asset Creation

## Creating a Schema Blueprint

A schema Blueprint defines the row struct and editor behavior for a repository.

Before creating a schema Blueprint, define the row struct it will use. Create a Blueprint struct asset in the Content Browser and add the properties that make up a single row:

![Creating a Blueprint struct for row data](../assets/images/bp-struct-creation.png)

1. In the **Content Browser**, right-click → **Blueprint Class**
2. In the class picker, search for `DataIndexerSchema`
3. Select it and click **Select**
4. Name the asset (e.g., `BP_ItemSchema`) and double-click to open it
5. In the **Class Defaults** panel:
   - Set **Row Struct** to the `USTRUCT` type that defines your row data
   - Optionally override **Get Row Display Name** (Blueprint event) to return a human-readable label

![Setting the row struct on a DataIndexerSchema Blueprint](../assets/images/schema-blueprint-creation.png)

Optionally implement **Get Row Display Name** to return a human-readable label for each row. In the example below, `AbilityClass` is used directly as the display name:

![GetRowDisplayName implementation returning AbilityClass as display name](../assets/images/schema-get-row-display-name.png)

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

![Repository with schema class assigned in the Details panel](../assets/images/repository-schema-binding.png)

!!! warning "Schema binding is permanent"
    Set the schema class before adding any rows. Changing it later is a destructive operation that will lose all existing row data.

## Setting Parent Repositories

A repository can reference one or more parent repositories. All rows owned by a parent are visible in the child's Data View, so you can build shared base tables and domain-specific overlays without duplicating rows.

**Typical patterns:**

- A global item repository as the parent; shop or chest repositories as children that only define their own unique entries.
- A base character-stat repository; per-faction children that override or extend specific rows.

**Setup:**

1. Open the child repository asset.
2. In **Details → Parent Repositories**, click the **+** button.
3. Select the parent repository asset.
4. Repeat to add multiple parents — the full array is walked recursively at runtime.

**Behaviour:**

- Inherited rows appear in the Data View with a distinct background tint. They are hidden by default; enable **Filters → Show Inherited Rows** to show them.
- Inherited rows cannot be edited in the child — open the parent to modify them.
- To edit an inherited row in the child, select it and press **Override** (`Shift+O`). This copies the row into the child repository with the same primary key so it can be edited independently.
- Parent-side **Editor Flags** can restrict what children can do: **Not Overridable in Children** blocks the override operation, and **Hidden in Children** hides the row from the child's Data View entirely.

!!! warning "Cycle detection"
    The editor blocks circular parent chains (`A → B → A`). If you attempt to create one, the parent field will reject the selection.

## Verifying the setup

Double-click the repository asset. The custom editor should open with:

- **Asset Details** panel (left) showing Schema Class and Parent Repositories
- **Selection Details** panel (right, empty until a row is selected)
- **Data View** (center) showing the column headers from your row struct
