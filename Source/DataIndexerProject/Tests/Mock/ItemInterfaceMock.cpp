#include "ItemInterfaceMock.h"

#include "Schema/ItemSchema.h"

// ── GetDisplayName ────────────────────────────────────────────────────────────

FText FItemInterfaceMock::GetItemDisplayName( const UDataIndexerRepository& Repository, const FDataIndexerPrimaryKey& PrimaryKey )
{
	return FItemInterface::GetDisplayName( Repository, PrimaryKey );
}

// ── FindRow(Repository, PrimaryKey) ──────────────────────────────────────────

const FItemRow* FItemInterfaceMock::FindItemRow(
	const UDataIndexerRepository& Repository, const FDataIndexerPrimaryKey& PrimaryKey )
{
	return FItemInterface::FindRow( Repository, PrimaryKey );
}

// ── FindRow(RowHandle) ────────────────────────────────────────────────────────

const FItemRow* FItemInterfaceMock::FindItemRow( const FDataIndexerRowHandle& Handle )
{
	return FItemInterface::FindRow( Handle );
}

// ── ForEachPrimaryKeys() ──────────────────────────────────────────────────────

void FItemInterfaceMock::ForEachItem(
	const UDataIndexerRepository& Repository, const TFunctionRef<void( const FDataIndexerPrimaryKey& )>& Callback )
{
	FItemInterface::ForEachPrimaryKeys( Repository, Callback );
}

// ── ForEachPrimaryKeys(Index, Query) ─────────────────────────────────────────

void FItemInterfaceMock::ForEachItemsByType(
	const UDataIndexerRepository& Repository, EItemType Type, const TFunctionRef<void( const FDataIndexerPrimaryKey& )>& Callback )
{
	FItemRow Query;
	Query.Type = Type;

	FItemInterface::ForEachPrimaryKeys( Repository, UItemSchema::ByTypeIndex(), Query, Callback );
}

// ── GetPrimaryKeys() ──────────────────────────────────────────────────────────

TArray<FDataIndexerPrimaryKey> FItemInterfaceMock::GetAllItemKeys( const UDataIndexerRepository& Repository )
{
	return FItemInterface::GetPrimaryKeys( Repository );
}

// ── GetPrimaryKeys(Index, Query) ──────────────────────────────────────────────

TArray<FDataIndexerPrimaryKey> FItemInterfaceMock::GetItemsByType( const UDataIndexerRepository& Repository, EItemType Type )
{
	FItemRow Query;
	Query.Type = Type;

	return FItemInterface::GetPrimaryKeys( Repository, UItemSchema::ByTypeIndex(), Query );
}

TArray<FDataIndexerPrimaryKey> FItemInterfaceMock::GetItemsByRarity( const UDataIndexerRepository& Repository, EItemRarity Rarity )
{
	FItemRow Query;
	Query.Rarity = Rarity;

	return FItemInterface::GetPrimaryKeys( Repository, UItemSchema::ByRarityIndex(), Query );
}

TArray<FDataIndexerPrimaryKey> FItemInterfaceMock::GetItemsByTypeAndRarity(
	const UDataIndexerRepository& Repository, EItemType Type, EItemRarity Rarity )
{
	FItemRow Query;
	Query.Type = Type;
	Query.Rarity = Rarity;

	return FItemInterface::GetPrimaryKeys( Repository, UItemSchema::ByTypeAndRarityIndex(), Query );
}
