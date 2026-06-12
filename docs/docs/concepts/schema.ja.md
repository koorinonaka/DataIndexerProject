# Schema

`UDataIndexerSchema` は Repository とエディタ動作の間のコントラクトです。抽象 `UObject` サブクラスであり、直接インスタンス化せず、特定のデータ型を定義するために Blueprint または C++ でサブクラス化します。

## 役割

Schema は 3 つのことを担当します。

1. **行構造体の宣言** — `RowStruct` は Repository の `LocalEntries` に格納される構造体型を識別する `TObjectPtr<const UScriptStruct>` です。
2. **表示ロジックの提供** — `GetRowDisplayName` はバインドされた **Row Display Name Function**（または C++ オーバーライド）を解決し、任意の行に対して人間可読なラベルを返します。エディタ UI 全体で使用されます。
3. **拡張関数の登録** — Index キー生成・プロパティのセルウィジェットをカスタマイズする関数を、実際の row struct を直接受け取る Blueprint または C++ の名前付き関数としてバインドします。

## サブクラス化

=== "C++"

    ### 宣言と初期化

    `RowStruct` はコンストラクタで設定します。

    ```cpp
    UCLASS()
    class UItemSchema : public UDataIndexerSchema
    {
        GENERATED_BODY()

    public:
        UItemSchema();

        DI_DEFINE_INDEX(ByTypeIndex);
        DI_DEFINE_INDEX(ByRarityIndex);

    protected:
        virtual TOptional<FText> GetRowDisplayName(
            const FDataIndexerPrimaryKey& PrimaryKey,
            const FConstStructView& RowEntity) const override;

        UFUNCTION()
        static FGuid BuildTypeIndex(const FItemRow& Row);

        UFUNCTION()
        static FGuid BuildRarityIndex(const FItemRow& Row);
    };
    ```

    ```cpp
    UItemSchema::UItemSchema()
    {
        RowStruct = FItemRow::StaticStruct();

        DI_REGISTER_BUILD_INDEX( ByTypeIndex(),   FItemRow, BuildTypeIndex );
        DI_REGISTER_BUILD_INDEX( ByRarityIndex(), FItemRow, BuildRarityIndex );
    }
    ```

    `DI_DEFINE_INDEX` で宣言したIndexごとに `DI_REGISTER_BUILD_INDEX` を呼び出し、ビルダー関数を紐付けます。マクロはビルダーのシグネチャ（`FGuid( const RowType& )`）をコンパイル時に検証します。

    ### GetRowDisplayName

    `GetRowDisplayName` の `virtual` をオーバーライドして行の表示名を返します。`RowEntity` は `FConstStructView` なので、1 行で実際の row struct にアンパックしてフィールドを返します。

    ```cpp
    TOptional<FText> UItemSchema::GetRowDisplayName(
        const FDataIndexerPrimaryKey& PrimaryKey,
        const FConstStructView& RowEntity) const
    {
        return RowEntity.Get<const FItemRow>().DisplayName;
    }
    ```

    ### Build Index Functions

    `DI_DEFINE_INDEX` でIndexを宣言し、対応する `static UFUNCTION` をビルダーとして実装します。ビルダーは**実際の row struct** を直接受け取ります（[Index](indexes.md) 参照）。

    ```cpp
    FGuid UItemSchema::BuildTypeIndex(const FItemRow& Row)
    {
        return FGuid( static_cast<uint32>( Row.Type ), 0, 0, 0 );
    }
    ```

=== "Blueprint"

    ### Row Struct の設定

    1. 親クラスに `DataIndexerSchema` を指定して **Blueprint Class** を作成する
    2. **Class Defaults** で **Row Struct** を使用する `USTRUCT` に設定する

    ### GetRowDisplayName

    **Class Defaults** で **Row Display Name Function** を、行構造体のフィールドから意味のある `FText` を返す関数にバインドします。関数は**実際の row struct** を直接受け取るため `Get Instanced Struct Value` ノードは不要です。このラベルはエディタ UI 全体の行一覧・ピッカーで使用されます。

    ### Build Index Functions

    **Class Defaults** の **Build Index Functions** マップでIndex ビルダーを登録します。キーはIndex、値は**実際の row struct** を受け取り `FGuid` を返す関数です。ピッカーは一致する関数のみ絞り込み、「Create matching function」は実際の row 引数のスタブを生成します（[Index](indexes.md) 参照）。

    ### Property Cell Widget Customizations

    **Class Defaults** の **Property Widget Customizations** マップで、プロパティ名をキーに `UUserWidget*` を返す関数を登録します。Data View グリッドのセル表示をカスタマイズできます。

    カスタマイズ関数が `nullptr` を返した場合は、デフォルトのセル表示にフォールバックします。

## データバリデーション

検証は次のタイミングで自動的に実行されます。

- Content Browser で **右クリック → Validate Data** を選択したとき
- アセット保存時（エディタ設定で **Save Validation** を有効にした場合）
- クック時

### C++

`IsRowValid` をオーバーライド（エディタ専用）して各行のバリデーションロジックを追加します。戻り値が `EDataValidationResult::Invalid` の場合、`Context` に追加したエラーメッセージがエディタに表示され、保存・クックがブロックされます。

```cpp
#if WITH_EDITOR
EDataValidationResult UItemSchema::IsRowValid(
    FConstStructView RowEntity, FDataValidationContext& Context) const
{
    if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
    {
        if (Row->BaseValue < 0)
        {
            Context.AddError(NSLOCTEXT("Item", "BadValue", "BaseValue must be >= 0"));
            return EDataValidationResult::Invalid;
        }
    }
    return EDataValidationResult::Valid;
}
#endif
```

### Blueprint

Schema の Class Defaults で **RowValidationFunction** を `(const FRowStruct&, UDataIndexerRowValidationContext*)` シグネチャの関数にバインドします。Details パネルの **Create matching function** で正しいパラメータのスタブを自動生成できます。

関数内では Context オブジェクトの `AddError` または `AddWarning` を呼び出して問題を報告します。

- `AddError(FText)` — 行を無効としてマークし、エディタにメッセージを表示します。
- `AddWarning(FText)` — 保存・クックをブロックせずに警告を報告します。

| 結果 | バリデーション結果 |
|---|---|
| Error が1件以上 | `EDataValidationResult::Invalid` — 保存・クックがブロックされます |
| Warning のみ | `EDataValidationResult::Valid` — 警告はエディタに表示されますがブロックなし |
| Error/Warning ともになし | `EDataValidationResult::Valid` |

!!! note "シグネチャ検証"
    バインドした関数のシグネチャが不正（パラメータ型・数の不一致、または void 以外の戻り値）の場合、Schema アセットの検証時に `IsDataValid` がエラーを報告します。

## カラムレイアウト（ExpandedStructEntries）

`ExpandedStructEntries` は Data View グリッドでネスト構造体のどのプロパティを個別カラムとして表示するかを制御します。デフォルトでは `RowStruct` のすべてのトップレベルプロパティがカラムとして表示されます。

C++ で `InitializeExpandedStructEntries` をオーバーライドしてプログラムから設定できます。

```cpp
void UItemSchema::InitializeExpandedStructEntries()
{
    Super::InitializeExpandedStructEntries();

    // SomeField をトップレベルから除外（インライン表示に戻す）
    if (FDataIndexerExpandedStructEntry* Entry = ExpandedStructEntries.Find(RowStruct))
    {
        *Entry -= GET_MEMBER_NAME_CHECKED(FItemRow, SomeField);
    }

    // ネスト構造体のプロパティをカラムに展開
    ExpandedStructEntries.FindOrAdd(FMyInnerStruct::StaticStruct()) += {
        GET_MEMBER_NAME_CHECKED(FMyInnerStruct, A),
        GET_MEMBER_NAME_CHECKED(FMyInnerStruct, B),
    };
}
```

Blueprint では Class Defaults パネルの **Expanded Struct Entries** マップから切り替えられます。

![カラムごとの表示・非表示を切り替える Column Layout パネル](../assets/images/schema-column-layout.png)
