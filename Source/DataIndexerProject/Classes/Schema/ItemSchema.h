#pragma once

#include "DataIndexerKeyHelpers.h"
#include "DataIndexerSchema.h"

#include "ItemSchema.generated.h"

/**
 * Schema for ItemRepository.
 *
 * Indexes:
 *   ByTypeIndex   — group items by EItemType (Weapon / Armor / ...)
 *   ByRarityIndex — group items by EItemRarity (Common / Rare / ...)
 */
UCLASS()
class DATAINDEXERPROJECT_API UItemSchema : public UDataIndexerSchema
{
	GENERATED_BODY()

public:
	DI_DEFINE_INDEX( ByTypeIndex );
	DI_DEFINE_INDEX( ByRarityIndex );
	DI_DEFINE_INDEX( ByTypeAndRarityIndex );

protected:
	virtual void PostInitProperties() override;

	virtual FText GetRowDisplayName_Implementation(
		const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity ) const override;

	UFUNCTION()
	static FDataIndexerIndexValue BuildTypeIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName );

	UFUNCTION()
	static FDataIndexerIndexValue BuildRarityIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName );

	UFUNCTION()
	static FDataIndexerIndexValue BuildTypeAndRarityIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName );
};
