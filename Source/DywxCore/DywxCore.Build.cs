using UnrealBuildTool;

// Standard-library domain code. Core supplies only the UE entry point and conditional platform export shim.
public class DywxCore : ModuleRules
{
    public DywxCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        PublicDependencyModuleNames.Add("Core");
        PublicDefinitions.Add("DYWXCORE_WITH_UNREAL=1");
        CppStandard = CppStandardVersion.Cpp20;
    }
}
