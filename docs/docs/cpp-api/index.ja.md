# C++ API

`DataIndexer` ランタイムモジュールは、リポジトリへのクエリ・行の走査・インデックスキービルダーの構築用の型付き C++ API を公開しています。

## モジュール依存関係

モジュールの `.Build.cs` に `DataIndexer` を追加します。

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "DataIndexer",
});
```

!!! warning "DataIndexerEd を参照しないこと"
    `DataIndexerEd` は `UncookedOnly` です。Runtime またはゲームモジュールの `PublicDependencyModuleNames` や `PrivateDependencyModuleNames` には絶対に追加しないでください。パッケージビルドでは使用できず、ビルドエラーが発生します。

## 主要ヘッダー

| ヘッダー | インクルードパス | 内容 |
|---------|--------------|------|
| `DataIndexerTypes.h` | `#include "DataIndexerTypes.h"` | `FDataIndexerPrimaryKey`・`FDataIndexerRowHandle`・`FDataIndexerRowsHandle`・`FDataIndexerIndex`・`FDataIndexerIndexKey`・`FDataIndexerImmutableKey` |
| `DataIndexerRepository.h` | `#include "DataIndexerRepository.h"` | `UDataIndexerRepository` |
| `DataIndexerSchema.h` | `#include "DataIndexerSchema.h"` | `UDataIndexerSchema`・`FDataIndexerExpandedStructEntry` |
| `DataIndexerSchemaInterface.h` | `#include "DataIndexerSchemaInterface.h"` | `DataIndexer::TNativeSchemaInterface<T>` |
| `DataIndexerDrivenCollection.h` | `#include "DataIndexerDrivenCollection.h"` | `UDataIndexerDrivenCollection` |
| `DataIndexerFunctionLibrary.h` | `#include "DataIndexerFunctionLibrary.h"` | `UDataIndexerFunctionLibrary` |

## このセクションのページ

[**Repository API**](repository-api.md)
: `UDataIndexerRepository` — 行のクエリ・キーの走査・表示名。

[**Schema API**](schema-api.md)
: `UDataIndexerSchema` — 行構造体へのアクセス・表示名・インデックスキービルダー・プロパティカスタマイズの登録。

[**Native Schema Interface**](native-schema-interface.md)
: `DataIndexer::TNativeSchemaInterface<T>` — リポジトリクエリに推奨される型付き C++ ファサード。
