# C++ API

The `DataIndexer` runtime module exposes a typed C++ API for querying repositories, iterating rows, and building index key builders.

## Module dependency

Add `DataIndexer` to your module's `.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "DataIndexer",
});
```

!!! warning "Do not reference DataIndexerEd"
    `DataIndexerEd` is `UncookedOnly`. Never add it to `PublicDependencyModuleNames` or `PrivateDependencyModuleNames` in a Runtime or game module. It will not be available in packaged builds and will cause build failures.

## Primary headers

| Header | Include path | Contents |
|--------|-------------|----------|
| `DataIndexerTypes.h` | `#include "DataIndexerTypes.h"` | `FDataIndexerPrimaryKey`, `FDataIndexerRowHandle`, `FDataIndexerRowsHandle`, `FDataIndexerIndex`, `FDataIndexerIndexKey`, `FDataIndexerImmutableKey` |
| `DataIndexerRepository.h` | `#include "DataIndexerRepository.h"` | `UDataIndexerRepository` |
| `DataIndexerSchema.h` | `#include "DataIndexerSchema.h"` | `UDataIndexerSchema`, `FDataIndexerExpandedStructEntry` |
| `DataIndexerSchemaInterface.h` | `#include "DataIndexerSchemaInterface.h"` | `DataIndexer::TNativeSchemaInterface<T>` |
| `DataIndexerDrivenCollection.h` | `#include "DataIndexerDrivenCollection.h"` | `UDataIndexerDrivenCollection` |
| `DataIndexerFunctionLibrary.h` | `#include "DataIndexerFunctionLibrary.h"` | `UDataIndexerFunctionLibrary` |

## Pages in this section

[**Repository API**](repository-api.md)
: `UDataIndexerRepository` — querying rows, iterating keys, display names.

[**Schema API**](schema-api.md)
: `UDataIndexerSchema` — row struct access, display names, index key builders, property customization registration.

[**Native Schema Interface**](native-schema-interface.md)
: `DataIndexer::TNativeSchemaInterface<T>` — the recommended typed C++ façade for repository queries.
