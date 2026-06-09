#pragma once

#include "DataIndexerSchema.h"
#include "Types/ItemTypeTypes.h"

#include "ItemTypeSchema.generated.h"

/**
 * Schema for ItemTypeRepository.
 *
 * Each row represents a single item type (Weapon / Armor / ...). Rows are keyed by
 * FDataIndexerPrimaryKey so they can be referenced from FItemRow.Type.
 */
UCLASS()
class DATAINDEXERPROJECT_API UItemTypeSchema : public UDataIndexerSchema
{
	GENERATED_BODY()

public:
	UItemTypeSchema();

protected:
#if WITH_EDITOR
	virtual void InitializeExpandedStructEntries() override;
#endif

	virtual TOptional<FText> GetRowDisplayName(
		const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const override;
};
