---
hide:
  - navigation
---

# 変更履歴

DataIndexer の注目すべき変更はすべてここに記録します。
フォーマットは [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) に準拠します。
バージョンは `DataIndexer.uplugin` の `VersionName` フィールドに対応します。

---

## v1.0.2 { .release .release--current data-date="2026-10-25" }

NotOverridable を Blueprint から設定できるようにし、行の検証と JSON インポートのコマンドを足したパッチリリース。Data View とコンパイルまわりの不具合も修正。

### 追加 { .release-group .release-group--add }

- Schema の **Column Layout** の NotOverridable ロック：User Defined Struct の各フィールドで、表示切り替えの隣に鍵アイコンを置いた。ロックしたフィールドは Override 行で読み取り専用になる。
- **Validate**：Data View の行コンテキストメニューのコマンド。選択行に Schema の行検証を実行し、行ごとの結果を **Asset Check** メッセージログに出力する。
- `UDataIndexerEditorFunctionLibrary::ImportRowsFromJSON`：DataIndexer 形式の JSON 文字列を Repository にインポートする、Blueprint から呼べる関数。Repository の行を置き換え、Undo できる。
- Selection Details が `Parent|Child` 形式のカテゴリ名から入れ子のサブカテゴリを作り、`ShowOnlyInnerProperties` に従うようになった。カテゴリのフィルタチップはルートのカテゴリに働く。

### 変更 { .release-group .release-group--change }

- **互換性のない変更：** `UDataIndexerSchema::IsRowValid` の第 1 引数に、行の `FDataIndexerPrimaryKey` が加わった。C++ のオーバーライドを新しいシグネチャに合わせる必要がある。
- 派生Repositoryの NotOverridable フィールドは、常に最も近い親Repositoryの値に従う。値はロード時・エディタを開いたとき・JSON インポート時・行の Override 時に解決され、派生アセットには保存されず、コンパイル時に焼き込まれる。メタデータの付いたサブ構造体は、Override 行で丸ごと読み取り専用になる。
- Data View のカラム幅を、Schema のセルが実際に描くテキスト（`CreateAsSimpleText` を含む）から決めるようにした。RowName カラムは自動サイズになり、手動のリサイズも保つ。オブジェクトのセルはアセット名を表示し、フルパスでも検索できる。
- Shift + マウスホイールで Data View を横スクロールできる。
- 行の検証結果の先頭に、その行の Repository へのリンクが付く。
- `AddRowFromView` / `UpdateRowFromView` が const の Repository を受け取れるようになった。

### 修正 { .release-group .release-group--fix }

- User Defined Struct フィールドの NotOverridable のチェックボックスが、構造体エディタに表示されていなかった。Schema の Column Layout に移した（「追加」を参照）。
- 菱形の親グラフで共通の祖先を経路ごとに走査し、その行が Data View に重複して表示されていた。各Repositoryを 1 回だけ走査するよう修正。
- フィルタで行が隠れているときや継承行が混ざっているときに、ドラッグで並び替えた行が違う位置に入っていた。
- 警告だけのコンパイル（Build Index 関数が未バインドの場合など）が失敗しなくなった。
- ある行が検証に失敗すると、それ以降の行もすべて Invalid と報告されていた。各行を、その行の Issue だけで判定するよう修正。
- DataTable JSON のインポートで、データを置き換える前に全要素を検証する。不正なファイルでアセットが中途半端にインポートされることはなくなった。
- セルのコピー＆ペーストで、値に含まれるタブと改行をエスケープするよう修正。
- Schema の RowStruct が Repository に保存された行と合わないとき、すべての編集コマンドが無効のままになっていた。Repository を開いたときに行のリセットを提案するよう修正。
- Blueprint のロード中に、カスタム K2 ノードのピンがワイルドカードに戻らなくなった。**Get Keys by Index** の `OutKeys` ピンの接続も修正。
- `Repository` メタデータが `TOptional` プロパティを通しても解決されるよう修正。
- Data View セルの Repository ピッカーが、共有サムネイルプールにフォールバックするよう修正。
- 行の挿入を Undo した直後の選択と、PrimaryKey コンボの選択解除で起きていたクラッシュを修正。
- 継承行を複製すると、コピーを先頭ではなく末尾に追加するよう修正。
- Data View の UI：エディタタブ名を「Data Indexer」に修正（従来は「State Tree」）。フィルハンドルのドラッグでセル選択を保つ。混在状態からの **Show All** ですべてのカラムを表示する。セル選択からでも **Duplicate** が働く。
- カラムレイアウトを読むだけでユーザー設定に項目が増えていた問題を修正。Selection Details のカテゴリ選択は、プロジェクトの Default 設定ではなくユーザー設定に保存する。

---

## v1.0.1 { .release data-date="2026-06-07" }

セルウィジェットのカスタマイズとインライン編集まわりの修正を中心としたパッチリリース。

### 追加 { .release-group .release-group--add }

- `DI_REGISTER_BUILD_INDEX`：Index ビルダーを登録するマクロ。ビルダーのシグネチャをコンパイル時に検証。
- 仮想列（`FDataIndexerVirtualColumn`、`UDataIndexerSchema::VirtualColumns`）：Data View に追加列を宣言可能に。`SourceProperty` 指定時は行プロパティをエイリアス（ネストは `"Inner.A"` のドット区切り）し、1 つの値を異なる見せ方の複数列として表示できる。空の場合は unbound 列となり、セルは schema が `CustomizePropertyCellWidget` で全面提供する。
- 列並び替え：Data View のカラムヘッダーをドラッグしてデータプロパティカラムおよび Virtual Column を並び替えられます。並び順は **Schema Layouts** にSchemaごとに保存されます。
- `FDataIndexerVirtualColumn::PreferredWidth`：仮想列の初期幅（ピクセル）を任意指定。未設定の場合は内容に合わせて自動サイズ。
- `RowValidationFunction`：Schema に設定できる任意の行ごとの検証フック。Blueprint（関数ライブラリ可）で割り当てられ、C++ の `IsRowValid` を補完。検証結果は `UDataIndexerRowValidationContext` 経由で報告されます。
- `UDataIndexerEditorFunctionLibrary` によるエディタ行 CRUD：Blueprint から呼べる **Add Row** / **Update Row** / **Delete Row(s)**。エディタのトランザクション（Undo 可能）ステージング層を介して書き込みます。
- `IDataIndexerInterface_CellContext`（`SetCellContext`）：このインターフェースを実装したセルウィジェットに、Data View が所属 Repository と PrimaryKey を注入。Blueprint で作った編集可能セルが **Update Row** で編集を書き戻せるようになります。

### 変更 { .release-group .release-group--change }

- `CustomizePropertyCellWidget` のカスタマイズ関数が行の PrimaryKey を受け取れるようになり、行ごとに表示を切り替え可能に。

### 修正 { .release-group .release-group--fix }

- `CustomizePropertyCellWidget` で UMG ウィジェットを生成するカスタマイズ関数が正しく表示されるよう修正（従来は World コンテキストを解決できず失敗していた）。
- 検証エラーで、原因の種類と対象カラム名を表示するよう改善（従来は汎用メッセージのみ）。
- 無効な行やスキーマ未設定の行でも、行の表示名が正しく解決されるよう修正。
- インライン（ダブルクリック）セル編集の配置・余白が正しく表示されるよう修正。

---

## v1.0.0 { .release data-date="2026-05-24" }

初回公開リリース。

### 追加 { .release-group .release-group--add }

**検索**

- `FDataIndexerPrimaryKey`（GUID ベース）・`FDataIndexerRowHandle`・`FDataIndexerIndex`：コア識別型。
- `DI_DEFINE_INDEX`：クラスパスと名前から安定した GUID を生成して Secondary Index を宣言。
- `ForEachPrimaryKeys(Index, Key, ...)`：逆引きテーブルを直参照する O(matches) 走査。全行スキャンなし。
- `FDataIndexerKeysHandle`：Blueprint 側の Index クエリ ハンドル。使用時に一致する PrimaryKey を解決する。

**オーサリング**

- 3 パネル構成のカスタムエディタ：行リスト・詳細フォーム・カラム設定。
- JSON サイドカーのエクスポート / インポートと、オンディスクサイドカーとのインライン差分ビュー。
- `UDataIndexerDrivenCollection`：行ごとのサブアセット（アイコン・メッシュ・Ability クラスなど）を管理するコンパニオンアセット。ソース Repository の保存時に自動再ビルド。
- `IsRowValid`：`UDataIndexerSchema` の C++ バリデーション フック。**Validate Data** の実行・アセット保存・クック時に動作し、エラーがあると処理をブロック。
- プロパティ メタデータ：`meta=(Repository=...)` で行ピッカーを紐付け、`meta=(Schema=...)` でアセット ピッカーをフィルタリング、`meta=(ReadOnlyKeys)` で読み取り専用ピッカーを指定。
- 行ごとのエディタ フラグ：`Comment Out`・`Not Overridable in Children`・`Hidden in Children`。
- 行とタブ区切りセル範囲のコピー / ペースト。
- Reference Viewer カラム、Schema ごとのカラム レイアウト保存、**Show Inherited Rows** / **Show Only Unreferred Rows** フィルター。

**構造**

- `UDataIndexerRepository`：マルチレベルの親合成と行単位のオーバーライド制御を備えたデータアセット。
- `UDataIndexerSchema`：行構造体・表示ロジック・Index ビルダーを定義する Blueprint サブクラス化可能なコントラクトクラス。

**統合**

- `TNativeSchemaInterface<T>`：`FindRow`・`ForEachPrimaryKeys`・`GetPrimaryKeys` をまとめた型付き C++ アクセサ テンプレート。
- **Get Row** K2 ノード：`FDataIndexerRowHandle` から型付き行データを取得。成功・失敗の実行ピンで分岐。
- **Get Keys by Index** K2 ノード：`FDataIndexerKeysHandle` とクエリ構造体のペアから一致する PrimaryKey を取得。
- **Select by Primary Key** K2 ノード：行の Identity に基づいて値を選択する Pure ノード。
- **Switch on Primary Key** K2 ノード：行の Identity に基づいて実行フローをディスパッチ。
