#include "Schema/CharacterSchema.h"

#include "Types/CharacterTypes.h"

UCharacterSchema::UCharacterSchema()
{
	RowStruct = FCharacterRow::StaticStruct();

	DI_REGISTER_BUILD_INDEX( ByClassIndex(), FCharacterRow, BuildClassIndex );
	DI_REGISTER_BUILD_INDEX( ByDefaultWeaponIndex(), FCharacterRow, BuildDefaultWeaponIndex );
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
	return static_cast<FGuid>( Row.Class );
}

FGuid UCharacterSchema::BuildDefaultWeaponIndex( const FCharacterRow& Row )
{
	return static_cast<FGuid>( Row.DefaultWeapon );
}
