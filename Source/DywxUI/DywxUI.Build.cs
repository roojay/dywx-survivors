using UnrealBuildTool;
public class DywxUI : ModuleRules
{
    public DywxUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "ApplicationCore", "ImageWrapper", "RenderCore" });
        CppStandard = CppStandardVersion.Cpp20;
    }
}
