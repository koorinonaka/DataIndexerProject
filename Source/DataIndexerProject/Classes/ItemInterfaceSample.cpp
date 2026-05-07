#include "ItemInterfaceSample.h"

#include "Schema/ItemSchema.h"

// ── GetDisplayName ────────────────────────────────────────────────────────────

FText FItemInterfaceSample::GetItemDisplayName(
	const UDataIndexerRepository& Repository, const FDataIndexerPrimaryKey& PrimaryKey )
{
	return FItemInterface::GetDisplayName( Repository, PrimaryKey );
}

// ── FindRow(Repository, PrimaryKey) ──────────────────────────────────────────

const FItemRow* FItemInterfaceSample::FindItemRow(
	const UDataIndexerRepository& Repository, const FDataIndexerPrimaryKey& PrimaryKey )
{
	return FItemInterface::FindRow( Repository, PrimaryKey );
}

// ── FindRow(RowHandle) ────────────────────────────────────────────────────────

const FItemRow* FItemInterfaceSample::FindItemRow( const FDataIndexerRowHandle& Handle )
{
	return FItemInterface::FindRow( Handle );
}

// ── ForEachPrimaryKeys() ──────────────────────────────────────────────────────

void FItemInterfaceSample::ForEachItem(
	const UDataIndexerRepository& Repository, const TFunctionRef<void( const FDataIndexerPrimaryKey& )>& Callback )
{
	FItemInterface::ForEachPrimaryKeys( Repository, Callback );
}

// ── ForEachPrimaryKeys(Index, Query) ─────────────────────────────────────────

void FItemInterfaceSample::ForEachItemsByType( const UDataIndexerRepository& Repository, EItemType Type,
	const TFunctionRef<void( const FDataIndexerPrimaryKey& )>& Callback )
{
	FItemRow Query;
	Query.Type = Type;

	FItemInterface::ForEachPrimaryKeys( Repository, UItemSchema::ByTypeIndex(), Query, Callback );
}

// ── GetPrimaryKeys() ──────────────────────────────────────────────────────────

TArray<FDataIndexerPrimaryKey> FItemInterfaceSample::GetAllItemKeys( const UDataIndexerRepository& Repository )
{
	return FItemInterface::GetPrimaryKeys( Repository );
}

// ── GetPrimaryKeys(Index, Query) ──────────────────────────────────────────────

TArray<FDataIndexerPrimaryKey> FItemInterfaceSample::GetItemsByType( const UDataIndexerRepository& Repository, EItemType Type )
{
	FItemRow Query;
	Query.Type = Type;

	return FItemInterface::GetPrimaryKeys( Repository, UItemSchema::ByTypeIndex(), Query );
}

TArray<FDataIndexerPrimaryKey> FItemInterfaceSample::GetItemsByRarity(
	const UDataIndexerRepository& Repository, EItemRarity Rarity )
{
	FItemRow Query;
	Query.Rarity = Rarity;

	return FItemInterface::GetPrimaryKeys( Repository, UItemSchema::ByRarityIndex(), Query );
}

TArray<FDataIndexerPrimaryKey> FItemInterfaceSample::GetItemsByTypeAndRarity(
	const UDataIndexerRepository& Repository, EItemType Type, EItemRarity Rarity )
{
	FItemRow Query;
	Query.Type = Type;
	Query.Rarity = Rarity;

	return FItemInterface::GetPrimaryKeys( Repository, UItemSchema::ByTypeAndRarityIndex(), Query );
}
