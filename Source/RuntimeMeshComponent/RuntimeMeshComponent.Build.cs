// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RuntimeMeshComponent : ModuleRules
{
    public RuntimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

#if UE_4_22_OR_LATER
        // This is to access RayTracing Definitions
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Shaders", "Shared"));
#endif

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
#if !UE_4_22_OR_LATER
                // This goes away in 4.22
                "ShaderCore",
#endif
#if UE_4_23_OR_LATER
                "PhysicsCore",
#endif
            }
            );


    }
}