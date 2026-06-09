#pragma once

#include "DataIndexerSchema.h"
#include "Types/ItemRarityTypes.h"

#include "ItemRaritySchema.generated.h"

/**
 * Schema for ItemRarityRepository.
 *
 * Each row represents a single item rarity (Common / Rare / ...). Rows are keyed by
 * FDataIndexerPrimaryKey so they can be referenced from FItemRow.Rarity.
 */
UCLASS()
class DATAINDEXERPROJECT_API UItemRaritySchema : public UDataIndexerSchema
{
	GENERATED_BODY()

public:
	UItemRaritySchema();

protected:
#if WITH_EDITOR
	virtual void InitializeExpandedStructEntries() override;
#endif

	virtual TOptional<FText> GetRowDisplayName(
		const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const override;
};
