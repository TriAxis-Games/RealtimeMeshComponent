// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RealtimeMeshComponent : ModuleRules
{
    public RealtimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        //IWYUSupport = IWYUSupport.None;
        bLegacyPublicIncludePaths = false;
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
                "Core", 
                "CoreUObject", 
                "GeometryCore",
                "RenderCore",
                "RHI",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Engine",
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