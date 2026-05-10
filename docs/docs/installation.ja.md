---
title: インストール
---

# インストール

DataIndexer をプロジェクトに追加して、エディタで動作するまでの最短手順です。


## 必要環境

<div class="grid cards" markdown>

- :material-unreal:{ .lg .middle } &nbsp; **Unreal Engine 5.3 以降**

    ---

    UE 5.3 / 5.4 / 5.5 で動作確認済み。古いバージョンは未サポートです。

</div>

---

## プラグインを有効化

Unreal Editor を起動 → **Edit → Plugins** から **DataIndexer** を検索して有効化。プロンプトに従ってエディタを再起動します。

??? note "C++ プロジェクトの場合"

    C++ プロジェクトでソースから利用する場合は、有効化前に以下の手順が必要です。

    #### 1. プラグインを配置

    `DataIndexer` フォルダをプロジェクトの `Plugins/` ディレクトリ直下にコピーします。

    ``` { .text .no-copy }
    YourProject/
    ├── YourProject.uproject
    ├── Source/
    └── Plugins/
        └── DataIndexer/         ← プラグインルート
            ├── Source/
            ├── DataIndexer.uplugin
            └── ...
    ```

    #### 2. プロジェクトファイルを再生成

    === ":fontawesome-brands-windows: Windows"

        `.uproject` を右クリック → **Generate Visual Studio project files** を選択。

    === ":fontawesome-brands-apple: macOS"

        ターミナルでプロジェクトのルートディレクトリに移動し、`GenerateProjectFiles.command` を実行。

    === ":fontawesome-brands-linux: Linux"

        ```bash
        ./GenerateProjectFiles.sh
        ```

    #### 3. プロジェクトをビルド

    IDE でソリューションを開き、ビルド実行。以下の 2 モジュールが自動でコンパイルされます。

    | モジュール | タイプ | 役割 |
    | --- | --- | --- |
    | `DataIndexer` | Runtime | アセット型・PrimaryKey・Schema インターフェース・Blueprint ライブラリ |
    | `DataIndexerEd` | Editor (UncookedOnly) | 専用アセットエディタ・プロパティカスタマイズ・データビュー |

    #### モジュール依存関係の追加

    ゲームモジュールから Runtime API を使用するには、`.Build.cs` に `DataIndexer` を追加します。

    ```csharp title="YourGame.Build.cs" hl_lines="4"
    PublicDependencyModuleNames.AddRange(new string[]
    {
        "Core", "CoreUObject", "Engine",
        "DataIndexer",
    });
    ```

    !!! warning "エディタ専用モジュールに注意"
        `DataIndexerEd` は Runtime / ゲームモジュールから絶対に参照しないでください。`UncookedOnly` 宣言のため、パッケージビルドに含まれません。エディタ拡張からのみ依存可能です。

---

## トラブルシューティング

??? question "エディタ起動時に「プラグインがロードできません」と表示される"

    プロジェクトファイルの再生成（手順 2）とビルド（手順 3）が完了していない可能性があります。クリーンビルドを試してください。

??? question "アセット種別が Content Browser に表示されない"

    プラグインが有効化されていない可能性があります。**Edit → Plugins** で **DataIndexer** が ON になっているか確認し、必要に応じてエディタを再起動してください。

---

## 次のステップ

<div class="grid cards" markdown>

- :material-rocket-launch:{ .lg .middle } &nbsp; **[クイックスタート](quick-start.md)**

    ---

    最初の Schema・Repository を作成し、Blueprint と C++ から Row をクエリするまで 10 分。

- :material-book-open-variant:{ .lg .middle } &nbsp; **[コアコンセプト](concepts/index.md)**

    ---

    Repository / Schema / Keys & Handles / Indexes の関係を理解。

</div>
