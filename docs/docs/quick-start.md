---
title: Quick Start
---

# Quick Start

A complete walkthrough — from creating a schema and repository to authoring rows and querying them at runtime.

<div class="qs-prereq">
  <p class="qs-prereq-title">Prerequisites</p>
  <div class="qs-prereq-grid">
    <div class="qs-prereq-row">
      <span class="qs-prereq-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Unreal Engine</strong><span class="qs-prereq-meta">5.7+</span></span>
    </div>
    <div class="qs-prereq-row">
      <span class="qs-prereq-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Plugin enabled</strong><span class="qs-prereq-meta">→ <a href="https://dev.epicgames.com/documentation/unreal-engine/working-with-plugins-in-unreal-engine">(Official docs)</a></span></span>
    </div>
    <div class="qs-prereq-row">
      <span class="qs-prereq-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Blueprint or C++</strong><span class="qs-prereq-meta">Either works</span></span>
    </div>
    <div class="qs-prereq-row">
      <span class="qs-prereq-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Time needed</strong><span class="qs-prereq-meta">~10 min</span></span>
    </div>
  </div>
</div>

<div class="qs-progress" role="navigation" aria-label="Progress">
  <div class="qs-progress-rail">
    <div class="qs-progress-bar-line"></div>
    <div class="qs-progress-step" data-step="1">
      <div class="qs-progress-dot">1</div>
      <div class="qs-progress-label">Schema</div>
    </div>
    <div class="qs-progress-step" data-step="2">
      <div class="qs-progress-dot">2</div>
      <div class="qs-progress-label">Repository</div>
    </div>
    <div class="qs-progress-step" data-step="3">
      <div class="qs-progress-dot">3</div>
      <div class="qs-progress-label">Query</div>
    </div>
    <div class="qs-progress-step" data-step="finish">
      <div class="qs-progress-dot">✓</div>
      <div class="qs-progress-label">Done</div>
    </div>
  </div>
</div>

## Create a Struct and Schema { #step-1 .step }

=== ":material-puzzle: Editor & Blueprint"

    **Create a Blueprint Struct**{ .step-label }

    Define the shape of a single row as a Blueprint struct in the Content Browser.

    ![Creating a Blueprint struct for row data](assets/images/bp-struct-creation.png)

    1. Right-click in the **Content Browser** → **Blueprints → Structure**
    2. Name it (e.g., `S_ItemRow`) and double-click to open
    3. Add a variable for each data field — `DisplayName` (Text), `Type` (Enum), `BaseValue` (Integer), etc.

    **Create a Schema Blueprint**{ .step-label }

    A Schema Blueprint links your struct to a repository and controls editor behavior.

    1. Right-click → **Blueprint Class**, search for `DataIndexerSchema`, select it
    2. Name it (e.g., `BP_ItemSchema`) and open it
    3. In **Class Defaults → Row Struct**, select your struct (`S_ItemRow`)

    Optionally implement **Get Row Display Name** to show a readable label per row in the Data View:

    ![GetRowDisplayName implementation](assets/images/schema-get-row-display-name.png)

    !!! note
        This logic handles the per-row display equivalent of DataTable's RowName. Rows are automatically renamed by referencing this function, keeping labels always in sync.

=== ":material-language-cpp: C++"

    **Define a Row Struct**{ .step-label }

    Declare the struct that holds row data and a type alias for the native schema interface:

    ```cpp title="ItemTypes.h"
    #pragma once
    #include "DataIndexerSchemaInterface.h"
    #include "ItemTypes.generated.h"

    UENUM(BlueprintType)
    enum class EItemType : uint8
    {
        Weapon, Armor, Accessory, Material,
    };

    UENUM(BlueprintType)
    enum class EItemRarity : uint8
    {
        Common, Uncommon, Rare, Epic, Legendary,
    };

    USTRUCT(BlueprintType)
    struct FItemRow
    {
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FText DisplayName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        EItemType Type = EItemType::Weapon;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        EItemRarity Rarity = EItemRarity::Common;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int32 BaseValue = 0;
    };

    using FItemInterface = DataIndexer::TNativeSchemaInterface<FItemRow>;
    ```

    **Implement a C++ Schema**{ .step-label }

    Set `RowStruct` and implement `GetRowDisplayName`:

    ```cpp title="ItemSchema.h"
    #pragma once
    #include "DataIndexerSchema.h"
    #include "ItemSchema.generated.h"

    UCLASS()
    class UItemSchema : public UDataIndexerSchema
    {
        GENERATED_BODY()

    public:
        UItemSchema();

    protected:
        virtual FText GetRowDisplayName_Implementation(
            const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity) const override;
    };
    ```

    ```cpp title="ItemSchema.cpp"
    #include "ItemSchema.h"
    #include "ItemTypes.h"

    UItemSchema::UItemSchema()
    {
        RowStruct = FItemRow::StaticStruct();
    }

    FText UItemSchema::GetRowDisplayName_Implementation(
        const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity) const
    {
        if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
        {
            return Row->DisplayName;
        }

        return Super::GetRowDisplayName_Implementation(PrimaryKey, RowEntity);
    }
    ```

<div class="qs-complete">
  <p class="qs-complete-title">
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" stroke="currentColor" stroke-width="2"><circle cx="9" cy="9" r="8"/><path d="m5 9 3 3 5-6"/></svg>
    What you have now
  </p>
  <ul class="qs-complete-list">
    <li>
      <span class="qs-complete-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Row type defined</strong> — struct for row data is declared</span>
    </li>
    <li>
      <span class="qs-complete-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Schema configured</strong> — schema is linked to the row struct</span>
    </li>
    <li>
      <span class="qs-complete-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Display name logic</strong> — <code>GetRowDisplayName</code> is implemented (optional)</span>
    </li>
  </ul>
</div>

---

## Create a Repository and Author Rows { #step-2 .step }

**Create a Repository**{ .step-label }

1. Right-click → **Miscellaneous → DataIndexer** and select the Repository asset type
2. In the Pick Class Dialog, select your schema (e.g., `BP_ItemSchema` or `UItemSchema`)
3. Name it (e.g., `DI_Items`) and open it

!!! note "Changing the schema"
    The bound schema **can be changed** as long as the Row Struct matches. If the Row Struct differs, migrate via JSON Export / Import.

**Author Rows**{ .step-label }

Double-click the repository asset to open the Data View.

![Adding a row with the Insert key](assets/images/data-view-add-row.png)

- Press **Insert** to add a row — a GUID primary key is generated automatically
- Edit values inline in the grid, or select a row to use the **Selection Details** panel on the right
- **Save** — reverse lookup tables rebuild automatically

For a full breakdown of the editor, see the [Editor Guide](editor-guide/index.md).

<div class="qs-complete">
  <p class="qs-complete-title">
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" stroke="currentColor" stroke-width="2"><circle cx="9" cy="9" r="8"/><path d="m5 9 3 3 5-6"/></svg>
    What you have now
  </p>
  <ul class="qs-complete-list">
    <li>
      <span class="qs-complete-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Repository asset</strong> — created and bound to the schema</span>
    </li>
    <li>
      <span class="qs-complete-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Row data</strong> — at least one row saved in the Data View</span>
    </li>
  </ul>
</div>

---

## Query Examples { #step-3 .step }

=== ":material-puzzle: Editor & Blueprint"

    **Reference a specific row**{ .step-label }

    Spawn a **Get Row** node directly in the graph:

    1. Right-click in the graph → search for **Get Row** and add it
    2. Set **Handle Repository** to the repository you created
    3. Select a row via the **Handle Primary Key** ComboBox
    4. Wire the **Out Row** output pin into your logic

    ![Get Row node example](assets/images/get-row-node.png)

    **Iterate all rows**{ .step-label }

    Pass a repository to **Get All Primary Keys** to retrieve all primary keys, then loop with **For Each Loop** and call **Get Row** for each key.

    ![Node graph for iterating all rows](assets/images/iterate-all-rows.png){ .qs-zoomable }

    For index-based filtering, see [Blueprint API → Function Library](api-reference/function-library.md).

=== ":material-language-cpp: C++"

    Use `FItemInterface` (alias for `TNativeSchemaInterface<FItemRow>`).

    **Reference a specific row**{ .step-label }

    ```cpp title="Example"
    if (const FItemRow* Row = FItemInterface::FindRow(Repository, PrimaryKey))
    {
        FText Name = Row->DisplayName;
        // ...
    }
    ```

    **Iterate all rows**{ .step-label }

    ```cpp title="Example"
    for (const FDataIndexerPrimaryKey& Key : FItemInterface::GetAllPrimaryKeys(Repository))
    {
        if (const FItemRow* Row = FItemInterface::FindRow(Repository, Key))
        {
            // ...
        }
    }
    ```

<div class="qs-complete">
  <p class="qs-complete-title">
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" stroke="currentColor" stroke-width="2"><circle cx="9" cy="9" r="8"/><path d="m5 9 3 3 5-6"/></svg>
    What you have now
  </p>
  <ul class="qs-complete-list">
    <li>
      <span class="qs-complete-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Basic query</strong> — retrieving a row from Blueprint or C++</span>
    </li>
    <li>
      <span class="qs-complete-check"><svg width="10" height="10" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m3 8 3 3 7-7"/></svg></span>
      <span><strong>Full iteration</strong> — looping over all rows and processing them</span>
    </li>
  </ul>
</div>

---

## Next steps

<div class="grid cards" markdown>

- :material-graph-outline:{ .lg .middle } &nbsp; **[Core Concepts](concepts/index.md)**

    ---

    Understand the relationship between Repository, Schema, Keys, and Indexes.

- :material-puzzle-outline:{ .lg .middle } &nbsp; **[Blueprint API](api-reference/index.md)**

    ---

    Full reference for Blueprint nodes, the function library, and Driven Collection.

- :material-language-cpp:{ .lg .middle } &nbsp; **[C++ API](api-reference/index.md)**

    ---

    Type-safe query patterns and the Native Schema Interface.

- :material-table-edit:{ .lg .middle } &nbsp; **[Editor Guide](editor-guide/index.md)**

    ---

    Data View, JSON import/export, and custom widgets.

</div>

<div class="qs-lightbox" id="qs-lightbox" onclick="qsCloseLightbox()">
  <button class="qs-lightbox-close" onclick="qsCloseLightbox()" aria-label="Close">&#x2715;</button>
  <img class="qs-lightbox-img" src="" alt="Enlarged view" onclick="event.stopPropagation()">
</div>
<script>
function qsOpenLightbox(src) {
  var lb = document.getElementById('qs-lightbox');
  lb.querySelector('.qs-lightbox-img').src = src;
  lb.classList.add('is-open');
  document.body.style.overflow = 'hidden';
}
function qsCloseLightbox() {
  var lb = document.getElementById('qs-lightbox');
  if (!lb) return;
  lb.classList.remove('is-open');
  document.body.style.overflow = '';
}
document.addEventListener('DOMContentLoaded', function() {
  document.querySelectorAll('img.qs-zoomable').forEach(function(img) {
    img.addEventListener('click', function() { qsOpenLightbox(img.src); });
  });
  document.addEventListener('keydown', function(e) {
    if (e.key === 'Escape') qsCloseLightbox();
  });
});
</script>
