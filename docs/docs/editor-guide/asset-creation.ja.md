# アセット作成

## Schema Blueprint の作成

Schema Blueprint はリポジトリの行構造体とエディタ動作を定義します。

Schema Blueprint を作成する前に、使用する行構造体を定義します。Content Browser で Blueprint 構造体アセットを作成し、1 行を構成するプロパティを追加します。

![行データ用の Blueprint 構造体を作成](../assets/images/bp-struct-creation.png)

1. **Content Browser** で右クリック → **Blueprint Class**
2. クラスピッカーで `DataIndexerSchema` を検索する
3. 選択して **Select** をクリックする
4. アセット名を付けて（例：`BP_ItemSchema`）ダブルクリックして開く
5. **Class Defaults** パネルで：
   - **Row Struct** を行データを定義する `USTRUCT` 型に設定する
   - 必要に応じて **Get Row Display Name**（Blueprint イベント）をオーバーライドして人間可読なラベルを返す

![DataIndexerSchema Blueprint に行構造体を設定](../assets/images/schema-blueprint-creation.png)

**Get Row Display Name** を実装すると、各行に人間可読なラベルを付けられます。以下の例では `AbilityClass` をそのまま表示名として使用しています。

![AbilityClass を表示名として返す GetRowDisplayName の実装](../assets/images/schema-get-row-display-name.png)

!!! tip "命名規則"
    Schema Blueprint には `BP_` プレフィックスと `Schema` サフィックスを付けてください — 例：`BP_ItemSchema`、`BP_QuestSchema`。

## Repository アセットの作成

1. **Content Browser** で右クリック → **Miscellaneous → Data Asset**
2. アセットクラスピッカーで `DataIndexerRepository` を検索する
3. 選択して **Select** をクリックする
4. アセット名を付ける（例：`DA_ItemRepository`）

## Schema を Repository にバインド

行をオーサリングする前に、リポジトリはどのスキーマ（ひいてはどの行構造体）を使うかを知る必要があります。

1. リポジトリアセットを開く（Details パネルを開くにはシングルクリック、フルエディタを開くにはダブルクリック）
2. **Details** パネルで **Schema Class** を見つける
3. 作成した Schema Blueprint を設定する（例：`BP_ItemSchema`）

スキーマをバインドすると、リポジトリの行構造体がロックされます。**行が存在する状態でスキーマクラスを変更すると、すべての行データが無効になります。**

![Details パネルでスキーマクラスを設定した Repository](../assets/images/repository-schema-binding.png)

!!! warning "スキーマバインドは恒久的"
    行を追加する前にスキーマクラスを設定してください。後から変更するのは既存の行データをすべて失う破壊的操作です。

## 親 Repository の設定

リポジトリには 1 つ以上の親リポジトリを設定できます。親が持つすべての行が子の Data View から参照でき、行を複製することなく共有ベーステーブルとドメイン固有のオーバーレイを構築できます。

**代表的なパターン：**

- グローバルアイテムリポジトリを親に、ショップやチェストリポジトリを子として独自のエントリーのみ定義する。
- キャラクターステータスのベースリポジトリを親に、派閥ごとの子リポジトリで特定の行をオーバーライド・拡張する。

**設定手順：**

1. 子リポジトリアセットを開く
2. **Details → Parent Repositories** の **+** ボタンをクリックする
3. 親リポジトリアセットを選択する
4. 同様に繰り返して複数の親を追加できます — 実行時は配列全体を再帰的に走査します

**動作：**

- 継承行は Data View で異なる背景色で表示されます。デフォルトでは非表示のため、**Filters → Show Inherited Rows** を有効にしてください。
- 継承行は子リポジトリでは編集できません — 編集するには親を開いてください。
- 子リポジトリで継承行を編集したい場合は、その行を選択して **Override**（`Shift+O`）を実行します。同じプライマリキーで行が子リポジトリにコピーされ、独立して編集できるようになります。
- 親側の **Editor Flags** で子の操作を制限できます：**Not Overridable in Children** を設定するとオーバーライド操作がブロックされ、**Hidden in Children** を設定すると子の Data View にその行が表示されなくなります。

!!! warning "循環参照の検出"
    エディタは循環する親チェーン（`A → B → A`）をブロックします。循環参照を作成しようとすると、親フィールドが選択を拒否します。

## セットアップの確認

リポジトリアセットをダブルクリックします。カスタムエディタが以下の構成で開きます。

- **Asset Details** パネル（左）：Schema Class と Parent Repositories
- **Selection Details** パネル（右、行が選択されるまで空）
- **Data View**（中央）：行構造体のカラムヘッダー
