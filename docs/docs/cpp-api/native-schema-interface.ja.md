# ネイティブスキーマインターフェース

`DataIndexer::TNativeSchemaInterface<TRowType>`（`DataIndexerSchemaInterface.h`）は `UDataIndexerRepository` に対する型安全な C++ ファサードです。仮想ディスパッチやキャストなしに、コンパイル時型チェックを伴うクエリが書けます。

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
struct DATAINDEXERPROJECT_API FItemRow
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

リポジトリからフェッチした行エンティティでスキーマの `GetRowDisplayName` を呼び出します。スキーマが null の場合は `FText::FromName(NAME_None)` を返します。

```cpp
FText UGameDataSubsystem::GetItemDisplayName(
    const UDataIndexerRepository& Repository, const FDataIndexerPrimaryKey& PrimaryKey) const
{
    return FItemInterface::GetDisplayName(Repository, PrimaryKey);
}
```

---

## FindRow（PrimaryKey 指定）

```cpp
static const TRowType* FindRow(
    const UDataIndexerRepository& Repository,
    const FDataIndexerPrimaryKey& PrimaryKey);
```

`Repository.FindRowEntity(PrimaryKey)` に委譲した後、`TRowType` へのポインタを返します。キーが見つからない場合・型が一致しない場合は `nullptr` を返します。

```cpp
const FItemRow* UGameDataSubsystem::FindItemRow(
    const UDataIndexerRepository& Repository, const FDataIndexerPrimaryKey& PrimaryKey) const
{
    return FItemInterface::FindRow(Repository, PrimaryKey);
}
```

呼び出し側では null チェックしてから使います。

```cpp
if (const FItemRow* Row = FItemInterface::FindRow(*ItemRepository, Key))
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

行に保存されたリポジトリ間リレーションを解決するときに使います。

```cpp
const FItemRow* UGameDataSubsystem::FindItemRow(const FDataIndexerRowHandle& Handle) const
{
    return FItemInterface::FindRow(Handle);
}
```

`FCharacterRow::DefaultWeapon`（ItemRepository を指す `FDataIndexerRowHandle`）を解決する例：

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

リポジトリ内（親を含む）のすべての PrimaryKey を走査します。

```cpp
void UGameDataSubsystem::ForEachItem(
    const UDataIndexerRepository& Repository,
    TFunctionRef<void(const FDataIndexerPrimaryKey&)> Callback) const
{
    FItemInterface::ForEachPrimaryKeys(Repository, Callback);
}
```

---

## ForEachPrimaryKeys（インデックス指定）

```cpp
static void ForEachPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const FDataIndexerIndex& Index,
    const TRowType& Row,
    const TFunctionRef<void(const FDataIndexerPrimaryKey&)>& Callback);
```

`Row` からインデックスキーを計算し、そのキーに一致する PrimaryKey だけを走査します。「このクエリ行と同じフィールド値を持つ行をすべて取得する」意図でフィールドを部分的に埋めた行を渡します。

```cpp
void UGameDataSubsystem::ForEachItemsByType(
    const UDataIndexerRepository& Repository, EItemType Type,
    TFunctionRef<void(const FDataIndexerPrimaryKey&)> Callback) const
{
    FItemRow Query;
    Query.Type = Type;

    FItemInterface::ForEachPrimaryKeys(Repository, UItemSchema::ByTypeIndex(), Query, Callback);
}
```

---

## GetPrimaryKeys

```cpp
static TArray<FDataIndexerPrimaryKey> GetPrimaryKeys(
    const UDataIndexerRepository& Repository);

static TArray<FDataIndexerPrimaryKey> GetPrimaryKeys(
    const UDataIndexerRepository& Repository,
    const FDataIndexerIndex& Index,
    const TRowType& Row);
```

`ForEachPrimaryKeys` の結果を `TArray` にまとめるラッパーです。

```cpp
// 全 Row のキーを取得
TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetAllItemKeys(
    const UDataIndexerRepository& Repository) const
{
    return FItemInterface::GetPrimaryKeys(Repository);
}

// Type で絞り込み
TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByType(
    const UDataIndexerRepository& Repository, EItemType Type) const
{
    FItemRow Query;
    Query.Type = Type;
    return FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByTypeIndex(), Query);
}

// Rarity で絞り込み
TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByRarity(
    const UDataIndexerRepository& Repository, EItemRarity Rarity) const
{
    FItemRow Query;
    Query.Rarity = Rarity;
    return FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByRarityIndex(), Query);
}

// Type × Rarity の複合インデックスで絞り込み
TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByTypeAndRarity(
    const UDataIndexerRepository& Repository, EItemType Type, EItemRarity Rarity) const
{
    FItemRow Query;
    Query.Type   = Type;
    Query.Rarity = Rarity;
    return FItemInterface::GetPrimaryKeys(Repository, UItemSchema::ByTypeAndRarityIndex(), Query);
}
```
