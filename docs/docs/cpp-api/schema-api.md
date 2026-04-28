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
UFUNCTION(BlueprintNativeEvent)
FText GetRowDisplayName(
    const FDataIndexerPrimaryKey& PrimaryKey,
    const FInstancedStruct& RowEntity) const;
```

Returns a human-readable display name for a row. `BlueprintNativeEvent` — override `GetRowDisplayName_Implementation` in C++:

```cpp
FText UMyItemSchema::GetRowDisplayName_Implementation(
    const FDataIndexerPrimaryKey& PrimaryKey,
    const FInstancedStruct& RowEntity) const
{
    if (const FMyItemRow* Row = RowEntity.GetPtr<const FMyItemRow>())
    {
        return Row->DisplayName;
    }
    return Super::GetRowDisplayName_Implementation(PrimaryKey, RowEntity);
}
```

---

## ForEachIndex

```cpp
void ForEachIndex(
    const TFunctionRef<void(const FDataIndexerImmutableKey&)>& Callback) const;
```

Iterates all registered index keys (the keys of `BuildIndexKeyFunctions`). Used by the compiler to enumerate indexes when building `ReverseLookups`.

---

## BuildIndexKeyCall

```cpp
using FIndexResult = TPair<FDataIndexerIndexKey, FText>;
TOptional<FIndexResult> BuildIndexKeyCall(
    const FDataIndexerIndex& Index,
    const FConstStructView& RowEntity) const;
```

Calls the registered builder function for `Index` with the given row entity. Returns the `(IndexKey, DisplayName)` pair, or `NullOpt` if no builder is registered for this index.

Called by the compiler at save time. Not typically called directly from game code.

---

## RegisterFunction_BuildIndexKey

```cpp
void RegisterFunction_BuildIndexKey(
    const FDataIndexerIndex& Index,
    FName FunctionName,
    const FString& Description = FString());
```

Registers a named function as the index key builder for `Index`. Call this in `PostInitProperties` from the CDO:

```cpp
void UMyItemSchema::PostInitProperties()
{
    Super::PostInitProperties();
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        RegisterFunction_BuildIndexKey(
            FDataIndexerIndex(CategoryIndex.Identifier),
            GET_FUNCTION_NAME_CHECKED(UMyItemSchema, BuildCategoryKey));
    }
}
```

The function must match the `Prototype_BuildIndexKey` signature:
`(const FInstancedStruct& RowEntity, FText& OutDisplayName) → FDataIndexerIndexKey`

---

## RegisterFunction_CustomizePropertyText

```cpp
void RegisterFunction_CustomizePropertyText(
    FName PropertyName,
    FName FunctionName);
```

**(Editor-only)** Registers a function that provides custom text rendering for a specific property column in the Data View grid.

The function must match the `Prototype_CustomizePropertyText` signature:
`(const FInstancedStruct& RowEntity) → FText`

---

## CustomizePropertyCellWidget

```cpp
virtual TSharedRef<SWidget> CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const;
```

**(Editor-only)** Override to provide a custom Slate widget for a cell in the Data View. The base implementation calls `CustomizePropertyWidgetCall` for registered widget customizations, falling back to the default property widget.

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
