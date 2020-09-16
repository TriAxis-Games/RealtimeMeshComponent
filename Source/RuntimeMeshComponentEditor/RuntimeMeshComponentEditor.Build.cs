// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RuntimeMeshComponentEditor : ModuleRules
{
    public RuntimeMeshComponentEditor(ReadOnlyTargetRules rules) : base(rules)
    {
        bEnforceIWYU = true;
        bLegacyPublicIncludePaths = false;

#if UE_4_23_OR_LATER
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
#endif

#if UE_4_24_OR_LATER
#else
#endif


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
				// ... add other public dependencies that you statically link with here ...
                
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                // ... add private dependencies that you statically link with here ...	
                "Engine",
                "Slate",
                "SlateCore",
                "RenderCore",
                "RHI",
                "NavigationSystem",
                "UnrealEd",
                "LevelEditor",
                "PropertyEditor",
                "RawMesh",
                "AssetTools",
                "AssetRegistry",
                "Projects",
                "EditorStyle",
                "InputCore",

                "RuntimeMeshComponent",
            }
            );
    }
}