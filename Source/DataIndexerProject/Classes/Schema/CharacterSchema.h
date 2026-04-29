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
	DI_DEFINE_INDEX( ByClassIndex );
	DI_DEFINE_INDEX( ByDefaultWeaponIndex );

protected:
	virtual void PostInitProperties() override;

	virtual FText GetRowDisplayName_Implementation(
		const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity ) const override;

	UFUNCTION()
	static FDataIndexerIndexValue BuildClassIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName );

	UFUNCTION()
	static FDataIndexerIndexValue BuildDefaultWeaponIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName );
};
