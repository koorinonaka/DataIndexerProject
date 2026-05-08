#if WITH_EDITOR

#include "DataIndexerProjectTestTypes.h"
#include "JsonObjectConverter.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Mock/ItemInterfaceMock.h"
#include "Schema/ItemSchema.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StructUtils/InstancedStruct.h"

// ── Fixture GUIDs — must match Tests/Fixtures/Items.json ─────────────────────
//
//  FGuid(A, B, C, D).ToString(DigitsWithHyphens) == "AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD"
//  where B/C are each split at bit 16.
//
//  FGuid(0x00000001, 0, 0, N) → "00000001-0000-0000-0000-0000000000NN"

namespace DataIndexerProject::Tests::ItemFixture
{

static const FDataIndexerPrimaryKey IronSwordKey( FGuid( 0x00000001, 0, 0, 0x00000001 ) );
static const FDataIndexerPrimaryKey SteelSwordKey( FGuid( 0x00000001, 0, 0, 0x00000002 ) );
static const FDataIndexerPrimaryKey IronShieldKey( FGuid( 0x00000001, 0, 0, 0x00000003 ) );
static const FDataIndexerPrimaryKey SilverRingKey( FGuid( 0x00000001, 0, 0, 0x00000004 ) );
static const FDataIndexerPrimaryKey BattleAxeKey( FGuid( 0x00000001, 0, 0, 0x00000005 ) );

static void AddItem( UItemSchemaTestRepository* Repo, const FDataIndexerPrimaryKey& Key, const FItemRow& Row )
{
	Repo->LocalEntries.Emplace( Key, FInstancedStruct::Make( Row ) );
	Repo->EntryOwners.Emplace( Key, Repo );

	Repo->ReverseLookups.FindOrAdd( UItemSchema::ByTypeIndex() )
		.Entries.FindOrAdd( FGuid( static_cast<uint32>( Row.Type ), 0, 0, 0 ) )
		.Add( Key );
	Repo->ReverseLookups.FindOrAdd( UItemSchema::ByRarityIndex() )
		.Entries.FindOrAdd( FGuid( static_cast<uint32>( Row.Rarity ), 0, 0, 0 ) )
		.Add( Key );
	Repo->ReverseLookups.FindOrAdd( UItemSchema::ByTypeAndRarityIndex() )
		.Entries.FindOrAdd( FGuid( static_cast<uint32>( Row.Type ), static_cast<uint32>( Row.Rarity ), 0, 0 ) )
		.Add( Key );
}

static UItemSchemaTestRepository* LoadItemFixture( FAutomationTestBase& Test )
{
	const FString FixturePath =
		FPaths::Combine( FPaths::ProjectDir(), TEXT( "Source/DataIndexerProject/Tests/Fixtures/Items.json" ) );

	FString JsonContent;
	if ( !FFileHelper::LoadFileToString( JsonContent, *FixturePath ) )
	{
		Test.AddError( FString::Printf( TEXT( "Could not load fixture: %s" ), *FixturePath ) );
		return nullptr;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create( JsonContent );
	if ( !FJsonSerializer::Deserialize( Reader, JsonArray ) )
	{
		Test.AddError( TEXT( "Failed to parse Items.json" ) );
		return nullptr;
	}

	auto* Repo = NewObject<UItemSchemaTestRepository>( GetTransientPackage() );
	Repo->SchemaClass = UItemSchema::StaticClass();

	for ( const TSharedPtr<FJsonValue>& Entry : JsonArray )
	{
		const TSharedPtr<FJsonObject> Obj = Entry->AsObject();
		if ( !Obj )
		{
			continue;
		}

		FString GuidStr;
		if ( !Obj->TryGetStringField( TEXT( "PrimaryKey" ), GuidStr ) )
		{
			continue;
		}

		FGuid Guid;
		if ( !FGuid::Parse( GuidStr, Guid ) )
		{
			continue;
		}

		const TSharedPtr<FJsonObject>* RowEntityPtr;
		if ( !Obj->TryGetObjectField( TEXT( "RowEntity" ), RowEntityPtr ) )
		{
			continue;
		}

		FItemRow Row;
		FJsonObjectConverter::JsonObjectToUStruct( RowEntityPtr->ToSharedRef(), FItemRow::StaticStruct(), &Row, 0, 0 );

		AddItem( Repo, FDataIndexerPrimaryKey( Guid ), Row );
	}

	return Repo;
}

}	 // namespace DataIndexerProject::Tests::ItemFixture

// ── Tests ─────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_GetDisplayName_Test, "DataIndexerProject.ItemInterface.GetDisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_GetDisplayName_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	const FText Name = FItemInterfaceMock::GetItemDisplayName( *Repo, IronSwordKey );
	TestTrue( TEXT( "Iron Sword display name" ), Name.ToString() == TEXT( "Iron Sword" ) );

	const FText MissingName = FItemInterfaceMock::GetItemDisplayName( *Repo, FDataIndexerPrimaryKey( FGuid( 0xDEAD, 0, 0, 0 ) ) );
	TestTrue( TEXT( "Missing key returns empty text" ), MissingName.IsEmpty() );

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_FindRowByKey_Test, "DataIndexerProject.ItemInterface.FindRowByKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_FindRowByKey_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	// Happy path — Iron Sword
	const FItemRow* Row = FItemInterfaceMock::FindItemRow( *Repo, IronSwordKey );
	if ( !TestNotNull( TEXT( "Iron Sword row found" ), Row ) )
	{
		return false;
	}
	TestTrue( TEXT( "Iron Sword type is Weapon" ), Row->Type == EItemType::Weapon );
	TestTrue( TEXT( "Iron Sword rarity is Common" ), Row->Rarity == EItemRarity::Common );
	TestEqual( TEXT( "Iron Sword base value" ), Row->BaseValue, 100 );

	// Missing key
	TestNull( TEXT( "Missing key returns null" ),
		FItemInterfaceMock::FindItemRow( *Repo, FDataIndexerPrimaryKey( FGuid( 0xDEAD, 0, 0, 0 ) ) ) );

	// Battle Axe is Weapon/Epic
	const FItemRow* BattleAxeRow = FItemInterfaceMock::FindItemRow( *Repo, BattleAxeKey );
	if ( !TestNotNull( TEXT( "Battle Axe row found" ), BattleAxeRow ) )
	{
		return false;
	}
	TestTrue( TEXT( "Battle Axe type is Weapon" ), BattleAxeRow->Type == EItemType::Weapon );
	TestTrue( TEXT( "Battle Axe rarity is Epic" ), BattleAxeRow->Rarity == EItemRarity::Epic );

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_FindRowByHandle_Test, "DataIndexerProject.ItemInterface.FindRowByHandle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_FindRowByHandle_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	// Valid handle
	FDataIndexerRowHandle Handle;
	Handle.Repository = Repo;
	Handle.PrimaryKey = SilverRingKey;

	const FItemRow* Row = FItemInterfaceMock::FindItemRow( Handle );
	if ( !TestNotNull( TEXT( "Silver Ring via handle found" ), Row ) )
	{
		return false;
	}
	TestTrue( TEXT( "Silver Ring type is Accessory" ), Row->Type == EItemType::Accessory );
	TestTrue( TEXT( "Silver Ring rarity is Rare" ), Row->Rarity == EItemRarity::Rare );

	// Null repository
	FDataIndexerRowHandle NullRepoHandle;
	NullRepoHandle.Repository = nullptr;
	NullRepoHandle.PrimaryKey = SilverRingKey;
	TestNull( TEXT( "Null repository returns null" ), FItemInterfaceMock::FindItemRow( NullRepoHandle ) );

	// Invalid key in valid repository
	FDataIndexerRowHandle BadKeyHandle;
	BadKeyHandle.Repository = Repo;
	BadKeyHandle.PrimaryKey = FDataIndexerPrimaryKey( FGuid( 0xDEAD, 0, 0, 0 ) );
	TestNull( TEXT( "Bad key in valid repo returns null" ), FItemInterfaceMock::FindItemRow( BadKeyHandle ) );

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_ForEachItem_Test, "DataIndexerProject.ItemInterface.ForEachItem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_ForEachItem_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	int32 Count = 0;
	FItemInterfaceMock::ForEachItem( *Repo, [&]( const FDataIndexerPrimaryKey& ) { ++Count; } );

	TestEqual( TEXT( "ForEachItem visits all 5 rows" ), Count, 5 );

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_ForEachItemsByType_Test, "DataIndexerProject.ItemInterface.ForEachItemsByType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_ForEachItemsByType_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	// Weapon: Iron Sword, Steel Sword, Battle Axe = 3
	int32 WeaponCount = 0;
	FItemInterfaceMock::ForEachItemsByType( *Repo, EItemType::Weapon, [&]( const FDataIndexerPrimaryKey& ) { ++WeaponCount; } );
	TestEqual( TEXT( "ForEachItemsByType Weapon = 3" ), WeaponCount, 3 );

	// Armor: Iron Shield = 1
	int32 ArmorCount = 0;
	FItemInterfaceMock::ForEachItemsByType( *Repo, EItemType::Armor, [&]( const FDataIndexerPrimaryKey& ) { ++ArmorCount; } );
	TestEqual( TEXT( "ForEachItemsByType Armor = 1" ), ArmorCount, 1 );

	// Material: none = 0
	int32 MaterialCount = 0;
	FItemInterfaceMock::ForEachItemsByType( *Repo, EItemType::Material, [&]( const FDataIndexerPrimaryKey& ) { ++MaterialCount; } );
	TestEqual( TEXT( "ForEachItemsByType Material = 0" ), MaterialCount, 0 );

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_GetAllItemKeys_Test, "DataIndexerProject.ItemInterface.GetAllItemKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_GetAllItemKeys_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	const TArray<FDataIndexerPrimaryKey> Keys = FItemInterfaceMock::GetAllItemKeys( *Repo );
	TestEqual( TEXT( "GetAllItemKeys returns 5 keys" ), Keys.Num(), 5 );

	TestTrue( TEXT( "Contains Iron Sword" ), Keys.Contains( IronSwordKey ) );
	TestTrue( TEXT( "Contains Steel Sword" ), Keys.Contains( SteelSwordKey ) );
	TestTrue( TEXT( "Contains Iron Shield" ), Keys.Contains( IronShieldKey ) );
	TestTrue( TEXT( "Contains Silver Ring" ), Keys.Contains( SilverRingKey ) );
	TestTrue( TEXT( "Contains Battle Axe" ), Keys.Contains( BattleAxeKey ) );

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_GetItemsByType_Test, "DataIndexerProject.ItemInterface.GetItemsByType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_GetItemsByType_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	// Weapon: Iron Sword, Steel Sword, Battle Axe
	const TArray<FDataIndexerPrimaryKey> Weapons = FItemInterfaceMock::GetItemsByType( *Repo, EItemType::Weapon );
	TestEqual( TEXT( "GetItemsByType Weapon = 3" ), Weapons.Num(), 3 );
	TestTrue( TEXT( "Weapons contains Iron Sword" ), Weapons.Contains( IronSwordKey ) );
	TestTrue( TEXT( "Weapons contains Steel Sword" ), Weapons.Contains( SteelSwordKey ) );
	TestTrue( TEXT( "Weapons contains Battle Axe" ), Weapons.Contains( BattleAxeKey ) );

	// Accessory: Silver Ring only
	const TArray<FDataIndexerPrimaryKey> Accessories = FItemInterfaceMock::GetItemsByType( *Repo, EItemType::Accessory );
	TestEqual( TEXT( "GetItemsByType Accessory = 1" ), Accessories.Num(), 1 );
	TestTrue( TEXT( "Accessories contains Silver Ring" ), Accessories.Contains( SilverRingKey ) );

	// Material: empty
	const TArray<FDataIndexerPrimaryKey> Materials = FItemInterfaceMock::GetItemsByType( *Repo, EItemType::Material );
	TestEqual( TEXT( "GetItemsByType Material = 0" ), Materials.Num(), 0 );

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_GetItemsByRarity_Test, "DataIndexerProject.ItemInterface.GetItemsByRarity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_GetItemsByRarity_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	// Common: Iron Sword, Steel Sword
	const TArray<FDataIndexerPrimaryKey> Commons = FItemInterfaceMock::GetItemsByRarity( *Repo, EItemRarity::Common );
	TestEqual( TEXT( "GetItemsByRarity Common = 2" ), Commons.Num(), 2 );
	TestTrue( TEXT( "Common contains Iron Sword" ), Commons.Contains( IronSwordKey ) );
	TestTrue( TEXT( "Common contains Steel Sword" ), Commons.Contains( SteelSwordKey ) );

	// Rare: Silver Ring only
	const TArray<FDataIndexerPrimaryKey> Rares = FItemInterfaceMock::GetItemsByRarity( *Repo, EItemRarity::Rare );
	TestEqual( TEXT( "GetItemsByRarity Rare = 1" ), Rares.Num(), 1 );
	TestTrue( TEXT( "Rare contains Silver Ring" ), Rares.Contains( SilverRingKey ) );

	// Legendary: empty
	const TArray<FDataIndexerPrimaryKey> Legendaries = FItemInterfaceMock::GetItemsByRarity( *Repo, EItemRarity::Legendary );
	TestEqual( TEXT( "GetItemsByRarity Legendary = 0" ), Legendaries.Num(), 0 );

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST( FItemInterface_GetItemsByTypeAndRarity_Test,
	"DataIndexerProject.ItemInterface.GetItemsByTypeAndRarity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter )

bool FItemInterface_GetItemsByTypeAndRarity_Test::RunTest( const FString& Parameters )
{
	using namespace DataIndexerProject::Tests::ItemFixture;

	UItemSchemaTestRepository* Repo = LoadItemFixture( *this );
	if ( !TestNotNull( TEXT( "Fixture loaded" ), Repo ) )
	{
		return false;
	}

	// Weapon + Common: Iron Sword, Steel Sword
	const TArray<FDataIndexerPrimaryKey> WeaponCommon =
		FItemInterfaceMock::GetItemsByTypeAndRarity( *Repo, EItemType::Weapon, EItemRarity::Common );
	TestEqual( TEXT( "Weapon+Common = 2" ), WeaponCommon.Num(), 2 );
	TestTrue( TEXT( "Weapon+Common contains Iron Sword" ), WeaponCommon.Contains( IronSwordKey ) );
	TestTrue( TEXT( "Weapon+Common contains Steel Sword" ), WeaponCommon.Contains( SteelSwordKey ) );
	TestFalse( TEXT( "Weapon+Common excludes Battle Axe" ), WeaponCommon.Contains( BattleAxeKey ) );

	// Weapon + Epic: Battle Axe only
	const TArray<FDataIndexerPrimaryKey> WeaponEpic =
		FItemInterfaceMock::GetItemsByTypeAndRarity( *Repo, EItemType::Weapon, EItemRarity::Epic );
	TestEqual( TEXT( "Weapon+Epic = 1" ), WeaponEpic.Num(), 1 );
	TestTrue( TEXT( "Weapon+Epic contains Battle Axe" ), WeaponEpic.Contains( BattleAxeKey ) );

	// Armor + Common: empty (Iron Shield is Uncommon)
	const TArray<FDataIndexerPrimaryKey> ArmorCommon =
		FItemInterfaceMock::GetItemsByTypeAndRarity( *Repo, EItemType::Armor, EItemRarity::Common );
	TestEqual( TEXT( "Armor+Common = 0" ), ArmorCommon.Num(), 0 );

	return true;
}

#endif	  // WITH_EDITOR
