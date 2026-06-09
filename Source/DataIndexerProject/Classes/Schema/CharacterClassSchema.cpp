#include "Schema/CharacterClassSchema.h"

#include "Types/CharacterClassTypes.h"

UCharacterClassSchema::UCharacterClassSchema()
{
	RowStruct = FCharacterClassRow::StaticStruct();
}

#if WITH_EDITOR

void UCharacterClassSchema::InitializeExpandedStructEntries()
{
	Super::InitializeExpandedStructEntries();

	if ( FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find( RowStruct ) )
	{
		*RowStructEntry -= {
			GET_MEMBER_NAME_CHECKED( FCharacterClassRow, DisplayName ),
		};
	}
}

#endif

TOptional<FText> UCharacterClassSchema::GetRowDisplayName(
	const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const
{
	if ( const FCharacterClassRow* Row = RowEntity.GetPtr<const FCharacterClassRow>() )
	{
		return Row->DisplayName;
	}

	return {};
}
