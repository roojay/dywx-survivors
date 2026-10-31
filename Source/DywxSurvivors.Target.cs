using UnrealBuildTool;
public class DywxSurvivorsTarget : TargetRules
{
    public DywxSurvivorsTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        // Installed-engine Development targets share trace-enabled engine binaries.
        // The player package uses Shipping, whose precompiled engine supports this.
        // Keep Editor and explicit Development builds available for diagnostics.
        if (Configuration == UnrealTargetConfiguration.Shipping)
        {
            bEnableTrace = false;
        }
        ExtraModuleNames.AddRange(new[] { "DywxCore", "DywxRuntime", "DywxUI" });
    }
}
