# Function Library

`UDataIndexerFunctionLibrary` は `UBlueprintFunctionLibrary` で、Blueprint グラフで DataIndexer 型を操作するユーティリティノードを提供します。すべてのノードは **DataIndexer** カテゴリに属します。

---

## Primary Key ノード

### Equal / Not Equal

```
EqualEqual_PrimaryKeyPrimaryKey(A, B) → bool
NotEqual_PrimaryKeyPrimaryKey(A, B) → bool
```

コンパクトノードタイトルは `==` と `!=`。`BlueprintThreadSafe`。

2 つの `FDataIndexerPrimaryKey` の等値比較を行います。`FGuid::operator==` に委譲します。

---

### To String

```
Conv_PrimaryKeyToString(PrimaryKey) → FString
```

コンパクトノードタイトルは `→`。`BlueprintAutocast`、`BlueprintThreadSafe`。

プライマリキーを GUID 文字列表現（例：`"A1B2C3D4-..."`）に変換します。

---

### Cast Primary Key

```
CastPrimaryKey(PrimaryKey, OutKey) → bool  [CustomThunk]
```

ワイルドカード出力 — `OutKey` ピンにはリポジトリのスキーマに登録された具体的なキー構造体型を接続します。キャストが成功した場合に `true` を返します。K2 ノードが内部で使用するため、直接呼び出すことはほとんどありません。

---

### Is Valid Primary Key

```
IsValidPrimaryKey(PrimaryKey) → bool
```

プライマリキーがゼロでない場合（`FGuid::IsValid()`）に `true` を返します。

---

## Row Handle ノード

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

行のスキーマ駆動の表示名を取得します。ハンドルが無効またはスキーマが表示名を返さない場合は `false` を返します。

---

### Get Row Handle Value

```
GetRowHandleValue(Handle, OutValue) → bool  [CustomThunk, BlueprintInternalUseOnly]
```

ハンドルから型付き行構造体を `OutValue` に取り出します。`OutValue` ワイルドカードピンには正しい行構造体型の変数を接続する必要があります。ハンドルが無効な場合は `false` を返します。

!!! note
    このノードは `BlueprintInternalUseOnly` としてマークされています。推奨される型付き取得には `UK2Node_DataIndexerGetRow` の **Get Row** K2 ノードを使用してください。

---

## Rows Handle ノード

### Get Rows Handle Keys

```
GetRowsHandleKeys(Handle, Query) → TArray<FDataIndexerPrimaryKey>  [CustomThunk, BlueprintInternalUseOnly]
```

Rows Handle のリポジトリと、ワイルドカードの `Query` 構造体にマッチするすべてのプライマリキーを返します。`Query` ピンにはスキーマに登録された具体的なインデックスクエリ構造体型を接続する必要があります。

!!! note
    このノードは `BlueprintInternalUseOnly` としてマークされています。`FDataIndexerRowsHandle` を解決するときに K2 ノードが内部で使用します。

---

### Get All Primary Keys

```
GetAllPrimaryKeys(Repository) → TArray<FDataIndexerPrimaryKey>
```

リポジトリを通して見えるすべてのプライマリキーを返します（親リポジトリを含む）。Blueprint で UI リストへの表示やすべての行の走査に使います。
