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

## DI_REGISTER_BUILD_INDEX

```cpp
DI_REGISTER_BUILD_INDEX(Index, RowType, Func);
```

Registers `Func` as the index key builder for `Index`. The macro is provided by `DataIndexerSchema.h`. Call it in the schema constructor, once per index:

```cpp
UItemSchema::UItemSchema()
{
    RowStruct = FItemRow::StaticStruct();
    DI_REGISTER_BUILD_INDEX(ByTypeIndex(),   FItemRow, BuildTypeIndex);
    DI_REGISTER_BUILD_INDEX(ByRarityIndex(), FItemRow, BuildRarityIndex);
}
```

`Func` must be a `static UFUNCTION` on the schema class whose signature takes the concrete row struct directly and returns the index key:
`(const FRowStruct& Row) → FGuid`

The macro validates this signature at **compile time** via `static_assert`: the return type must be `FGuid`, the row must be passed by `const` reference, and the function's row type must match the `RowType` argument. A mismatch is a compile error, not a deferred editor-validation warning.

!!! warning "Deprecated overload"
    The earlier `void RegisterFunction_BuildIndex(const FDataIndexerIndex&, FName)` overload is deprecated. It still binds by name but performs no compile-time signature checking. Migrate to `DI_REGISTER_BUILD_INDEX`.

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
