#include "Schema/CharacterSchema.h"

#include "Types/CharacterTypes.h"

UCharacterSchema::UCharacterSchema()
{
	RowStruct = FCharacterRow::StaticStruct();

	RegisterFunction_BuildIndex( ByClassIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildClassIndex ) );
	RegisterFunction_BuildIndex( ByDefaultWeaponIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildDefaultWeaponIndex ) );
}

#if WITH_EDITOR

void UCharacterSchema::InitializeExpandedStructEntries()
{
	Super::InitializeExpandedStructEntries();

	if ( FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find( RowStruct ) )
	{
		*RowStructEntry -= {
			GET_MEMBER_NAME_CHECKED( FCharacterRow, DisplayName ),
		};
	}
}

#endif

FText UCharacterSchema::GetRowDisplayName_Implementation(
	const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity ) const
{
	if ( const FCharacterRow* Row = RowEntity.GetPtr<const FCharacterRow>() )
	{
		return Row->DisplayName;
	}

	return Super::GetRowDisplayName_Implementation( PrimaryKey, RowEntity );
}

FGuid UCharacterSchema::BuildClassIndex( const FInstancedStruct& RowEntity )
{
	if ( const FCharacterRow* Row = RowEntity.GetPtr<const FCharacterRow>() )
	{
		return FGuid( static_cast<uint32>( Row->Class ), 0, 0, 0 );
	}

	return {};
}

FGuid UCharacterSchema::BuildDefaultWeaponIndex( const FInstancedStruct& RowEntity )
{
	if ( const FCharacterRow* Row = RowEntity.GetPtr<const FCharacterRow>() )
	{
		if ( Row->DefaultWeapon.IsValid() )
		{
			return FGuid( Row->DefaultWeapon );
		}
	}

	return {};
}
