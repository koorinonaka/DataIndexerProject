# Custom Cell Widgets

Data View は行の各プロパティをセルとして描画する。Schema はデフォルトのセルウィジェットを差し替えて、
カスタム表示やインライン編集を提供できる。さらに **仮想列 (virtual column)** — 既存プロパティを別名で
エイリアスする追加列 — を宣言でき、*同じ*値を異なる見せ方で複数回出せる。

`FCharacterRow` はこの両方を同時に示す。単一の `PawnClass` フィールド（`TSubclassOf<APawn>`）を持ち、
`UCharacterSchema` がそれを **2 つの仮想列** として出す。どちらも **編集可能**だが、書き戻しの経路が
異なる。

| 列 | 手法 | 書き戻し経路 | 宣言場所 |
| --- | --- | --- | --- |
| `PawnClassInline` | C++ `CreateAsEditInline` | C++ context — 行メモリを直接編集 | `UCharacterSchema` の C++ |
| `PawnClassWidget` | Blueprint `PropertyWidgetCustomizations` + `USinglePropertyView` をホストする UserWidget | Blueprint `UpdateRow` — 注入された repository context 経由 | `DIS_Character` Blueprint + `WBP_PawnClassCell` |

どちらの列も **同じ** `PawnClass` プロパティをエイリアスするので、片方を編集するともう片方の表示も
更新される。両者とも同じエディタ staging 層 (`UDataIndexerEditorData`) を通して永続化するので、編集は
トランザクション扱い＝Undo 可能。生の `PawnClass` プロパティ自体は Data View から隠され、2 つの仮想列
だけが表示される。

---

## 仮想列

Data View は通常、行 struct のチェック済み `FProperty` ごとに 1 列を生成する。Schema はこれに加えて、
任意の `ColumnName` で描画する **仮想列** を追加できる。各列は `FDataIndexerVirtualColumn`:

```cpp
USTRUCT()
struct FDataIndexerVirtualColumn
{
    FName ColumnName;     // 一意・安定な列 ID（layout/visibility キー、ルーティングキー）
    FText DisplayName;    // ヘッダラベル。空なら ColumnName にフォールバック
    FName SourceProperty; // 任意。エイリアスする RowStruct メンバ。ネストは "Inner.A"。空 = unbound
};
```

Schema は `VirtualColumns` 配列でこれを公開する。Schema Blueprint の Class Defaults パネルで編集でき、
C++ から設定してもよい。仮想列はプロパティ由来の列の **後** に追加される。`SourceProperty` により2種類:

- **エイリアス列**（`SourceProperty` あり）— その RowStruct メンバをエイリアスし、`FProperty` を再利用するので
  セル・書き戻し・grid 値操作（copy/paste・fill・既定表示）が既存経路でそのまま動く。`ColumnName` と
  `DisplayName` だけが異なる。ドット区切り（`"Inner.A"`）でネスト struct メンバを指定できる。
- **unbound 列**（`SourceProperty` 空）— 紐づくプロパティを持たない「真の」仮想列。セルは schema が
  `CustomizePropertyCellWidget` で全面提供する（読み取り専用/計算表示、またはカスタムバインド widget）。
  grid 値操作は**持たない** — copy/paste/fill/既定表示は対象外。

（空でない）`SourceProperty` が解決できない、または `ColumnName` が既存列と衝突する仮想列は、警告とともに
スキップされる。

!!! note "生列を隠す"
    あるプロパティを仮想列*だけ*で見せたい場合は、`InitializeExpandedStructEntries` で行 struct の
    expanded set から除外する（`DisplayName` を隠すのと同じパターン）:

    ```cpp
    if (FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find(RowStruct))
    {
        *RowStructEntry -= {
            GET_MEMBER_NAME_CHECKED(FCharacterRow, DisplayName),
            GET_MEMBER_NAME_CHECKED(FCharacterRow, PawnClass),
        };
    }
    ```

---

## セルの入口

```cpp
virtual TSharedRef<SWidget> CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const;
```

base 実装はその列の Blueprint `PropertyWidgetCustomizations` バインドを探し、無ければ単純なテキスト
ウィジェットにフォールバックする。`Context`（`IPropertyWidgetContext`）は override が再利用する
ビルディングブロックを公開する。

- `CreateAsSimpleText(Text)` — 読み取り専用のテキストラベル。
- `CreateAsEditInline(Property, bDisplayDefaultPropertyButtons = true)` — 行に紐づくインライン
  プロパティエディタ。書き戻しあり。
- `WrapUserWidget(UserWidget)` — UMG `UserWidget` をセルにホストする。
- `GetColumnName()` / `GetProperty()` / `GetRow<FRowType>()` / `GetPrimaryKey()` / `GetRepository()` —
  コンテキストアクセサ。仮想列の場合 `GetProperty()` はエイリアス先の `SourceProperty` を返す。

---

## Sample 1 — C++ によるインライン編集（`PawnClassInline`）

仮想列を C++ Schema のコンストラクタで宣言し、`CustomizePropertyCellWidget` でその列に対して
`CreateAsEditInline` を返す。セルのプロパティはエイリアス先の `PawnClass` に解決されるため、
`CreateAsEditInline` は行メモリ上に実プロパティエディタを構築し、編集が永続する。

```cpp title="CharacterSchema.cpp"
namespace
{
const FName PawnClassInlineColumn(TEXT("PawnClassInline"));
}

UCharacterSchema::UCharacterSchema()
{
    // ...
#if WITH_EDITORONLY_DATA
    FDataIndexerVirtualColumn& InlineColumn = VirtualColumns.AddDefaulted_GetRef();
    InlineColumn.ColumnName = PawnClassInlineColumn;
    InlineColumn.DisplayName = NSLOCTEXT("CharacterSchema", "PawnClassInlineColumn", "Pawn (C++)");
    InlineColumn.SourceProperty = GET_MEMBER_NAME_CHECKED(FCharacterRow, PawnClass);
#endif
}

TSharedRef<SWidget> UCharacterSchema::CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const
{
    if (Context.GetColumnName() == PawnClassInlineColumn)
    {
        // GetProperty() はエイリアス先の PawnClass に解決されるので、エディタはそこへ書き戻す。
        return Context.CreateAsEditInline(Context.GetProperty(), /*bDisplayDefaultPropertyButtons=*/true);
    }

    return Super::CustomizePropertyCellWidget(Context);
}
```

`PawnClass` は `TSubclassOf<APawn>`（単一の object/class プロパティ）なので、セルにはインラインの
class picker が表示される。`CreateAsEditInline` は任意の単一プロパティと、`IPropertyTypeCustomization`
が登録された struct に対応する。コンテナプロパティ（`TArray`/`TMap`/`TSet`）は対象外。

`bDisplayDefaultPropertyButtons`（デフォルト `true`）は標準のプロパティボタン — reset-to-default /
browse / use-selected — を値エディタの横に付け、Details パネルの 1 行と同じ操作感にする。`false` を
渡すと値エディタ単体になる。（内部では `CreatePropertyValueWidgetWithCustomization` を使う。これは型
customization を維持する代わりにボタンを付けないので、`CreateDefaultPropertyButtonWidgets` で別途
付け直している。）

!!! note
    `UCharacterSchema` の C++ 実装は、通常の仮想ディスパッチにより Blueprint サブクラス
    `DIS_Character` にも適用される。Sample 1 で Blueprint を触る必要はない。

---

## Sample 2 — `PropertyWidgetCustomizations` 経由の編集可能 UserWidget（`PawnClassWidget`）

この経路は Schema 側に C++ を要さない。仮想列は `DIS_Character` の Class Defaults で宣言し、Blueprint
関数がセル用の `UserWidget` を返して、Schema の `PropertyWidgetCustomizations` マップでバインドする。
ウィジェットは `USinglePropertyView` をホストし、`UpdateRow` で編集を行へ書き戻す。

!!! info "なぜ Editor Utility Widget か"
    `USinglePropertyView` はエディタ専用モジュール `ScriptableEditorWidgets` に属するため、その
    `SetObject` / `SetPropertyName` は **Editor Utility Widget** からしか呼べない。`WBP_PawnClassCell` の
    親クラスは `UserWidget` ではなく `EditorUtilityWidget` にする（`UUserWidget*` として返せる）。

### セルが repository を得る仕組み

セルの customization 関数には `(PrimaryKey, Row)` しか渡らず、`UpdateRow` が要求する所属 repository は
来ない。プラグインがこれを橋渡しする。Data View はウィジェット生成後、Blueprint interface
`IDataIndexerInterface_CellContext` 経由でエディタ context を注入する。

```cpp
// DataIndexer モジュール — 編集可能なセルウィジェットに実装する。
void SetCellContext(UDataIndexerRepository* Repository, const FDataIndexerPrimaryKey& PrimaryKey);
```

`UDataIndexerSchema::CustomizePropertyCellWidget` は、生成直後のウィジェットがこの interface を実装して
いれば `SetCellContext` を呼ぶ（`Context.GetRepository()` と `Context.GetPrimaryKey()` を渡す）。これに
より、関数シグネチャを変えずに任意のセルウィジェットが書き戻しに参加できる。

### Step 1 — 仮想列を宣言

`DIS_Character` の Class Defaults → **Virtual Columns** でエントリを追加:

- `ColumnName` = `PawnClassWidget`
- `DisplayName` = `Pawn (BP)`
- `SourceProperty` = `PawnClass`

### Step 2 — Editor Utility Widget（`WBP_PawnClassCell`）

1. `/Game/GameData/CustomWidget/WBP_PawnClassCell` に Editor Utility Widget Blueprint を作成
   （親 `EditorUtilityWidget`）。
2. **Single Property View** を root に配置。
3. 変数を追加:
    - `PawnClass : TSubclassOf<APawn>` — 表示・編集する値。Instance Editable + **Expose on Spawn**。
    - `Row : FCharacterRow` — 書き戻し用のフル行キャッシュ。Instance Editable + **Expose on Spawn**。
    - `Repository : UDataIndexerRepository` と `PrimaryKey : FDataIndexerPrimaryKey` — interface で設定。
4. **Class Settings → Interfaces** で `IDataIndexerInterface_CellContext` を実装。

### Step 3 — `SetCellContext` で初期化

`SetCellContext` はバインド関数が `PawnClass` / `Row` を設定した**後**に発火する。そのため、view を
実値へ向けるのはここが適切（`Pre Construct` だと spawn 時の値設定と競合し得る）。

- `Repository` と `PrimaryKey` を保存。
- `Single Property View → Set Object (Self)` → `Set Property Name ("PawnClass")`。
- `Bind Event to On Property Changed` → カスタムハンドライベント。

### Step 4 — 編集時に書き戻す

`On Property Changed` ハンドラ内で:

1. キャッシュした `Row` の `PawnClass` を編集後の `PawnClass` で差し替えて再構築（Break + Make
   `CharacterRow`）。
2. `Update Row (Repository, PrimaryKey, Row)`（`UDataIndexerEditorFunctionLibrary::UpdateRow`）を呼ぶ。
   これは `UDataIndexerEditorData` をトランザクション内で更新するので、Sample 1 のインライン編集と同様に
   Undo 可能。

### Step 5 — `DIS_Character` のバインド関数

セルウィジェットのシグネチャ
`(const FDataIndexerPrimaryKey&, const FCharacterRow&) → UUserWidget*` の関数を追加する。

1. クラス `WBP_PawnClassCell` を `Create Widget`。行の `PawnClass` をウィジェットの `PawnClass` ピンに、
   行全体を `Row` ピンに渡す（どちらも Expose on Spawn）。
2. ウィジェットを返す。

### Step 6 — 列にバインド

`DIS_Character` の Class Defaults → **Property Widget Customizations** でエントリを追加: キー
`PawnClassWidget`（仮想列の `ColumnName`）、関数 `GetPawnClassWidget`。これで `PawnClassWidget` の
セルは編集可能な `SinglePropertyView` を描画し、編集は `UpdateRow` で永続化する。

!!! note "2 つのセルの見た目が違う理由"
    `PawnClassInline` は class picker の値ウィジェット（と標準プロパティボタン）だけを表示する。
    `PawnClassWidget` は左側にプロパティ名ラベルが出る。これは `USinglePropertyView` が値ウィジェット
    単体ではなく、名前**と**値からなる完成した単一プロパティ行を描画するため。
