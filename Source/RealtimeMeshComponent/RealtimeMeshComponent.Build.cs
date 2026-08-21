// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

using System;
using System.IO;
using System.Text.RegularExpressions;
using EpicGames.Core;
using UnrealBuildBase;
using UnrealBuildTool;

public class RealtimeMeshComponent : ModuleRules
{
    static int CachedProviderApiVersion = -1;

    // Detects the runtime-Nanite provider engine fork by probing the engine's NaniteResources.h TEXT for
    // the UE_NANITE_PROVIDER_API_VERSION marker (E1). Probing the header text is the only viable option:
    // the header exists on both stock and fork engines (so `#if __has_include` can't discriminate), and it
    // ships with installed builds too, so File.ReadAllText works everywhere. Returns the provider API
    // version, or 0 when the fork isn't present. Static so RealtimeMeshNanite.Build.cs can reuse it for
    // tier-dependent build logic (e.g. skipping the ISPC setup on tier C).
    public static int GetNaniteProviderApiVersion()
    {
        if (CachedProviderApiVersion < 0)
        {
            // Test override: RMC_FORCE_STOCK_NANITE=1 short-circuits the probe to 0 so a fork checkout can
            // exercise the stock-engine tier-B/C code path (validation only — see the fallback plan traps).
            string ForceStock = Environment.GetEnvironmentVariable("RMC_FORCE_STOCK_NANITE");
            if (!string.IsNullOrEmpty(ForceStock) && ForceStock != "0")
            {
                Log.TraceInformation("RealtimeMesh: RMC_FORCE_STOCK_NANITE set - forcing stock Nanite path (tier B/C), ignoring any provider fork.");
                CachedProviderApiVersion = 0;
                return CachedProviderApiVersion;
            }

            CachedProviderApiVersion = 0;
            string Header = Path.Combine(Unreal.EngineDirectory.FullName,
                "Source", "Runtime", "Engine", "Public", "Rendering", "NaniteResources.h");
            if (File.Exists(Header))
            {
                string Text = File.ReadAllText(Header);
                Match M = Regex.Match(Text, @"#define\s+UE_NANITE_PROVIDER_API_VERSION\s+(\d+)");
                if (M.Success)
                {
                    CachedProviderApiVersion = int.Parse(M.Groups[1].Value);
                }
                // Transition heuristic: current fork checkouts may predate the marker macro. Remove once the
                // macro has landed in every fork checkout. Logged so stale forks are visible in build output.
                else if (Regex.IsMatch(Text, @"struct\s+FResourcesProvider\b"))
                {
                    CachedProviderApiVersion = 1;
                    Log.TraceInformation("RealtimeMesh: detected the Nanite provider fork via the struct-name heuristic (UE_NANITE_PROVIDER_API_VERSION macro not found - stale fork checkout).");
                }
            }
        }
        return CachedProviderApiVersion;
    }

    public RealtimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        //IWYUSupport = IWYUSupport.None;
        bLegacyPublicIncludePaths = false;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        PublicDefinitions.Add("WITH_REALTIME_MESH=1");

        // Runtime-Nanite build tier detection (see RMC-Plan-Nanite-Stock-Engine-Fallback). Defined on the
        // CORE module so PublicDefinitions propagate to every dependent module (Nanite, Editor, Ext,
        // Examples, Tests) - no risk of modules disagreeing on the tier.
        int ProviderApi = GetNaniteProviderApiVersion();
        PublicDefinitions.Add("RMC_NANITE_ENGINE_PROVIDER_VERSION=" + ProviderApi);
        PublicDefinitions.Add("RMC_NANITE_ENGINE_PROVIDER=" + (ProviderApi >= 1 ? "1" : "0"));

        // This is to access RayTracing Definitions
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Shaders", "Shared"));

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
                // Static-mesh converter (RealtimeMeshStaticMeshConverter): MeshDescription build
                // path (editor) + FStaticMeshLODResourcesMeshAdapter render-data import path.
                "MeshDescription",
                "StaticMeshDescription",
                "MeshConversion",
            }
            );
    }
}
