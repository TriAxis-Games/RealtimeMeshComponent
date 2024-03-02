// Copyright TriAxis Games, L.L.C. All Rights Reserved.

using UnrealBuildTool;

public class RealtimeMeshExamples : ModuleRules
{
    public RealtimeMeshExamples(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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