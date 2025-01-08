// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

using UnrealBuildTool;

public class RealtimeMeshExamples : ModuleRules
{
    public RealtimeMeshExamples(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
#if UE_5_1_OR_LATER
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
#endif

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                
                // Add RealtimeMeshComponent support to C++
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
            }
        );
    }
}