#pragma once

#include "DataIndexerTypes.h"
#include "Subsystems/EngineSubsystem.h"
#include "Types/CharacterTypes.h"
#include "Types/ItemTypes.h"

#include "GameDataSubsystem.generated.h"

class UGameDataSettings;

/**
 * Engine subsystem exposing typed query methods over DataIndexer repositories.
 *
 * Repository references live in UGameDataSettings (Engine.GameSingletonClassName).
 *
 * Pattern summary:
 *   GetItemsByType / GetItemsByRarity / GetCharactersByClass
 *     → forward index lookup via TNativeSchemaInterface::GetPrimaryKeys
 *   GetCharactersUsingWeapon
 *     → reverse lookup (Item PrimaryKey → Characters) via ByDefaultWeaponIndex
 *   GetDefaultWeapon
 *     → A→B relation: CharacterRow.DefaultWeapon RowHandle → FItemRow
 */
UCLASS()
class DATAINDEXERPROJECT_API UGameDataSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	// ── Forward index lookups ──────────────────────────────────────────────

	// All items whose Type == the given value.
	TArray<FDataIndexerPrimaryKey> GetItemsByType( EItemType Type ) const;

	// All items whose Rarity == the given value.
	TArray<FDataIndexerPrimaryKey> GetItemsByRarity( EItemRarity Rarity ) const;

	// All items matching both Type and Rarity simultaneously.
	TArray<FDataIndexerPrimaryKey> GetItemsByTypeAndRarity( EItemType Type, EItemRarity Rarity ) const;

	// All characters whose Class == the given value.
	TArray<FDataIndexerPrimaryKey> GetCharactersByClass( ECharacterClass Class ) const;

	// ── Reverse index lookup ───────────────────────────────────────────────

	// All characters whose DefaultWeapon points to WeaponKey.
	TArray<FDataIndexerPrimaryKey> GetCharactersUsingWeapon( const FDataIndexerPrimaryKey& WeaponKey ) const;

	// ── Relation resolution ────────────────────────────────────────────────

	// Fetches the FItemRow for a character's DefaultWeapon (A→B: Character→Item).
	TConstStructView<FItemRow> GetDefaultWeapon( const FDataIndexerPrimaryKey& CharacterKey ) const;

};
