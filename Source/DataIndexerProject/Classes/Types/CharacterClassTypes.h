#pragma once

#include "CoreMinimal.h"
#include "DataIndexerSchemaInterface.h"

#include "CharacterClassTypes.generated.h"

USTRUCT( BlueprintType )
struct DATAINDEXERPROJECT_API FCharacterClassRow
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText DisplayName;
};

using FCharacterClassInterface = DataIndexer::TNativeSchemaInterface<FCharacterClassRow>;
