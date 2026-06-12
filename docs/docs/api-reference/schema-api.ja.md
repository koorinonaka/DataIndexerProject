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
virtual TOptional<FText> GetRowDisplayName(
    const FDataIndexerPrimaryKey& PrimaryKey,
    const FConstStructView& RowEntity) const;
```

行の人間可読な表示名を返します。利用できなければ `NullOpt` を返します。基底実装は `RowDisplayNameFunction` バインド（実際の row struct を直接受け取る関数）を解決して呼びます。C++ では `virtual` をオーバーライドして表示名を求めます — `RowEntity` を concrete row struct に unpack してフィールドを返します。

```cpp
TOptional<FText> UItemSchema::GetRowDisplayName(
    const FDataIndexerPrimaryKey& PrimaryKey,
    const FConstStructView& RowEntity) const
{
    return RowEntity.Get<const FItemRow>().DisplayName;
}
```

Blueprint からは、Schema の Class Defaults で `RowDisplayNameFunction` を `(const FDataIndexerPrimaryKey&, const FRowStruct&) → FText` のシグネチャを持つ関数にバインドします。row は実際の row struct で渡るため `Get Instanced Struct Value` でアンパックするノードは不要です。

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

## DI_REGISTER_BUILD_INDEX

```cpp
DI_REGISTER_BUILD_INDEX(Index, RowType, Func);
```

`Index` のIndexKey ビルダーとして `Func` を登録します。マクロは `DataIndexerSchema.h` で提供されます。Schema のコンストラクタで、Indexごとに呼び出してください。

```cpp
UItemSchema::UItemSchema()
{
    RowStruct = FItemRow::StaticStruct();
    DI_REGISTER_BUILD_INDEX(ByTypeIndex(),   FItemRow, BuildTypeIndex);
    DI_REGISTER_BUILD_INDEX(ByRarityIndex(), FItemRow, BuildRarityIndex);
}
```

`Func` は Schema クラスの `static UFUNCTION` で、実際の row struct を直接受け取り IndexKey を返すシグネチャである必要があります：  
`(const FRowStruct& Row) → FGuid`

マクロはこのシグネチャを **コンパイル時** に `static_assert` で検証します。戻り値は `FGuid`、row は `const` 参照渡し、関数の row 型は `RowType` 引数と一致している必要があります。不整合はエディタ検証の警告ではなくコンパイルエラーになります。

!!! warning "非推奨オーバーロード"
    旧来の `void RegisterFunction_BuildIndex(const FDataIndexerIndex&, FName)` オーバーロードは非推奨です。名前でのバインドは行いますが、コンパイル時のシグネチャ検証はありません。`DI_REGISTER_BUILD_INDEX` へ移行してください。

Blueprint からバインドする場合、Schema の details パネルが候補関数をこのシグネチャで絞り込み、「Create matching function」が `const FRowStruct&` 引数のスタブを生成します。row は実際の struct なので `Get Instanced Struct Value` ノードは不要です。

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

`IsRowValid` は **Validate Data**（`UEditorValidatorSubsystem` 経由）時に各行ごとに呼び出されます。C++ でオーバーライドするか、Blueprint から `RowValidationFunction` をバインドして各行のバリデーションロジックを追加します。

---

## RowValidationFunction

```cpp
#if WITH_EDITORONLY_DATA
UPROPERTY( EditDefaultsOnly, Category = Functions, meta = ( AllowFunctionLibraries ) )
FMemberReference RowValidationFunction;
#endif
```

Schema の Class Defaults パネルから設定できる任意の行バリデーションフックです。バインドする関数は次のシグネチャを持つ必要があります。

```
void FunctionName(const FRowStruct& Row, UDataIndexerRowValidationContext* Context)
```

バインドが設定されているとき、基底 `IsRowValid` はすべての行に対してこの関数を呼び出します。エラーと警告は `FDataValidationContext` に転送されます。`IsDataValid` はアセット検証時にシグネチャを検証し、不整合があればエラーを報告します。

---

## UDataIndexerRowValidationContext

`UDataIndexerRowValidationContext`（`DataIndexerRowValidationContext.h`）は、`RowValidationFunction` にバインドした関数の第2引数として渡されるトランジェントな `BlueprintType` オブジェクトです。Blueprint からエラーや警告を報告するために使用します。

| 関数 | 説明 |
|---|---|
| `AddError(FText Error)` | エラーを追加します。行が `Invalid` として報告され、保存・クックがブロックされます。 |
| `AddWarning(FText Warning)` | 警告を追加します。保存・クックはブロックされません。 |
