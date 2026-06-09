#include "Schema/ItemRaritySchema.h"

#include "Types/ItemRarityTypes.h"

UItemRaritySchema::UItemRaritySchema()
{
	RowStruct = FItemRarityRow::StaticStruct();
}

#if WITH_EDITOR

void UItemRaritySchema::InitializeExpandedStructEntries()
{
	Super::InitializeExpandedStructEntries();

	if ( FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find( RowStruct ) )
	{
		*RowStructEntry -= {
			GET_MEMBER_NAME_CHECKED( FItemRarityRow, DisplayName ),
		};
	}
}

#endif

TOptional<FText> UItemRaritySchema::GetRowDisplayName(
	const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const
{
	if ( const FItemRarityRow* Row = RowEntity.GetPtr<const FItemRarityRow>() )
	{
		return Row->DisplayName;
	}

	return {};
}
