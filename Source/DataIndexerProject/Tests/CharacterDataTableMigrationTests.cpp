#if WITH_EDITOR

#include "DataIndexerEditorData.h"
#include "DataIndexerJsonSupport.h"
#include "DataIndexerProjectTestTypes.h"
#include "DataIndexerRepository.h"
#include "Engine/DataTable.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace DataIndexerProject::Tests::CharacterDataTableMigration
{

static void SetEditorData( UDataIndexerRepository& InRepository, UObject* InEditorData )
{
	const auto* EditorDataProperty =
		FindFProperty<FObjectPropertyBase>( UDataIndexerRepository::StaticClass(), TEXT( "EditorData" ) );
	check( EditorDataProperty != nullptr );
	EditorDataProperty->SetObjectPropertyValue_InContainer( &InRepository, InEditorData );
}

static void SetRowStruct( UDataIndexerEditorData& InEditorData, const UScriptStruct* InRowStruct )
{
	const auto* RowStructProperty =
		FindFProperty<FObjectPropertyBase>( UDataIndexerEditorData::StaticClass(), TEXT( "RowStruct" ) );
	check( RowStructProperty != nullptr );
	RowStructProperty->SetObjectPropertyValue_InContainer( &InEditorData, const_cast<UScriptStruct*>( InRowStruct ) );
}

struct FRowTuple
{
	FString Class;	  // Enum name string (normalized across DataTable/DataIndexer output forms).
	int32 MaxHP = 0;
	FString DefaultWeapon;

	bool operator==( const FRowTuple& Other ) const
	{
		return Class.Equals( Other.Class ) && MaxHP == Other.MaxHP &&
			   DefaultWeapon.Equals( Other.DefaultWeapon, ESearchCase::IgnoreCase );
	}
};

// "Warrior" stays "Warrior"; 0 becomes "Warrior". Returns empty on unrecognized input.
static FString NormalizeClassField( const TSharedPtr<FJsonValue>& Value )
{
	if ( !Value.IsValid() )
	{
		return FString();
	}
	if ( Value->Type == EJson::String )
	{
		return Value->AsString();
	}
	if ( Value->Type == EJson::Number )
	{
		const UEnum* Enum = StaticEnum<ECharacterClass>();
		return Enum ? Enum->GetNameStringByValue( static_cast<int64>( Value->AsNumber() ) ) : FString();
	}
	return FString();
}

static bool ReadTupleFromJsonObject( const TSharedPtr<FJsonObject>& Obj, FRowTuple& OutTuple )
{
	if ( !Obj )
	{
		return false;
	}

	OutTuple.Class = NormalizeClassField( Obj->TryGetField( TEXT( "Class" ) ) );
	if ( OutTuple.Class.IsEmpty() )
	{
		return false;
	}

	double MaxHPValue = 0.0;
	if ( !Obj->TryGetNumberField( TEXT( "MaxHP" ), MaxHPValue ) )
	{
		return false;
	}
	if ( !Obj->TryGetStringField( TEXT( "DefaultWeapon" ), OutTuple.DefaultWeapon ) )
	{
		return false;
	}

	OutTuple.MaxHP = static_cast<int32>( MaxHPValue );
	return true;
}

// Order tuples deterministically so two collections can be compared element-wise as multisets.
// Row identity (the DataTable "Name") lives only on the DataTable side, so content is the only
// stable basis for comparison after the row struct drops it.
static bool TupleLess( const FRowTuple& A, const FRowTuple& B )
{
	if ( !A.Class.Equals( B.Class ) )
	{
		return A.Class < B.Class;
	}
	if ( A.MaxHP != B.MaxHP )
	{
		return A.MaxHP < B.MaxHP;
	}
	return A.DefaultWeapon < B.DefaultWeapon;
}

// Build [FRowTuple] from a DataTable-style export:
//   [{"Name":"Arthur","Class":0,"MaxHP":200,"DefaultWeapon":"..."}, ...]
static bool BuildTuplesFromDataTableJson( const FString& JsonString, TArray<FRowTuple>& OutTuples )
{
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create( JsonString );
	if ( !FJsonSerializer::Deserialize( Reader, JsonArray ) )
	{
		return false;
	}

	for ( const TSharedPtr<FJsonValue>& Value : JsonArray )
	{
		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		FRowTuple Tuple;
		if ( !Obj || !ReadTupleFromJsonObject( Obj, Tuple ) )
		{
			return false;
		}
		OutTuples.Emplace( MoveTemp( Tuple ) );
	}

	OutTuples.Sort( TupleLess );
	return true;
}

// Build [FRowTuple] from a DataIndexer-native export:
//   [{"PrimaryKey":"...","RowEntity":{"Class":0,...},"EditorFlags":0}, ...]
// Rows are anonymous on the DataIndexer side (keyed by PrimaryKey), so compare by content only.
static bool BuildTuplesFromDataIndexerJson( const FString& JsonString, TArray<FRowTuple>& OutTuples )
{
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create( JsonString );
	if ( !FJsonSerializer::Deserialize( Reader, JsonArray ) )
	{
		return false;
	}

	for ( const TSharedPtr<FJsonValue>& Value : JsonArray )
	{
		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if ( !Obj )
		{
			return false;
		}

		const TSharedPtr<FJsonObject>* RowEntityPtr = nullptr;
		if ( !Obj->TryGetObjectField( TEXT( "RowEntity" ), RowEntityPtr ) )
		{
			return false;
		}

		FRowTuple Tuple;
		if ( !ReadTupleFromJsonObject( *RowEntityPtr, Tuple ) )
		{
			return false;
		}
		OutTuples.Emplace( MoveTemp( Tuple ) );
	}

	OutTuples.Sort( TupleLess );
	return true;
}

}	 // namespace DataIndexerProject::Tests::CharacterDataTableMigration

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FCharacterDataTableMigration_RoundTrip_Test,
	"DataIndexerProject.Migration.CharacterDataTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FCharacterDataTableMigration_RoundTrip_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::CharacterDataTableMigration;

	// 1) Load fixture.
	const FString FixturePath =
		FPaths::Combine( FPaths::ProjectDir(), TEXT( "Source/DataIndexerProject/Tests/Fixtures/Characters_DataTable.json" ) );

	FString FixtureJson;
	if ( !TestTrue( FString::Printf( TEXT( "Load fixture: %s" ), *FixturePath ),
			 FFileHelper::LoadFileToString( FixtureJson, *FixturePath ) ) )
	{
		return false;
	}

	// 2) DataTable JSON → UDataTable.
	auto* DataTable = NewObject<UDataTable>( GetTransientPackage() );
	DataTable->RowStruct = FCharacterDataTableTestRow::StaticStruct();

	const TArray<FString> CreateErrors = DataTable->CreateTableFromJSONString( FixtureJson );
	if ( !TestEqual( TEXT( "CreateTableFromJSONString produces no errors" ), CreateErrors.Num(), 0 ) )
	{
		for ( const FString& Err : CreateErrors )
		{
			AddError( Err );
		}
		return false;
	}
	if ( !TestEqual( TEXT( "DataTable row count" ), DataTable->GetRowMap().Num(), 2 ) )
	{
		return false;
	}

	// 3) UDataTable → re-exported DataTable JSON.
	const FString ReExportedJson = DataTable->GetTableAsJSON();
	if ( !TestFalse( TEXT( "Re-exported DataTable JSON is non-empty" ), ReExportedJson.IsEmpty() ) )
	{
		return false;
	}

	// 4) Build Repository + EditorData, bind RowName via reflection.
	auto* Repo = NewObject<UDataIndexerRepository>( GetTransientPackage() );
	auto* EditorData = NewObject<UDataIndexerEditorData>( Repo );
	SetEditorData( *Repo, EditorData );
	SetRowStruct( *EditorData, FCharacterDataTableTestRow::StaticStruct() );

	// 5) DataTable JSON → DataIndexer Repository. The row struct carries no name field, so the
	//    DataTable "Name" is not bound (NAME_None); rows become anonymous, keyed by PrimaryKey.
	if ( !TestTrue( TEXT( "ImportFromDataTableJSON should succeed" ),
			 FDataIndexerSerializer::ImportFromDataTableJSON( *Repo, ReExportedJson, NAME_None ) ) )
	{
		return false;
	}

	// 6) DataIndexer Repository → JSON.
	FString DataIndexerJson;
	if ( !TestTrue( TEXT( "ExportAsJSON should succeed" ), FDataIndexerSerializer::ExportAsJSON( *Repo, DataIndexerJson ) ) )
	{
		return false;
	}

	// 7) Parse both sides into sorted [FRowTuple] collections.
	TArray<FRowTuple> DataTableTuples;
	TArray<FRowTuple> DataIndexerTuples;
	if ( !TestTrue( TEXT( "Parse re-exported DataTable JSON" ), BuildTuplesFromDataTableJson( ReExportedJson, DataTableTuples ) ) )
	{
		return false;
	}
	if ( !TestTrue( TEXT( "Parse DataIndexer JSON" ), BuildTuplesFromDataIndexerJson( DataIndexerJson, DataIndexerTuples ) ) )
	{
		return false;
	}

	// 8) Content equality as multisets (structure- and order-agnostic).
	if ( !TestEqual( TEXT( "Same row count" ), DataTableTuples.Num(), DataIndexerTuples.Num() ) )
	{
		return false;
	}

	for ( int32 Index = 0; Index < DataTableTuples.Num(); ++Index )
	{
		TestTrue( *FString::Printf( TEXT( "Row %d fields match" ), Index ), DataTableTuples[Index] == DataIndexerTuples[Index] );
	}

	return true;
}

#endif	  // WITH_EDITOR
