using UnrealBuildTool;
public class DywxSurvivorsTarget : TargetRules
{
    public DywxSurvivorsTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "DywxCore", "DywxRuntime", "DywxUI" });
    }
}
