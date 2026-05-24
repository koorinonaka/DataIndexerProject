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
- Identifies and stores the owning repository from metadata, enabling tracking.

**When to use:** Low-level C++ iteration (`ForEachPrimaryKeys`), as map keys in game systems, or when building index lookup tables.

### `Repository` metadata

Bare primary keys can be stored as a `UPROPERTY` on actors, data assets, or structs. Add `meta = (Repository = "...")` so the editor knows which repository to drive the row-picker with.

The value is the **name of a property or function** on the same class that returns a `UDataIndexerRepository*`. A no-arg `UFUNCTION` is also accepted.

=== "C++"

    ```cpp
    // Property reference — Repository must be a UPROPERTY on the same object
    UPROPERTY(EditAnywhere, meta = (Repository = "Repository"))
    FDataIndexerPrimaryKey SingleKey;

    UPROPERTY(EditAnywhere, meta = (Repository = "Repository"))
    TArray<FDataIndexerPrimaryKey> MultipleKeys;

    // Function reference — no-arg UFUNCTION returning UDataIndexerRepository*
    UPROPERTY(EditAnywhere, meta = (Repository = "GetRepository"))
    FDataIndexerPrimaryKey KeyFromFunction;
    ```


=== "Blueprint"

    Open the Blueprint variable's **Details** panel. With a `FDataIndexerPrimaryKey` variable selected, a **Repository** dropdown appears. Choose the property or function that supplies the row set.
### `ReadOnlyKeys` metadata

Marks a `FDataIndexerPrimaryKey` Blueprint variable as read-only. The row-picker is hidden; the stored value can only be set programmatically.

=== "C++"

    ```cpp
    UPROPERTY(EditAnywhere, meta = (ReadOnlyKeys))
    FDataIndexerPrimaryKey ReadOnlyKey;
    ```

    The stored value can also be controlled programmatically via the Blueprint variable metadata API:

    ```cpp
    // Lock — hide the row-picker
    FBlueprintEditorUtils::SetBlueprintVariableMetaData(
        Blueprint, VarName, nullptr,
        DataIndexer::MetaDataKeys::ReadOnlyKeys, TEXT("true"));

    // Unlock — show the row-picker again
    FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(
        Blueprint, VarName, nullptr,
        DataIndexer::MetaDataKeys::ReadOnlyKeys);
    ```


=== "Blueprint"

    Open the Blueprint variable's **Details** panel. With a `FDataIndexerPrimaryKey` variable selected, toggle the **Read Only Keys** checkbox. When checked, the row-picker disappears and the key value is locked.
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

### `Schema` metadata

Filters the repository picker to repositories of a specific schema. Add `meta = (Schema = "AssetPath")`. The editor resolves the path and restricts the asset picker to matching repositories.

=== "C++"

    ```cpp
    UPROPERTY(EditAnywhere,
        meta = (Schema = "/Game/DataIndexer/DA_MySchema.DA_MySchema"))
    FDataIndexerRowHandle RowHandle;
    ```


=== "Blueprint"

    Open the Blueprint variable's **Details** panel. With a `FDataIndexerRowHandle` variable selected, a **Schema** picker appears. Select a schema asset to restrict the repository picker to matching repositories. Clear it to show all repositories.
## FDataIndexerKeysHandle

`FDataIndexerKeysHandle` addresses a set of rows via a secondary index. It stores a repository and an index identifier. The matching row set is determined at query time by passing a partially-filled row struct.

```cpp
USTRUCT(BlueprintType)
struct DATAINDEXER_API FDataIndexerKeysHandle
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
- Use the `GetKeysByIndex` custom K2Node in Blueprint to get a `TArray<FDataIndexerPrimaryKey>`.

**When to use:** When a Blueprint or asset needs to express "look up rows by this index" without fixing the filter value at authoring time. The filter (query struct) is supplied at call time and resolved against the repository's reverse lookup tables.

## Summary

| Type | Addresses | Blueprint-friendly | Stores repository | Query time |
|------|-----------|-------------------|-------------------|-----------|
| `FDataIndexerPrimaryKey` | 1 row | Yes (BlueprintType) | No | n/a |
| `FDataIndexerRowHandle` | 1 row | Yes (BlueprintType) | Yes | n/a |
| `FDataIndexerKeysHandle` | N rows (via index) | Yes (BlueprintType) | Yes | pass query struct |
