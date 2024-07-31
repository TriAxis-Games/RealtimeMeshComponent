// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RealtimeMeshComponent : ModuleRules
{
    public RealtimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        //bEnforceIWYU = true;
        //IWYUSupport = 
        //bLegacyPublicIncludePaths = false;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        //IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        // This is to access RayTracing Definitions
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Shaders", "Shared"));
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CoreUObject", "GeometryCore",
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
                "Chaos",
                "ChaosCore",
            }
            );
    }
}