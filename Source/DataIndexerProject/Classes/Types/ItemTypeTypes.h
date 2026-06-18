#pragma once

#include "CoreMinimal.h"
#include "DataIndexerSchemaInterface.h"

#include "ItemTypeTypes.generated.h"

USTRUCT( BlueprintType )
struct DATAINDEXERPROJECT_API FItemTypeRow
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText DisplayName;
};

using FItemTypeInterface = DataIndexer::TNativeSchemaInterface<FItemTypeRow>;
