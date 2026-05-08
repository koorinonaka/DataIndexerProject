---
title: リポジトリ
quickref:
  type: UDataIndexerRepository
  inherits: UDataAsset
  pills:
    - ランタイム
    - エディタ
    - Blueprintable
  module: DataIndexer
  methods:
    - name: FindRowEntity
      desc: キーで1行を検索
    - name: ForEachPrimaryKeys
      desc: 可視行を走査
    - name: GetSchema
      desc: バインドされたスキーマ CDO
    - name: GetDisplayName
      desc: スキーマ経由の表示名
  related:
    - UDataIndexerSchema
    - FDataIndexerPrimaryKey
    - FDataIndexerRowHandle
    - FDataIndexerRowsHandle
    - FInstancedStruct
  added: v0.1.0
  verified: UE 5.5
---

# Repository

`UDataIndexerRepository` は `UDataAsset` のサブクラスで、型付きの行コレクションを格納します。DataIndexer ワークフローの主要な成果物です。デザイナーはここで行をオーサリングし、ランタイムコードはここから行をクエリします。

## 内部ストレージ

| フィールド | 型 | 役割 |
|-----------|---|------|
| `SchemaClass` | `TSubclassOf<UDataIndexerSchema>` | 行構造体とエディタ動作を決定する |
| `LocalEntries` | `TMap<FDataIndexerPrimaryKey, FInstancedStruct>` | このリポジトリが直接所有する行 |
| `EntryOwners` | `TMap<FDataIndexerPrimaryKey, TObjectPtr<UDataIndexerRepository>>` | 階層内でどのリポジトリが各キーを所有するかを追跡する |
| `ReverseLookups` | `TMap<FDataIndexerIndex, FLookupIndex>` | 保存時に構築されるセカンダリインデックステーブル |
| `ParentRepositories` | `TArray<TObjectPtr<UDataIndexerRepository>>` | 継承元リポジトリ（エディタ専用のオーサリング機能） |

## 親 Repository の合成

リポジトリは 1 つ以上の親リポジトリを参照できます。走査・クエリ時には階層が再帰的に辿られ、親の行は子を通して見えるようになります。

これにより、共有ベーステーブル（例：グローバルアイテムリポジトリ）とドメイン固有のオーバーレイ（例：ショップ用サブセット）を行を複製せずに構築できます。

!!! note "循環検出"
    `IncludesRepository(Repository)` で親チェーンの循環を検出します。エディタは循環参照を防ぎます。

## パブリック API

### `GetSchema()`

```cpp
const UDataIndexerSchema* GetSchema() const;
```

このリポジトリにバインドされたスキーマ CDO を返します。スキーマクラスが設定されていない場合は `nullptr` を返すことがあります。

### `FindRowEntity(Key)`

```cpp
FConstStructView FindRowEntity(const FDataIndexerPrimaryKey& Key) const;
```

プライマリキーで行を検索し、ローカルマップを参照した後、親リポジトリを再帰的に検索します。見つからない場合は空の `FConstStructView` を返します。`FConstStructView::GetPtr<const TMyRow>()` でキャストするか、`TNativeSchemaInterface<T>::FindRow` を使用してください。

### `ForEachPrimaryKeys(Callback)`

```cpp
void ForEachPrimaryKeys(const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

このリポジトリを通して見えるすべてのプライマリキーを走査します（親を含む）。ローカル行は `LocalEntries` の挿入順、親行はその後に続きます。

### `ForEachPrimaryKeys(Index, Query, Callback)`

```cpp
void ForEachPrimaryKeys(
    const FDataIndexerIndex& Index,
    const FConstStructView Query,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback) const;
```

セカンダリインデックス検索にマッチするプライマリキーを走査します。`Query` は部分的に埋めた行構造体で、スキーマのビルダー関数がこれを元にルックアップ GUID を算出し、`ReverseLookups` を直接参照します。全行の走査は行いません。

### `GetDisplayName(PrimaryKey)`

```cpp
FText GetDisplayName(const FDataIndexerPrimaryKey& PrimaryKey) const;
```

スキーマの `GetRowDisplayName` に委譲します。エディタと Blueprint ノードが人間可読なラベルを表示するために使用します。

## シリアライズ

バイナリアセット形式はカスタムの `Serialize` オーバーライドで `LocalEntries`・`EntryOwners`・`ReverseLookups` をシリアライズします。`ReverseLookups` は保存時にスキーマの `BuildIndexFunctions` から再構築されます。

JSON エクスポートは人間可読で差分に適したサイドカーを生成します。[JSON Import & Export](../editor-guide/json-import-export.md) を参照してください。
