#pragma once

#include "CoreMinimal.h"
#include "DataIndexerSchemaInterface.h"

#include "ItemRarityTypes.generated.h"

USTRUCT( BlueprintType )
struct DATAINDEXERPROJECT_API FItemRarityRow
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText DisplayName;
};

using FItemRarityInterface = DataIndexer::TNativeSchemaInterface<FItemRarityRow>;
