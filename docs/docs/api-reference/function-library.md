# Function Library

`UDataIndexerFunctionLibrary` is a `UBlueprintFunctionLibrary` that provides utility nodes for working with DataIndexer types in Blueprint graphs. All nodes are in the **DataIndexer** category.

---

## Get All Primary Keys

```
GetAllPrimaryKeys(Repository) → TArray<FDataIndexerPrimaryKey>
```

![Get All Primary Keys node](../assets/images/thumbnails/fn-get-all-primary-keys.png){ .fn-node }

Returns every primary key visible through the repository (including parent repositories). Useful for populating UI lists or iterating all rows in Blueprint.

---

## Primary Key nodes

### Is Valid Primary Key

```
IsValidPrimaryKey(PrimaryKey) → bool
```

![Is Valid Primary Key node](../assets/images/thumbnails/fn-is-valid-primary-key.png){ .fn-node }

Returns `true` if the primary key is non-zero (i.e., `FGuid::IsValid()`).

---

### Equal / Not Equal

```
EqualEqual_PrimaryKeyPrimaryKey(A, B) → bool
NotEqual_PrimaryKeyPrimaryKey(A, B) → bool
```

![Equal / Not Equal nodes](../assets/images/thumbnails/fn-equal-not-equal.png){ .fn-node }

Compact node titles `==` and `!=`. `BlueprintThreadSafe`.

Compare two `FDataIndexerPrimaryKey` values for equality. Delegates to `FGuid::operator==`.

---

### To String

```
Conv_PrimaryKeyToString(PrimaryKey) → FString
```

![To String node](../assets/images/thumbnails/fn-to-string.png){ .fn-node }

Compact node title `→`. `BlueprintAutocast`, `BlueprintThreadSafe`.

Converts a primary key to its GUID string representation (e.g., `"A1B2C3D4-..."`).

---

## Row Handle nodes

### Is Valid Row Handle

```
IsValidRowHandle(RowHandle) → bool
```

![Is Valid Row Handle node](../assets/images/thumbnails/fn-is-valid-row-handle.png){ .fn-node }

Returns `true` if both `RowHandle.Repository` and `RowHandle.PrimaryKey` are valid.

---

### Get Row Handle Name

```
GetRowHandleName(RowHandle, OutDisplayName) → bool
```

![Get Row Handle Name node](../assets/images/thumbnails/fn-get-row-handle-name.png){ .fn-node }

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

### Get Rows Handle Value

```
GetRowsHandleValue(Handle, Query, OutKeys) → bool  [CustomThunk, BlueprintInternalUseOnly]
```

Writes all primary keys matching the wildcard `Query` struct into `OutKeys`. The `Query` pin must be wired to the concrete index query struct type registered for the schema. Returns `true` if one or more keys match.

!!! note
    This node is marked `BlueprintInternalUseOnly`. Used internally by K2 nodes when resolving `FDataIndexerKeysHandle`.

---

