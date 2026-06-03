# Schema API

`UDataIndexerSchema` (`DataIndexerSchema.h`) is the abstract base class for all schemas. Subclass it in C++ to define the row struct, display logic, and index key builders for a repository.

## GetRowStruct

```cpp
const UScriptStruct& GetRowStruct() const;
```

Returns the `UScriptStruct` that defines the row shape. Set automatically from `RowStruct` UPROPERTY on the CDO; in C++ subclasses call `Super::PostInitProperties()` to ensure it is initialized.

---

## GetRowDisplayName

```cpp
virtual TOptional<FText> GetRowDisplayName(
    const FDataIndexerPrimaryKey& PrimaryKey,
    const FConstStructView& RowEntity) const;
```

Returns a human-readable display name for a row, or `NullOpt` when none is available. The base implementation resolves the `RowDisplayNameFunction` binding (a function that takes the concrete row struct directly). Override the `virtual` in C++ to compute it natively — unpack `RowEntity` to the concrete row struct and return the field:

```cpp
TOptional<FText> UItemSchema::GetRowDisplayName(
    const FDataIndexerPrimaryKey& PrimaryKey,
    const FConstStructView& RowEntity) const
{
    return RowEntity.Get<const FItemRow>().DisplayName;
}
```

From Blueprint, bind `RowDisplayNameFunction` in the Schema's Class Defaults to a function whose signature is `(const FDataIndexerPrimaryKey&, const FRowStruct&) → FText` — the row arrives as the concrete row struct, so no `Get Instanced Struct Value` unpacking node is needed.

---

## ForEachIndex

```cpp
void ForEachIndex(
    const TFunctionRef<void(const FDataIndexerIndex&)>& Callback) const;
```

Iterates all registered index keys (the keys of `BuildIndexFunctions`). Used by the compiler to enumerate indexes when building `ReverseLookups`.

---

## BuildIndexCall

```cpp
TOptional<FGuid> BuildIndexCall(
    const FDataIndexerIndexKey& IndexKey,
    const FConstStructView& RowEntity) const;
```

Calls the registered builder function for `IndexKey` with the given row entity. Returns the computed index key (`FGuid`), or `NullOpt` if no builder is registered for this index.

Called by the compiler at save time. Not typically called directly from game code.

---

## RegisterFunction_BuildIndex

```cpp
void RegisterFunction_BuildIndex(
    const FDataIndexerIndex& Index,
    FName FunctionName);
```

Registers a named function as the index key builder for `Index`. Call this in `PostInitProperties` from the CDO:

```cpp
void UItemSchema::PostInitProperties()
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        RowStruct = FItemRow::StaticStruct();
        RegisterFunction_BuildIndex(ByTypeIndex(),
            GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeIndex));
        RegisterFunction_BuildIndex(ByRarityIndex(),
            GET_FUNCTION_NAME_CHECKED(ThisClass, BuildRarityIndex));
    }
    Super::PostInitProperties();
}
```

The function takes the concrete row struct directly and returns the index key:
`(const FRowStruct& Row) → FGuid`

When binding from Blueprint, the Schema details panel filters candidate functions to this signature and "Create matching function" generates a stub with a `const FRowStruct&` parameter — the row is already the concrete struct, so no `Get Instanced Struct Value` node is needed.

---

## CustomizePropertyCellWidget

```cpp
virtual TSharedRef<SWidget> CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const;
```

**(Editor-only)** Override to provide a custom Slate widget for a cell in the Data View. The base implementation calls `CustomizePropertyWidgetCall` for registered Blueprint widget customizations, falling back to a simple text widget. `Context` exposes building blocks an override can reuse: `CreateAsSimpleText()`, `CreateAsEditInline()`, and `WrapUserWidget()`.

---

## Data validation

```cpp
#if WITH_EDITOR
virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
virtual EDataValidationResult IsRowValid(
    FConstStructView RowEntity, FDataValidationContext& Context) const;
#endif
```

`IsRowValid` is called for each row during **Validate Data** (via `UEditorValidatorSubsystem`). Override it to add per-row validation logic.
