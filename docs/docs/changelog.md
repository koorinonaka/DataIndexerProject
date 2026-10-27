---
hide:
  - navigation
---

# Changelog

All notable changes to DataIndexer are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions correspond to the `VersionName` field in `DataIndexer.uplugin`.

---

## v1.0.2 { .release .release--current data-date="2026-10-25" }

Patch release that makes the NotOverridable lock usable from Blueprint, adds row validation and JSON import commands, and fixes Data View and compile robustness issues.

### Added { .release-group .release-group--add }

- NotOverridable lock in the schema's **Column Layout** — a lock icon next to each User Defined Struct field's visibility toggle. Locked fields are read-only in override rows.
- **Validate** — a row context-menu command in the Data View that runs schema row validation on the selected rows and logs per-row results to the **Asset Check** message log.
- `UDataIndexerEditorFunctionLibrary::ImportRowsFromJSON` — Blueprint-callable import of a DataIndexer-format JSON string into a repository. It replaces the repository's rows and can be undone.
- Selection Details builds nested subcategories from `Parent|Child` category names and honors `ShowOnlyInnerProperties`. The category filter chips act on root categories.

### Changed { .release-group .release-group--change }

- **Breaking:** `UDataIndexerSchema::IsRowValid` now takes the row's `FDataIndexerPrimaryKey` as its first parameter. Update C++ overrides to the new signature.
- NotOverridable fields in derived repositories always follow the nearest parent repository. The value is resolved on load, editor open, JSON import, and row override, is not stored in the derived asset, and is baked in at compile time. A tagged sub-struct becomes read-only as a whole in override rows.
- Data View columns size from the text the schema's cells actually draw, including `CreateAsSimpleText`. The RowName column auto-sizes and keeps a manual resize. Object cells show the asset name; the full path stays searchable.
- Shift + mouse wheel scrolls the Data View horizontally.
- Validation issues for a row are prefixed with a link to its repository.
- `AddRowFromView` / `UpdateRowFromView` accept a const repository.

### Fixed { .release-group .release-group--fix }

- The NotOverridable checkbox for User Defined Struct fields never appeared in the struct editor. It now lives in the schema's Column Layout (see Added).
- Diamond-shaped parent graphs visited the shared ancestor once per path, duplicating its rows in the Data View. Each repository is now visited once.
- Dragging rows to reorder dropped them at the wrong position while a filter hid rows or inherited rows were interleaved.
- A compile with only warnings (for example, an unbound Build Index function) no longer fails.
- Once one row failed validation, every later row was also reported Invalid. Each row is now judged by its own issues.
- DataTable JSON import validates every element before replacing data, so a malformed file no longer leaves the asset half-imported.
- Cell copy and paste escape tabs and newlines in values.
- When a schema's RowStruct no longer matches a repository's stored rows, opening the repository offers to reset the rows instead of leaving every edit command disabled.
- Custom K2 nodes no longer fall back to wildcard pins while a Blueprint loads, and the **Get Keys by Index** `OutKeys` pin is wired correctly.
- `Repository` metadata now resolves through `TOptional` properties.
- The repository picker in Data View cells falls back to the shared thumbnail pool.
- Fixed crashes from a stale selection after undoing a row insert and from clearing a PrimaryKey combo selection.
- Duplicating an inherited row appends the copy instead of inserting it at the top.
- Data View UI: the editor tab is named "Data Indexer" (was "State Tree"), a fill-handle drag keeps the cell selection, **Show All** shows every column from a mixed state, and **Duplicate** works from a cell selection.
- Reading column layout no longer adds entries to the per-user settings, and Selection Details category choices are saved to the user config instead of the project's Default config.

---

## v1.0.1 { .release data-date="2026-06-07" }

Patch release focused on cell-widget customization and inline-editing fixes.

### Added { .release-group .release-group--add }

- `DI_REGISTER_BUILD_INDEX` — macro to register an index builder, with the builder signature checked at compile time.
- Virtual columns (`FDataIndexerVirtualColumn`, `UDataIndexerSchema::VirtualColumns`) — declare extra Data View columns. With a `SourceProperty` they alias a row property (dotted, e.g. `"Inner.A"`, for nested members) so one value can be shown as several columns with different presentations; with an empty `SourceProperty` they are unbound columns whose cell the schema supplies entirely via `CustomizePropertyCellWidget`.
- Column reordering — drag column headers in the Data View to rearrange data property and Virtual columns. Order is persisted per-schema in **Schema Layouts**.
- `FDataIndexerVirtualColumn::PreferredWidth` — optional initial width (pixels) for a virtual column; left unset, the column auto-sizes from its content.
- `RowValidationFunction` — an optional per-row validation hook on the schema, assignable in Blueprint (function libraries allowed) and complementing the C++ `IsRowValid`. Issues are reported through a `UDataIndexerRowValidationContext`.
- Editor row CRUD via `UDataIndexerEditorFunctionLibrary` — Blueprint-callable **Add Row**, **Update Row**, and **Delete Row(s)** that write through the editor's transactional (undoable) staging layer.
- `IDataIndexerInterface_CellContext` (`SetCellContext`) — the Data View injects the owning repository and primary key into a freshly built cell widget that implements this interface, so a Blueprint-authored editable cell can write edits back via **Update Row**.

### Changed { .release-group .release-group--change }

- `CustomizePropertyCellWidget` customization functions now receive the row's primary key, so customizations can vary by row identity.

### Fixed { .release-group .release-group--fix }

- `CustomizePropertyCellWidget` customizations that create UMG widgets now render correctly (previously failed to resolve a World context).
- Validation errors now name the specific problem and the affected column instead of a generic message.
- Row display names resolve correctly for invalid or schema-less rows.
- Inline (double-click) cell editors now lay out with correct alignment and padding.

---

## v1.0.0 { .release data-date="2026-05-24" }

First public release.

### Added { .release-group .release-group--add }

**Search**

- `FDataIndexerPrimaryKey` (GUID-backed), `FDataIndexerRowHandle`, `FDataIndexerIndex` — core identifier types.
- `DI_DEFINE_INDEX` — declare a secondary index with a stable GUID derived from class path and name.
- `ForEachPrimaryKeys(Index, Key, ...)` — O(matches) reverse-lookup traversal; no full-row scan.
- `FDataIndexerKeysHandle` — Blueprint-side index-query handle that resolves matching keys at use time.

**Authoring**

- Custom three-panel editor: row list, detail form, column configurator.
- JSON sidecar export / import with inline diff view against the on-disk sidecar.
- `UDataIndexerDrivenCollection` — per-key companion asset (icons, meshes, ability classes) that rebuilds automatically on repository save.
- `IsRowValid` — C++ validation hook in `UDataIndexerSchema`; runs on **Validate Data**, asset save, and cook; blocks on error.
- Property metadata: `meta=(Repository=...)` for row pickers, `meta=(Schema=...)` for asset filtering, `meta=(ReadOnlyKeys)` for read-only pickers.
- Per-row editor flags: `Comment Out`, `Not Overridable in Children`, `Hidden in Children`.
- Copy / paste for rows and tab-delimited cell ranges.
- Reference Viewer column, column layout persistence per schema, **Show Inherited Rows** / **Show Only Unreferred Rows** filters.

**Structure**

- `UDataIndexerRepository` data asset with multi-level parent composition and per-row override control.
- `UDataIndexerSchema` — Blueprint-subclassable contract class binding row struct, display logic, and index builders.

**Integration**

- `TNativeSchemaInterface<T>` — typed C++ accessor wrapping `FindRow`, `ForEachPrimaryKeys`, and `GetPrimaryKeys`.
- **Get Row** K2 node — typed retrieval from `FDataIndexerRowHandle` with success / failure execution pins.
- **Get Keys by Index** K2 node — retrieves matching primary keys from an `FDataIndexerKeysHandle` + query struct.
- **Select by Primary Key** K2 node — pure value selector by row identity.
- **Switch on Primary Key** K2 node — execution dispatch by row identity.
