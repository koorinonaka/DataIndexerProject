#pragma once

#include "DataIndexerKeyHelpers.h"
#include "DataIndexerSchema.h"

#include "ItemSchema.generated.h"

/**
 * Schema for ItemRepository.
 *
 * Indexes:
 *   ByTypeIndex          — group items by EItemType (Weapon / Armor / ...)
 *   ByRarityIndex        — group items by EItemRarity (Common / Rare / ...)
 *   ByTypeAndRarityIndex — composite lookup by EItemType × EItemRarity
 */
UCLASS()
class DATAINDEXERPROJECT_API UItemSchema : public UDataIndexerSchema
{
	GENERATED_BODY()

public:
	UItemSchema();

	DI_DEFINE_INDEX( ByTypeIndex );
	DI_DEFINE_INDEX( ByRarityIndex );
	DI_DEFINE_INDEX( ByTypeAndRarityIndex );

protected:
#if WITH_EDITOR
	virtual void InitializeExpandedStructEntries() override;
#endif

	virtual FText GetRowDisplayName_Implementation(
		const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity ) const override;

	UFUNCTION()
	static FGuid BuildTypeIndex( const FInstancedStruct& RowEntity );

	UFUNCTION()
	static FGuid BuildRarityIndex( const FInstancedStruct& RowEntity );

	UFUNCTION()
	static FGuid BuildTypeAndRarityIndex( const FInstancedStruct& RowEntity );
};
