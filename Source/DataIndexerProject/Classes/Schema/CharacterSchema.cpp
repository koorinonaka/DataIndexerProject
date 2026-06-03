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

TOptional<FText> UCharacterSchema::GetRowDisplayName(
	const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const
{
	return RowEntity.Get<const FCharacterRow>().DisplayName;
}

FGuid UCharacterSchema::BuildClassIndex( const FCharacterRow& Row )
{
	return FGuid( static_cast<uint32>( Row.Class ), 0, 0, 0 );
}

FGuid UCharacterSchema::BuildDefaultWeaponIndex( const FCharacterRow& Row )
{
	if ( Row.DefaultWeapon.IsValid() )
	{
		return FGuid( Row.DefaultWeapon );
	}

	return {};
}
