# Data View

The Data View is the central panel of the repository editor. It presents rows as a grid where each column maps to a property of the row struct.

## Editor layout

The repository editor has three panels:

| Panel | Location | Purpose |
|-------|----------|---------|
| Asset Details | Left | Repository-level properties (Schema Class, Parent Repositories) |
| Data View | Center | Row grid — add, delete, and inline-edit rows |
| Selection Details | Right | Full property editor for the currently selected row |

## Adding rows

Click **Add Row** in the toolbar. A new row is created with:

- A freshly generated `FDataIndexerPrimaryKey` (GUID)
- Default-constructed values for all row struct properties

The new row appears at the bottom of the Data View.

## Deleting rows

Select one or more rows in the Data View, then:

- Press **Delete**, or
- Right-click → **Delete Selected Rows**

!!! warning "Deletion is permanent"
    Deleting a row removes its primary key. Any `FDataIndexerRowHandle` or `FDataIndexerPrimaryKey` elsewhere in the project that referenced it will become invalid. There is no undo after save.

## Editing rows

There are two ways to edit a row:

**Inline editing (Data View)**
: Click a cell directly in the grid. Simple scalar properties, enums, and short strings support inline editing.

**Selection Details (full editor)**
: Click a row to select it. The **Selection Details** panel on the right shows the full property editor with nested struct support, asset pickers, and custom widgets registered by the schema.

## Column layout

Which properties appear as columns is controlled by the schema's **Expanded Struct Entries** configuration.

- Properties in `RowStruct` that are listed in `ExpandedStructEntries` appear as individual columns.
- Nested struct properties can be expanded into their own columns by adding the nested struct type to `ExpandedStructEntries`.
- Properties not in the expanded set are shown as a combined text cell in the grid but fully editable in the Selection Details panel.

The **MaxSimpleTextStructMembers** editor setting controls the threshold below which a nested struct is rendered inline vs. collapsed. See [Editor Settings](editor-settings.md).

## Inherited rows

Rows from parent repositories appear in the Data View with a distinct background tint. They cannot be edited in the child repository — double-click on them to navigate to the parent asset.

## Column width and ordering

Column widths and ordering are persisted per-schema in the **Schema Layouts** editor setting. Drag column headers to reorder. Drag the divider between column headers to resize.

To reset column layout to defaults: **Edit → Project Settings → DataIndexer (Editor) → Schema Layouts** → clear the entry for the relevant schema.
