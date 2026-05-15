# Changelog

All notable changes to DataIndexer are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions correspond to the `VersionName` field in `DataIndexer.uplugin`.

---

## v0.9.2 { .release .release--current data-date="2026-05-08" data-tag="Current" }

Stability improvements and faster index traversal. No schema migration needed from 0.9.1.

### Added { .release-group .release-group--add }

- **`ForEachPrimaryKeys(Index, Key, ...)`** — Index-traversal overload. Loads from the reverse-lookup table directly; O(matches) path without scanning all rows.
- **`DI_DECLARE_MIGRATION`** — Declarative row-to-row migration macro.
- **JSON sidecar diff view** — Inline diff against the on-disk sidecar inside the editor.

### Changed { .release-group .release-group--change }

- `BuildIndexFunctions` now runs in deterministic order (alphabetical by builder name) to produce stable sidecar diffs.

### Fixed { .release-group .release-group--fix }

- Fixed a crash when `SchemaClass` was changed on a repository that still held rows from the previous schema.
- Editor columns no longer disappear after an undo that removes the last row.

---

## v0.9.1 { .release data-date="2026-04-10" }

Initial public release.

### Added { .release-group .release-group--add }

- `UDataIndexerRepository` data asset with parent composition.
- `UDataIndexerSchema` Blueprint-subclassable contract class.
- `FDataIndexerPrimaryKey` (GUID-backed), `FDataIndexerRowHandle`, `FDataIndexerIndex`.
- Custom three-panel editor (row list, detail form, column configurator).
- JSON sidecar export/import for diff-friendly review workflows.
- Initial documentation site.
