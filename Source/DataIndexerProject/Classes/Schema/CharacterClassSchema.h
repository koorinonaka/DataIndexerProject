#pragma once

#include "DataIndexerSchema.h"

#include "CharacterClassSchema.generated.h"

/**
 * Schema for CharacterClassRepository.
 *
 * Each row represents a single character class (Warrior / Mage / ...). Rows are keyed by
 * FDataIndexerPrimaryKey so they can drive a UCharacterClassIconCollection and be referenced
 * from FCharacterRow.Class.
 */
UCLASS()
class DATAINDEXERPROJECT_API UCharacterClassSchema : public UDataIndexerSchema
{
	GENERATED_BODY()

public:
	UCharacterClassSchema();

protected:
#if WITH_EDITOR
	virtual void InitializeExpandedStructEntries() override;
#endif

	virtual TOptional<FText> GetRowDisplayName(
		const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const override;
};
