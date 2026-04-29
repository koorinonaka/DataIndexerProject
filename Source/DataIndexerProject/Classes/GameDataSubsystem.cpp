#include "GameDataSubsystem.h"

#include "GameDataSettings.h"
#include "Schema/CharacterSchema.h"
#include "Schema/ItemSchema.h"

// ── Forward index lookups ──────────────────────────────────────────────────────

TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByType( EItemType Type ) const
{
	const UGameDataSettings* Settings = UGameDataSettings::Get();
	if ( !Settings || !Settings->ItemRepository )
	{
		return {};
	}

	FItemRow Query;
	Query.Type = Type;

	return FItemInterface::GetPrimaryKeys( *Settings->ItemRepository, UItemSchema::ByTypeIndex().Identifier, Query );
}

TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByRarity( EItemRarity Rarity ) const
{
	const UGameDataSettings* Settings = UGameDataSettings::Get();
	if ( !Settings || !Settings->ItemRepository )
	{
		return {};
	}

	FItemRow Query;
	Query.Rarity = Rarity;

	return FItemInterface::GetPrimaryKeys(
		*Settings->ItemRepository, FDataIndexerIndex( UItemSchema::ByRarityIndex().Identifier ), Query );
}

TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByTypeAndRarity(
	EItemType Type, EItemRarity Rarity ) const
{
	const UGameDataSettings* Settings = UGameDataSettings::Get();
	if ( !Settings || !Settings->ItemRepository )
	{
		return {};
	}

	FItemRow Query;
	Query.Type = Type;
	Query.Rarity = Rarity;

	return FItemInterface::GetPrimaryKeys(
		*Settings->ItemRepository, UItemSchema::ByTypeAndRarityIndex(), Query );
}

TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetCharactersByClass( ECharacterClass Class ) const
{
	const UGameDataSettings* Settings = UGameDataSettings::Get();
	if ( !Settings || !Settings->CharacterRepository )
	{
		return {};
	}

	FCharacterRow Query;
	Query.Class = Class;

	return FCharacterInterface::GetPrimaryKeys(
		*Settings->CharacterRepository, FDataIndexerIndex( UCharacterSchema::ByClassIndex().Identifier ), Query );
}

// ── Reverse index lookup ───────────────────────────────────────────────────────

TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetCharactersUsingWeapon( const FDataIndexerPrimaryKey& WeaponKey ) const
{
	const UGameDataSettings* Settings = UGameDataSettings::Get();
	if ( !Settings || !Settings->CharacterRepository )
	{
		return {};
	}

	// BuildDefaultWeaponKey stores DefaultWeapon.PrimaryKey as the index key,
	// so the query key is simply the weapon's PrimaryKey cast to FDataIndexerIndexValue.
	TArray<FDataIndexerPrimaryKey> Result;
	Settings->CharacterRepository->ForEachPrimaryKeys( UCharacterSchema::ByDefaultWeaponIndex(),
		FDataIndexerIndexValue( WeaponKey ), [&]( const FDataIndexerPrimaryKey& Key ) { Result.Emplace( Key ); } );

	return Result;
}

// ── Relation resolution ────────────────────────────────────────────────────────

TConstStructView<FItemRow> UGameDataSubsystem::GetDefaultWeapon( const FDataIndexerPrimaryKey& CharacterKey ) const
{
	const UGameDataSettings* Settings = UGameDataSettings::Get();
	if ( !Settings || !Settings->CharacterRepository || !Settings->ItemRepository )
	{
		return TConstStructView<FItemRow>{};
	}

	// Step 1: fetch the character row (A)
	const TConstStructView<FCharacterRow> Character = FCharacterInterface::FindRow( *Settings->CharacterRepository, CharacterKey );
	if ( !Character.IsValid() )
	{
		return TConstStructView<FItemRow>{};
	}

	// Step 2: resolve the relation to ItemRepository (A→B)
	return FItemInterface::FindRow( *Settings->ItemRepository, Character.Get().DefaultWeapon.PrimaryKey );
}
