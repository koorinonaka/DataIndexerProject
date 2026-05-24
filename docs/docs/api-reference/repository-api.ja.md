# Repository API

`UDataIndexerRepository`（`DataIndexerRepository.h`）はランタイムのデータアセットクラスです。すべての行クエリはこのパブリックメソッドを通じて行います。

!!! tip "C++ から使う場合は NativeSchemaInterface を推奨"
    型安全なクエリには [`TNativeSchemaInterface<T>`](./native-schema-interface.md) を使ってください。`FindRow`・`ForEachPrimaryKeys` など、このページと同等のAPIをコンパイル時型チェック付きで提供します。直接 `UDataIndexerRepository` を操作するのは、`FConstStructView` を扱う低レベルコードや内部実装に限定してください。

## GetSchema

```cpp
const UDataIndexerSchema* GetSchema() const;
```

このRepositoryにバインドされたSchema CDO を返します。`SchemaClass` が設定されていない場合は `nullptr` を返すことがあります。使用前に必ず null チェックしてください。

```cpp
if (const UDataIndexerSchema* Schema = Repository->GetSchema())
{
    // Schema を安全に使用できる
}
```

---

## FindRowEntity

```cpp
FConstStructView FindRowEntity(const FDataIndexerPrimaryKey& Key) const;
```

PrimaryKeyで行を検索します。`LocalEntries` を最初に検索し、次に `ParentRepositories` を再帰的に検索します。

キーが見つからない場合は空の `FConstStructView` を返します。確認には `FConstStructView::IsValid()` を使用してください。

**型付きアクセス：**

```cpp
FConstStructView View = Repository->FindRowEntity(Key);
if (const FItemRow* Row = View.GetPtr<const FItemRow>())
{
    // Row を使用する
}
```

---

## ForEachPrimaryKeys（全行）

```cpp
void ForEachPrimaryKeys(
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

このRepositoryを通して見えるすべてのPrimaryKeyを走査します（親Repositoryを含む）。順序：ローカルキーが先（挿入順）、次に親キー。

```cpp
Repository->ForEachPrimaryKeys([&](const FDataIndexerPrimaryKey& Key)
{
    FConstStructView Row = Repository->FindRowEntity(Key);
    // ...
});
```

---

## ForEachPrimaryKeys（Index 指定）

```cpp
void ForEachPrimaryKeys(
    const FDataIndexerIndexKey& IndexKey,
    const FConstStructView Query,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

セカンダリ Index 検索にマッチするPrimaryKeyを走査します。`ReverseLookups` テーブルを直接参照するため O(マッチ数)、全行を走査しません。

```cpp
FItemRow Query;
Query.Type = EItemType::Weapon;

Repository->ForEachPrimaryKeys(
    UItemSchema::ByTypeIndex(),
    FConstStructView::Make(Query),
    [&](const FDataIndexerPrimaryKey& Key)
    {
        // Weapon Row のみ
    });
```

---

## GetDisplayName

```cpp
FText GetDisplayName(const FDataIndexerPrimaryKey& PrimaryKey) const;
```

Schemaの `GetRowDisplayName` を呼び出して人間可読なラベルを返します。Schemaが null または行が見つからない場合は `FText::GetEmpty()` を返します。

---

## エディタ専用メソッド

以下のメソッドは `WITH_EDITOR` ビルドでのみ使用できます。

| メソッド | 説明 |
|---------|------|
| `GetRowStruct() const` | 行型の `UScriptStruct*` を返す（Schemaに委譲） |
| `FindTableProperty(PropertyName)` | 行構造体内のプロパティを名前で検索する |
| `IncludesRepository(Repository)` | `Repository` がこのRepositoryの親階層にあれば true を返す（循環検出） |
| `GetScopedFilterKey()` | フィルター評価中の現在のスコープFilterKeyを返す（エディタ専用） |
