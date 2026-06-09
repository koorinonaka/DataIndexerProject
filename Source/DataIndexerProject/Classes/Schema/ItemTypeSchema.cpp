#include "Schema/ItemTypeSchema.h"

#include "Types/ItemTypeTypes.h"

UItemTypeSchema::UItemTypeSchema()
{
	RowStruct = FItemTypeRow::StaticStruct();
}

#if WITH_EDITOR

void UItemTypeSchema::InitializeExpandedStructEntries()
{
	Super::InitializeExpandedStructEntries();

	if ( FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find( RowStruct ) )
	{
		*RowStructEntry -= {
			GET_MEMBER_NAME_CHECKED( FItemTypeRow, DisplayName ),
		};
	}
}

#endif

TOptional<FText> UItemTypeSchema::GetRowDisplayName(
	const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const
{
	if ( const FItemTypeRow* Row = RowEntity.GetPtr<const FItemTypeRow>() )
	{
		return Row->DisplayName;
	}

	return {};
}
