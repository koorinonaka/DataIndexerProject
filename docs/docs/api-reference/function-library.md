# Function Library

`UDataIndexerFunctionLibrary` is a `UBlueprintFunctionLibrary` that provides utility nodes for working with DataIndexer types in Blueprint graphs. All nodes are in the **DataIndexer** category.

---

## Primary Key nodes

### Equal / Not Equal

```
EqualEqual_PrimaryKeyPrimaryKey(A, B) → bool
NotEqual_PrimaryKeyPrimaryKey(A, B) → bool
```

Compact node titles `==` and `!=`. `BlueprintThreadSafe`.

Compare two `FDataIndexerPrimaryKey` values for equality. Delegates to `FGuid::operator==`.

---

### To String

```
Conv_PrimaryKeyToString(PrimaryKey) → FString
```

Compact node title `→`. `BlueprintAutocast`, `BlueprintThreadSafe`.

Converts a primary key to its GUID string representation (e.g., `"A1B2C3D4-..."`).

---

### Cast Primary Key

```
CastPrimaryKey(PrimaryKey, OutKey) → bool  [CustomThunk]
```

Wildcard output — the `OutKey` pin takes the concrete key struct type registered for the repository's schema. Returns `true` if the cast succeeds. Used internally by K2 nodes; rarely called directly.

---

### Is Valid Primary Key

```
IsValidPrimaryKey(PrimaryKey) → bool
```

Returns `true` if the primary key is non-zero (i.e., `FGuid::IsValid()`).

---

## Row Handle nodes

### Is Valid Row Handle

```
IsValidRowHandle(RowHandle) → bool
```

Returns `true` if both `RowHandle.Repository` and `RowHandle.PrimaryKey` are valid.

---

### Get Row Handle Name

```
GetRowHandleName(RowHandle, OutDisplayName) → bool
```

Retrieves the schema-driven display name for the row. Returns `false` if the handle is invalid or the schema returns no display name.

---

### Get Row Handle Value

```
GetRowHandleValue(Handle, OutValue) → bool  [CustomThunk, BlueprintInternalUseOnly]
```

Extracts the typed row struct from a handle into `OutValue`. The `OutValue` wildcard pin must be wired to a variable of the correct row struct type. Returns `false` if the handle is invalid.

!!! note
    This node is marked `BlueprintInternalUseOnly`. Use the **Get Row** K2 node (from `UK2Node_DataIndexerGetRow`) for the recommended typed retrieval experience.

---

## Rows Handle nodes

### Get Rows Handle Keys

```
GetRowsHandleKeys(Handle, Query) → TArray<FDataIndexerPrimaryKey>  [CustomThunk, BlueprintInternalUseOnly]
```

Returns all primary keys matching the rows handle's repository and the wildcard `Query` struct. The `Query` pin must be wired to the concrete index query struct type registered for the schema.

!!! note
    This node is marked `BlueprintInternalUseOnly`. Used internally by K2 nodes when resolving `FDataIndexerKeysHandle`.

---

### Get All Primary Keys

```
GetAllPrimaryKeys(Repository) → TArray<FDataIndexerPrimaryKey>
```

Returns every primary key visible through the repository (including parent repositories). Useful for populating UI lists or iterating all rows in Blueprint.
