#pragma once

#include "DataIndexerRepository.h"

#include "GameDataSettings.generated.h"

/**
 * Game singleton that holds repository references for DataIndexer.
 *
 * Set GameSingletonClassName=/Script/DataIndexerProject.GameDataSettings in DefaultEngine.ini,
 * then assign ItemRepository and CharacterRepository in the class defaults.
 * Accessed at runtime via GEngine->GameSingleton.
 */
UCLASS()
class DATAINDEXERPROJECT_API UGameDataSettings : public UObject
{
	GENERATED_BODY()

public:
	static const UGameDataSettings* Get();

	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = GameData,	//
		meta = ( Schema = "/Script/DataIndexerProject.ItemSchema" ) )
	TObjectPtr<UDataIndexerRepository> ItemRepository;

	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = GameData,	//
		meta = ( Schema = "/Script/DataIndexerProject.CharacterSchema" ) )
	TObjectPtr<UDataIndexerRepository> CharacterRepository;
};
