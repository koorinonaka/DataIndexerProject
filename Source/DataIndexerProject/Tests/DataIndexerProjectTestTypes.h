#pragma once

#include "DataIndexerRepository.h"

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
	using UDataIndexerRepository::LocalEntries;
	using UDataIndexerRepository::EntryOwners;
	using UDataIndexerRepository::ReverseLookups;
	using UDataIndexerRepository::SchemaClass;
};
