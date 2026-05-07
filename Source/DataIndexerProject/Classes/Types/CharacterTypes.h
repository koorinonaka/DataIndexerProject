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

	UPROPERTY( EditAnywhere, BlueprintReadOnly, meta = ( SourceRepository = "ItemRepository" ) )
	FDataIndexerPrimaryKey DefaultWeapon;

	UPROPERTY( EditAnywhere, BlueprintReadOnly, meta = ( Schema = "/Script/DataIndexerProject.ItemSchema" ) )
	FDataIndexerRowHandle DefaultWeapon2;
};

using FCharacterInterface = DataIndexer::TNativeSchemaInterface<FCharacterRow>;
