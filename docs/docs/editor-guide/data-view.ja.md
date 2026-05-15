# Data View

Data View はリポジトリエディタの中央パネルです。行をグリッド形式で表示し、各カラムが行構造体のプロパティに対応します。

## 行の追加

![Insert キーで新規行を追加する Data View](../assets/images/data-view-add-row.png)

ツールバーの **Insert** をクリックするか、`Ctrl+N` を押します。以下の状態で新しい行が作成されます。

- 新たに生成された `FDataIndexerPrimaryKey`（GUID）
- 行構造体のすべてのプロパティがデフォルト構築された値

新しい行は Data View の末尾に表示されます。

## 行の削除

Data View で 1 つ以上の行を選択してから：

- `Delete` キーを押す、または
- 右クリック → **Delete**

!!! warning "削除は恒久的"
    行を削除するとそのプライマリキーが削除されます。参照されている行を削除しようとすると確認ダイアログが表示されます。参照ビューアボタンがアクティブな行は、削除前に参照先を確認してください。

## 行の編集

行の編集方法は 2 通りあります。

**インライン編集（Data View）**
: グリッドのセルを直接クリックします。単純なスカラープロパティ・enum・短い文字列はインライン編集に対応しています。

![インライン編集](../assets/images/feature-05.gif)

**Selection Details（フルエディタ）**
: 行をクリックして選択します。右の **Selection Details** パネルにフルプロパティエディタが表示されます。

![Selection Details パネル](../assets/images/data-view-selection-details.png)

## コンテキストメニュー

行を右クリックすると操作メニューが表示されます。

![行の右クリックメニュー](../assets/images/data-view-context-menu.png)

| 項目 | ショートカット | 説明 |
|------|--------------|------|
| **Copy ID** | — | 選択行のプライマリキー（GUID）をクリップボードにコピー |
| **Search this row** | — | この行のデータをグリッド内で検索 |
| **Copy** | `Ctrl+C` | 選択行をコピー |
| **Paste** | `Ctrl+V` | コピーした行を選択位置にペースト |
| **Duplicate** | `Ctrl+D` | 選択行を複製して末尾に追加 |
| **Insert** | `Ctrl+N` | 新しい行を追加 |
| **Delete** | `Delete` | 選択行を削除 |
| **MoveUp** | `Ctrl+K` | 選択行を 1 つ上に移動 |
| **MoveDown** | `Ctrl+J` | 選択行を 1 つ下に移動 |
| **Override** | `Shift+O` | 親リポジトリの行をこのリポジトリでオーバーライド |
| **Toggle** | `Shift+T` | DevOnlyRow フラグを切り替え |

行左端の **ドラッグハンドル**（`⠿` アイコン）をドラッグしても並び替えができます。

![ドラッグで行を並び替える](../assets/images/data-view-move-row.gif)

## Row Name

各行の先頭カラムに表示されるラベルです。DataTable の RowName とは異なり、このカラムは直接編集できません。

表示される値はスキーマで実装した `DisplayName` によって決まります。フォーマットは実装側で自由に定義できます（例: ID のゼロ埋め、複数プロパティの連結など）。

Row Name の重複は可能ですが、視認性が下がるため推奨しません。

## カラムレイアウト

どのプロパティをカラムとして表示するかはスキーマの **Expanded Struct Entries** 設定で制御されます。

- `RowStruct` の中で `ExpandedStructEntries` に列挙されているプロパティが個別カラムとして表示されます。
- ネスト構造体のプロパティは、その構造体型を `ExpandedStructEntries` に追加することで独立したカラムに展開できます。
- 展開セットに含まれないプロパティはグリッドで結合テキストセルとして表示されますが、Selection Details パネルでは完全に編集できます。

## フィルター

ツールバーの **Filters** ドロップダウンで表示する行・カラムを絞り込めます。

![Filters メニュー](../assets/images/data-view-filters.png)

### ROW フィルター

| オプション | 説明 |
|-----------|------|
| **Show Inherited Rows** | 親から継承しこのリポジトリで上書きした行について、上書き元の行を表示します。 |
| **Show Only Unreferred Rows** | どこからも参照されていない行のみ表示します。未使用データの整理に役立ちます。 |
| **Show Hidden Rows** | 親リポジトリで `Hidden in Children` に設定され非表示になった行を表示します。 |

### COLUMN フィルター

| オプション | 説明 |
|-----------|------|
| **Visibility** | 各行の Editor Flags 状態を示す Visibility カラムを表示/非表示 |
| **Reference Viewer** | 参照状態アイコンと参照ビューアボタンのカラムを表示/非表示 |
| **Properties** | 表示するプロパティカラムを個別に制御するサブメニュー。**Show All** で全カラムを一括表示。 |

## 継承行

継承行は子リポジトリでは編集できません — ダブルクリックで親アセットに移動します。

**Override**（`Shift+O`）を実行すると、親行の値をコピーした状態でこのリポジトリに行が追加され、独立して編集できるようになります。

## 参照ビューア

各行の左端に参照状態アイコンが表示されます。アイコンをクリックすると UE5 の **Reference Viewer** が開き、その行を参照しているアセットを一覧できます。

![Reference Viewer ボタン](../assets/images/data-view-reference-viewer.gif)

- アイコンが**アクティブ**な行 — いずれかのアセットから参照されています
- アイコンが**非アクティブ**な行 — どこからも参照されていません

!!! tip "データ整理に最適"
    **Filters → Show Only Unreferred Rows** と同様に、未使用の行を一覧して不要なデータを安全に削除するための整理作業に役立ちます。

## Editor Flags

Selection Details パネルの **Development** カテゴリに **Editor Flags** プロパティがあります。行ごとに開発用フラグをビットフラグで設定できます。

![Editor Flags ドロップダウン](../assets/images/data-view-editor-flags.png)

| フラグ | 説明 |
|-------|------|
| **Comment Out** | 行を無効化します。ゲームランタイムには含まれませんが、データは保持されます。一時的に除外したい行に使います。 |
| **Not Overridable in Children** | 子リポジトリがこの行を Override できなくなります。親で固定したい行に設定します。 |
| **Hidden in Children** | 子リポジトリの Data View にこの行を表示しません。子が誤って参照・編集するのを防ぎます。 |

複数フラグの同時設定が可能です。フラグなしの状態は **No Flags Set** と表示されます。

## カラム幅と並び順

カラムの幅と並び順はエディタ設定の **Schema Layouts** にスキーマごとに保存されます。カラムヘッダーをドラッグして並び替えられます。カラムヘッダー間の仕切りをドラッグしてリサイズできます。

![カラム幅の変更](../assets/images/data-view-column-resize.gif)

### Reset Layout

ツールバーの **Reset Layout** ボタンをクリックすると、このスキーマのカラム幅・並び順の保存データをすべてリセットします。

![Reset Layout 確認ダイアログ](../assets/images/data-view-reset-layout.png)

確認ダイアログで **Yes** を選択するとリセットが実行されます。スキーマのデフォルトカラム構成に戻り、幅・順序の変更はすべて失われます。
