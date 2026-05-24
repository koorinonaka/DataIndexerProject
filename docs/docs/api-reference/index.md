---
hide:
  - navigation
---

# API Reference

DataIndexer exposes its runtime surface through two APIs — Blueprint nodes for designer-facing workflows and a typed C++ API for code-side integration.

---

## Blueprint API

DataIndexer exposes a Blueprint-friendly surface through three mechanisms:

| Mechanism | Description |
|-----------|-------------|
| `UDataIndexerFunctionLibrary` | Pure and callable utility nodes for key/handle operations |
| Custom K2 Nodes | Typed row retrieval and primary key dispatch |
| Struct types | `FDataIndexerRowHandle` and `FDataIndexerKeysHandle` as UPROPERTY variable types |

Most Blueprint nodes are runtime-safe and available in packaged builds. A small number of nodes (those wrapping editor subsystem calls) are editor-only and will not appear in runtime Blueprints.

<div class="grid cards" markdown>

- :material-function-variant:{ .lg .middle } &nbsp; **[Function Library](function-library.md)**

    ---

    Complete reference for all `UDataIndexerFunctionLibrary` nodes — equality, conversion, validation, and index key construction.

- :material-graph:{ .lg .middle } &nbsp; **[Custom K2 Nodes](k2-nodes.md)**

    ---

    The `GetRow`, `SelectPrimaryKey`, and `SwitchPrimaryKey` graph nodes that provide typed row access and key-based dispatch.

</div>

---

## C++ API

The `DataIndexer` runtime module exposes a typed C++ API for querying repositories, iterating rows, and building index key builders.

Add `DataIndexer` to your module's `.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "DataIndexer",
});
```

!!! warning "Do not reference DataIndexerEd"
    `DataIndexerEd` is `UncookedOnly`. Never add it to `PublicDependencyModuleNames` or `PrivateDependencyModuleNames` in a Runtime or game module. It will not be available in packaged builds and will cause build failures.

| Header | Contents |
|--------|----------|
| `DataIndexerTypes.h` | Core value types for row access: primary keys, row/keys handles, index definitions, and index key builders |
| `DataIndexerRepository.h` | Repository class for querying rows, iterating index keys, and resolving display names |
| `DataIndexerSchema.h` | Schema class for row struct access, index key builder registration, and property customization |
| `DataIndexerSchemaInterface.h` | Typed C++ façade for type-safe repository queries without casting |
| `DataIndexerDrivenCollection.h` | Data-driven collection that auto-updates when index contents change |
| `DataIndexerFunctionLibrary.h` | Blueprint utility nodes for key/handle operations, validation, and conversion |

<div class="grid cards" markdown>

- :material-database-outline:{ .lg .middle } &nbsp; **[Repository API](repository-api.md)**

    ---

    `UDataIndexerRepository` — querying rows, iterating keys, display names.

- :material-file-cog-outline:{ .lg .middle } &nbsp; **[Schema API](schema-api.md)**

    ---

    `UDataIndexerSchema` — row struct access, display names, index key builders, property customization registration.

- :material-code-braces:{ .lg .middle } &nbsp; **[Native Schema Interface](native-schema-interface.md)**

    ---

    `DataIndexer::TNativeSchemaInterface<T>` — the recommended typed C++ façade for repository queries.

</div>
