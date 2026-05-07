# Blueprint API

DataIndexer exposes a Blueprint-friendly surface through three mechanisms:

| Mechanism | Description |
|-----------|-------------|
| `UDataIndexerFunctionLibrary` | Pure and callable utility nodes for key/handle operations |
| Custom K2 Nodes | Typed row retrieval and primary key dispatch |
| Struct types | `FDataIndexerRowHandle` and `FDataIndexerRowsHandle` as UPROPERTY variable types |

## Runtime vs. Editor-only nodes

Most Blueprint nodes are runtime-safe and available in packaged builds. A small number of nodes (those wrapping editor subsystem calls) are editor-only and will not appear in runtime Blueprints.

---

## Pages in this section

[**Function Library**](function-library.md)
: Complete reference for all `UDataIndexerFunctionLibrary` nodes — equality, conversion, validation, and index key construction.

[**Custom K2 Nodes**](k2-nodes.md)
: The `GetRow`, `SelectPrimaryKey`, and `SwitchPrimaryKey` graph nodes that provide typed row access and key-based dispatch.

