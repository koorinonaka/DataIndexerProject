# Custom Cell Widgets

The Data View renders each row property as a cell. A schema can replace the default cell widget to
show a custom display or an inline editor. It can also declare **virtual columns** — extra columns that
alias an existing property under a distinct name — so the *same* value can appear several times with
different presentations.

`FCharacterRow` demonstrates both ideas at once. It has a single `PawnClass` field
(`TSubclassOf<APawn>`), and `UCharacterSchema` surfaces it as **two virtual columns**, each **editable**
but reaching write-back through a different mechanism:

| Column | Approach | Write-back mechanism | Where it's declared |
| --- | --- | --- | --- |
| `PawnClassInline` | C++ `CreateAsEditInline` | C++ context — edits the row's memory directly | `UCharacterSchema` C++ |
| `PawnClassWidget` | Blueprint `PropertyWidgetCustomizations` + a UserWidget hosting `USinglePropertyView` | Blueprint `UpdateRow` — via injected repository context | `DIS_Character` Blueprint + `WBP_PawnClassCell` |

Both columns alias the **same** `PawnClass` property, so editing either one updates the field the other
displays. Both persist edits through the same editor staging layer (`UDataIndexerEditorData`), so changes
are transactional and undoable. The raw `PawnClass` property itself is hidden from the Data View — only
the two virtual columns are shown.

---

## Virtual columns

The Data View normally derives one column per checked `FProperty` on the row struct. A schema can append
**virtual columns** that render under a custom `ColumnName`. Each is a `FDataIndexerVirtualColumn`:

```cpp
USTRUCT()
struct FDataIndexerVirtualColumn
{
    FName ColumnName;     // stable, unique column identity (layout/visibility key, routing key)
    FText DisplayName;    // header label; falls back to ColumnName when empty
    FName SourceProperty; // optional aliased RowStruct member; dotted for nested ("Inner.A"); empty = unbound
};
```

Schemas expose them through a `VirtualColumns` array editable in the Schema Blueprint Class Defaults
panel — or set from C++. Virtual columns are appended **after** the property-derived columns. There are
two flavours, decided by `SourceProperty`:

- **Alias column** (`SourceProperty` set) — aliases that RowStruct member, reusing its `FProperty` so the
  cell, write-back, and grid value semantics (copy/paste, fill, default display) all work through the
  existing path; only `ColumnName` and `DisplayName` differ. A dotted `SourceProperty` (`"Inner.A"`)
  addresses a nested struct member.
- **Unbound column** (`SourceProperty` empty) — a "true" virtual column with no backing property. The
  schema supplies its cell entirely via `CustomizePropertyCellWidget` (a read-only/computed display or a
  custom-binding widget). It has **no** grid value semantics — copy/paste/fill/default-display do not apply.

A virtual column whose (non-empty) `SourceProperty` does not resolve, or whose `ColumnName` collides with
an existing column, is skipped with a warning.

!!! note "Hiding the raw column"
    To show a property *only* through virtual columns, remove it from the row struct's expanded set in
    `InitializeExpandedStructEntries` (same pattern used to hide `DisplayName`):

    ```cpp
    if (FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find(RowStruct))
    {
        *RowStructEntry -= {
            GET_MEMBER_NAME_CHECKED(FCharacterRow, DisplayName),
            GET_MEMBER_NAME_CHECKED(FCharacterRow, PawnClass),
        };
    }
    ```

---

## The cell entry point

```cpp
virtual TSharedRef<SWidget> CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const;
```

The base implementation looks up a Blueprint `PropertyWidgetCustomizations` binding for the column; if
none is found it falls back to a simple text widget. `Context` (`IPropertyWidgetContext`) exposes the
building blocks an override reuses:

- `CreateAsSimpleText(Text)` — a read-only text label.
- `CreateAsEditInline(Property, bDisplayDefaultPropertyButtons = true)` — a live inline property editor
  wired to the row, with write-back.
- `WrapUserWidget(UserWidget)` — hosts a UMG `UserWidget` in the cell.
- `GetColumnName()`, `GetProperty()`, `GetRow<FRowType>()`, `GetPrimaryKey()`, `GetRepository()` —
  context accessors. For a virtual column, `GetProperty()` resolves to the aliased `SourceProperty`.

---

## Sample 1 — inline editor in C++ (`PawnClassInline`)

Declare the virtual column in the C++ schema constructor, then return `CreateAsEditInline` for it in
`CustomizePropertyCellWidget`. Because the cell's property resolves to the aliased `PawnClass`,
`CreateAsEditInline` builds a real property editor over the row's memory, so edits persist.

```cpp title="CharacterSchema.cpp"
namespace
{
const FName PawnClassInlineColumn(TEXT("PawnClassInline"));
}

UCharacterSchema::UCharacterSchema()
{
    // ...
#if WITH_EDITORONLY_DATA
    FDataIndexerVirtualColumn& InlineColumn = VirtualColumns.AddDefaulted_GetRef();
    InlineColumn.ColumnName = PawnClassInlineColumn;
    InlineColumn.DisplayName = NSLOCTEXT("CharacterSchema", "PawnClassInlineColumn", "Pawn (C++)");
    InlineColumn.SourceProperty = GET_MEMBER_NAME_CHECKED(FCharacterRow, PawnClass);
#endif
}

TSharedRef<SWidget> UCharacterSchema::CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const
{
    if (Context.GetColumnName() == PawnClassInlineColumn)
    {
        // GetProperty() resolves to the aliased PawnClass property, so the editor writes back to it.
        return Context.CreateAsEditInline(Context.GetProperty(), /*bDisplayDefaultPropertyButtons=*/true);
    }

    return Super::CustomizePropertyCellWidget(Context);
}
```

`PawnClass` is a `TSubclassOf<APawn>` — a single object/class property — so the cell shows an inline
class picker. `CreateAsEditInline` supports any single property and any struct that has a registered
`IPropertyTypeCustomization`; it rejects container properties (`TArray`/`TMap`/`TSet`).

`bDisplayDefaultPropertyButtons` (default `true`) appends the standard property buttons —
reset-to-default, browse, use-selected — next to the value editor, matching a Details-panel row. Pass
`false` for a bare value-only editor. (Internally the cell uses
`CreatePropertyValueWidgetWithCustomization`, which preserves type customizations but omits those
buttons, then re-adds them via `CreateDefaultPropertyButtonWidgets`.)

!!! note
    The C++ work on `UCharacterSchema` also applies to its Blueprint subclass `DIS_Character` through
    normal virtual dispatch — you do not need to touch the Blueprint for Sample 1.

---

## Sample 2 — editable UserWidget via `PropertyWidgetCustomizations` (`PawnClassWidget`)

This path needs no C++ in the schema. The virtual column is declared in `DIS_Character` Class Defaults,
and a Blueprint function returns a `UserWidget` for the cell, bound through the schema's
`PropertyWidgetCustomizations` map. The widget hosts a `USinglePropertyView` and writes edits back to the
row with `UpdateRow`.

!!! info "Why an Editor Utility Widget"
    `USinglePropertyView` lives in the editor-only `ScriptableEditorWidgets` module, so its `SetObject` /
    `SetPropertyName` functions are only callable from an **Editor Utility Widget**. Make
    `WBP_PawnClassCell`'s parent class `EditorUtilityWidget`, not `UserWidget` (it still returns fine as a
    `UUserWidget*`).

### How the cell gets its repository

The cell-customization function only receives `(PrimaryKey, Row)` — not the owning repository that
`UpdateRow` requires. The plugin bridges this: after the Data View builds the widget, it injects the
editor context through a Blueprint interface, `IDataIndexerInterface_CellContext`:

```cpp
// DataIndexer module — implement this on any editable cell widget.
void SetCellContext(UDataIndexerRepository* Repository, const FDataIndexerPrimaryKey& PrimaryKey);
```

`UDataIndexerSchema::CustomizePropertyCellWidget` calls `SetCellContext` on the freshly created widget
(using `Context.GetRepository()` and `Context.GetPrimaryKey()`) if it implements the interface — so any
cell widget can opt into write-back without changing the function signature.

### Step 1 — declare the virtual column

In `DIS_Character` Class Defaults → **Virtual Columns**, add an entry:

- `ColumnName` = `PawnClassWidget`
- `DisplayName` = `Pawn (BP)`
- `SourceProperty` = `PawnClass`

### Step 2 — the Editor Utility Widget (`WBP_PawnClassCell`)

1. Create an Editor Utility Widget Blueprint at `/Game/GameData/CustomWidget/WBP_PawnClassCell`
   (parent `EditorUtilityWidget`).
2. Place a **Single Property View** as the root.
3. Add variables:
    - `PawnClass : TSubclassOf<APawn>` — the displayed/edited value. Instance Editable + **Expose on Spawn**.
    - `Row : FCharacterRow` — the full row, cached for write-back. Instance Editable + **Expose on Spawn**.
    - `Repository : UDataIndexerRepository` and `PrimaryKey : FDataIndexerPrimaryKey` — set via the interface.
4. **Class Settings → Interfaces** — implement `IDataIndexerInterface_CellContext`.

### Step 3 — initialize on `SetCellContext`

`SetCellContext` fires *after* the binding function has set `PawnClass` / `Row`, so it is the right
place to bind the view to the live value (doing this in `Pre Construct` can race the spawn-time values):

- Store `Repository` and `PrimaryKey`.
- `Single Property View → Set Object (Self)`, then `Set Property Name ("PawnClass")`.
- `Bind Event to On Property Changed` → a custom handler event.

### Step 4 — write back on edit

In the `On Property Changed` handler:

1. Rebuild the cached `Row` with its `PawnClass` replaced by the edited `PawnClass` (Break + Make
   `CharacterRow`).
2. Call `Update Row (Repository, PrimaryKey, Row)`
   (`UDataIndexerEditorFunctionLibrary::UpdateRow`). It writes through `UDataIndexerEditorData` inside a
   transaction, so the edit is undoable — exactly like the Sample 1 inline editor.

### Step 5 — the binding function on `DIS_Character`

Add a function with the cell-widget signature
`(const FDataIndexerPrimaryKey&, const FCharacterRow&) → UUserWidget*`:

1. `Create Widget` of class `WBP_PawnClassCell`, feeding the row's `PawnClass` into the widget's
   `PawnClass` pin and the whole row into the `Row` pin (both exposed on spawn).
2. Return the widget.

### Step 6 — bind the column

In `DIS_Character` Class Defaults → **Property Widget Customizations**, add an entry: key
`PawnClassWidget` (the virtual column's `ColumnName`), function `GetPawnClassWidget`. The
`PawnClassWidget` cell now renders an editable `SinglePropertyView`, and edits persist via `UpdateRow`.

!!! note "Why the two cells look different"
    `PawnClassInline` shows only the class-picker value widget (plus the default property buttons).
    `PawnClassWidget` shows a property-name label to its left because `USinglePropertyView` renders a
    complete single-property row — name **and** value — rather than just the value widget.
