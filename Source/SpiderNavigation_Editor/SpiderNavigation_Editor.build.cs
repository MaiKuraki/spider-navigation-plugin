using UnrealBuildTool;

public class SpiderNavigation_Editor : ModuleRules
{
    public SpiderNavigation_Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = true;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "EditorSubsystem",
            "UnrealEd",               // Required for GEditor, editor world access
            "Blutility",              // For UEditorUtilityWidget
            "UMG",                    // For buttons, widget UI
            "UMGEditor",
            "Projects",
            "PropertyEditor",
            "LevelEditor",
            "Kismet",                 // For UGameplayStatics
            "KismetCompiler",
            "RenderCore",
            "RHI",
            "ApplicationCore",
            "Json",
            "JsonUtilities",
            "DeveloperSettings",
            "ToolMenus",
            "PhysicsCore",            // Needed for traces & overlaps
            "NavigationSystem",       // Optional: for nav debug
            "AIModule",               // Optional: for nav helpers
            "EditorWidgets",          // For UEditorUtilityWidget base
            "AppFramework",
            "ContentBrowser",
            "AssetRegistry",
            "AssetTools",
            "Projects",
            "SourceControl",
            "NiagaraEditor",          // Optional, but helps if you use it
            "MessageLog",
            "SpiderNavigation",
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "EditorSubsystem",
                "Blutility",
                "KismetWidgets",
                "UMGEditor",
                "MainFrame",
                "EditorFramework",
                "PropertyEditor",
                "GraphEditor",
                "ApplicationCore",
                "AssetTools",
            });
        }

        // To access FTSTicker (used for delayed async scheduling)
        PrivateDependencyModuleNames.Add("Core");

        // Enable editor-only code paths
        //bBuildLocallyWithSlate = true;
        bTreatAsEngineModule = false;
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
    }
}
