using UnrealBuildTool;
using System.Collections.Generic;

public class RunikEditorTarget : TargetRules
{
	public RunikEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Runik");
	}
}
