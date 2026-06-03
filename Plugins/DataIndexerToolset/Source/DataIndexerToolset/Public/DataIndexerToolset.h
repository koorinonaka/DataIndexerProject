// Copyright 2026 koorinonaka, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"

#include "DataIndexerToolset.generated.h"

/**
 * MCP tools for authoring DataIndexer repository assets headlessly.
 *
 * These wrap the editor-only DataIndexer serialization pipeline so an agent can create a
 * repository, import native DataIndexer JSON (which preserves exact PrimaryKey GUIDs), and
 * export it back out for verification -- none of which the generic object/asset tools can do,
 * because the editor row data lives in a non-editable property behind UDataIndexerEditorData.
 */
UCLASS( BlueprintType, Hidden )
class UDataIndexerToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Creates a new DataIndexer repository asset bound to the given schema.
	 * This mirrors the in-editor "DataIndexer Repository" create flow (sets the schema class and
	 * initializes the editor data), so the asset is immediately importable.
	 * @param PackagePath The content folder for the asset, e.g. "/Game/GameData".
	 * @param AssetName The asset name without extension, e.g. "DI_CharacterClass".
	 * @param SchemaClassPath The schema class to bind. Either a native class path
	 *   ("/Script/DataIndexerProject.CharacterClassSchema") or a Blueprint generated-class path
	 *   ("/Game/GameData/DIS_Character.DIS_Character_C").
	 * @return The object path of the created repository ("/Game/GameData/DI_CharacterClass.DI_CharacterClass"),
	 *   or an empty string on failure.
	 */
	UFUNCTION( meta = ( AICallable ), Category = "DataIndexer" )
	static FString CreateRepository( const FString& PackagePath, const FString& AssetName, const FString& SchemaClassPath );

	/**
	 * Imports native DataIndexer JSON into a repository, replacing its rows.
	 * The JSON is the DataIndexer native array form -- each element has "PrimaryKey", "RowEntity",
	 * and "EditorFlags" -- so exact PrimaryKey GUIDs are preserved. The asset is saved on success.
	 * @param RepositoryPath The repository object or package path, e.g. "/Game/GameData/DI_CharacterClass".
	 * @param JSONString The native DataIndexer JSON array as a string.
	 * @return True if the import and save succeeded.
	 */
	UFUNCTION( meta = ( AICallable ), Category = "DataIndexer" )
	static bool ImportRepositoryJSON( const FString& RepositoryPath, const FString& JSONString );

	/**
	 * Same as ImportRepositoryJSON but reads the native JSON from a file on disk.
	 * @param RepositoryPath The repository object or package path.
	 * @param JSONFilePath An absolute path to a .json file containing native DataIndexer JSON.
	 * @return True if the file was read, imported, and saved successfully.
	 */
	UFUNCTION( meta = ( AICallable ), Category = "DataIndexer" )
	static bool ImportRepositoryJSONFile( const FString& RepositoryPath, const FString& JSONFilePath );

	/**
	 * Exports a repository's rows as native DataIndexer JSON for inspection or diffing.
	 * @param RepositoryPath The repository object or package path.
	 * @return The native DataIndexer JSON array string, or an empty string on failure.
	 */
	UFUNCTION( meta = ( AICallable ), Category = "DataIndexer" )
	static FString ExportRepositoryJSON( const FString& RepositoryPath );
};
