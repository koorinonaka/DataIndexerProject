# コアコンセプト

DataIndexer は 4 つの相互に関連するコンセプトで構成されています。これらの関係を理解することで、他のすべてがつながります。

```mermaid
flowchart LR
    Schema -->|行構造体を定義| Repository
    Schema -->|Index ビルダーを登録| Index
    Repository -->|PrimaryKey で行を格納| PrimaryKey
    Index -.->|逆引き| Repository
```

## 4 つのコンセプト

<div class="grid cards" markdown>

- :material-database:{ .lg .middle } &nbsp; **[Repository](repository.md)**

    ---

    行を保持するデータAsset。プライマリKeyからインスタンス化された行構造体への `TMap`、およびセカンダリIndex用の逆引きテーブルを格納します。

- :material-file-document-outline:{ .lg .middle } &nbsp; **[Schema](schema.md)**

    ---

    Repository とエディタ動作の間のコントラクト。行構造体の型を定義し、表示名ロジックを提供し、Data View に表示するカラムを制御し、Index ビルダー関数を登録します。

- :material-key-variant:{ .lg .middle } &nbsp; **[Keys & Handles](keys-and-handles.md)**

    ---

    行を特定するアドレス型。`FDataIndexerPrimaryKey` は単一行を識別する GUID。`FDataIndexerRowHandle` はRepository参照とKeyをペアにし、`FDataIndexerKeysHandle` はIndexQuery用のKeyセットを解決します。

- :material-table-search:{ .lg .middle } &nbsp; **[Indexes](indexes.md)**

    ---

    セカンダリ検索軸。`FDataIndexerIndex`（GUID）はカテゴリ・陣営・レアリティなどの属性をプライマリKeyのセットにマップします。Schema がビルダー関数を登録します。


</div>
