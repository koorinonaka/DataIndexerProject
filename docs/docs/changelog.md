# Changelog

All notable changes to DataIndexer are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions correspond to the `VersionName` field in `DataIndexer.uplugin`.

---

## v1.0.0 { .release .release--current data-date="2026-05-24" }

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
