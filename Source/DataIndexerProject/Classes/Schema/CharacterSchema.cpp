#include "Schema/CharacterSchema.h"

#include "Types/CharacterTypes.h"

void UCharacterSchema::PostInitProperties()
{
	Super::PostInitProperties();

	if ( HasAnyFlags( RF_ClassDefaultObject ) )
	{
		RowStruct = FCharacterRow::StaticStruct();

		RegisterFunction_BuildIndexKey( ByClassIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildClassIndex ) );
		RegisterFunction_BuildIndexKey( ByDefaultWeaponIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildDefaultWeaponIndex ) );
	}
}

FText UCharacterSchema::GetRowDisplayName_Implementation(
	const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity ) const
{
	if ( const FCharacterRow* Row = RowEntity.GetPtr<const FCharacterRow>() )
	{
		return Row->DisplayName;
	}

	return Super::GetRowDisplayName_Implementation( PrimaryKey, RowEntity );
}

FDataIndexerIndexValue UCharacterSchema::BuildClassIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName )
{
	if ( const FCharacterRow* Row = RowEntity.GetPtr<const FCharacterRow>() )
	{
		OutDisplayName = UEnum::GetDisplayValueAsText( Row->Class );
		return FGuid( static_cast<uint32>( Row->Class ), 0, 0, 0 );
	}

	return {};
}

FDataIndexerIndexValue UCharacterSchema::BuildDefaultWeaponIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName )
{
	if ( const FCharacterRow* Row = RowEntity.GetPtr<const FCharacterRow>() )
	{
		if ( Row->DefaultWeapon.IsValid() )
		{
			OutDisplayName = Row->DefaultWeapon.GetRowDisplayName().Get( FText::GetEmpty() );
			return Row->DefaultWeapon.PrimaryKey;
		}
	}

	return {};
}
