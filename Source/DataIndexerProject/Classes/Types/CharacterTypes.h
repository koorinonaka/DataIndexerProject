#pragma once

#include "CoreMinimal.h"
#include "DataIndexerSchemaInterface.h"
#include "DataIndexerTypes.h"

#include "CharacterTypes.generated.h"

USTRUCT( BlueprintType )
struct DATAINDEXERPROJECT_API FCharacterRow
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText DisplayName;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, meta = ( Repository = "CharacterClassRepository" ) )
	FDataIndexerPrimaryKey Class;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int32 MaxHP = 100;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, meta = ( Repository = "ItemRepository" ) )
	FDataIndexerPrimaryKey DefaultWeapon;
};

using FCharacterInterface = DataIndexer::TNativeSchemaInterface<FCharacterRow>;
