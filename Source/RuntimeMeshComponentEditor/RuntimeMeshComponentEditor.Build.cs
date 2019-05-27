// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

using UnrealBuildTool;

public class RuntimeMeshComponentEditor : ModuleRules
{
    public RuntimeMeshComponentEditor(ReadOnlyTargetRules rules) : base(rules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Target.Version.MinorVersion <= 19)
        {
            PublicIncludePaths.AddRange(
                new string[] {
                    "RuntimeMeshComponentEditor/Public"
                });

            PrivateIncludePaths.AddRange(
                new string[] {
                    "RuntimeMeshComponentEditor/Private",
                });
        }

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
            });

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "RenderCore",
                "ShaderCore",
                "RHI",
                "UnrealEd",
                "LevelEditor",
                "PropertyEditor",
                "RawMesh",
                "AssetTools",
                "AssetRegistry",
                "Projects",
                "EditorStyle",
                "InputCore",

                "RuntimeMeshComponent",
            });
    }
}
