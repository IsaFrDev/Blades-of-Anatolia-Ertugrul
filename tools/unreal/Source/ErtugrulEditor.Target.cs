using UnrealBuildTool;
using System.Collections.Generic;

public class ErtugrulEditorTarget : TargetRules
{
	public ErtugrulEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Ertugrul");
	}
}
