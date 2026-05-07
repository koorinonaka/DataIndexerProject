#pragma once

#include "DataIndexerKeyHelpers.h"
#include "DataIndexerSchema.h"

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

	virtual FText GetRowDisplayName_Implementation(
		const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity ) const override;

	UFUNCTION()
	static FGuid BuildClassIndex( const FInstancedStruct& RowEntity );

	UFUNCTION()
	static FGuid BuildDefaultWeaponIndex( const FInstancedStruct& RowEntity );

protected:
	UPROPERTY( EditDefaultsOnly, Category = DataIndexer, meta = ( Schema = "/Script/DataIndexerProject.ItemSchema" ) )
	TObjectPtr<UDataIndexerRepository> ItemRepository;
};
