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
        
        PublicDefinitions.Add("WITH_REALTIME_MESH=1");
        PublicDefinitions.Add("IS_REALTIME_MESH_LIBRARY=1");

        // This is to access RayTracing Definitions
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Shaders", "Shared"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public", "Interface"));

        //PublicDefinitions.Add("REALTIME_MESH_INTERFACE_ROOT=");
        
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