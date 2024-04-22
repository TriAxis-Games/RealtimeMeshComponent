// Copyright TriAxis Games, L.L.C. All Rights Reserved.

using UnrealBuildTool;

public class RealtimeMeshEditor : ModuleRules
{
    public RealtimeMeshEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        //bUseUnity = false;
#if UE_5_1_OR_LATER
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
#endif

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "RealtimeMeshComponent", "AssetTools", "BlueprintGraph"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "RealtimeMeshComponent",
                "UnrealEd"
            }
        );
    }
}