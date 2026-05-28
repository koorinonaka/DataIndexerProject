---
title: DataTable から移行する
---

# DataTable から移行する

既存の `UDataTable` を DataIndexer Repository へ移行する手順を説明します。構造体の定義はそのまま流用でき、変換スクリプトも不要です。DataTable から書き出した JSON を Repository に**直接インポート**でき、形式は自動判定されます。

## 主な違い

| | DataTable | DataIndexer |
|-|-----------|-------------|
| 行の識別子 | `FName`（手動で命名する文字列） | `FDataIndexerPrimaryKey`（自動生成 GUID） |
| アセット型 | `UDataTable` | `UDataIndexerRepository` |
| Schema | 暗黙的 — 構造体がアセットに内包 | 明示的 — 別途 `UDataIndexerSchema` アセットを作成 |
| インポート形式 | CSV または フラットな行オブジェクトの JSON | DataTable JSON を直接取り込み可。ネイティブ形式は `PrimaryKey` + `RowEntity` ラッパー |
| セカンダリ Index | 手動コード | 宣言的な `BuildIndex` 関数 |
| 階層・継承 | 非対応 | 親 Repository の合成 |

## Step 1 — DataTable の行を JSON でエクスポートする { #export-datatable-rows-as-json }

1. コンテンツブラウザで DataTable アセットを右クリック
2. **Asset Actions → Export** を選択
3. **JSON** 形式を選んで保存

エクスポートされたファイルは以下のような形式になります。各行の `Name` が DataTable の行名です。

```json
[
  {
    "Name": "IronSword",
    "DisplayName": "Iron Sword",
    "MaxStack": 1,
    "Category": "Weapon"
  },
  {
    "Name": "HealthPotion",
    "DisplayName": "Health Potion",
    "MaxStack": 10,
    "Category": "Consumable"
  }
]
```

## Step 2 — Schema と Repository を作成する

この型の Schema がまだない場合は、先に作成してください。手順は [クイックスタート](../quick-start.md) と同じです。

1. DataTable と同じ `RowStruct` を持つ **Schema Blueprint**（または C++ サブクラス）を作成する
2. **DataIndexer Repository** アセットを作成し、Schema をバインドする

行構造体自体は変更不要です。

## Step 3 — インポートする { #import }

DataIndexer は DataTable 形式の JSON を直接インポートできます。変換スクリプトは不要です。

1. Repository アセットを右クリック → **Import JSON**
2. Step 1 で書き出した JSON を選択

ファイルの先頭要素に `RowEntity` フィールドがあるかどうかで形式が自動判定されます。`RowEntity` がなければ DataTable 形式とみなされ、**行名のバインド先を尋ねるダイアログ**が表示されます。

- コンボボックスには行構造体の `FName` / `FString` / `FText` プロパティが列挙されます。
- DataTable の `Name`（行名）を保持したい場合は、保存先のプロパティを選びます。
- 行名が不要な場合は **(Ignore)** を選びます。行は匿名となり、`PrimaryKey`（GUID）でのみ識別されます。

`PrimaryKey` は各行に対して自動生成されます（JSON に `PrimaryKey` フィールドがあればその GUID が使われます）。インポートは**完全置換**として実行されます — 既存の行はすべて削除され、JSON の内容で置き換えられます。保存後、エディタがセカンダリ Index を自動再構築します。

!!! note "行名を表示名として使う場合"
    DataTable の行名は人間可読なラベルとしても機能していることがあります。多くの場合は **(Ignore)** で行名を捨て、代わりに Schema の `GetRowDisplayName` を実装して実際の行フィールド（例: `DisplayName`）から意味のある `FText` を返すのが推奨です。DataIndexer のエディタや Blueprint ノードは、行名が表示されていた場所でこの関数の結果を使用します。

## Step 4 — ランタイム参照を更新する

### `FindRow` 呼び出しの置き換え

=== "移行前（DataTable）"

    ```cpp
    if (const FItemRow* Row = ItemTable->FindRow<FItemRow>(FName("IronSword"), TEXT("")))
    {
        // Row を使用
    }
    ```

=== "移行後（DataIndexer C++）"

    DataIndexer の行は `FDataIndexerPrimaryKey` で取得します。Repository と PrimaryKey をまとめて保持する `FDataIndexerRowHandle` をプロパティに持たせ、ランタイムにクエリします。

    ```cpp
    // Component や Asset にハンドルを持たせる
    UPROPERTY(EditAnywhere)
    FDataIndexerRowHandle ItemHandle;

    // Query — ハンドルは Repository と PrimaryKey を内包する
    if (const FItemRow* Row = FItemInterface::FindRow(ItemHandle))
    {
        // Row を使用
    }
    ```

=== "移行後（Blueprint）"

    **DataIndexer Row Handle** 変数を使って行の参照を保持します。グラフ上でハンドルから **Get Row** を呼び出してください。ノードの詳細パネルにあるコンボボックスで、デザイナーが表示名から特定の行を選択できます。

### 全行イテレーションの置き換え

=== "移行前（DataTable）"

    ```cpp
    TArray<FItemRow*> AllRows;
    ItemTable->GetAllRows(TEXT(""), AllRows);
    for (const FItemRow* Row : AllRows) { /* ... */ }
    ```

=== "移行後（DataIndexer）"

    ```cpp
    for (const FDataIndexerPrimaryKey& Key : FItemInterface::GetPrimaryKeys(*Repository))
    {
        if (const FItemRow* Row = FItemInterface::FindRow(*Repository, Key))
        {
            // Row を使用
        }
    }
    ```

### 名前ベースのルックアップを Index に置き換える

文字列識別子（アイテム ID など）で行を引いていた既存コードは、全行走査ではなくセカンダリ Index を使うように移行してください。設定手順は [Index](../concepts/indexes.md) を参照してください。

```cpp
// 武器タイプのアイテムをすべて取得する
FItemRow Query;
Query.Type = EItemType::Weapon;

TArray<FDataIndexerPrimaryKey> Keys =
    FItemInterface::GetPrimaryKeys(*Repository, UItemSchema::ByTypeIndex(), Query);
```

## 移行後のワークフロー

移行完了後は [JSON サポート](../editor-guide/json-support.md) で以後のエクスポート・再インポート・VCS Diff を管理してください。以降はネイティブの JSON 形式（`PrimaryKey` + `RowEntity`）で完結し、再インポート時は行名バインドのダイアログも表示されません。
