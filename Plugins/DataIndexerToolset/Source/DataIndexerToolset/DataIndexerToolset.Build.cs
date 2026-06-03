// Copyright 2026 koorinonaka, All Rights Reserved.

using UnrealBuildTool;

public class DataIndexerToolset : ModuleRules
{
	public DataIndexerToolset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;
		bUseUnity = true;

		PublicDependencyModuleNames.Add("Core");

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AssetTools",
			"EditorScriptingUtilities",
			"ToolsetRegistry",
			"DataIndexer",
			"DataIndexerEd",
		});
	}
}