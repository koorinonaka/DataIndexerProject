# Schema API

`UDataIndexerSchema`（`DataIndexerSchema.h`）はすべてのSchemaの抽象基底クラスです。行構造体・表示ロジック・IndexKey ビルダーを定義するために C++ でサブクラス化します。

## GetRowStruct

```cpp
const UScriptStruct& GetRowStruct() const;
```

行の形状を定義する `UScriptStruct` を返します。CDO の `RowStruct` UPROPERTY から自動で設定されます。C++ サブクラスでは `Super::PostInitProperties()` を呼び出して初期化を確保してください。

---

## GetRowDisplayName

```cpp
virtual FText GetRowDisplayName(
    const FDataIndexerPrimaryKey& PrimaryKey,
    const FConstStructView& RowEntity) const;
```

行の人間可読な表示名を返します。基底実装は `RowDisplayNameFunction` バインド（具象 row struct を直接受け取る関数）を解決して呼びます。C++ では `virtual` をオーバーライドして表示名を求め、`Super` を呼べばバインドにフォールバックします。

```cpp
FText UItemSchema::GetRowDisplayName(
    const FDataIndexerPrimaryKey& PrimaryKey,
    const FConstStructView& RowEntity) const
{
    if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
    {
        return Row->DisplayName;
    }
    return Super::GetRowDisplayName(PrimaryKey, RowEntity);
}
```

Blueprint からは、Schema の Class Defaults で `RowDisplayNameFunction` を `(const FDataIndexerPrimaryKey&, const FRowStruct&) → FText` のシグネチャを持つ関数にバインドします。row は具象 row struct で渡るため `Get Instanced Struct Value` でアンパックするノードは不要です。

---

## ForEachIndex

```cpp
void ForEachIndex(
    const TFunctionRef<void(const FDataIndexerIndex&)>& Callback) const;
```

登録されているすべてのIndexKeyを走査します（`BuildIndexFunctions` のキー）。`ReverseLookups` を構築する際にコンパイラがIndexを列挙するために使用します。

---

## BuildIndexCall

```cpp
TOptional<FGuid> BuildIndexCall(
    const FDataIndexerIndexKey& IndexKey,
    const FConstStructView& RowEntity) const;
```

指定した行エンティティに対して `IndexKey` の登録済みビルダー関数を呼び出します。算出したIndexKey（`FGuid`）を返します。このIndexにビルダーが登録されていない場合は `NullOpt` を返します。

保存時にコンパイラから呼び出されます。通常、ゲームコードから直接呼び出すことはありません。

---

## RegisterFunction_BuildIndex

```cpp
void RegisterFunction_BuildIndex(
    const FDataIndexerIndex& Index,
    FName FunctionName);
```

`Index` のIndexKey ビルダーとして名前付き関数を登録します。CDO から `PostInitProperties` で呼び出してください。

```cpp
void UItemSchema::PostInitProperties()
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        RowStruct = FItemRow::StaticStruct();
        RegisterFunction_BuildIndex(ByTypeIndex(),
            GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeIndex));
        RegisterFunction_BuildIndex(ByRarityIndex(),
            GET_FUNCTION_NAME_CHECKED(ThisClass, BuildRarityIndex));
    }
    Super::PostInitProperties();
}
```

関数は具象 row struct を直接受け取り IndexKey を返します：  
`(const FRowStruct& Row) → FGuid`

Blueprint からバインドする場合、Schema の details パネルが候補関数をこのシグネチャで絞り込み、「Create matching function」が `const FRowStruct&` 引数のスタブを生成します。row は具象 struct なので `Get Instanced Struct Value` ノードは不要です。

---

## CustomizePropertyCellWidget

```cpp
virtual TSharedRef<SWidget> CustomizePropertyCellWidget(
    DataIndexer::IPropertyWidgetContext& Context) const;
```

**（エディタ専用）** Data View のセルにカスタム Slate ウィジェットを提供するためにオーバーライドします。基底実装は登録された Blueprint ウィジェットカスタマイズに対して `CustomizePropertyWidgetCall` を呼び出し、なければシンプルなテキストウィジェットにフォールバックします。`Context` はオーバーライドが再利用できる部品（`CreateAsSimpleText()`、`CreateAsEditInline()`、`WrapUserWidget()`）を公開します。

---

## データバリデーション

```cpp
#if WITH_EDITOR
virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
virtual EDataValidationResult IsRowValid(
    FConstStructView RowEntity, FDataValidationContext& Context) const;
#endif
```

`IsRowValid` は **Validate Data**（`UEditorValidatorSubsystem` 経由）時に各行ごとに呼び出されます。各行のバリデーションロジックを追加するためにオーバーライドしてください。
