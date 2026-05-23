# 変更履歴

DataIndexer のすべての注目すべき変更をここに記録します。
フォーマットは [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) に準拠します。
バージョンは `DataIndexer.uplugin` の `VersionName` フィールドに対応します。

---

## Unreleased { .release .release--current data-tag="開発中" }

### 変更 { .release-group .release-group--change }

- **データセルの選択動作** — 通常クリックが行選択モードになりました。ドラッグで行範囲を選択します。Ctrl+クリックでセル選択モードに切り替わります（矩形選択）。
- **右クリック選択** — 未選択のデータセルへの右クリックは行選択モード（行コンテキストメニューを表示）、Ctrl+右クリックはセル選択モード（セルコンテキストメニューを表示）になりました。

### 修正 { .release-group .release-group--fix }

- `ShowOnlyInnerProperties` を持つ基底構造体がある場合、NamedStruct のヘッダーが抑制されない問題を修正。直接の構造体だけでなく、スーパー構造体チェーン全体を走査するようになりました。

---

## v0.9.2 { .release data-date="2026-05-08" }

安定性向上 + Index走査の高速化。0.9.1 からのSchema移行は不要。

### 追加 { .release-group .release-group--add }

- **`ForEachPrimaryKeys(Index, Key, ...)`** — Index走査のオーバーロード。リバースルックアップテーブルを直接参照する O(matches) 経路。全行走査なし。
- **`DI_DECLARE_MIGRATION`** — 宣言的な行間移行マクロ。
- **JSON サイドカー差分ビュー** — エディタ内でオンディスクサイドカーとのインライン差分を表示。

### 変更 { .release-group .release-group--change }

- `BuildIndexFunctions` の実行順序を決定論的に統一（ビルダー名のアルファベット順）し、サイドカー差分を安定化。

### 修正 { .release-group .release-group--fix }

- 前のSchemaの行が残ったまま `SchemaClass` を変更するとクラッシュする問題を修正。
- 最後の行を削除する Undo 操作後にエディタカラムが消える問題を修正。

---

## v0.9.1 { .release data-date="2026-04-10" }

初回公開リリース。

### 追加 { .release-group .release-group--add }

- 親合成機能付き `UDataIndexerRepository` データアセット。
- Blueprint サブクラス化可能な `UDataIndexerSchema` コントラクトクラス。
- `FDataIndexerPrimaryKey`（GUID ベース）・`FDataIndexerRowHandle`・`FDataIndexerIndex`。
- 3 パネル構成のカスタムエディタ（行リスト・詳細フォーム・カラム設定）。
- コードレビューワークフロー向けの JSON サイドカーエクスポート / インポート。
- 初回ドキュメントサイト。
