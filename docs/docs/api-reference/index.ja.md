---
hide:
  - navigation
---

# API リファレンス

DataIndexer のランタイム API は 2 種類あります — デザイナー向けの Blueprint ノードと、コードサイド統合用の型付き C++ API です。

---

## Blueprint API

DataIndexer は 3 つのメカニズムで Blueprint に対応した API を公開しています。

| メカニズム | 説明 |
|-----------|------|
| `UDataIndexerFunctionLibrary` | キー・ハンドル操作用の Pure および Callable ユーティリティノード |
| カスタム K2 ノード | 型付き行取得とPrimaryKeyのディスパッチ |
| 構造体型 | UPROPERTY 変数型としての `FDataIndexerRowHandle` と `FDataIndexerKeysHandle` |

ほとんどの Blueprint ノードはランタイムセーフでパッケージビルドでも使用できます。エディタサブシステム呼び出しをラップする一部のノードはエディタ専用で、ランタイム Blueprint では表示されません。

<div class="grid cards" markdown>

- :material-function-variant:{ .lg .middle } &nbsp; **[Function Library](function-library.md)**

    ---

    `UDataIndexerFunctionLibrary` のすべてのノードの完全リファレンス — 等値比較・変換・検証・IndexKey構築。

- :material-graph:{ .lg .middle } &nbsp; **[Custom K2 Nodes](k2-nodes.md)**

    ---

    `GetRow`・`SelectPrimaryKey`・`SwitchPrimaryKey` グラフノード — 型付き行アクセスとキーベースのディスパッチを提供します。

</div>

---

## C++ API

`DataIndexer` ランタイムモジュールは、Repositoryへのクエリ・行の走査・IndexKey ビルダーの構築用の型付き C++ API を公開しています。

モジュールの `.Build.cs` に `DataIndexer` を追加します。

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "DataIndexer",
});
```

!!! warning "DataIndexerEd を参照しないこと"
    `DataIndexerEd` は `UncookedOnly` です。Runtime またはゲームモジュールの `PublicDependencyModuleNames` や `PrivateDependencyModuleNames` には絶対に追加しないでください。パッケージビルドでは使用できず、ビルドエラーが発生します。

| ヘッダー | 内容 |
|---------|------|
| `DataIndexerTypes.h` | 行アクセス用の基本値型 — PrimaryKey・RowHandle・KeysHandle・Index・IndexKey |
| `DataIndexerRepository.h` | 行クエリ・IndexKey 走査・表示名解決を行うリポジトリクラス |
| `DataIndexerSchema.h` | 行構造体アクセス・IndexKey ビルダー登録・プロパティカスタマイズを管理するスキーマクラス |
| `DataIndexerSchemaInterface.h` | キャストなしで型安全な Repository クエリを行う型付き C++ ファサード |
| `DataIndexerDrivenCollection.h` | インデックス内容の変更に自動追従するデータ駆動コレクション |
| `DataIndexerFunctionLibrary.h` | キー・ハンドル操作・検証・変換用の Blueprint ユーティリティノード群 |

<div class="grid cards" markdown>

- :material-database-outline:{ .lg .middle } &nbsp; **[Repository API](repository-api.md)**

    ---

    `UDataIndexerRepository` — 行のクエリ・キーの走査・表示名。

- :material-file-cog-outline:{ .lg .middle } &nbsp; **[Schema API](schema-api.md)**

    ---

    `UDataIndexerSchema` — 行構造体へのアクセス・表示名・IndexKey ビルダー・プロパティカスタマイズの登録。

- :material-code-braces:{ .lg .middle } &nbsp; **[Native Schema Interface](native-schema-interface.md)**

    ---

    `DataIndexer::TNativeSchemaInterface<T>` — RepositoryQueryに推奨される型付き C++ ファサード。

</div>
