using UnrealBuildTool;

public class DataIndexerProjectTarget : TargetRules
{
	public DataIndexerProjectTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DataIndexerProject");
	}
}