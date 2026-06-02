#pragma once

#include "DataIndexerKeyHelpers.h"
#include "DataIndexerSchema.h"
#include "Types/CharacterTypes.h"

#include "CharacterSchema.generated.h"

/**
 * Schema for CharacterRepository.
 *
 * Indexes:
 *   ByClassIndex         — group characters by ECharacterClass
 *   ByDefaultWeaponIndex — reverse lookup: given an Item PrimaryKey, find all characters
 *                          whose DefaultWeapon points to that item
 */
UCLASS()
class DATAINDEXERPROJECT_API UCharacterSchema : public UDataIndexerSchema
{
	GENERATED_BODY()

public:
	UCharacterSchema();

	DI_DEFINE_INDEX( ByClassIndex );
	DI_DEFINE_INDEX( ByDefaultWeaponIndex );

protected:
#if WITH_EDITOR
	virtual void InitializeExpandedStructEntries() override;
#endif

	virtual TOptional<FText> GetRowDisplayName(
		const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const override;

	UFUNCTION()
	static FGuid BuildClassIndex( const FCharacterRow& Row );

	UFUNCTION()
	static FGuid BuildDefaultWeaponIndex( const FCharacterRow& Row );

protected:
	UPROPERTY( EditDefaultsOnly, Category = DataIndexer, meta = ( Schema = "/Script/DataIndexerProject.ItemSchema" ) )
	TObjectPtr<UDataIndexerRepository> ItemRepository;
};
