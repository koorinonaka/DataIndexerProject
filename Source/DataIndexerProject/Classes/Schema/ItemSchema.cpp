#include "Schema/ItemSchema.h"

#include "DataIndexerRepository.h"
#include "Types/ItemTypes.h"

UItemSchema::UItemSchema()
{
	RowStruct = FItemRow::StaticStruct();

	DI_REGISTER_BUILD_INDEX( ByTypeIndex(), FItemRow, BuildTypeIndex );
	DI_REGISTER_BUILD_INDEX( ByRarityIndex(), FItemRow, BuildRarityIndex );
	DI_REGISTER_BUILD_INDEX( ByTypeAndRarityIndex(), FItemRow, BuildTypeAndRarityIndex );
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

TSharedRef<SWidget> UItemSchema::CustomizePropertyCellWidget( DataIndexer::IPropertyWidgetContext& Context ) const
{
	if ( const FName ColumnName = GET_MEMBER_NAME_CHECKED( FItemRow, MetadataTags ); Context.GetColumnName() == ColumnName )
	{
		return Context.CreateAsEditInline( Context.GetProperty() );
	}

	if ( const FName ColumnName = GET_MEMBER_NAME_CHECKED( FItemRow, Type ); Context.GetColumnName() == ColumnName )
	{
		if ( const FItemRow* Row = Context.GetRow<FItemRow>(); Row && ItemTypeRepository )
		{
			return Context.CreateAsSimpleText( ItemTypeRepository->GetDisplayName( Row->Type ) );
		}
	}

	if ( const FName ColumnName = GET_MEMBER_NAME_CHECKED( FItemRow, Rarity ); Context.GetColumnName() == ColumnName )
	{
		if ( const FItemRow* Row = Context.GetRow<FItemRow>(); Row && ItemRarityRepository )
		{
			return Context.CreateAsSimpleText( ItemRarityRepository->GetDisplayName( Row->Rarity ) );
		}
	}

	return Super::CustomizePropertyCellWidget( Context );
}

#endif

TOptional<FText> UItemSchema::GetRowDisplayName( const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const
{
	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		return Row->DisplayName;
	}

	return {};
}

FGuid UItemSchema::BuildTypeIndex( const FItemRow& Row )
{
	return static_cast<FGuid>( Row.Type );
}

FGuid UItemSchema::BuildRarityIndex( const FItemRow& Row )
{
	return static_cast<FGuid>( Row.Rarity );
}

FGuid UItemSchema::BuildTypeAndRarityIndex( const FItemRow& Row )
{
	return FGuid::NewDeterministicGuid( Row.Type.ToString() + Row.Rarity.ToString() );
}
