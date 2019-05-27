// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;

public class RuntimeMeshComponent : ModuleRules
{
    private string ModulePath
    {
        get { return Path.GetDirectoryName(ModuleDirectory); }
    }

    public RuntimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Target.Version.MinorVersion <= 19)
        {
            PublicIncludePaths.AddRange(
                new string[] {
                    "RuntimeMeshComponent/Public"
                });

            PrivateIncludePaths.AddRange(
                new string[] {
                    "RuntimeMeshComponent/Private"
                });
        }
        
        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core"
			});

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "RenderCore",
                "ShaderCore",
                "RHI",
            });
    }
}
