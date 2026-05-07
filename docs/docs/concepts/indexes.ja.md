# インデックス

インデックスはセカンダリ検索軸で、カテゴリ・陣営・レアリティなど任意のドメイン属性で行のセットを取得できます。リポジトリ全体を走査せず、直接マルチマップ参照によって O(マッチ数) で結果を返します。

## 型

| 型 | 役割 |
|----|------|
| `FDataIndexerIndex` | 検索軸を識別する。決定論的 GUID とエディタ専用の `DevComment` を保持 |

インデックスキーはビルダー関数が返す生の `FGuid` 値です。`FDataIndexerIndexKey` のような独立した型はありません。

## インデックスの仕組み

保存時にコンパイラが各行に対してすべての **BuildIndex** 関数を呼び出します。ビルダーはインデックスキーとなる `FGuid` を返し、任意でエディタ表示用の `FText` を設定します。結果はリポジトリの `ReverseLookups` テーブルに格納されます。

```
ReverseLookups[FDataIndexerIndex] → { TMap<FGuid, TArray<FDataIndexerPrimaryKey>> }
```

実行時の `Repository.ForEachPrimaryKeys(Index, Query, Callback)` はビルダーをクエリ構造体に対して呼び出してルックアップ GUID を導出し、マップを直接引くため高速です。

## インデックスの定義

### C++ の場合

スキーマクラスに `DI_DEFINE_INDEX` マクロでインデックスを宣言します。マクロはクラスパスと名前から決定論的 GUID を生成する静的アクセサ関数を展開します。

```cpp title="ItemSchema.h"
UCLASS()
class UItemSchema : public UDataIndexerSchema
{
    GENERATED_BODY()
public:
    DI_DEFINE_INDEX(ByTypeIndex);
    DI_DEFINE_INDEX(ByRarityIndex);

protected:
    virtual void PostInitProperties() override;

    UFUNCTION()
    static FGuid BuildTypeIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName);

    UFUNCTION()
    static FGuid BuildRarityIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName);
};
```

`PostInitProperties` でビルダーを登録します。

```cpp title="ItemSchema.cpp"
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

FGuid UItemSchema::BuildTypeIndex(const FInstancedStruct& RowEntity, FText& OutDisplayName)
{
    if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
    {
        OutDisplayName = UEnum::GetDisplayValueAsText(Row->Type);
        return FGuid(static_cast<uint32>(Row->Type), 0, 0, 0);
    }
    return {};
}
```

### Blueprint の場合

1. スキーマ Blueprint を開いて **Class Defaults** へ
2. **Build Index Functions** にエントリを追加：
   - **Key**：`FDataIndexerIndex` 変数（固定 GUID を変数デフォルト値に設定）
   - **Value**：`Prototype_BuildIndex` シグネチャに合った関数参照（`RowEntity → FGuid`）

## インデックスによるクエリ

**C++** — クエリとして使いたいフィールドだけを埋めた行を渡します。

```cpp
// Weapon タイプのアイテムをすべて取得
FItemRow Query;
Query.Type = EItemType::Weapon;

TArray<FDataIndexerPrimaryKey> Keys =
    FItemInterface::GetPrimaryKeys(*Repository, UItemSchema::ByTypeIndex(), Query);
```

逆引きインデックス（例：特定のアイテムを `DefaultWeapon` に持つキャラクターを取得）:

```cpp
FCharacterRow Query;
Query.DefaultWeapon.PrimaryKey = WeaponKey;

TArray<FDataIndexerPrimaryKey> Characters =
    FCharacterInterface::GetPrimaryKeys(*Repository,
        UCharacterSchema::ByDefaultWeaponIndex(), Query);
```

**Blueprint:**

`FDataIndexerRowsHandle` UPROPERTY と **Get Rows Handle Keys** 関数ライブラリノードを使います。ノードはハンドルと **Query** ワイルドカード構造体ピンを受け取ります。インデックスを駆動するフィールドを埋めてください（例：`ByType` インデックスなら `Type = Weapon` を設定）。

## インデックスキーの安定性

ビルダー関数が返す GUID はエディタをまたいで不変でなければなりません。enum の序数・文字列ハッシュなど安定した値から導出してください。ビルダー内で `FGuid::NewGuid()` を使うのは NG です。

`DI_DEFINE_INDEX` は `FGuid::NewDeterministicGuid(StaticClass()->GetPathName() + "." + インデックス名)` を使用するため、インデックス名やスキーマクラス名（`GetPathName()` が変わる）のリネームは GUID 変更を引き起こします。これらの名前は安定した API として扱ってください。

!!! warning "スキーマ変更後は再保存が必要"
    既存の行に対してビルダーが返すキーを変更した場合、リポジトリを再保存して `ReverseLookups` テーブルを再構築してください。
