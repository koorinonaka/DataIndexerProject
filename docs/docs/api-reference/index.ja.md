# API リファレンス

DataIndexer のランタイム API は 2 種類あります — デザイナー向けの Blueprint Nodeと、コードサイド統合用の型付き C++ API です。

---

## Blueprint API

DataIndexer は 3 つのメカニズムで Blueprint に対応した API を公開しています。

| メカニズム | 説明 |
|-----------|------|
| `UDataIndexerFunctionLibrary` | Key・ハンドル操作用の Pure および Callable ユーティリティNode |
| カスタム K2 Node | 型付き行取得とプライマリKeyのディスパッチ |
| 構造体型 | UPROPERTY 変数型としての `FDataIndexerRowHandle` と `FDataIndexerKeysHandle` |

ほとんどの Blueprint Nodeはランタイムセーフでパッケージビルドでも使用できます。エディタサブシステム呼び出しをラップする一部のNodeはエディタ専用で、ランタイム Blueprint では表示されません。

<div class="grid cards" markdown>

- :material-function-variant:{ .lg .middle } &nbsp; **[Function Library](function-library.md)**

    ---

    `UDataIndexerFunctionLibrary` のすべてのNodeの完全リファレンス — 等値比較・変換・検証・IndexKey構築。

- :material-graph:{ .lg .middle } &nbsp; **[Custom K2 Nodes](k2-nodes.md)**

    ---

    `GetRow`・`SelectPrimaryKey`・`SwitchPrimaryKey` グラフNode — 型付き行アクセスとKeyベースのディスパッチを提供します。

</div>

---

## C++ API

`DataIndexer` ランタイムモジュールは、RepositoryへのQuery・行の走査・IndexKey ビルダーの構築用の型付き C++ API を公開しています。

モジュールの `.Build.cs` に `DataIndexer` を追加します。

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "DataIndexer",
});
```

!!! warning "DataIndexerEd を参照しないこと"
    `DataIndexerEd` は `UncookedOnly` です。Runtime またはゲームモジュールの `PublicDependencyModuleNames` や `PrivateDependencyModuleNames` には絶対に追加しないでください。パッケージビルドでは使用できず、ビルドエラーが発生します。

| ヘッダー | インクルードパス | 内容 |
|---------|--------------|------|
| `DataIndexerTypes.h` | `#include "DataIndexerTypes.h"` | `FDataIndexerPrimaryKey`・`FDataIndexerRowHandle`・`FDataIndexerKeysHandle`・`FDataIndexerIndex`・`FDataIndexerIndexKey` |
| `DataIndexerRepository.h` | `#include "DataIndexerRepository.h"` | `UDataIndexerRepository` |
| `DataIndexerSchema.h` | `#include "DataIndexerSchema.h"` | `UDataIndexerSchema`・`FDataIndexerExpandedStructEntry` |
| `DataIndexerSchemaInterface.h` | `#include "DataIndexerSchemaInterface.h"` | `DataIndexer::TNativeSchemaInterface<T>` |
| `DataIndexerDrivenCollection.h` | `#include "DataIndexerDrivenCollection.h"` | `UDataIndexerDrivenCollection` |
| `DataIndexerFunctionLibrary.h` | `#include "DataIndexerFunctionLibrary.h"` | `UDataIndexerFunctionLibrary` |

<div class="grid cards" markdown>

- :material-database-outline:{ .lg .middle } &nbsp; **[Repository API](repository-api.md)**

    ---

    `UDataIndexerRepository` — 行のQuery・Keyの走査・表示名。

- :material-file-cog-outline:{ .lg .middle } &nbsp; **[Schema API](schema-api.md)**

    ---

    `UDataIndexerSchema` — 行構造体へのアクセス・表示名・IndexKey ビルダー・プロパティカスタマイズの登録。

- :material-code-braces:{ .lg .middle } &nbsp; **[Native Schema Interface](native-schema-interface.md)**

    ---

    `DataIndexer::TNativeSchemaInterface<T>` — RepositoryQueryに推奨される型付き C++ ファサード。

</div>
