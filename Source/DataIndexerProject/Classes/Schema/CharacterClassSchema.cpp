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

EDataValidationResult UCharacterClassSchema::IsRowValid( FConstStructView RowEntity, FDataValidationContext& Context ) const
{
	EDataValidationResult Result = Super::IsRowValid( RowEntity, Context );

	const auto AddError = [&]( const FText& Msg )
	{
		Context.AddError( Msg );
		Result = EDataValidationResult::Invalid;
	};

	if ( const FCharacterClassRow* Row = RowEntity.GetPtr<const FCharacterClassRow>() )
	{
		if ( Row->DisplayName.IsEmpty() )
		{
			AddError( NSLOCTEXT( "CharacterClassSchema", "EmptyDisplayName", "DisplayName must not be empty." ) );
		}
	}

	return Result;
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
