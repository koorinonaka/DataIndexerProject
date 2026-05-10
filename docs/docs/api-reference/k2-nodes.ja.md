# カスタム K2 ノード

DataIndexer は標準の `UFUNCTION` では表現できない 3 種類のカスタム Blueprint グラフノードを提供します。これらのノードはスキーマとリポジトリの設定に基づいてコンパイル時に型付きピンを生成します。

---

## Get Row（`UK2Node_DataIndexerGetRow`）

Blueprint で型付き行を取得する主要な方法です。

**目的：** `FDataIndexerRowHandle` を受け取り、型安全な方法で行構造体の値を取得します。成功・失敗の実行パスを持ちます。

**ピン構成：**

| ピン | 方向 | 型 | 説明 |
|-----|------|----|------|
| `Handle` | 入力 | `FDataIndexerRowHandle` | 取得する行 |
| `Schema Class` | 入力（隠れた詳細） | `TSubclassOf<UDataIndexerSchema>` | `Handle` を特定スキーマに制限する |
| ✓（then） | 出力（exec） | — | 行が見つかった |
| ✗（else） | 出力（exec） | — | 行が見つからないまたはハンドルが無効 |
| `Row` | 出力 | ワイルドカード → `FMyRowStruct` | 型付き行構造体 |

**使い方：**

1. Blueprint グラフに **Get Row** ノードを配置する
2. ノードの Details パネルで **Schema Class** を設定する
3. `Handle` ピンに `FDataIndexerRowHandle` 変数を接続する
4. `Row` 出力ピンが具体的な行構造体型に解決される

---

## Select Primary Key（`UK2Node_DataIndexerSelectPrimaryKey`）

`FDataIndexerPrimaryKey` の値に基づいて実行をルーティングするスイッチスタイルのノードです。

**目的：** プライマリキーがどの行をアドレスしているかによって実行を分岐します。DataIndexer 行用の `Switch on Enum` ノードに相当します。

**ピン構成：**

| ピン | 方向 | 型 | 説明 |
|-----|------|----|------|
| `Repository` | 入力 | `UDataIndexerRepository*` | 有効な選択肢となるキーを制限する |
| `Key` | 入力 | `FDataIndexerPrimaryKey` | スイッチ対象のキー |
| `Default` | 出力（exec、省略可） | — | 設定済みピンにどれもマッチしない場合に発火（`bHasDefaultPin = true` のとき） |
| `[Row Name]` | 出力（exec、キーごと） | — | `PinKeys` に設定された各キーに 1 つ |

**設定：**

ノードの Details パネルで **Pin Keys** にエントリを追加します。各エントリで `FDataIndexerPrimaryKey` の GUID を指定し、ラベル付きの出力実行ピンが作成されます。

---

## Switch Primary Key（`UK2Node_DataIndexerSwitchPrimaryKey`）

Select Primary Key に似ていますが、標準 UE Blueprint ライブラリの `Switch` ノード規約に従います。

**Select Primary Key との違い：** Select Primary Key は単一の出力値ピンを返す（三項セレクターのような）のに対し、Switch Primary Key は実行出力ピンのみを提供します（`Switch on Int` のような）。値を選択せずに実行を分岐したい場合は Switch を使用してください。

**ピン構成：**

| ピン | 方向 | 型 | 説明 |
|-----|------|----|------|
| （exec） | 入力 | — | エントリ実行 |
| `Selection` | 入力 | `FDataIndexerPrimaryKey` | スイッチ対象のキー |
| `Default` | 出力（exec） | — | マッチなしの場合に発火 |
| `[Row Name]` | 出力（exec、キーごと） | — | 設定された各キーに 1 つ |

---

!!! tip "どのノードを使うか"
    - **Get Row** — ハンドルを持っていて行データが欲しい場合
    - **Select Primary Key** — キーを持っていて、どの行かによって値を選びたい場合
    - **Switch Primary Key** — キーを持っていて、どの行かによって実行を分岐したい場合
