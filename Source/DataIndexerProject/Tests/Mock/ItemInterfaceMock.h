#pragma once

#include "DataIndexerTypes.h"
#include "Types/ItemTypes.h"

class UDataIndexerRepository;

/**
 * Mock for all TNativeSchemaInterface<FItemRow> methods.
 * Used as test fixture to verify FItemInterface behaviour end-to-end.
 * Mirrors the method order of TNativeSchemaInterface.
 */
struct FItemInterfaceMock
{
	// ── GetDisplayName ─────────────────────────────────────────────────────────

	static FText GetItemDisplayName( const UDataIndexerRepository& Repository, const FDataIndexerPrimaryKey& PrimaryKey );

	// ── FindRow(Repository, PrimaryKey) ───────────────────────────────────────

	static const FItemRow* FindItemRow( const UDataIndexerRepository& Repository, const FDataIndexerPrimaryKey& PrimaryKey );

	// ── FindRow(RowHandle) ────────────────────────────────────────────────────

	static const FItemRow* FindItemRow( const FDataIndexerRowHandle& Handle );

	// ── ForEachPrimaryKeys() — all ────────────────────────────────────────────

	static void ForEachItem(
		const UDataIndexerRepository& Repository, const TFunctionRef<void( const FDataIndexerPrimaryKey& )>& Callback );

	// ── ForEachPrimaryKeys(Index, Query) — filtered ───────────────────────────

	static void ForEachItemsByType( const UDataIndexerRepository& Repository, EItemType Type,
		const TFunctionRef<void( const FDataIndexerPrimaryKey& )>& Callback );

	// ── GetPrimaryKeys() — all ────────────────────────────────────────────────

	static TArray<FDataIndexerPrimaryKey> GetAllItemKeys( const UDataIndexerRepository& Repository );

	// ── GetPrimaryKeys(Index, Query) — filtered ───────────────────────────────

	static TArray<FDataIndexerPrimaryKey> GetItemsByType( const UDataIndexerRepository& Repository, EItemType Type );

	static TArray<FDataIndexerPrimaryKey> GetItemsByRarity( const UDataIndexerRepository& Repository, EItemRarity Rarity );

	static TArray<FDataIndexerPrimaryKey> GetItemsByTypeAndRarity(
		const UDataIndexerRepository& Repository, EItemType Type, EItemRarity Rarity );
};
