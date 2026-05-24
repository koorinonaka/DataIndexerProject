# 変更履歴

DataIndexer のすべての注目すべき変更をここに記録します。
フォーマットは [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) に準拠します。
バージョンは `DataIndexer.uplugin` の `VersionName` フィールドに対応します。

---

## v1.0.0 { .release .release--current data-date="2026-05-24" }

初回公開リリース。

### 追加 { .release-group .release-group--add }

**検索**

- `FDataIndexerPrimaryKey`（GUID ベース）・`FDataIndexerRowHandle`・`FDataIndexerIndex` — コア識別型。
- `DI_DEFINE_INDEX` — クラスパスと名前から安定した GUID を生成して Secondary Index を宣言。
- `ForEachPrimaryKeys(Index, Key, ...)` — 逆引きテーブルを直参照する O(matches) 走査。全行スキャンなし。
- `FDataIndexerKeysHandle` — Blueprint 側の Index クエリ ハンドル。使用時に一致する PrimaryKey を解決する。

**オーサリング**

- 3 パネル構成のカスタムエディタ：行リスト・詳細フォーム・カラム設定。
- JSON サイドカーのエクスポート / インポートと、オンディスクサイドカーとのインライン差分ビュー。
- `UDataIndexerDrivenCollection` — 行ごとのサブアセット（アイコン・メッシュ・Ability クラスなど）を管理するコンパニオンアセット。ソース Repository の保存時に自動再ビルド。
- `IsRowValid` — `UDataIndexerSchema` の C++ バリデーション フック。**Validate Data** の実行・アセット保存・クック時に動作し、エラーがあると処理をブロック。
- プロパティ メタデータ：`meta=(Repository=...)` で行ピッカーを紐付け、`meta=(Schema=...)` でアセット ピッカーをフィルタリング、`meta=(ReadOnlyKeys)` で読み取り専用ピッカーを指定。
- 行ごとのエディタ フラグ：`Comment Out`・`Not Overridable in Children`・`Hidden in Children`。
- 行とタブ区切りセル範囲のコピー / ペースト。
- Reference Viewer カラム、Schema ごとのカラム レイアウト保存、**Show Inherited Rows** / **Show Only Unreferred Rows** フィルター。

**構造**

- `UDataIndexerRepository` — マルチレベルの親合成と行単位のオーバーライド制御を備えたデータアセット。
- `UDataIndexerSchema` — 行構造体・表示ロジック・Index ビルダーを定義する Blueprint サブクラス化可能なコントラクトクラス。

**統合**

- `TNativeSchemaInterface<T>` — `FindRow`・`ForEachPrimaryKeys`・`GetPrimaryKeys` をまとめた型付き C++ アクセサ テンプレート。
- **Get Row** K2 ノード — `FDataIndexerRowHandle` から型付き行データを取得。成功・失敗の実行ピンで分岐。
- **Get Keys by Index** K2 ノード — `FDataIndexerKeysHandle` とクエリ構造体のペアから一致する PrimaryKey を取得。
- **Select by Primary Key** K2 ノード — 行の Identity に基づいて値を選択する Pure ノード。
- **Switch on Primary Key** K2 ノード — 行の Identity に基づいて実行フローをディスパッチ。
