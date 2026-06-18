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

EDataValidationResult UItemRaritySchema::IsRowValid( FConstStructView RowEntity, FDataValidationContext& Context ) const
{
	EDataValidationResult Result = Super::IsRowValid( RowEntity, Context );

	const auto AddError = [&]( const FText& Msg )
	{
		Context.AddError( Msg );
		Result = EDataValidationResult::Invalid;
	};

	if ( const FItemRarityRow* Row = RowEntity.GetPtr<const FItemRarityRow>() )
	{
		if ( Row->DisplayName.IsEmpty() )
		{
			AddError( NSLOCTEXT( "ItemRaritySchema", "EmptyDisplayName", "DisplayName must not be empty." ) );
		}
	}

	return Result;
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
