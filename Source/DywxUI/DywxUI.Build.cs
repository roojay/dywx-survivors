using UnrealBuildTool;
public class DywxUI : ModuleRules
{
    public DywxUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "DywxCore", "DywxRuntime" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "ApplicationCore", "ImageWrapper", "RenderCore", "MeshDescription", "StaticMeshDescription", "Json" });
        CppStandard = CppStandardVersion.Cpp20;
    }
}
