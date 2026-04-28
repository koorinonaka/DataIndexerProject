# DataIndexer

DataIndexer is an Unreal Engine plugin for building structured, indexable game data assets with a dedicated editor workflow.

It sits between a traditional DataTable and a fully custom data tool: designers get asset-based authoring, schema-driven validation, indexed lookup paths, and editor utilities such as JSON import/export and text-based diff support.

---

## Why DataIndexer

Large game projects usually need more than a flat table. DataIndexer is built for cases where you want:

- **Asset-based authoring** instead of spreadsheet-only workflows
- **Stable primary keys** and index-based lookups that survive asset moves and renames
- **Schema-specific display names** and custom editor presentation per data type
- **Repository composition** through parent repositories for shared base data
- **Diff-friendly workflows** with JSON sidecar export for version control

---

## Core Concepts at a Glance

<div class="grid cards" markdown>

- **Repository**

    The data asset that holds rows. Each row is identified by a `FDataIndexerPrimaryKey` (GUID) and stores a typed struct via `FInstancedStruct`. Repositories can reference parent repositories for hierarchical data composition.

    [:octicons-arrow-right-24: Repository](concepts/repository.md)

- **Schema**

    Defines the row struct and editor behavior. A schema controls display names, property text rendering, custom widgets, and the index key builders that power secondary lookups.

    [:octicons-arrow-right-24: Schema](concepts/schema.md)

- **Keys & Handles**

    `FDataIndexerPrimaryKey` uniquely identifies a single row. `FDataIndexerRowHandle` pairs a repository with a key for Blueprint-friendly references. `FDataIndexerRowsHandle` references a set of rows via a secondary index.

    [:octicons-arrow-right-24: Keys & Handles](concepts/keys-and-handles.md)

- **Indexes**

    Secondary lookup dimensions defined on a schema. Each index maps rows to a `FDataIndexerIndexKey`, enabling fast lookup by any domain-specific attribute (e.g., category, faction, rarity).

    [:octicons-arrow-right-24: Indexes](concepts/indexes.md)

</div>

---

## Intended Use Cases

DataIndexer is a good fit for data sets such as:

- Item definitions and equipment catalogs
- Skill or ability tables
- NPC metadata and dialogue lookup
- Quest and mission definitions
- Localized design data that benefits from asset workflows
- Any data set that needs both designer-friendly editing and code-friendly lookup

---

## Get Started

<div class="grid cards" markdown>

- [:octicons-download-24: **Installation**](installation.md)

    Add the plugin to your project and get the modules building.

- [:octicons-rocket-24: **Quick Start**](quick-start.md)

    Create a schema, a repository, author rows, and query them from C++ and Blueprint in under ten minutes.

</div>
