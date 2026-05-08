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

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText DisplayName;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	ECharacterClass Class = ECharacterClass::Warrior;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int32 MaxHP = 100;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, meta = ( SourceRepository = "ItemRepository" ) )
	FDataIndexerPrimaryKey DefaultWeapon;
};

using FCharacterInterface = DataIndexer::TNativeSchemaInterface<FCharacterRow>;
