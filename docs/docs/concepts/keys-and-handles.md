# Keys & Handles

DataIndexer uses three distinct address types to locate rows. Each is optimized for a different context.

## FDataIndexerPrimaryKey

`FDataIndexerPrimaryKey` extends `FGuid` and is the stable, permanent identity of a single row inside a repository.

```cpp
USTRUCT(BlueprintType)
struct DATAINDEXER_API FDataIndexerPrimaryKey : public FGuid { ... };
```

**Key properties:**

- Generated once when a row is created in the editor; never changes.
- Survives asset moves, renames, and repository refactors.
- Used as the key of `LocalEntries` in the repository.
- Tracks the owning repository via a `TWeakObjectPtr` (set during serialization, not stored in the UPROPERTY).

**When to use:** Low-level C++ iteration (`ForEachPrimaryKeys`), as map keys in game systems, or when building index lookup tables.

**When NOT to use:** Don't store a bare primary key as a UPROPERTY on an actor or data asset — use `FDataIndexerRowHandle` instead, which carries repository context.

## FDataIndexerRowHandle

`FDataIndexerRowHandle` pairs a repository reference with a primary key. It is the recommended UPROPERTY type for Blueprint variables and asset references to a single row.

```cpp
USTRUCT(BlueprintType, meta = (PresentAsType = "NamedStruct"))
struct DATAINDEXER_API FDataIndexerRowHandle
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UDataIndexerRepository> Repository;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDataIndexerPrimaryKey PrimaryKey;
    ...
};
```

**Key properties:**

- `IsValid()` — returns true if both `Repository` and `PrimaryKey` are non-null/non-zero.
- `FindRowEntity()` — delegates to `Repository->FindRowEntity(PrimaryKey)`.
- `GetRowDisplayName()` — returns the schema-driven display label.
- The editor renders this as a named row picker, showing display names from the schema.

**When to use:** UPROPERTY on actors, data assets, or save game structs when you want to point to a specific row. Blueprint variables for row references.

## FDataIndexerRowsHandle

`FDataIndexerRowsHandle` addresses a set of rows via a secondary index. It stores a repository, an index identifier, and an index key value.

```cpp
USTRUCT(BlueprintType)
struct DATAINDEXER_API FDataIndexerRowsHandle
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UDataIndexerRepository> Repository;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDataIndexerIndex Index;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDataIndexerIndexKey IndexKey;
    ...
};
```

**Key properties:**

- `IsValid()` — checks Repository, Index, and IndexKey are non-null/non-zero.
- `ForEachPrimaryKeys(Callback)` — delegates to `Repository->ForEachPrimaryKeys(Index, IndexKey, Callback)`.
- Use `GetRowsHandleKeys` from the Blueprint function library to get a `TArray<FDataIndexerPrimaryKey>`.

**When to use:** When a Blueprint or asset needs to express "the set of items with Category = Weapon" without enumerating them at authoring time. The exact set is resolved at runtime from the repository's reverse lookup tables.

## Summary

| Type | Addresses | Blueprint-friendly | Stores repository |
|------|-----------|-------------------|-------------------|
| `FDataIndexerPrimaryKey` | 1 row | Yes (BlueprintType) | No |
| `FDataIndexerRowHandle` | 1 row | Yes (BlueprintType) | Yes |
| `FDataIndexerRowsHandle` | N rows (via index) | Yes (BlueprintType) | Yes |
