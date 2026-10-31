using UnrealBuildTool;
public class DywxSurvivorsEditorTarget : TargetRules
{
    public DywxSurvivorsEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "DywxCore", "DywxRuntime", "DywxUI" });
    }
}
