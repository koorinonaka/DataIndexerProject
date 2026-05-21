# ネイティブSchemaインターフェース

`DataIndexer::TNativeSchemaInterface<TRowType>`（`DataIndexerSchemaInterface.h`）は `UDataIndexerRepository` に対する型安全な C++ ファサードです。仮想ディスパッチやキャストなしに、コンパイル時型チェックを伴うQueryが書けます。

## セットアップ

行構造体と同じヘッダーの末尾でエイリアスを宣言するのが推奨パターンです。

```cpp title="ItemTypes.h"
#pragma once
#include "DataIndexerSchemaInterface.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8 { Weapon, Armor, Accessory, Material };

UENUM(BlueprintType)
enum class EItemRarity : uint8 { Common, Uncommon, Rare, Epic, Legendary };

USTRUCT(BlueprintType)
struct FItemRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EItemType Type = EItemType::Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EItemRarity Rarity = EItemRarity::Common;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 BaseValue = 0;
};

using FItemInterface = DataIndexer::TNativeSchemaInterface<FItemRow>;
```

このヘッダーをインクルードするだけで `FItemInterface` が使えます。別のセットアップファイルは不要です。

---

## GetDisplayName

```cpp
static FText GetDisplayName(
    const UDataIndexerRepository& Repository,
    const FDataIndexerPrimaryKey& PrimaryKey);
```

Repositoryからフェッチした行エンティティでSchemaの `GetRowDisplayName` を呼び出します。Schemaが null の場合は `FText::FromName(NAME_None)` を返します。

```cpp
FText Name = FItemInterface::GetDisplayName(Repository, PrimaryKey);
```

---

## FindRow（PrimaryKey 指定）

```cpp
static const TRowType* FindRow(
    const UDataIndexerRepository& Repository,
    const FDataIndexerPrimaryKey& PrimaryKey);
```

`Repository.FindRowEntity(PrimaryKey)` に委譲した後、`TRowType` へのポインタを返します。Keyが見つからない場合・型が一致しない場合は `nullptr` を返します。

```cpp
if (const FItemRow* Row = FItemInterface::FindRow(Repository, Key))
{
    UE_LOG(LogTemp, Log, TEXT("Item: %s  BaseValue: %d"),
        *Row->DisplayName.ToString(), Row->BaseValue);
}
```

---

## FindRow（RowHandle 指定）

```cpp
static const TRowType* FindRow(
    const FDataIndexerRowHandle& RowHandle);
```

ハンドルを検証してから `FindRow(*RowHandle.Repository, RowHandle.PrimaryKey)` に委譲します。ハンドルが無効な場合は `nullptr` を返します。

行に保存されたRepository間リレーションを解決するときに使います。`FCharacterRow::DefaultWeapon`（ItemRepository を指す `FDataIndexerRowHandle`）を解決する例：

```cpp
// CharacterTypes.h
UPROPERTY(EditAnywhere, BlueprintReadOnly,
    meta = (Schema = "/Script/DataIndexerProject.ItemSchema"))
FDataIndexerRowHandle DefaultWeapon;
```

```cpp
// 使用例
if (const FItemRow* Weapon = FItemInterface::FindRow(Character->DefaultWeapon))
{
    // Weapon->DisplayName, Weapon->BaseValue, ...
}
```

---

## ForEachPrimaryKeys（全行）

```cpp
static void ForEachPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback);
```

Repository内（親を含む）のすべての PrimaryKey を走査します。

```cpp
FItemInterface::ForEachPrimaryKeys(Repository, [](const FDataIndexerPrimaryKey& Key)
{
    // ...
});
```

---

## ForEachPrimaryKeys（Index指定）

```cpp
static void ForEachPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const FDataIndexerIndexKey& IndexKey,
    const TRowType& Query,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback);
```

`Row` からIndexKeyを計算し、そのKeyに一致する PrimaryKey だけを走査します。「このQuery行と同じフィールド値を持つ行をすべて取得する」意図でフィールドを部分的に埋めた行を渡します。

```cpp
FItemRow Query;
Query.Type = EItemType::Weapon;

FItemInterface::ForEachPrimaryKeys(Repository, UItemSchema::ByTypeIndex().IndexKey, Query,
    [](const FDataIndexerPrimaryKey& Key)
    {
        // ...
    });
```

---

## GetPrimaryKeys

```cpp
static TArray<FDataIndexerPrimaryKey> GetPrimaryKeys(
    const UDataIndexerRepository& Repository);

static TArray<FDataIndexerPrimaryKey> GetPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const FDataIndexerIndexKey& IndexKey,
    const TRowType& Query);
```

`ForEachPrimaryKeys` の結果を `TArray` にまとめるラッパーです。

```cpp
// 全行
TArray<FDataIndexerPrimaryKey> Keys = FItemInterface::GetPrimaryKeys(Repository);

// Type で絞り込み
FItemRow QueryByType;
QueryByType.Type = EItemType::Weapon;
TArray<FDataIndexerPrimaryKey> WeaponKeys =
    FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByTypeIndex(), QueryByType);

// Rarity で絞り込み
FItemRow QueryByRarity;
QueryByRarity.Rarity = EItemRarity::Rare;
TArray<FDataIndexerPrimaryKey> RareKeys =
    FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByRarityIndex(), QueryByRarity);

// Type × Rarity の複合Indexで絞り込み
FItemRow QueryByTypeAndRarity;
QueryByTypeAndRarity.Type   = EItemType::Weapon;
QueryByTypeAndRarity.Rarity = EItemRarity::Rare;
TArray<FDataIndexerPrimaryKey> RareWeaponKeys =
    FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByTypeAndRarityIndex(), QueryByTypeAndRarity);
```
