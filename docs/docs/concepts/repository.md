# Repository

`UDataIndexerRepository` is a `UDataAsset` subclass that stores a typed collection of rows. It is the primary deliverable of the DataIndexer workflow: designers author rows here, and runtime code queries rows from here.

## Internal storage

| Field | Type | Purpose |
|-------|------|---------|
| `SchemaClass` | `TSubclassOf<UDataIndexerSchema>` | Determines the row struct and editor behavior |
| `LocalEntries` | `TMap<FDataIndexerPrimaryKey, FInstancedStruct>` | The rows owned directly by this repository |
| `EntryOwners` | `TMap<FDataIndexerPrimaryKey, TObjectPtr<UDataIndexerRepository>>` | Tracks which repository in the hierarchy owns each key |
| `ReverseLookups` | `TMap<FDataIndexerIndex, FLookupIndex>` | Secondary index tables built at save time |
| `ParentRepositories` | `TArray<TObjectPtr<UDataIndexerRepository>>` | Inherited repositories (editor-only authoring) |

## Parent repository composition

A repository can reference one or more parent repositories. When iterating or querying, the hierarchy is walked recursively — a row in a parent is visible through any child that includes it.

This lets you build shared base tables (e.g., a global item repository) and domain-specific overlays (e.g., a shop's subset) without duplicating rows.

!!! note "Cycle detection"
    `IncludesRepository(Repository)` detects cycles in the parent chain. The editor prevents circular references.

## Public API

### `GetSchema()`

```cpp
const UDataIndexerSchema* GetSchema() const;
```

Returns the schema CDO bound to this repository. May return `nullptr` if no schema class is set.

### `FindRowEntity(Key)`

```cpp
FConstStructView FindRowEntity(const FDataIndexerPrimaryKey& Key) const;
```

Looks up a row by primary key, searching the local map and recursing into parent repositories. Returns an empty `FConstStructView` if not found. Cast via `FConstStructView::GetPtr<const TMyRow>()` or use `TNativeSchemaInterface<T>::FindRow`.

### `ForEachPrimaryKeys(Callback)`

```cpp
void ForEachPrimaryKeys(const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

Iterates all primary keys visible through this repository (including parents). Order matches `LocalEntries` insertion order for local rows; parent rows follow.

### `ForEachPrimaryKeys(Index, Query, Callback)`

```cpp
void ForEachPrimaryKeys(
    const FDataIndexerIndex& Index,
    const FConstStructView Query,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

Iterates primary keys that match a secondary index lookup. `Query` is a partially-filled row struct — the schema's builder function is called on it to derive the lookup GUID, then `ReverseLookups` is consulted directly. No per-row iteration.

### `GetDisplayName(PrimaryKey)`

```cpp
FText GetDisplayName(const FDataIndexerPrimaryKey& PrimaryKey) const;
```

Delegates to the schema's `GetRowDisplayName`. Used by the editor and Blueprint nodes for human-readable labels.

## Serialization

The binary asset format serializes `LocalEntries`, `EntryOwners`, and `ReverseLookups` via a custom `Serialize` override. `ReverseLookups` are rebuilt at save time from `BuildIndexFunctions` on the schema.

JSON export produces a sidecar that is human-readable and diff-friendly. See [JSON Import & Export](../editor-guide/json-import-export.md).
