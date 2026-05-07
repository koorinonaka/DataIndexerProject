# Repository API

`UDataIndexerRepository`（`DataIndexerRepository.h`）はランタイムのデータアセットクラスです。すべての行クエリはこのパブリックメソッドを通じて行います。

## GetSchema

```cpp
const UDataIndexerSchema* GetSchema() const;
```

このリポジトリにバインドされたスキーマ CDO を返します。`SchemaClass` が設定されていない場合は `nullptr` を返すことがあります。使用前に必ず null チェックしてください。

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

プライマリキーで行を検索します。`LocalEntries` を最初に検索し、次に `ParentRepositories` を再帰的に検索します。

キーが見つからない場合は空の `FConstStructView` を返します。確認には `FConstStructView::IsValid()` を使用してください。

**型付きアクセス：**

```cpp
FConstStructView View = Repository->FindRowEntity(Key);
if (const FItemRow* Row = View.GetPtr<const FItemRow>())
{
    // Row を使用する
}
```

よりクリーンな型付き API には `TNativeSchemaInterface<T>::FindRow` を優先して使用してください。

---

## ForEachPrimaryKeys（全行）

```cpp
void ForEachPrimaryKeys(
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

このリポジトリを通して見えるすべてのプライマリキーを走査します（親リポジトリを含む）。順序：ローカルキーが先（挿入順）、次に親キー。

```cpp
Repository->ForEachPrimaryKeys([&](const FDataIndexerPrimaryKey& Key)
{
    FConstStructView Row = Repository->FindRowEntity(Key);
    // ...
});
```

---

## ForEachPrimaryKeys（インデックス指定）

```cpp
void ForEachPrimaryKeys(
    const FDataIndexerIndex& Index,
    const FDataIndexerIndexKey& IndexKey,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

セカンダリインデックス検索にマッチするプライマリキーを走査します。`ReverseLookups` テーブルを直接参照するため O(マッチ数)、全行を走査しません。

```cpp
Repository->ForEachPrimaryKeys(
    UItemSchema::ByTypeIndex(),
    WeaponIndexKey,
    [&](const FDataIndexerPrimaryKey& Key)
    {
        // Weapon Row のみ
    });
```

---

## GetDisplayName

```cpp
FText GetDisplayName(const FDataIndexerPrimaryKey& PrimaryKey) const;
FText GetDisplayName(const FDataIndexerIndex& Index, const FDataIndexerIndexKey& IndexKey) const;
```

スキーマの `GetRowDisplayName` を呼び出して人間可読なラベルを返します。スキーマが null または行が見つからない場合は `FText::GetEmpty()` を返します。

---

## エディタ専用メソッド

以下のメソッドは `WITH_EDITOR` ビルドでのみ使用できます。

| メソッド | 説明 |
|---------|------|
| `GetRowStruct() const` | 行型の `UScriptStruct*` を返す（スキーマに委譲） |
| `FindTableProperty(PropertyName)` | 行構造体内のプロパティを名前で検索する |
| `IncludesRepository(Repository)` | `Repository` がこのリポジトリの親階層にあれば true を返す（循環検出） |
| `GetScopedFilterKey()` | フィルタ評価中の現在のスコープフィルタキーを返す（エディタ専用） |
