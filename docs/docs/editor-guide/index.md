---
hide:
  - navigation
---

# Editor Guide

The `DataIndexerEd` module adds a custom editing workflow on top of the runtime asset types. Use the editor to define schemas, create repositories, and author row data — no code required.

!!! note "Editor-only"
    Everything in this section requires Unreal Editor. The `DataIndexerEd` module is declared `UncookedOnly` and is not available in packaged builds.

## Workflow overview

Setting up a new data table follows this sequence:

1. **Create a Blueprint struct** — defines the shape of a single row
2. **Create a Schema Blueprint** — `DataIndexerSchema` subclass, binds to the struct from step 1
3. **Create a Repository asset** — `DataIndexerRepository`
4. **Set Schema Class on the Repository**
5. **Double-click the Repository** — opens the Data View
6. **Press Insert to add rows** — edit inline or via the Selection Details panel
7. **Save** — reverse lookup tables rebuild automatically

## Pages in this section

<div class="grid cards" markdown>

- :material-folder-plus:{ .lg .middle } &nbsp; **[Asset Creation](asset-creation.md)**

    ---

    Create the Blueprint struct and Schema Blueprint, create the Repository asset, and bind the schema. Also covers setting up parent repositories for inherited rows.

- :material-table-eye:{ .lg .middle } &nbsp; **[Data View](data-view.md)**

    ---

    The three-panel custom editor. Insert, edit, and delete rows; configure which columns appear; navigate between parent and child repositories.

- :material-code-json:{ .lg .middle } &nbsp; **[JSON Support](json-support.md)**

    ---

    Export row data to a diff-friendly JSON format for code review, and import JSON back as a merge operation.

- :material-layers-triple:{ .lg .middle } &nbsp; **[Driven Collection](driven-collection.md)**

    ---

    `UDataIndexerDrivenCollection` — C++ base class for editor assets that manage per-key sub-assets (icons, ability classes, etc.) keyed by `FDataIndexerPrimaryKey`.

</div>
