#pragma once

#include "DataIndexerRepository.h"
#include "DataIndexerTypes.h"
#include "Engine/DataTable.h"

#include "DataIndexerProjectTestTypes.generated.h"

/**
 * Exposes UDataIndexerRepository's protected internals for automation tests.
 * Mirrors the pattern used by the plugin's own UDataIndexerRepositoryTests.
 */
UCLASS( NotBlueprintable, HideDropdown )
class UItemSchemaTestRepository final : public UDataIndexerRepository
{
	GENERATED_BODY()

public:
	using UDataIndexerRepository::EntryOwners;
	using UDataIndexerRepository::LocalEntries;
	using UDataIndexerRepository::ReverseLookups;
	using UDataIndexerRepository::SchemaClass;
};

/**
 * DataTable row variant of FCharacterRow used by CharacterDataTableMigrationTests.
 * Drops DisplayName (FText) — RowName (FName) carries identity via UDataTable RowName
 * and ImportFromDataTableJSON's RowNameProperty binding.
 */
USTRUCT()
struct FCharacterDataTableTestRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere )
	FName Class;

	UPROPERTY( EditAnywhere )
	int32 MaxHP = 100;

	UPROPERTY( EditAnywhere )
	FDataIndexerPrimaryKey DefaultWeapon;
};
