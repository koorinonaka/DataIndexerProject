# DataIndexer

UE5向けのデータ管理プラグインです。DataTableを完全に置き換えるワークフローを構築し、大規模なゲームデータの編集・検索・管理を効率化します。

<!-- TODO: スクリーンショット — DataIndexerエディタ全体像（データビュー＋Repository一覧） -->

---

## DataIndexerの特徴

DataTableの機能を引き継ぎながら、大規模なプロジェクトで実感しやすいメンテナンス性の向上や、よりリッチなエディタ体験を提供します。

### DataTableの完全な代替

DataTableで行っていた操作の多くはDataIndexerでも同様に行えます。既存のワークフローを大きく変えることなく、必要な機能から段階的に導入できます。

### 高いメンテナンス性

- **Key参照の追跡**
  あるRowがどのアセット・コードから参照されているかをエディタ上で一覧できます。
  DataTableでは参照元の把握が困難でしたが、DataIndexerでは使用箇所を明示的に追跡できます。

- **リネームしても参照が壊れない**
  DataTableではRowのリネーム時にBlueprintやコードの参照が壊れます。
  DataIndexerはGUIDベースのPrimaryKeyを採用しているため、キー名を変更しても参照はそのまま維持されます。

<!-- TODO: スクリーンショット — Key参照の追跡UI（使用箇所一覧パネル） -->

### データのコンポジション — 継承と統合

- **Repositoryの継承と統合**
  親Repositoryのデータを継承する機能と、複数のRepositoryをひとつにまとめる機能を標準でサポートしています。
  共通データを親側で管理し、地域・難易度ごとの差分を子側で表現できます。

- **継承時の上書き**
  子Repository内で特定のRowだけを上書きできます。
  DataTableでのコピー＆ペースト管理と異なり、どこを変更したかが明確に追えます。

<!-- TODO: スクリーンショット — Repository継承ツリーのエディタ表示 -->

### インデックス — 属性による高速な逆引き検索

- **複数の属性からの逆引き検索**
  スキーマにインデックスを定義すると、カテゴリ・レアリティ・ファクションなど任意の属性からRowを高速に検索できます。
  DataTableのForEachによるフィルタと異なり、O(1)に近い効率で動作します。

<!-- TODO: スクリーンショット — BlueprintでのIndex逆引き検索ノード（GetRowsByIndex など） -->

### 専用エディタ — データ編集のための統合された体験

- **専用データビュー**
  DataIndexer専用の表形式エディタを提供します。スキーマに応じた列の定義と表示が行えます。

- **レイアウト・表示列のカスタマイズ**
  表示する列の選択・順序・列幅をエディタ上で設定できます。

- **インライン編集**
  表ビュー上でセルをクリックしてそのまま値を編集できます。
  DataTableエディタのようにダイアログを開く必要がなく、素早く編集できます。

- **UserWidgetによる表示拡張**
  UMG UserWidgetを定義してセルにカスタム表示を組み込めます。
  カラーピッカーやテクスチャサムネイルをセル内に直接表示することも可能です。

- **データ検証**
  スキーマに定義したバリデーションルールでデータの整合性をチェックできます。
  アセット保存時やCook時の自動検証もサポートしています。

<!-- TODO: スクリーンショット — データビューエディタ（インライン編集中の様子、カスタムセル表示） -->

---

## コアコンセプト

<div class="grid cards" markdown>

- **Repository**

    データを格納するアセットです。各Rowは`FDataIndexerPrimaryKey`（GUID）で識別され、スキーマが定義した型の構造体を保持します。親Repositoryを参照することで階層的なコンポジションが行えます。

    [:octicons-arrow-right-24: リポジトリ](concepts/repository.md)

- **Schema**

    Rowの構造体とエディタ挙動を定義します。表示名、プロパティのレンダリング、カスタムウィジェット、インデックスキーのビルダーを制御します。

    [:octicons-arrow-right-24: スキーマ](concepts/schema.md)

- **Keys & Handles**

    `FDataIndexerPrimaryKey`は単一のRowを一意に識別します。`FDataIndexerRowHandle`はRepositoryとキーをペアにしてBlueprintで扱いやすくしたものです。`FDataIndexerRowsHandle`はセカンダリインデックスを経由してRowのセットを参照します。

    [:octicons-arrow-right-24: キーとハンドル](concepts/keys-and-handles.md)

- **Indexes**

    スキーマに定義するセカンダリ検索軸です。各インデックスはRowを`FDataIndexerIndexKey`にマッピングし、任意の属性による高速検索を実現します。

    [:octicons-arrow-right-24: インデックス](concepts/indexes.md)

</div>

---

## はじめる

<div class="grid cards" markdown>

- [:octicons-download-24: **インストール**](installation.md)

    プロジェクトにプラグインを追加してモジュールをビルドします。

- [:octicons-rocket-24: **クイックスタート**](quick-start.md)

    スキーマの作成、Repositoryの設定、RowのオーサリングをC++とBlueprintから10分で実行します。

</div>
