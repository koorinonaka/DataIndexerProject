# エディタガイド

`DataIndexerEd` モジュールはランタイムアセット型の上にカスタムエディタワークフローを追加します。スキーマの定義・Repository の作成・行データのオーサリングをコードなしで行えます。

!!! note "エディタ専用"
    このセクションのすべての機能は Unreal Editor が必要です。`DataIndexerEd` モジュールは `UncookedOnly` で宣言されており、パッケージビルドでは使用できません。

## ワークフロー概要

新しいデータテーブルのセットアップは次の手順で行います：

1. **Blueprint 構造体を作成** — 行の形状を定義
2. **Schema Blueprint を作成** — `DataIndexerSchema` のサブクラス、ステップ 1 の構造体を指定
3. **Repository アセットを作成** — `DataIndexerRepository`
4. **Repository に Schema Class を設定**
5. **Repository をダブルクリック** — Data View を開く
6. **Insert キーで行を追加** — インライン編集または Selection Details パネルで編集
7. **保存** — 逆引きテーブルが自動再構築

## このセクションのページ

<div class="grid cards" markdown>

- :material-folder-plus:{ .lg .middle } &nbsp; **[Asset Creation](asset-creation.md)**

    ---

    Blueprint 構造体と Schema Blueprint の作成、Repository アセットの作成とスキーマのバインド。親 Repository の設定による行継承の手順も解説。

- :material-table-eye:{ .lg .middle } &nbsp; **[Data View](data-view.md)**

    ---

    3 パネル構成のカスタムエディタ。行の追加・編集・削除、表示カラムの設定、親・子 Repository 間のナビゲーション。

- :material-code-json:{ .lg .middle } &nbsp; **[JSON Import & Export](json-import-export.md)**

    ---

    コードレビュー向け差分対応 JSON へのエクスポートと、マージ操作としての JSON インポート。

- :material-layers-triple:{ .lg .middle } &nbsp; **[Driven Collection](driven-collection.md)**

    ---

    `UDataIndexerDrivenCollection` — `FDataIndexerPrimaryKey` をキーとするサブアセット（アイコン・アビリティクラス等）を管理する C++ エディタアセット基底クラス。

</div>
