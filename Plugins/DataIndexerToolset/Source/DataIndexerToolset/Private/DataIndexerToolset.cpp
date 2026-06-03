// Copyright 2026 koorinonaka, All Rights Reserved.

#include "DataIndexerToolset.h"

#include "AssetToolsModule.h"
#include "DataIndexerEditorData.h"
#include "DataIndexerJsonSupport.h"
#include "DataIndexerRepository.h"
#include "DataIndexerSchema.h"
#include "EditorAssetLibrary.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DataIndexerToolset)

namespace
{
// Accepts either an object path ("/Game/X/DI_Foo.DI_Foo") or a package path ("/Game/X/DI_Foo")
// and resolves it to the repository asset.
UDataIndexerRepository* LoadRepository( const FString& RepositoryPath )
{
	FString ObjectPath = RepositoryPath;
	if ( !ObjectPath.Contains( TEXT( "." ) ) )
	{
		ObjectPath = ObjectPath + TEXT( "." ) + FPackageName::GetShortName( RepositoryPath );
	}

	return LoadObject<UDataIndexerRepository>( nullptr, *ObjectPath );
}

// Replicates the post-import notification the reimport handler performs so the repository is
// marked dirty and its cached "clean" view (indexes) is rebuilt on next access.
void NotifyRowsChanged( UDataIndexerRepository& Repository )
{
	if ( UDataIndexerEditorData* EditorData = UDataIndexerEditorData::Get( Repository ) )
	{
		FProperty* EditorRowsProperty = FindFProperty<FProperty>( UDataIndexerEditorData::StaticClass(), TEXT( "EditorRows" ) );
		const FPropertyChangedEvent ChangeEvent( EditorRowsProperty );
		EditorData->NotifyPostChange( ChangeEvent, EditorRowsProperty );
	}
}
}	 // namespace

FString UDataIndexerToolset::CreateRepository(
	const FString& PackagePath, const FString& AssetName, const FString& SchemaClassPath )
{
	UClass* SchemaClass = LoadObject<UClass>( nullptr, *SchemaClassPath );
	if ( !SchemaClass || !SchemaClass->IsChildOf( UDataIndexerSchema::StaticClass() ) )
	{
		UE_LOG( LogTemp, Warning, TEXT( "DataIndexerToolset::CreateRepository: invalid schema class '%s'" ), *SchemaClassPath );
		return {};
	}

	UClass* FactoryClass = LoadObject<UClass>( nullptr, TEXT( "/Script/DataIndexerEd.DataIndexerRepositoryFactory" ) );
	if ( !FactoryClass )
	{
		UE_LOG( LogTemp, Warning, TEXT( "DataIndexerToolset::CreateRepository: DataIndexerRepositoryFactory not found" ) );
		return {};
	}

	UFactory* Factory = NewObject<UFactory>( GetTransientPackage(), FactoryClass );
	if ( FObjectPropertyBase* SchemaProp = FindFProperty<FObjectPropertyBase>( FactoryClass, TEXT( "SchemaClass" ) ) )
	{
		SchemaProp->SetObjectPropertyValue_InContainer( Factory, SchemaClass );
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>( "AssetTools" ).Get();
	UObject* NewAsset = AssetTools.CreateAsset( AssetName, PackagePath, UDataIndexerRepository::StaticClass(), Factory );
	if ( !NewAsset )
	{
		return {};
	}

	UEditorAssetLibrary::SaveLoadedAsset( NewAsset, false );
	return NewAsset->GetPathName();
}

bool UDataIndexerToolset::ImportRepositoryJSON( const FString& RepositoryPath, const FString& JSONString )
{
	UDataIndexerRepository* Repository = LoadRepository( RepositoryPath );
	if ( !Repository )
	{
		UE_LOG( LogTemp, Warning, TEXT( "DataIndexerToolset::ImportRepositoryJSON: repository '%s' not found" ), *RepositoryPath );
		return false;
	}

	if ( !FDataIndexerSerializer::ImportAsJSON( *Repository, JSONString ) )
	{
		UE_LOG( LogTemp, Warning, TEXT( "DataIndexerToolset::ImportRepositoryJSON: import failed for '%s'" ), *RepositoryPath );
		return false;
	}

	NotifyRowsChanged( *Repository );
	return UEditorAssetLibrary::SaveLoadedAsset( Repository, false );
}

bool UDataIndexerToolset::ImportRepositoryJSONFile( const FString& RepositoryPath, const FString& JSONFilePath )
{
	FString JSONString;
	if ( !FFileHelper::LoadFileToString( JSONString, *JSONFilePath ) )
	{
		UE_LOG( LogTemp, Warning, TEXT( "DataIndexerToolset::ImportRepositoryJSONFile: cannot read '%s'" ), *JSONFilePath );
		return false;
	}

	return ImportRepositoryJSON( RepositoryPath, JSONString );
}

FString UDataIndexerToolset::ExportRepositoryJSON( const FString& RepositoryPath )
{
	UDataIndexerRepository* Repository = LoadRepository( RepositoryPath );
	if ( !Repository )
	{
		return {};
	}

	FString OutString;
	if ( !FDataIndexerSerializer::ExportAsJSON( *Repository, OutString ) )
	{
		return {};
	}

	return OutString;
}
