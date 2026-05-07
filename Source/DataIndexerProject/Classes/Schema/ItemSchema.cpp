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

FText UItemSchema::GetRowDisplayName_Implementation(
	const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity ) const
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		return Row->DisplayName;
	}

	return Super::GetRowDisplayName_Implementation( PrimaryKey, RowEntity );
}

FGuid UItemSchema::BuildTypeIndex( const FInstancedStruct& RowEntity )
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		return FGuid( static_cast<uint32>( Row->Type ), 0, 0, 0 );
	}

	return {};
}

FGuid UItemSchema::BuildRarityIndex( const FInstancedStruct& RowEntity )
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		return FGuid( static_cast<uint32>( Row->Rarity ), 0, 0, 0 );
	}

	return {};
}

FGuid UItemSchema::BuildTypeAndRarityIndex( const FInstancedStruct& RowEntity )
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		return FGuid( static_cast<uint32>( Row->Type ), static_cast<uint32>( Row->Rarity ), 0, 0 );
	}

	return {};
}
