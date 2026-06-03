// Copyright 2026 koorinonaka, All Rights Reserved.

#include "DataIndexerToolset.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

class FDataIndexerToolsetModule : public IModuleInterface
{
public:
	virtual void StartupModule() override { UToolsetRegistry::RegisterToolsetClass( UDataIndexerToolset::StaticClass() ); }
	virtual void ShutdownModule() override { UToolsetRegistry::UnregisterToolsetClass( UDataIndexerToolset::StaticClass() ); }
};

IMPLEMENT_MODULE( FDataIndexerToolsetModule, DataIndexerToolset );
