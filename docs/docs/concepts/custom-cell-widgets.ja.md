# Custom Cell Widgets

Data View は行の各プロパティをセルとして描画する。Schema はデフォルトのセルウィジェットを差し替えて、
カスタム表示やインライン編集を提供できる。方法は 2 つあり、`FCharacterRow` 上で並べて示す。どちらも
**編集可能**だが、書き戻しの経路が異なる。

| 列 | 手法 | 書き戻し経路 | 実装場所 |
| --- | --- | --- | --- |
| `PawnClass1` | C++ `CreateAsEditInline` | C++ context — 行メモリを直接編集 | `UCharacterSchema` の C++ override |
| `PawnClass2` | Blueprint `PropertyWidgetCustomizations` + `USinglePropertyView` をホストする UserWidget | Blueprint `UpdateRow` — 注入された repository context 経由 | `DIS_Character` Blueprint + `WBP_PawnClassCell`、Schema C++ なし |

どちらも同じ入口 `CustomizePropertyCellWidget` をカスタマイズするが、到達経路が異なる。両者とも同じ
エディタ staging 層 (`UDataIndexerEditorData`) を通して永続化するので、編集はトランザクション扱い＝
Undo 可能。

## 入口

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
  コンテキストアクセサ。

---

## Sample 1 — C++ によるインライン編集（`PawnClass1`）

C++ Schema で `CustomizePropertyCellWidget` を override し、対象列で `CreateAsEditInline` を返す。
`CreateAsEditInline` は行メモリ上に実プロパティエディタを構築するため、編集が永続する。

```cpp title="CharacterSchema.cpp"
TSharedRef<SWidget> UCharacterSchema::CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const
{
    if (const FName Col = GET_MEMBER_NAME_CHECKED(FCharacterRow, PawnClass1);
        Context.GetColumnName() == Col)
    {
        return Context.CreateAsEditInline(Context.GetProperty(), /*bDisplayDefaultPropertyButtons=*/true);
    }

    return Super::CustomizePropertyCellWidget(Context);
}
```

`PawnClass1` は `TSubclassOf<APawn>`（単一の object/class プロパティ）なので、セルにはインラインの
class picker が表示される。`CreateAsEditInline` は任意の単一プロパティと、`IPropertyTypeCustomization`
が登録された struct に対応する。コンテナプロパティ（`TArray`/`TMap`/`TSet`）は対象外。

`bDisplayDefaultPropertyButtons`（デフォルト `true`）は標準のプロパティボタン — reset-to-default /
browse / use-selected — を値エディタの横に付け、Details パネルの 1 行と同じ操作感にする。`false` を
渡すと値エディタ単体になる。（内部では `CreatePropertyValueWidgetWithCustomization` を使う。これは型
customization を維持する代わりにボタンを付けないので、`CreateDefaultPropertyButtonWidgets` で別途
付け直している。）

!!! note
    `UCharacterSchema` の C++ override は、通常の仮想ディスパッチにより Blueprint サブクラス
    `DIS_Character` にも適用される。Sample 1 で Blueprint を触る必要はない。

---

## Sample 2 — `PropertyWidgetCustomizations` 経由の編集可能 UserWidget（`PawnClass2`）

この経路は Schema 側に C++ を要さない。Blueprint 関数がセル用の `UserWidget` を返し、Schema の
`PropertyWidgetCustomizations` マップでバインドする。ウィジェットは `USinglePropertyView` をホストし、
`UpdateRow` で編集を行へ書き戻す。

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

### Step 1 — Editor Utility Widget（`WBP_PawnClassCell`）

1. `/Game/GameData/CustomWidget/WBP_PawnClassCell` に Editor Utility Widget Blueprint を作成
   （親 `EditorUtilityWidget`）。
2. **Single Property View** を root に配置。
3. 変数を追加:
    - `PawnClass : TSubclassOf<APawn>` — 表示・編集する値。Instance Editable + **Expose on Spawn**。
    - `Row : FCharacterRow` — 書き戻し用のフル行キャッシュ。Instance Editable + **Expose on Spawn**。
    - `Repository : UDataIndexerRepository` と `PrimaryKey : FDataIndexerPrimaryKey` — interface で設定。
4. **Class Settings → Interfaces** で `IDataIndexerInterface_CellContext` を実装。

### Step 2 — `SetCellContext` で初期化

`SetCellContext` はバインド関数が `PawnClass` / `Row` を設定した**後**に発火する。そのため、view を
実値へ向けるのはここが適切（`Pre Construct` だと spawn 時の値設定と競合し得る）。

- `Repository` と `PrimaryKey` を保存。
- `Single Property View → Set Object (Self)` → `Set Property Name ("PawnClass")`。
- `Bind Event to On Property Changed` → カスタムハンドライベント。

### Step 3 — 編集時に書き戻す

`On Property Changed` ハンドラ内で:

1. キャッシュした `Row` の `PawnClass2` を編集後の `PawnClass` で差し替えて再構築（Break + Make
   `CharacterRow`）。
2. `Update Row (Repository, PrimaryKey, Row)`（`UDataIndexerEditorFunctionLibrary::UpdateRow`）を呼ぶ。
   これは `UDataIndexerEditorData` をトランザクション内で更新するので、Sample 1 のインライン編集と同様に
   Undo 可能。

### Step 4 — `DIS_Character` のバインド関数

セルウィジェットのシグネチャ
`(const FDataIndexerPrimaryKey&, const FCharacterRow&) → UUserWidget*` の関数を追加する。

1. クラス `WBP_PawnClassCell` を `Create Widget`。行の `PawnClass2` をウィジェットの `PawnClass` ピンに、
   行全体を `Row` ピンに渡す（どちらも Expose on Spawn）。
2. ウィジェットを返す。

これは `EUW_CharacterClassIcon` を返す既存の `GetPropertyWidget` 関数と同じ形。

### Step 5 — 列にバインド

`DIS_Character` の Class Defaults → **Property Widget Customizations** でエントリを追加: キー
`PawnClass2`、関数 `GetPawnClass2Widget`。これで `PawnClass2` のセルは編集可能な `SinglePropertyView`
を描画し、編集は `UpdateRow` で永続化する。

!!! note "2 つのセルの見た目が違う理由"
    `PawnClass1` は class picker の値ウィジェット（と標準プロパティボタン）だけを表示する。`PawnClass2`
    は左側にプロパティ名ラベルが出る。これは `USinglePropertyView` が値ウィジェット単体ではなく、名前
    **と**値からなる完成した単一プロパティ行を描画するため。
