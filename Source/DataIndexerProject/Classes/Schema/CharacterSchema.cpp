#include "Schema/CharacterSchema.h"

#include "ItemSchema.h"
#include "Types/CharacterTypes.h"

UCharacterSchema::UCharacterSchema()
{
	RowStruct = FCharacterRow::StaticStruct();

	DI_REGISTER_BUILD_INDEX( ByClassIndex(), FCharacterRow, BuildClassIndex );
	DI_REGISTER_BUILD_INDEX( ByDefaultWeaponIndex(), FCharacterRow, BuildDefaultWeaponIndex );
}

#if WITH_EDITOR

void UCharacterSchema::InitializeExpandedStructEntries()
{
	Super::InitializeExpandedStructEntries();

	if ( FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find( RowStruct ) )
	{
		*RowStructEntry -= {
			GET_MEMBER_NAME_CHECKED( FCharacterRow, DisplayName ),
		};
	}
}

TSharedRef<SWidget> UCharacterSchema::CustomizePropertyCellWidget( DataIndexer::IPropertyWidgetContext& Context ) const
{
	if ( const FName ColumnName = GET_MEMBER_NAME_CHECKED( FCharacterRow, DefaultWeapon ); Context.GetColumnName() == ColumnName )
	{
		if ( const FCharacterRow* Row = Context.GetRow<FCharacterRow>(); Row && ItemRepository )
		{
			return Context.CreateAsSimpleText( ItemRepository->GetDisplayName( Row->DefaultWeapon ) );
		}
	}

	return Super::CustomizePropertyCellWidget( Context );
}

#endif

TOptional<FText> UCharacterSchema::GetRowDisplayName(
	const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const
{
	return RowEntity.Get<const FCharacterRow>().DisplayName;
}

FGuid UCharacterSchema::BuildClassIndex( const FCharacterRow& Row )
{
	return static_cast<FGuid>( Row.Class );
}

FGuid UCharacterSchema::BuildDefaultWeaponIndex( const FCharacterRow& Row )
{
	return static_cast<FGuid>( Row.DefaultWeapon );
}
