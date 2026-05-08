#pragma once

#include "DataIndexerSchemaInterface.h"

#include "ItemTypes.generated.h"

UENUM( BlueprintType )
enum class EItemType : uint8
{
	Weapon UMETA( DisplayName = "Weapon" ),
	Armor UMETA( DisplayName = "Armor" ),
	Accessory UMETA( DisplayName = "Accessory" ),
	Material UMETA( DisplayName = "Material" ),
};

UENUM( BlueprintType )
enum class EItemRarity : uint8
{
	Common UMETA( DisplayName = "Common" ),
	Uncommon UMETA( DisplayName = "Uncommon" ),
	Rare UMETA( DisplayName = "Rare" ),
	Epic UMETA( DisplayName = "Epic" ),
	Legendary UMETA( DisplayName = "Legendary" ),
};

USTRUCT( BlueprintType )
struct DATAINDEXERPROJECT_API FItemRow
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText DisplayName;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	EItemType Type = EItemType::Weapon;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	EItemRarity Rarity = EItemRarity::Common;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int32 BaseValue = 0;
};

using FItemInterface = DataIndexer::TNativeSchemaInterface<FItemRow>;
