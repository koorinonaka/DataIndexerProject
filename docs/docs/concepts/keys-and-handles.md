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
- `GetRowDisplayName()` — returns `TOptional<FText>` with the schema-driven display label; empty if the handle is invalid.
- The editor renders this as a named row picker, showing display names from the schema.

**When to use:** UPROPERTY on actors, data assets, or save game structs when you want to point to a specific row. Blueprint variables for row references.

## FDataIndexerRowsHandle

`FDataIndexerRowsHandle` addresses a set of rows via a secondary index. It stores a repository and an index identifier. The matching row set is determined at query time by passing a partially-filled row struct.

```cpp
USTRUCT(BlueprintType)
struct DATAINDEXER_API FDataIndexerRowsHandle
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UDataIndexerRepository> Repository;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDataIndexerIndex Index;
    ...
};
```

**Key properties:**

- `IsValid()` — checks Repository and Index are non-null/non-zero.
- `ForEachPrimaryKeys(Query, Callback)` — calls `Repository->ForEachPrimaryKeys(Index, Query, Callback)`, where `Query` is a `FConstStructView` of a partially-filled row struct.
- Use `GetRowsHandleKeys(Handle, Query)` from the Blueprint function library to get a `TArray<FDataIndexerPrimaryKey>`. `Query` is a wildcard struct pin in Blueprint.

**When to use:** When a Blueprint or asset needs to express "look up rows by this index" without fixing the filter value at authoring time. The filter (query struct) is supplied at call time and resolved against the repository's reverse lookup tables.

## Summary

| Type | Addresses | Blueprint-friendly | Stores repository | Query time |
|------|-----------|-------------------|-------------------|-----------|
| `FDataIndexerPrimaryKey` | 1 row | Yes (BlueprintType) | No | n/a |
| `FDataIndexerRowHandle` | 1 row | Yes (BlueprintType) | Yes | n/a |
| `FDataIndexerRowsHandle` | N rows (via index) | Yes (BlueprintType) | Yes | pass query struct |
