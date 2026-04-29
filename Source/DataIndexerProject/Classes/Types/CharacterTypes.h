#pragma once

#include "CoreMinimal.h"
#include "DataIndexerSchemaInterface.h"
#include "DataIndexerTypes.h"

#include "CharacterTypes.generated.h"

UENUM( BlueprintType )
enum class ECharacterClass : uint8
{
	Warrior UMETA( DisplayName = "Warrior" ),
	Mage UMETA( DisplayName = "Mage" ),
	Rogue UMETA( DisplayName = "Rogue" ),
	Priest UMETA( DisplayName = "Priest" ),
};

USTRUCT( BlueprintType )
struct DATAINDEXERPROJECT_API FCharacterRow
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadOnly )
	FText DisplayName;

	UPROPERTY( EditAnywhere, BlueprintReadOnly )
	ECharacterClass Class = ECharacterClass::Warrior;

	UPROPERTY( EditAnywhere, BlueprintReadOnly )
	int32 MaxHP = 100;

	// Relation to ItemRepository — must point to a Weapon-type item.
	// Enables reverse lookup: "which characters use this weapon?" via ByDefaultWeaponIndex.
	UPROPERTY( EditAnywhere, BlueprintReadOnly )
	FDataIndexerRowHandle DefaultWeapon;
};

using FCharacterInterface = DataIndexer::TNativeSchemaInterface<FCharacterRow>;
