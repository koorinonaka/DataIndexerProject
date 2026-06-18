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

EDataValidationResult UItemTypeSchema::IsRowValid( FConstStructView RowEntity, FDataValidationContext& Context ) const
{
	EDataValidationResult Result = Super::IsRowValid( RowEntity, Context );

	const auto AddError = [&]( const FText& Msg )
	{
		Context.AddError( Msg );
		Result = EDataValidationResult::Invalid;
	};

	if ( const FItemTypeRow* Row = RowEntity.GetPtr<const FItemTypeRow>() )
	{
		if ( Row->DisplayName.IsEmpty() )
		{
			AddError( NSLOCTEXT( "ItemTypeSchema", "EmptyDisplayName", "DisplayName must not be empty." ) );
		}
	}

	return Result;
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
