# JSON サポート

DataIndexer Repositoryは JSON によるエクスポート・インポート・Diff に対応しています。各操作はコンテンツブラウザのコンテキストメニューから実行します。

## エクスポート

1. コンテンツブラウザでRepositoryAssetを右クリック
2. **Export JSON** を選択
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
- エクスポートにはローカル行のみ含まれます — 親Repositoryの行はエクスポートされません

## インポート

1. コンテンツブラウザでRepositoryAssetを右クリック
2. **Import JSON** を選択
3. JSON ファイルを選択する

インポートは**完全置換**として実行されます — 既存の行はすべて削除され、JSON ファイルの内容で置き換えられます。

## Reimport

カスタムエディタで開いたRepositoryはツールバーの **Reimport** ボタンから再インポートできます。

- インポート元ファイルが保存済みの場合：そのファイルから自動的に再インポートします
- 未保存の場合：ファイル選択ダイアログを開きます（Import JSON と同じ動作）

再インポート後、データビューは自動的に更新されます。

## Diff ワークフロー

VCS 連携（Perforce・Git）を設定済みの環境では、コンテンツブラウザからリビジョン間の差分を視覚的に確認できます。

1. コンテンツブラウザでRepositoryAssetを右クリック
2. **ソース管理** → **Diff Against Depot**（Perforce）または **Diff Against Revision**（Git）を選択

DataIndexer は比較対象の両リビジョンを一時 JSON ファイルとしてエクスポートし、設定済みのテキスト差分ツール（Beyond Compare・WinMerge など）を起動して差分を表示します。

差分ツールのパスは **Editor Preferences → General → Source Control → Text Diff Tool Path** で設定します。