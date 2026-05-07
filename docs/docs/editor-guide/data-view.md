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

Click **Add Row** in the toolbar, or press **Insert** with the Data View focused. A new row is created with:

- A freshly generated `FDataIndexerPrimaryKey` (GUID)
- Default-constructed values for all row struct properties

The new row appears at the bottom of the Data View.

![Pressing Insert to add a new row in the Data View](../assets/images/data-view-add-row.png)

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

## Context menu

Right-click a row to open the action menu.

![Row right-click context menu](../assets/images/data-view-context-menu.png)

| Item | Shortcut | Description |
|------|----------|-------------|
| **Copy ID** | — | Copy the selected row's primary key (GUID) to the clipboard |
| **Search this row** | — | Search for this row's data within the grid |
| **Copy** | `Ctrl+C` | Copy the selected row |
| **Paste** | `Ctrl+V` | Paste the copied row onto the selected position |
| **Duplicate** | `Ctrl+D` | Duplicate the selected row and append it at the end |
| **Insert** | `Ctrl+N` | Add a new row |
| **Delete** | `Delete` | Delete the selected row |
| **MoveUp** | `Ctrl+K` | Move the selected row up one position |
| **MoveDown** | `Ctrl+J` | Move the selected row down one position |
| **Override** | `Shift+O` | Override an inherited parent-repository row in this repository |
| **Toggle** | `Shift+T` | Toggle the DevOnlyRow flag |

## Column layout

Which properties appear as columns is controlled by the schema's **Expanded Struct Entries** configuration.

- Properties in `RowStruct` that are listed in `ExpandedStructEntries` appear as individual columns.
- Nested struct properties can be expanded into their own columns by adding the nested struct type to `ExpandedStructEntries`.
- Properties not in the expanded set are shown as a combined text cell in the grid but fully editable in the Selection Details panel.

The **MaxSimpleTextStructMembers** editor setting (**Project Settings → Plugins → DataIndexer (Editor)**) controls the threshold below which a nested struct is rendered inline vs. collapsed.

## Filters

Use the **Filters** dropdown in the toolbar to narrow which rows and columns are shown.

![Filters menu](../assets/images/data-view-filters.png)

### ROW filters

| Option | Description |
|--------|-------------|
| **Show Inherited Rows** | Show rows inherited from parent repositories. Hidden by default. |
| **Show Only Unreferred Rows** | Show only rows that are not referenced by any asset. Useful for cleaning up unused data. |
| **Show Hidden Rows** | Show rows that a parent repository has marked **Hidden in Children**. |

### COLUMN filters

| Option | Description |
|--------|-------------|
| **Visibility** | Show/hide the Visibility column that indicates each row's Editor Flags state |
| **Reference Viewer** | Show/hide the reference-state icon and Reference Viewer button column |
| **Properties** | Submenu to control individual property columns. Use **Show All** to show all columns at once. |

## Inherited rows

Rows from parent repositories appear in the Data View with a distinct background tint (enable **Filters → Show Inherited Rows** to see them). They cannot be edited in the child repository — double-click on them to navigate to the parent asset.

To edit an inherited row in the child, select it and press **Override** (`Shift+O`). The row is copied into the child repository with the same primary key and can then be edited independently.

## Reference Viewer

A reference-state icon appears at the left edge of each row. Click it to open UE5's **Reference Viewer** and list all assets that reference that row.

![Reference Viewer button](../assets/images/data-view-reference-viewer.gif)

- **Active icon** — the row is referenced by at least one asset
- **Inactive icon** — the row is not referenced anywhere

!!! tip
    Combine with **Filters → Show Only Unreferred Rows** to list all unreferenced rows and safely remove unused data.

## Editor Flags

The **Development** category in the Selection Details panel exposes **Editor Flags** per row. Multiple flags can be set simultaneously.

![Editor Flags dropdown](../assets/images/data-view-editor-flags.png)

| Flag | Description |
|------|-------------|
| **Comment Out** | Disables the row. Not included at game runtime, but data is preserved. Use for rows you want to temporarily exclude. |
| **Not Overridable in Children** | Prevents child repositories from overriding this row via the Override operation. |
| **Hidden in Children** | Hides this row from child repositories' Data View to prevent accidental reference or editing. |

## Keyboard shortcuts

Shortcuts available when the Data View has focus:

| Command | Shortcut | Description |
|---------|----------|-------------|
| Insert | `Ctrl+N` | Add a new row |
| Copy | `Ctrl+C` | Copy selected row |
| Paste | `Ctrl+V` | Paste on selected row |
| Duplicate | `Ctrl+D` | Duplicate selected row |
| Delete | `Delete` | Delete current selection |
| MoveUp | `Ctrl+K` | Move selected row up |
| MoveDown | `Ctrl+J` | Move selected row down |
| Override | `Shift+O` | Override selected inherited row |
| Toggle DevOnlyRow | `Shift+T` | Toggle DevOnlyRow flag |

You can also drag the **drag handle** icon (`⠿`) at the left edge of a row to reorder it.

![Drag to reorder rows](../assets/images/data-view-move-row.gif)

## Column width and ordering

Column widths and ordering are persisted per-schema in the **Schema Layouts** editor setting. Drag column headers to reorder. Drag the divider between column headers to resize.

### Reset Layout

Click **Reset Layout** in the toolbar to clear the saved column width and ordering data for this schema. A confirmation dialog appears; select **Yes** to reset. All width and ordering changes are lost and the schema's default column configuration is restored.

![Reset Layout confirmation dialog](../assets/images/data-view-reset-layout.png)
