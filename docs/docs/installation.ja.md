# インストール

## 前提条件

- Unreal Engine 5.7 以降

## Fab からインストール

Fab で購入したプラグインはエンジンの `Plugins/Marketplace` ディレクトリ（例: `\UE_5.7\Engine\Plugins\Marketplace`）にインストールされます。

1. Fab でプラグインをインストール
2. Unreal Editor の **Plugins** パネル（**Edit → Plugins**）を開き、**DataIndexer** を検索して有効にします。再起動を求めるダイアログが出たら再起動してください。

C++ プロジェクトの場合は、さらに以下が必要です。

3. `.uproject` ファイルを右クリックして **Generate Visual Studio project files**（Windows）を選択するか、`GenerateProjectFiles.sh`（Mac/Linux）を実行します。
4. ゲームモジュールの `.Build.cs` に `DataIndexer` を追加します（[モジュール依存関係の追加](#module-dependencies) を参照）。

## 手動インストール

ソースから組み込む場合の手順です。

### 1. プラグインをコピー

プロジェクトの `Plugins/` ディレクトリ下に `DataIndexer` フォルダを配置します。

```
YourProject/
└── Plugins/
    └── DataIndexer/        ← プラグインルート
        ├── Source/
        ├── DataIndexer.uplugin
        └── ...
```

### 2. プロジェクトファイルを再生成

`.uproject` ファイルを右クリックして **Generate Visual Studio project files**（Windows）を選択するか、`GenerateProjectFiles.sh`（Mac/Linux）を実行します。

### 3. ビルド

IDE でソリューションを開いてビルドします。以下の 2 モジュールが自動でコンパイルされます。

| モジュール | タイプ | 内容 |
|-----------|-------|------|
| `DataIndexer` | Runtime | アセット型・プライマリキー・スキーマインターフェース・Blueprint ライブラリ |
| `DataIndexerEd` | Editor (UncookedOnly) | カスタムアセットエディタ・プロパティカスタマイズ・データビュー |

### 4. プラグインを有効化

Unreal Editor の **Plugins** パネル（**Edit → Plugins**）を開き、**DataIndexer** を検索して有効にします。再起動を求めるダイアログが出たら再起動してください。

## モジュール依存関係の追加 { #module-dependencies }

ゲームモジュールから Runtime API を使用するには、`.Build.cs` に `DataIndexer` を追加します。

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "DataIndexer",
    // ...
});
```
