# JSON Import & Export

DataIndexer repositories support JSON export and import through the custom editor toolbar. The JSON format is designed to be human-readable and diff-friendly — suitable for version control review and merge workflows.

## Exporting

1. Open a repository in the custom editor
2. Click **Export JSON** in the toolbar (or use the **File** menu)
3. Choose a destination path

The exported file contains a JSON array of row objects:

```json
[
  {
    "PrimaryKey": "A1B2C3D4-E5F6-7890-ABCD-EF1234567890",
    "Row": {
      "DisplayName": "Iron Sword",
      "MaxStack": 1,
      "Category": "Weapon"
    }
  },
  {
    "PrimaryKey": "B2C3D4E5-F6A7-8901-BCDE-F12345678901",
    "Row": {
      "DisplayName": "Health Potion",
      "MaxStack": 10,
      "Category": "Consumable"
    }
  }
]
```

**Key points:**

- `PrimaryKey` is the row's `FDataIndexerPrimaryKey` as a GUID string
- `Row` is the row struct serialized using UE's JSON property serialization
- The export includes only local rows — parent repository rows are not exported

## Importing

1. Open a repository in the custom editor
2. Click **Import JSON** in the toolbar
3. Select a JSON file

The import performs a merge:

- Rows with existing `PrimaryKey` values are **updated** with the imported data
- Rows with new `PrimaryKey` values are **added**
- Rows absent from the import file are **left unchanged** (no deletion on import)

!!! tip "Full replacement"
    To fully replace the repository's contents from JSON, delete all rows first, then import.

## Diff workflows

The JSON export is the recommended format for code review and version control diffs.

**Recommended setup:**

1. Export JSON alongside the binary `.uasset` in your repository (e.g., `Content/Data/Items/DA_ItemRepository.json`)
2. Commit both files
3. In pull requests, reviewers can read the JSON diff to understand data changes without opening Unreal Editor

**`.gitattributes` for binary assets:**

```gitattributes
*.uasset binary
*.umap binary
```

The JSON sidecar remains text-diffable even as the binary changes.

## Merge support

DataIndexer provides merge hooks for the binary `.uasset` format. When a merge conflict occurs on a repository asset:

- Unreal Editor's merge dialog can be used to resolve conflicts at the row level
- The JSON sidecar can be used as a reference for manual conflict resolution

For team workflows, combining JSON export with a custom merge tool produces the most reviewable and resolvable data history.
