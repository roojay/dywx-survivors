using UnrealBuildTool;

public class DywxRuntime : ModuleRules
{
    public DywxRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "DywxCore" });
        PrivateDependencyModuleNames.Add("Json");
        CppStandard = CppStandardVersion.Cpp20;
    }
}
