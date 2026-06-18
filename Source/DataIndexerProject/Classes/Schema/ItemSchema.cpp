#include "Schema/ItemSchema.h"

#include "DataIndexerRepository.h"
#include "Types/ItemTypes.h"

#if WITH_EDITOR
namespace
{
// Column name for the WBP_InventoryRowCard preview — declared once here to keep all InventoryCard
// logic in one place (CustomizePropertyCellWidget below).
const FName InventoryCardColumn( TEXT( "InventoryCard" ) );

// Clears all VM_InventoryRowCard_C instances from the MVVM game-subsystem's global collection.
// Must be called before each CreateWidget(WBP_InventoryRowCard) call so that the widget's MVVM
// initializer (which does GET-or-CREATE via bGlobalViewModelCollectionUpdate=true) creates a
// fresh per-row VM instead of reusing the editor-preview VoidpierVM.
void ClearInventoryCardVMsFromCollection( UWorld* World )
{
	if ( !World ) return;
	UGameInstance* GI = World->GetGameInstance();
	if ( !GI ) return;

	// Locate MVVMGameSubsystem without pulling in ModelViewViewModel headers.
	static UClass* SubsysClass = nullptr;
	if ( !SubsysClass )
	{
		SubsysClass = FindObject<UClass>( nullptr, TEXT( "/Script/ModelViewViewModel.MVVMGameSubsystem" ) );
	}
	if ( !SubsysClass ) return;

	USubsystem* MVVMSub = GI->GetSubsystemBase( SubsysClass );
	if ( !MVVMSub ) return;

	UFunction* GetCollFn = SubsysClass->FindFunctionByName( TEXT( "GetViewModelCollection" ) );
	if ( !GetCollFn ) return;

	struct GetCollResult { UObject* RetVal = nullptr; };
	GetCollResult CollResult;
	MVVMSub->ProcessEvent( GetCollFn, &CollResult );
	UObject* Collection = CollResult.RetVal;
	if ( !Collection ) return;

	UFunction* RemoveFn = Collection->GetClass()->FindFunctionByName( TEXT( "RemoveAllViewModelInstance" ) );
	if ( !RemoveFn ) return;

	// The VM Blueprint class may not be loaded on the very first call; retry next time.
	static UClass* VMClass = nullptr;
	if ( !VMClass )
	{
		VMClass = FindObject<UClass>( nullptr, TEXT( "/Game/UI/VM_InventoryRowCard.VM_InventoryRowCard_C" ) );
	}
	if ( !VMClass ) return;

	// Scan all live UObjects for VM_InventoryRowCard_C instances and remove each from
	// the collection.  This includes VoidpierVM (created by the WBP editor preview) as
	// well as any VMs left from previous cell renders.
	for ( TObjectIterator<UObject> It; It; ++It )
	{
		UObject* Obj = *It;
		if ( Obj->HasAnyFlags( RF_ClassDefaultObject ) ) continue;
		if ( !Obj->GetClass()->IsChildOf( VMClass ) ) continue;

		// ProcessEvent param layout matches UFUNCTION(…) int32 RemoveAllViewModelInstance(UObject*)
		struct RemoveParams { UObject* ViewModel = nullptr; int32 ReturnValue = 0; };
		RemoveParams Params{ Obj, 0 };
		Collection->ProcessEvent( RemoveFn, &Params );
	}
}
}  // namespace
#endif

FText UItemSchema::GetTypeDisplayName( const FDataIndexerPrimaryKey& TypeKey ) const
{
	return ItemTypeRepository ? ItemTypeRepository->GetDisplayName( TypeKey ) : FText::GetEmpty();
}

FText UItemSchema::GetRarityDisplayName( const FDataIndexerPrimaryKey& RarityKey ) const
{
	return ItemRarityRepository ? ItemRarityRepository->GetDisplayName( RarityKey ) : FText::GetEmpty();
}

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

	if ( Context.GetColumnName() == InventoryCardColumn )
	{
		// Clear stale VM instances from the MVVM collection BEFORE the BP binding function's
		// CreateWidget runs, so each row gets its own fresh VM (not the shared VoidpierVM).
		ClearInventoryCardVMsFromCollection( GetWorld() );
	}

	return Super::CustomizePropertyCellWidget( Context );
}

EDataValidationResult UItemSchema::IsRowValid( FConstStructView RowEntity, FDataValidationContext& Context ) const
{
	EDataValidationResult Result = Super::IsRowValid( RowEntity, Context );

	const auto AddError = [&]( const FText& Msg )
	{
		Context.AddError( Msg );
		Result = EDataValidationResult::Invalid;
	};

	if ( const FItemRow* Row = RowEntity.GetPtr<const FItemRow>() )
	{
		if ( Row->DisplayName.IsEmpty() )
		{
			AddError( NSLOCTEXT( "ItemSchema", "EmptyDisplayName", "DisplayName must not be empty." ) );
		}
		if ( !Row->Type.IsValid() )
		{
			AddError( NSLOCTEXT( "ItemSchema", "InvalidType", "Type must not be empty." ) );
		}
		if ( !Row->Rarity.IsValid() )
		{
			AddError( NSLOCTEXT( "ItemSchema", "InvalidRarity", "Rarity must not be empty." ) );
		}
	}

	return Result;
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
