#include "Schema/ItemSchema.h"

#include "Types/ItemTypes.h"

void UItemSchema::PostInitProperties()
{
	Super::PostInitProperties();

	if ( HasAnyFlags( RF_ClassDefaultObject ) )
	{
		RowStruct = FItemRow::StaticStruct();

		RegisterFunction_BuildIndexKey( ByTypeIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildTypeIndex ) );
		RegisterFunction_BuildIndexKey( ByRarityIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildRarityIndex ) );
		RegisterFunction_BuildIndexKey( ByTypeAndRarityIndex(), GET_FUNCTION_NAME_CHECKED( ThisClass, BuildTypeAndRarityIndex ) );
	}
}

FText UItemSchema::GetRowDisplayName_Implementation(
	const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity ) const
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		return Row->DisplayName;
	}

	return Super::GetRowDisplayName_Implementation( PrimaryKey, RowEntity );
}

FDataIndexerIndexValue UItemSchema::BuildTypeIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName )
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		OutDisplayName = UEnum::GetDisplayValueAsText( Row->Type );
		return FDataIndexerIndexValue( FGuid( static_cast<uint32>( Row->Type ), 0, 0, 0 ) );
	}

	return {};
}

FDataIndexerIndexValue UItemSchema::BuildRarityIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName )
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		OutDisplayName = UEnum::GetDisplayValueAsText( Row->Rarity );
		return FDataIndexerIndexValue( FGuid( static_cast<uint32>( Row->Rarity ), 0, 0, 0 ) );
	}

	return {};
}

FDataIndexerIndexValue UItemSchema::BuildTypeAndRarityIndex( const FInstancedStruct& RowEntity, FText& OutDisplayName )
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		OutDisplayName = FText::Format(
			INVTEXT( "{0} / {1}" ), UEnum::GetDisplayValueAsText( Row->Type ), UEnum::GetDisplayValueAsText( Row->Rarity ) );
		return FDataIndexerIndexValue( FGuid( static_cast<uint32>( Row->Type ), static_cast<uint32>( Row->Rarity ), 0, 0 ) );
	}

	return {};
}
