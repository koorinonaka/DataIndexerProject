#pragma once

#include "DataIndexerKeyHelpers.h"
#include "DataIndexerSchema.h"
#include "Types/ItemTypes.h"

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
	virtual TSharedRef<SWidget> CustomizePropertyCellWidget( DataIndexer::IPropertyWidgetContext& Context ) const override;
#endif

	virtual TOptional<FText> GetRowDisplayName(
		const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const override;

	UFUNCTION()
	static FGuid BuildTypeIndex( const FItemRow& Row );

	UFUNCTION()
	static FGuid BuildRarityIndex( const FItemRow& Row );

	UFUNCTION()
	static FGuid BuildTypeAndRarityIndex( const FItemRow& Row );
};
