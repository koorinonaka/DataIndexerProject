# Editor Settings

DataIndexer's editor behavior is configured through **Project Settings → Plugins → DataIndexer (Editor)**.

These are per-project settings (`UDataIndexerEditorSettings`) stored in the project's `Config/` directory. They affect all users on the project when committed.

---

## MaxSimpleTextStructMembers

**Type:** `int32` | **Default:** `3`

Controls when a nested struct property in the Data View grid is rendered as combined inline text versus collapsed to a single cell.

When a nested `FStructProperty` has **≤ MaxSimpleTextStructMembers** members, the Data View renders each member as individual text in the cell. When the member count exceeds this threshold, the cell shows a collapsed summary instead.

**Example:**

- A `FVector` (3 members: X, Y, Z) with default `MaxSimpleTextStructMembers = 3` → renders as `"100.0, 200.0, 0.0"` inline
- A struct with 4+ members → collapses to `"[...]"` in the grid cell; full editing happens in the Selection Details panel

Increase this value if you want more nested struct members to be readable directly in the grid. Decrease it if your row structs have large nested types and the grid becomes cluttered.

---

## SchemaLayouts

**Type:** `TMap<FSoftObjectPath, FDataIndexerSchemaLayout>`

Persists per-schema column width and ordering state. This is managed automatically by the editor as you drag columns — you do not normally need to edit it directly.

**To reset column layout for a schema:**

1. Open **Edit → Project Settings → Plugins → DataIndexer (Editor)**
2. Find the **Schema Layouts** map
3. Locate the entry for the schema whose layout you want to reset
4. Delete the entry (click the **−** button)
5. Reopen the repository editor — columns revert to their default order and width

Committing `Config/DefaultEditor.ini` (where these settings are stored) shares column layout across the team. If team members have divergent column preferences, consider keeping `Config/DefaultEditor.ini` local (add it to `.gitignore` for DataIndexer-specific keys) or standardizing on a single layout.
