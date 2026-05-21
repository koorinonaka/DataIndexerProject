---
title: DataTable から移行する
---

# DataTable から移行する

既存の `UDataTable` を DataIndexer Repositoryへ移行する手順を説明します。構造体の定義はそのまま流用できます。主な作業は JSON フォーマットの変換と、ランタイムのQueryコードの更新です。

## 主な違い

| | DataTable | DataIndexer |
|-|-----------|-------------|
| 行の識別子 | `FName`（手動で命名する文字列） | `FDataIndexerPrimaryKey`（自動生成 GUID） |
| Asset型 | `UDataTable` | `UDataIndexerRepository` |
| Schema | 暗黙的 — 構造体がAssetに内包 | 明示的 — 別途 `UDataIndexerSchema` Assetを作成 |
| インポート形式 | CSV または フラットな行オブジェクトの JSON | `PrimaryKey` + `Row` ラッパー付き JSON |
| セカンダリIndex | 手動コード | 宣言的な `BuildIndex` 関数 |
| 階層・継承 | 非対応 | 親Repositoryの合成 |

## Step 1 — DataTable の行を JSON でエクスポートする { #export-datatable-rows-as-json }

1. コンテンツブラウザで DataTable Assetを右クリック
2. **Asset Actions → Export** を選択
3. **JSON** 形式を選んで保存

エクスポートされたファイルは以下のような形式になります。

```json
[
  {
    "---rowName---": "IronSword",
    "DisplayName": "Iron Sword",
    "MaxStack": 1,
    "Category": "Weapon"
  },
  {
    "---rowName---": "HealthPotion",
    "DisplayName": "Health Potion",
    "MaxStack": 10,
    "Category": "Consumable"
  }
]
```

## Step 2 — SchemaとRepositoryを作成する

この型のSchemaがまだない場合は、先に作成してください。手順は [クイックスタート](../quick-start.md) と同じです。

1. DataTable と同じ `RowStruct` を持つ **Schema Blueprint**（または C++ サブクラス）を作成する
2. **DataIndexer Repository** Assetを作成し、Schemaをバインドする

行構造体自体は変更不要です。

## Step 3 — JSON を変換する { #transform-the-json }

DataIndexer のインポート形式は各行を `{ "PrimaryKey": "...", "Row": { ... } }` のエンベロープで包み、文字列名の代わりに GUID をKeyとして使用します。

以下のスクリプトで DataTable エクスポートを DataIndexer インポート形式に変換できます。

```python title="dt_to_di.py"
import json
import uuid

with open("datatable_export.json", encoding="utf-8") as f:
    rows = json.load(f)

out = []
for row in rows:
    row.pop("---rowName---", None)
    out.append({
        "PrimaryKey": str(uuid.uuid4()),
        "Row": row
    })

with open("repository_import.json", "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, ensure_ascii=False)
```

実行:

```
python dt_to_di.py
```

生成された `repository_import.json` がインポートに使用できます。

!!! note "行名を表示名として使う場合"
    DataTable の行名は人間可読なラベルとしても機能していることがあります。移行後は、Schemaの `GetRowDisplayName` を実装して、実際の行フィールド（例: `DisplayName`）から意味のある `FText` を返すようにしてください。DataIndexer のエディタや Blueprint Nodeは行名が表示されていた場所でこの関数の結果を使用します。

## Step 4 — インポートする

1. RepositoryAssetを右クリック → **Import JSON**
2. `repository_import.json` を選択

インポートは**完全置換**として実行されます — 既存の行はすべて削除され、JSON ファイルの内容で置き換えられます。保存後、エディタがセカンダリIndexを自動再構築します。

## Step 5 — ランタイム参照を更新する

### `FindRow` 呼び出しの置き換え

=== "移行前（DataTable）"

    ```cpp
    if (const FItemRow* Row = ItemTable->FindRow<FItemRow>(FName("IronSword"), TEXT("")))
    {
        // Row を使用
    }
    ```

=== "移行後（DataIndexer C++）"

    DataIndexer の行は `FDataIndexerPrimaryKey` で取得します。Keyをハンドルプロパティとして保持し、ランタイムにQueryします。

    ```cpp
    // ComponentやAssetにハンドルを持たせる
    UPROPERTY(EditAnywhere)
    FDataIndexerHandle ItemHandle;

    // Query
    if (const FItemRow* Row = FItemInterface::FindRow(Repository, ItemHandle.PrimaryKey))
    {
        // Row を使用
    }
    ```

=== "移行後（Blueprint）"

    **DataIndexer Handle** 変数を使って行の参照を保持します。グラフ上でハンドルから **Get Row** を呼び出してください。Nodeの詳細パネルにあるコンボボックスで、デザイナーが表示名から特定の行を選択できます。

### 全行イテレーションの置き換え

=== "移行前（DataTable）"

    ```cpp
    TArray<FItemRow*> AllRows;
    ItemTable->GetAllRows(TEXT(""), AllRows);
    for (const FItemRow* Row : AllRows) { /* ... */ }
    ```

=== "移行後（DataIndexer）"

    ```cpp
    for (const FDataIndexerPrimaryKey& Key : FItemInterface::GetAllPrimaryKeys(Repository))
    {
        if (const FItemRow* Row = FItemInterface::FindRow(Repository, Key))
        {
            // Row を使用
        }
    }
    ```

### 名前ベースのルックアップをIndexに置き換える

文字列識別子（アイテム ID など）で行を引いていた既存コードは、全行走査ではなくセカンダリIndexを使うように移行してください。設定手順は [Index](../concepts/indexes.md) を参照してください。

```cpp
// 武器タイプのアイテムをすべて取得する
FItemRow Query;
Query.Type = EItemType::Weapon;

TArray<FDataIndexerPrimaryKey> Keys =
    FItemInterface::GetPrimaryKeys(*Repository, UItemSchema::ByTypeIndex(), Query);
```

## 移行後のワークフロー

移行完了後は [JSON サポート](../editor-guide/json-support.md) で以後のエクスポート・再インポート・VCS Diff を管理してください。`dt_to_di.py` スクリプトが必要なのは初回移行時のみです。以降のやり取りは DataIndexer ネイティブの JSON 形式で完結します。
