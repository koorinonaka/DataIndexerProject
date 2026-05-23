---
title: How-to ガイド
---

# How-to ガイド

DataIndexer の一般的なワークフローをタスク起点で解説するガイドです。やりたいことに合わせてページを選んでください。

## 逆引き一覧

| やりたいこと | ガイド |
|-------------|--------|
| DataTable の行を DataIndexer に移したい | [DataTable から移行する](migrate-from-datatable.md) |
| 構造体変更後に行を JSON で書き出して再取り込みしたい | [DataTable から移行する → JSON の変換](migrate-from-datatable.md#transform-the-json) |
| Repository の UPROPERTY ピッカーを特定のSchemaに絞り込みたい | [Repositoryメタデータ → Schemaフィルタリング](repository-metadata.md#schema-filtering) |
| 子Repositoryが変更できないフィールドをロックしたい | [Repositoryメタデータ → NotOverridable](repository-metadata.md#notoverridable) |
| 共有ベーステーブルにゾーンやショップ別のオーバーレイを持たせたい | [親Repository階層](parent-hierarchy.md) |
| データを複製せずに親行を子でオーバーライドしたい | [親Repository階層 → オーバーライドモード](parent-hierarchy.md#override-mode) |
| 1 つのRepositoryから親・子の行を透過的にクエリしたい | [親Repository階層 → ランタイムクエリ](parent-hierarchy.md#runtime-querying) |

## 関連ドキュメント

- [コアコンセプト](../concepts/index.md) — Repository・Schema・キー・Indexes の概念
- [エディタガイド](../editor-guide/index.md) — Data View・JSON サポート・Driven Collection
- [API リファレンス](../api-reference/index.md) — Blueprint ノードと C++ インターフェース
