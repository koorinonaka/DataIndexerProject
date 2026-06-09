# Custom Cell Widgets

The Data View renders each row property as a cell. A schema can replace the default cell widget to
show a custom display or an inline editor. There are two ways to do this, demonstrated side by side on
`FCharacterRow` — both **editable**, but reaching write-back through different mechanisms:

| Column | Approach | Write-back mechanism | Where it lives |
| --- | --- | --- | --- |
| `PawnClass1` | C++ `CreateAsEditInline` | C++ context — edits the row's memory directly | `UCharacterSchema` C++ override |
| `PawnClass2` | Blueprint `PropertyWidgetCustomizations` + a UserWidget hosting `USinglePropertyView` | Blueprint `UpdateRow` — via injected repository context | `DIS_Character` Blueprint + `WBP_PawnClassCell`, no schema C++ |

Both customize the same entry point — `CustomizePropertyCellWidget` — but reach it differently. Both
persist edits through the same editor staging layer (`UDataIndexerEditorData`), so changes are
transactional and undoable.

## The entry point

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
  context accessors.

---

## Sample 1 — inline editor in C++ (`PawnClass1`)

Override `CustomizePropertyCellWidget` in a C++ schema and return `CreateAsEditInline` for the target
column. `CreateAsEditInline` builds a real property editor over the row's memory, so edits persist.

```cpp title="CharacterSchema.cpp"
TSharedRef<SWidget> UCharacterSchema::CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const
{
    if (const FName Col = GET_MEMBER_NAME_CHECKED(FCharacterRow, PawnClass1);
        Context.GetColumnName() == Col)
    {
        return Context.CreateAsEditInline(Context.GetProperty(), /*bDisplayDefaultPropertyButtons=*/true);
    }

    return Super::CustomizePropertyCellWidget(Context);
}
```

`PawnClass1` is a `TSubclassOf<APawn>` — a single object/class property — so the cell shows an inline
class picker. `CreateAsEditInline` supports any single property and any struct that has a registered
`IPropertyTypeCustomization`; it rejects container properties (`TArray`/`TMap`/`TSet`).

`bDisplayDefaultPropertyButtons` (default `true`) appends the standard property buttons —
reset-to-default, browse, use-selected — next to the value editor, matching a Details-panel row. Pass
`false` for a bare value-only editor. (Internally the cell uses
`CreatePropertyValueWidgetWithCustomization`, which preserves type customizations but omits those
buttons, then re-adds them via `CreateDefaultPropertyButtonWidgets`.)

!!! note
    The C++ override on `UCharacterSchema` also applies to its Blueprint subclass `DIS_Character` through
    normal virtual dispatch — you do not need to touch the Blueprint for Sample 1.

---

## Sample 2 — editable UserWidget via `PropertyWidgetCustomizations` (`PawnClass2`)

This path needs no C++ in the schema. A Blueprint function returns a `UserWidget` for the cell, bound
through the schema's `PropertyWidgetCustomizations` map. The widget hosts a `USinglePropertyView` and
writes edits back to the row with `UpdateRow`.

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

### Step 1 — the Editor Utility Widget (`WBP_PawnClassCell`)

1. Create an Editor Utility Widget Blueprint at `/Game/GameData/CustomWidget/WBP_PawnClassCell`
   (parent `EditorUtilityWidget`).
2. Place a **Single Property View** as the root.
3. Add variables:
    - `PawnClass : TSubclassOf<APawn>` — the displayed/edited value. Instance Editable + **Expose on Spawn**.
    - `Row : FCharacterRow` — the full row, cached for write-back. Instance Editable + **Expose on Spawn**.
    - `Repository : UDataIndexerRepository` and `PrimaryKey : FDataIndexerPrimaryKey` — set via the interface.
4. **Class Settings → Interfaces** — implement `IDataIndexerInterface_CellContext`.

### Step 2 — initialize on `SetCellContext`

`SetCellContext` fires *after* the binding function has set `PawnClass` / `Row`, so it is the right
place to bind the view to the live value (doing this in `Pre Construct` can race the spawn-time values):

- Store `Repository` and `PrimaryKey`.
- `Single Property View → Set Object (Self)`, then `Set Property Name ("PawnClass")`.
- `Bind Event to On Property Changed` → a custom handler event.

### Step 3 — write back on edit

In the `On Property Changed` handler:

1. Rebuild the cached `Row` with its `PawnClass2` replaced by the edited `PawnClass` (Break + Make
   `CharacterRow`).
2. Call `Update Row (Repository, PrimaryKey, Row)`
   (`UDataIndexerEditorFunctionLibrary::UpdateRow`). It writes through `UDataIndexerEditorData` inside a
   transaction, so the edit is undoable — exactly like the Sample 1 inline editor.

### Step 4 — the binding function on `DIS_Character`

Add a function with the cell-widget signature
`(const FDataIndexerPrimaryKey&, const FCharacterRow&) → UUserWidget*`:

1. `Create Widget` of class `WBP_PawnClassCell`, feeding the row's `PawnClass2` into the widget's
   `PawnClass` pin and the whole row into the `Row` pin (both exposed on spawn).
2. Return the widget.

This mirrors the existing `GetPropertyWidget` function that returns `EUW_CharacterClassIcon`.

### Step 5 — bind the column

In `DIS_Character` Class Defaults → **Property Widget Customizations**, add an entry: key `PawnClass2`,
function `GetPawnClass2Widget`. The `PawnClass2` cell now renders an editable `SinglePropertyView`, and
edits persist via `UpdateRow`.

!!! note "Why the two cells look different"
    `PawnClass1` shows only the class-picker value widget (plus the default property buttons).
    `PawnClass2` shows a property-name label to its left because `USinglePropertyView` renders a complete
    single-property row — name **and** value — rather than just the value widget.
