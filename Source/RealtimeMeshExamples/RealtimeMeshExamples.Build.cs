// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

using UnrealBuildTool;

public class RealtimeMeshExamples : ModuleRules
{
    public RealtimeMeshExamples(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "RealtimeMeshComponent"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",

                // Pulled in by RealtimeMesh's async helpers (ENQUEUE_RENDER_COMMAND path) used in the AsyncBuild example
                "RenderCore",
                "RHI",
            }
        );
    }
}