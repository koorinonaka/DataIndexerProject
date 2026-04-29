#include "GameDataSettings.h"

#include "Engine/Engine.h"

const UGameDataSettings* UGameDataSettings::Get()
{
	return GEngine ? Cast<UGameDataSettings>( GEngine->GameSingleton ) : nullptr;
}
