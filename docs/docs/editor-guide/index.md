# Editor Guide

The `DataIndexerEd` module adds a custom editor workflow on top of the runtime asset types. This section covers how to use the editor to author and manage data.

!!! note "Editor-only"
    Everything in this section requires Unreal Editor. The `DataIndexerEd` module is declared `UncookedOnly` and is not available in packaged builds.

## Workflow overview

```
1. Create Schema Blueprint
        ↓
2. Create Repository Asset
        ↓
3. Bind Schema to Repository
        ↓
4. Open Repository in custom editor
        ↓
5. Author rows in the Data View
        ↓
6. Save → ReverseLookups rebuilt automatically
```

## Pages in this section

[**Asset Creation**](asset-creation.md)
: How to create Schema Blueprints and Repository data assets, and how to bind them together.

[**Data View**](data-view.md)
: The three-panel custom editor, row authoring, and column layout configuration.

[**JSON Import & Export**](json-import-export.md)
: Export rows to JSON for diff workflows, and import JSON back into a repository.

[**Editor Settings**](editor-settings.md)
: Project-level editor settings that control the Data View behavior.
