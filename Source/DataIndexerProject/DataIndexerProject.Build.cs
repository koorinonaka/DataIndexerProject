using UnrealBuildTool;

public class DataIndexerProject : ModuleRules
{
	public DataIndexerProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange([
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"SlateCore",
			"DataIndexer",
		]);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange([
				"Json",
				"JsonUtilities",
				"DataIndexerEd",
			]);
		}
	}
}