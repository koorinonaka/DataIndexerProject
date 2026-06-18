#pragma once

#include "DataIndexerSchemaInterface.h"
#include "DataIndexerTypes.h"
#include "GameplayTagContainer.h"

#include "ItemTypes.generated.h"

USTRUCT( BlueprintType )
struct DATAINDEXERPROJECT_API FItemRow
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText DisplayName;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText Subtitle;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FText Description;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, meta = ( Repository = "ItemTypeRepository" ) )
	FDataIndexerPrimaryKey Type;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, meta = ( Repository = "ItemRarityRepository" ) )
	FDataIndexerPrimaryKey Rarity;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	int32 BaseValue = 0;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FGameplayTagContainer MetadataTags;
};

using FItemInterface = DataIndexer::TNativeSchemaInterface<FItemRow>;
