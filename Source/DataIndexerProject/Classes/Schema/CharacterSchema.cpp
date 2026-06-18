#include "Schema/CharacterSchema.h"

#include "ItemSchema.h"
#include "Types/CharacterTypes.h"

namespace
{
// Virtual column that aliases PawnClass and is rendered by the C++ inline editor below.
// The Blueprint-widget counterpart ("PawnClassWidget") is declared in DIS_Character Class Defaults.
const FName PawnClassInlineColumn( TEXT( "PawnClassInline" ) );
}	 // namespace

UCharacterSchema::UCharacterSchema()
{
	RowStruct = FCharacterRow::StaticStruct();

	DI_REGISTER_BUILD_INDEX( ByClassIndex(), FCharacterRow, BuildClassIndex );
	DI_REGISTER_BUILD_INDEX( ByDefaultWeaponIndex(), FCharacterRow, BuildDefaultWeaponIndex );

#if WITH_EDITORONLY_DATA
	// Surface the single PawnClass as a C++ inline-editor column (Sample 1). Both this and the
	// Blueprint-widget column (Sample 2) alias the same PawnClass property and write back to it.
	FDataIndexerVirtualColumn& InlineColumn = VirtualColumns.AddDefaulted_GetRef();
	InlineColumn.ColumnName = PawnClassInlineColumn;
	InlineColumn.DisplayName = NSLOCTEXT( "CharacterSchema", "PawnClassInlineColumn", "Pawn (C++)" );
	InlineColumn.SourceProperty = GET_MEMBER_NAME_CHECKED( FCharacterRow, PawnClass );
#endif
}

#if WITH_EDITOR

void UCharacterSchema::InitializeExpandedStructEntries()
{
	Super::InitializeExpandedStructEntries();

	if ( FDataIndexerExpandedStructEntry* RowStructEntry = ExpandedStructEntries.Find( RowStruct ) )
	{
		// Hide DisplayName (shown in the fixed row header) and the raw PawnClass column — PawnClass is
		// surfaced instead through the two virtual columns (C++ inline + Blueprint widget).
		*RowStructEntry -= {
			GET_MEMBER_NAME_CHECKED( FCharacterRow, DisplayName ),
			GET_MEMBER_NAME_CHECKED( FCharacterRow, PawnClass ),
		};
	}
}

TSharedRef<SWidget> UCharacterSchema::CustomizePropertyCellWidget( DataIndexer::IPropertyWidgetContext& Context ) const
{
	if ( Context.GetColumnName() == PawnClassInlineColumn )
	{
		// GetProperty() resolves to the aliased PawnClass property, so the inline editor writes back to it.
		return Context.CreateAsEditInline( Context.GetProperty(), true );
	}

	if ( const FName ColumnName = GET_MEMBER_NAME_CHECKED( FCharacterRow, DefaultWeapon ); Context.GetColumnName() == ColumnName )
	{
		if ( const FCharacterRow* Row = Context.GetRow<FCharacterRow>(); Row && ItemRepository )
		{
			return Context.CreateAsSimpleText( ItemRepository->GetDisplayName( Row->DefaultWeapon ) );
		}
	}

	return Super::CustomizePropertyCellWidget( Context );
}

EDataValidationResult UCharacterSchema::IsRowValid( FConstStructView RowEntity, FDataValidationContext& Context ) const
{
	EDataValidationResult Result = Super::IsRowValid( RowEntity, Context );

	const auto AddError = [&]( const FText& Msg )
	{
		Context.AddError( Msg );
		Result = EDataValidationResult::Invalid;
	};

	if ( const FCharacterRow* Row = RowEntity.GetPtr<const FCharacterRow>() )
	{
		if ( Row->DisplayName.IsEmpty() )
		{
			AddError( NSLOCTEXT( "CharacterSchema", "EmptyDisplayName", "DisplayName must not be empty." ) );
		}
		if ( !Row->Class.IsValid() )
		{
			AddError( NSLOCTEXT( "CharacterSchema", "InvalidClass", "Class must not be empty." ) );
		}
		if ( Row->MaxHP < 1 )
		{
			AddError( NSLOCTEXT( "CharacterSchema", "MaxHPNotPositive", "MaxHP must be at least 1." ) );
		}
		if ( !Row->PawnClass )
		{
			AddError( NSLOCTEXT( "CharacterSchema", "PawnClassNull", "PawnClass must not be empty." ) );
		}
		if ( !Row->DefaultWeapon.IsValid() )
		{
			AddError( NSLOCTEXT( "CharacterSchema", "InvalidDefaultWeapon", "DefaultWeapon must not be empty." ) );
		}
	}

	return Result;
}

#endif

TOptional<FText> UCharacterSchema::GetRowDisplayName(
	const FDataIndexerPrimaryKey& PrimaryKey, const FConstStructView& RowEntity ) const
{
	if ( const FCharacterRow* Row = RowEntity.GetPtr<const FCharacterRow>() )
	{
		return Row->DisplayName;
	}

	return {};
}

FGuid UCharacterSchema::BuildClassIndex( const FCharacterRow& Row )
{
	return static_cast<FGuid>( Row.Class );
}

FGuid UCharacterSchema::BuildDefaultWeaponIndex( const FCharacterRow& Row )
{
	return static_cast<FGuid>( Row.DefaultWeapon );
}
