# アセット作成

## Schema Blueprint の作成

Schema Blueprint はRepositoryの行構造体とエディタ動作を定義します。

Schema Blueprint を作成する前に、使用する行構造体を定義します。Content Browser で Blueprint 構造体アセットを作成し、1 行を構成するプロパティを追加します。

![行データ用の Blueprint 構造体を作成](../assets/images/bp-struct-creation.png)

1. **Content Browser** で右クリック → **Blueprint Class**
2. クラスピッカーで `DataIndexerSchema` を検索する
3. 選択して **Select** をクリックする
4. アセット名を付けて（例：`BP_ItemSchema`）ダブルクリックして開く
5. **Class Defaults** パネルで：
   - **Row Struct** を行データを定義する `USTRUCT` 型に設定する
   - 必要に応じて **Get Row Display Name**（Blueprint イベント）をオーバーライドして人間可読なラベルを返す

**Get Row Display Name** を実装すると、各行に人間可読なラベルを付けられます。以下の例では `AbilityClass` をそのまま表示名として使用しています。

![AbilityClass を表示名として返す GetRowDisplayName の実装](../assets/images/schema-get-row-display-name.png)

!!! tip "命名規則（推奨）"
    Schema Blueprint には `BP_` プレフィックスと `Schema` サフィックスを付けることを推奨します — 例：`BP_ItemSchema`、`BP_QuestSchema`。

## Repository アセットの作成

1. **Content Browser** で右クリック → **Miscellaneous → DataIndexer**
2. **Pick Class** ダイアログで使用するSchema（例：`BP_ItemSchema`）を選択する
3. 名前を付けてアセットを開く

!!! note "Schemaの変更"
    **Row Struct が一致していれば**、バインド済みSchemaは別のSchemaに変更できます。Row Struct が異なる場合は JSON Export / Import でマイグレーションしてください。

Repository アセットをダブルクリックすると Data View が開きます。

- **Insert** キーで行を追加 — GUID のPrimaryKeyが自動生成されます
- グリッドでインライン編集するか、行を選択して右側の **Selection Details** パネルで編集
- **保存** — 逆引きテーブルが自動再構築されます

## 親 Repository の設定

Repositoryには 1 つ以上の親Repositoryを設定できます。親が持つすべての行が子の Data View から参照でき、行を複製することなく共有ベーステーブルとドメイン固有のオーバーレイを構築できます。親→子→孫のような多段階の階層構造も作成できます。

**代表的なパターン：**

- グローバルアイテムRepositoryを親に、ショップやチェストRepositoryを子として独自のエントリーのみ定義する。
- キャラクターステータスのベースRepositoryを親に、派閥ごとの子Repositoryで特定の行をオーバーライド・拡張する。

**設定手順：**

1. 子Repository アセットを開く
2. **Details → Parent Repositories** の **+** ボタンをクリックする
3. 親Repository アセットを選択する
4. 同様に繰り返して複数の親を追加できます — 実行時は配列全体を再帰的に走査します

**動作：**

- 継承行はデフォルトで表示されますが、子Repositoryでは編集できません — 編集するには親を開いてください。
- 子Repositoryで継承行をオーバーライドすると、その行の継承元は子の Data View には表示されなくなります。
- 子Repositoryで継承行を編集したい場合は、その行を選択して **Override**（`Shift+O`）を実行します。同じPrimaryKeyで行が子Repositoryにコピーされ、独立して編集できるようになります。

!!! warning "循環参照の検出"
    エディタは循環する親チェーン（`A → B → A`）をブロックします。循環参照を作成しようとすると、親フィールドが選択を拒否します。

## セットアップの確認

Repository アセットをダブルクリックします。カスタムエディタが以下の構成で開きます。

- **アセット Details** パネル（左）：Schema Class と Parent Repositories
- **Selection Details** パネル（右、行が選択されるまで空）
- **Data View**（中央）：行構造体のカラムヘッダー
