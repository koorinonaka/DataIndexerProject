using UnrealBuildTool;

public class DataIndexerProjectEditorTarget : TargetRules
{
	public DataIndexerProjectEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DataIndexerProject");
	}
}