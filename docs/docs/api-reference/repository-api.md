# Repository API

`UDataIndexerRepository` (`DataIndexerRepository.h`) is the runtime data asset class. All row queries go through its public methods.

## GetSchema

```cpp
const UDataIndexerSchema* GetSchema() const;
```

Returns the schema CDO bound to this repository. May return `nullptr` if no `SchemaClass` is set. Always null-check before use.

```cpp
if (const UDataIndexerSchema* Schema = Repository->GetSchema())
{
    // safe to use Schema
}
```

---

## FindRowEntity

```cpp
FConstStructView FindRowEntity(const FDataIndexerPrimaryKey& Key) const;
```

Looks up a row by primary key. Searches `LocalEntries` first, then recurses into `ParentRepositories`.

Returns an empty `FConstStructView` if the key is not found. Use `FConstStructView::IsValid()` to check.

**Typed access:**

```cpp
FConstStructView View = Repository->FindRowEntity(Key);
if (const FMyItemRow* Row = View.GetPtr<const FMyItemRow>())
{
    // use Row
}
```

Prefer `TNativeSchemaInterface<T>::FindRow` for a cleaner typed API.

---

## ForEachPrimaryKeys (all rows)

```cpp
void ForEachPrimaryKeys(
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

Iterates all primary keys visible through this repository, including those inherited from parent repositories. Order: local keys first (insertion order), then parent keys.

```cpp
Repository->ForEachPrimaryKeys([&](const FDataIndexerPrimaryKey& Key)
{
    FConstStructView Row = Repository->FindRowEntity(Key);
    // ...
});
```

---

## ForEachPrimaryKeys (by index)

```cpp
void ForEachPrimaryKeys(
    const FDataIndexerIndex& Index,
    const FDataIndexerIndexKey& IndexKey,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

Iterates primary keys matching a secondary index lookup. Consults the `ReverseLookups` table directly — O(matches), not O(all rows).

```cpp
Repository->ForEachPrimaryKeys(
    FDataIndexerIndex(UMyItemSchema::CategoryIndex.Identifier),
    WeaponIndexKey,
    [&](const FDataIndexerPrimaryKey& Key)
    {
        // only weapon rows
    });
```

---

## GetDisplayName

```cpp
FText GetDisplayName(const FDataIndexerPrimaryKey& PrimaryKey) const;
FText GetDisplayName(const FDataIndexerIndex& Index, const FDataIndexerIndexKey& IndexKey) const;
```

Calls the schema's `GetRowDisplayName` and returns a human-readable label. Returns `FText::GetEmpty()` if the schema is null or the row is not found.

---

## Editor-only methods

These methods are only available in `WITH_EDITOR` builds:

| Method | Description |
|--------|-------------|
| `GetRowStruct() const` | Returns the `UScriptStruct*` for the row type (delegates to schema) |
| `FindTableProperty(PropertyName)` | Finds an `FProperty` by name in the row struct |
| `IncludesRepository(Repository)` | Returns true if `Repository` is in this repository's parent hierarchy (cycle detection) |
| `GetScopedFilterKey()` | Returns the currently scoped filter key during filter evaluation (editor use only) |
