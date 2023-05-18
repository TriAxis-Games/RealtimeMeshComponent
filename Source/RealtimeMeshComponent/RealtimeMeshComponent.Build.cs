// Copyright TriAxis Games, L.L.C. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RealtimeMeshComponent : ModuleRules
{
    public RealtimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        bEnforceIWYU = true;
        bLegacyPublicIncludePaths = false;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // This is to access RayTracing Definitions
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Shaders", "Shared"));
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CoreUObject", "RHI",
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
                "PhysicsCore",
				"DeveloperSettings",
                "Projects",
            }
            );
    }
}