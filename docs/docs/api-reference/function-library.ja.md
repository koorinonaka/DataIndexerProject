# Function Library

`UDataIndexerFunctionLibrary` は `UBlueprintFunctionLibrary` で、Blueprint グラフで DataIndexer 型を操作するユーティリティNodeを提供します。すべてのNodeは **DataIndexer** カテゴリに属します。

---

## Primary Key Node

### Equal / Not Equal

```
EqualEqual_PrimaryKeyPrimaryKey(A, B) → bool
NotEqual_PrimaryKeyPrimaryKey(A, B) → bool
```

コンパクトNodeタイトルは `==` と `!=`。`BlueprintThreadSafe`。

2 つの `FDataIndexerPrimaryKey` の等値比較を行います。`FGuid::operator==` に委譲します。

---

### To String

```
Conv_PrimaryKeyToString(PrimaryKey) → FString
```

コンパクトNodeタイトルは `→`。`BlueprintAutocast`、`BlueprintThreadSafe`。

プライマリKeyを GUID 文字列表現（例：`"A1B2C3D4-..."`）に変換します。

---

### Is Valid Primary Key

```
IsValidPrimaryKey(PrimaryKey) → bool
```

プライマリKeyがゼロでない場合（`FGuid::IsValid()`）に `true` を返します。

---

## Row Handle Node

### Is Valid Row Handle

```
IsValidRowHandle(RowHandle) → bool
```

`RowHandle.Repository` と `RowHandle.PrimaryKey` の両方が有効な場合に `true` を返します。

---

### Get Row Handle Name

```
GetRowHandleName(RowHandle, OutDisplayName) → bool
```

行のSchema駆動の表示名を取得します。ハンドルが無効またはSchemaが表示名を返さない場合は `false` を返します。

---

### Get Row Handle Value

```
GetRowHandleValue(Handle, OutValue) → bool  [CustomThunk, BlueprintInternalUseOnly]
```

ハンドルから型付き行構造体を `OutValue` に取り出します。`OutValue` ワイルドカードピンには正しい行構造体型の変数を接続する必要があります。ハンドルが無効な場合は `false` を返します。

!!! note
    このNodeは `BlueprintInternalUseOnly` としてマークされています。推奨される型付き取得には `UK2Node_DataIndexerGetRow` の **Get Row** K2 Nodeを使用してください。

---

## Rows Handle Node

### Get Rows Handle Value

```
GetRowsHandleValue(Handle, Query, OutKeys) → bool  [CustomThunk, BlueprintInternalUseOnly]
```

ワイルドカードの `Query` 構造体にマッチするプライマリKeyを `OutKeys` に書き込みます。`Query` ピンにはSchemaに登録された具体的なIndexQuery構造体型を接続する必要があります。マッチするKeyが 1 件以上あれば `true` を返します。

!!! note
    このNodeは `BlueprintInternalUseOnly` としてマークされています。`FDataIndexerKeysHandle` を解決するときに K2 Nodeが内部で使用します。

---

### Get All Primary Keys

```
GetAllPrimaryKeys(Repository) → TArray<FDataIndexerPrimaryKey>
```

Repositoryを通して見えるすべてのプライマリKeyを返します（親Repositoryを含む）。Blueprint で UI リストへの表示やすべての行の走査に使います。
