using UnrealBuildTool;
 
public class SpiderNavigation_Editor : ModuleRules
{
	public SpiderNavigation_Editor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UnrealEd", "SpiderNavigation", "Blutility", "UMG"});
 
		PublicIncludePaths.AddRange(new string[] {"SpiderNavigation_Editor/Public"});
		PrivateIncludePaths.AddRange(new string[] {"SpiderNavigation_Editor/Private"});
	}
}