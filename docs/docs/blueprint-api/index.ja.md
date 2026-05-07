# Blueprint API

DataIndexer は 3 つのメカニズムで Blueprint に対応した API を公開しています。

| メカニズム | 説明 |
|-----------|------|
| `UDataIndexerFunctionLibrary` | キー・ハンドル操作用の Pure および Callable ユーティリティノード |
| カスタム K2 ノード | 型付き行取得とプライマリキーのディスパッチ |
| 構造体型 | UPROPERTY 変数型としての `FDataIndexerRowHandle` と `FDataIndexerRowsHandle` |

## ランタイム vs エディタ専用ノード

ほとんどの Blueprint ノードはランタイムセーフでパッケージビルドでも使用できます。エディタサブシステム呼び出しをラップする一部のノードはエディタ専用で、ランタイム Blueprint では表示されません。

---

## このセクションのページ

[**Function Library**](function-library.md)
: `UDataIndexerFunctionLibrary` のすべてのノードの完全リファレンス — 等値比較・変換・検証・インデックスキー構築。

[**Custom K2 Nodes**](k2-nodes.md)
: `GetRow`・`SelectPrimaryKey`・`SwitchPrimaryKey` グラフノード — 型付き行アクセスとキーベースのディスパッチを提供します。

