// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RuntimeMeshComponent : ModuleRules
{
    public RuntimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        bEnforceIWYU = true;
        bLegacyPublicIncludePaths = false;

#if UE_4_23_OR_LATER
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
#endif

#if UE_4_24_OR_LATER
#else
#endif

        // Setup the pro/community definitions
        PublicDefinitions.Add("RUNTIMEMESHCOMPONENT_PRO=0");
        PublicDefinitions.Add("RUNTIMEMESHCOMPONENT_VERSION=TEXT(\"Community\")");

        // This is to access RayTracing Definitions
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Shaders", "Shared"));

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "RenderCore",
                "RHI",
                "NavigationSystem",
#if UE_4_23_OR_LATER
                "PhysicsCore",
#endif
#if UE_4_26_OR_LATER
				"DeveloperSettings",
#endif		
            }
            );
    }
}