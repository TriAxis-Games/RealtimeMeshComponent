// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RealtimeMeshEditor : ModuleRules
{
    public RealtimeMeshEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        //bUseUnity = false;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

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
                "UnrealEd",
                "ToolMenus",
                "Projects",
                "RenderCore",
                "RHI",
                "PropertyEditor",
                "EditorStyle",
                "EditorWidgets",
                "ContentBrowser",
            }
        );

    }
}