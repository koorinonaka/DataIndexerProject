#include "Schema/ItemSchema.h"

#include "Types/ItemTypes.h"

UItemSchema::UItemSchema()
{
	RowStruct = FItemRow::StaticStruct();

	RegisterFunction_BuildIndex( ByTypeIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildTypeIndex ) );
	RegisterFunction_BuildIndex( ByRarityIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildRarityIndex ) );
	RegisterFunction_BuildIndex( ByTypeAndRarityIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildTypeAndRarityIndex ) );
}

#if WITH_EDITOR

void UItemSchema::InitializeExpandedStructEntries()
{
	Super::InitializeExpandedStructEntries();

	if ( FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find( RowStruct ) )
	{
		*RowStructEntry -= {
			GET_MEMBER_NAME_CHECKED( FItemRow, DisplayName ),
		};
	}
}

#endif

TOptional<FText> UItemSchema::GetRowDisplayName(
	const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const
{
	return RowEntity.Get<const FItemRow>().DisplayName;
}

FGuid UItemSchema::BuildTypeIndex( const FItemRow& Row )
{
	return FGuid( static_cast<uint32>( Row.Type ), 0, 0, 0 );
}

FGuid UItemSchema::BuildRarityIndex( const FItemRow& Row )
{
	return FGuid( static_cast<uint32>( Row.Rarity ), 0, 0, 0 );
}

FGuid UItemSchema::BuildTypeAndRarityIndex( const FItemRow& Row )
{
	return FGuid( static_cast<uint32>( Row.Type ), static_cast<uint32>( Row.Rarity ), 0, 0 );
}
