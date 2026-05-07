# JSON Import & Export

DataIndexer リポジトリはカスタムエディタのツールバーから JSON エクスポートとインポートをサポートしています。JSON 形式は人間可読で差分に適しており、バージョン管理のレビューやマージワークフローに使えます。

## エクスポート

1. カスタムエディタでリポジトリを開く
2. ツールバーの **Export JSON** をクリック（または **File** メニューから）
3. 保存先のパスを選択する

エクスポートされたファイルは行オブジェクトの JSON 配列です。

```json
[
  {
    "PrimaryKey": "A1B2C3D4-E5F6-7890-ABCD-EF1234567890",
    "Row": {
      "DisplayName": "Iron Sword",
      "MaxStack": 1,
      "Category": "Weapon"
    }
  },
  {
    "PrimaryKey": "B2C3D4E5-F6A7-8901-BCDE-F12345678901",
    "Row": {
      "DisplayName": "Health Potion",
      "MaxStack": 10,
      "Category": "Consumable"
    }
  }
]
```

**ポイント：**

- `PrimaryKey` は行の `FDataIndexerPrimaryKey` を GUID 文字列で表したもの
- `Row` は UE の JSON プロパティシリアライズで行構造体をシリアライズしたもの
- エクスポートにはローカル行のみ含まれます — 親リポジトリの行はエクスポートされません

## インポート

1. カスタムエディタでリポジトリを開く
2. ツールバーの **Import JSON** をクリックする
3. JSON ファイルを選択する

インポートはマージとして実行されます。

- 既存の `PrimaryKey` を持つ行はインポートデータで **更新** される
- 新しい `PrimaryKey` を持つ行は **追加** される
- インポートファイルにない行は **そのまま残る**（インポートで削除されない）

!!! tip "完全置換"
    JSON でリポジトリの内容を完全に置き換えるには、先にすべての行を削除してからインポートしてください。

## 差分ワークフロー

JSON エクスポートはコードレビューとバージョン管理の差分に推奨されるフォーマットです。

**推奨セットアップ：**

1. バイナリの `.uasset` と並べてリポジトリに JSON をエクスポートする（例：`Content/Data/Items/DA_ItemRepository.json`）
2. 両方のファイルをコミットする
3. Pull Request でレビュアーが Unreal Editor を開かずに JSON の差分でデータ変更を確認できる

**バイナリアセット用の `.gitattributes`：**

```gitattributes
*.uasset binary
*.umap binary
```

JSON サイドカーはバイナリが変わってもテキスト差分が可能なままです。

### 差分の例

行の追加・変更・削除は差分で明確に表示されます。

```diff
 [
   {
     "PrimaryKey": "A1B2C3D4-E5F6-7890-ABCD-EF1234567890",
     "Row": {
-      "DisplayName": "Iron Sword",
-      "MaxStack": 1
+      "DisplayName": "Iron Sword (Rusted)",
+      "MaxStack": 1,
+      "Category": "Weapon"
     }
   },
+  {
+    "PrimaryKey": "C3D4E5F6-A7B8-9012-CDEF-012345678901",
+    "Row": {
+      "DisplayName": "Steel Shield",
+      "MaxStack": 1,
+      "Category": "Armor"
+    }
+  },
   {
     "PrimaryKey": "B2C3D4E5-F6A7-8901-BCDE-F12345678901",
     "Row": {
       "DisplayName": "Health Potion",
       "MaxStack": 10,
       "Category": "Consumable"
     }
   }
 ]
```

行の変更は自己完結した差分ブロックになるため、Unreal Editor を開かずにレビュアーがデータ変更を把握できます。

## マージサポート

DataIndexer はバイナリ `.uasset` フォーマットのマージフックを提供しています。リポジトリアセットでマージコンフリクトが発生した場合：

- Unreal Editor のマージダイアログで行レベルのコンフリクト解決ができます
- JSON サイドカーを手動コンフリクト解決のリファレンスとして使えます

チームワークフローでは、JSON エクスポートとカスタムマージツールを組み合わせることで、最もレビューしやすく解決しやすいデータ履歴が得られます。
