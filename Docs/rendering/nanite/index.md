---
title: Runtime Nanite
description: Build Nanite-ready meshes at runtime from procedural geometry. Full LOD DAG, cluster hierarchy, and per-page streaming, with no cooking step.
---

Nanite is normally an offline process. You import a mesh, the editor builds its cluster hierarchy and LOD DAG, and that data ships baked into the asset.

RMC's `RealtimeMeshNanite` module does the same build **at runtime**, from geometry you hand it: raw vertex arrays, procedurally generated shapes, or a mesh you just modified. What comes out is a `FRealtimeMeshNaniteResources` that you apply to a Realtime Mesh, and from there Unreal renders it through the standard Nanite pipeline.

All the public builder API lives in the `RealtimeMesh::Nanite` namespace. The public headers are thin wrappers over a shared encoder core, so the type names below are aliases (`FRealtimeMeshNaniteMesh` is really `FNaniteMesh` underneath). Always write the `RealtimeMesh::Nanite::` spellings shown here.

## What engine you need

Runtime Nanite touches internal engine data layouts, so what you get depends on what you build against. RMC works out which tier applies automatically at build time, by having `RealtimeMeshComponent.Build.cs` inspect the engine's `NaniteResources.h`. The public API is identical across all three tiers, so no caller code ever changes.

- **Stock engine, UE 5.5 to 5.8.** The default. RMC wraps a real `Nanite::FResources` and serves its streaming pages from in-memory bulk data through the standard streaming manager. Full runtime Nanite, no engine modifications needed.
- **Provider fork.** The TriAxis engine fork exposes a Nanite streaming provider API. RMC owns its own page data and serves pages through that path, skipping the bulk data copy. Same features, less memory churn.
- **Unavailable.** Outside that engine window, meaning a stock engine newer than 5.8 without the fork, the API still compiles but the builder entry points and `Create*` calls return null and log a warning once. Components fall back to normal non-Nanite RMC rendering.

Because `BuildRealtimeNaniteMesh` returns null on the unavailable tier, and on any build failure, **always check the result before applying it**. The snippets below do.

> **Build override:** setting the environment variable `RMC_FORCE_STOCK_NANITE=1` before building forces the stock engine path even on a fork checkout. This is a build-time validation switch, not a runtime toggle.

## The build pipeline

Building a runtime Nanite mesh is four steps: create a working mesh from your geometry, build its cluster hierarchy, encode it into resources, then apply those resources to your mesh.

### 1. Create the working mesh

`FRealtimeMeshNaniteMesh::CreateFromRawMesh` takes raw geometry. UVs come in as an array of channels, one `TConstArrayView<FVector2f>` each. The trailing per-triangle material index array is optional, and leaving it out puts everything on material 0:

```cpp
#include "RealtimeMeshNaniteBuilder.h"

using namespace RealtimeMesh::Nanite;

// One UV channel.
TArray<TConstArrayView<FVector2f>> UVChannels;
UVChannels.Add(UVs);   // TConstArrayView<FVector2f>

FRealtimeMeshNaniteMesh NaniteMesh = FRealtimeMeshNaniteMesh::CreateFromRawMesh(
    Vertices,     // TConstArrayView<FVector3f>
    Normals,      // TConstArrayView<FVector3f>
    UVChannels,   // TConstArrayView<TConstArrayView<FVector2f>>
    Colors,       // TConstArrayView<FColor>   (can be empty)
    Triangles);   // TConstArrayView<int32>    (flat index list)
```

If you intend to build a full LOD DAG, which is covered below, use `CreateFromRawMeshOptimized` instead. It runs meshoptimizer first to produce spatially coherent, cache-friendly LOD0 clusters, which is what the LOD generator expects to work from:

```cpp
FRealtimeMeshNaniteMesh NaniteMesh = FRealtimeMeshNaniteMesh::CreateFromRawMeshOptimized(
    Vertices, Normals, UVChannels, Colors, Triangles);
```

### 2. Build the cluster hierarchy

For a single-LOD mesh, `BuildMinimalHierarchy` is all you need:

```cpp
FRealtimeMeshNaniteMesh::BuildMinimalHierarchy(NaniteMesh);
```

For a mesh with generated LODs, call `BuildLODAwareHierarchy` after generating them. See [Hierarchical LODs](#hierarchical-lods) below.

### 3. Encode it

`BuildRealtimeNaniteMesh` runs the encoder and gives you a `FRealtimeMeshNaniteResourcesPtr`. Tune it with `FRealtimeMeshNaniteBuildSettings`:

```cpp
FRealtimeMeshNaniteBuildSettings Settings;
Settings.PositionPrecision = 4;    // position quantization bits
Settings.NormalBits        = 8;    // normal quantization bits

FRealtimeMeshNaniteResourcesPtr Resources = BuildRealtimeNaniteMesh(NaniteMesh, Settings);
```

Other settings worth knowing:

- `TargetMinimumResidencyInKB` (default `256`) is the memory floor for pages that stay resident. Anything beyond that streams on demand.
- `Residency` (`ENaniteResidency`, default `Streamed`) chooses between `Streamed`, which keeps coarse LODs resident and streams the finer ones, and `FullyResident`, which lays out every page as a root page so the resource needs no streaming provider at all. `TargetMinimumResidencyInKB` is ignored in `FullyResident` mode.
- `bEnableParallel` and `MinClustersForParallel` control parallel cluster encoding.

### 4. Apply it

Resources are applied through the data layer using an update context. This is the pattern the examples use, where `RealtimeMesh` is a `URealtimeMeshSimple*`:

```cpp
#include "Data/RealtimeMeshData.h"

if (Resources.IsValid())   // null on build failure or an unsupported engine
{
    RealtimeMesh::FRealtimeMeshUpdateContext UpdateContext(RealtimeMesh->GetMesh()->GetContext());
    RealtimeMesh->GetMesh()->SetNaniteResources(UpdateContext, MoveTemp(Resources));
}
```

`SetNaniteResources` takes the pointer by rvalue, so move it in.

The whole flow is wrapped up in the shared example helper `BuildAndApplyNaniteMesh` (`Source/RealtimeMeshNaniteExamples/Private/NaniteExampleHelpers.h`), which most of the from-raw-data examples call.

## Hierarchical LODs

A minimal hierarchy renders at a single level of detail. To get Nanite's signature seamless LOD, generate a full LOD DAG with `BuildHierarchicalLODs` before building the hierarchy.

The order is: `CreateFromRawMeshOptimized`, then `BuildHierarchicalLODs`, then `BuildLODAwareHierarchy`, then `BuildRealtimeNaniteMesh`.

```cpp
#include "RealtimeMeshNaniteLODBuilder.h"

using namespace RealtimeMesh::Nanite;

// LOD0 has to be the meshopt-optimized variant so the LOD generator can partition it.
FRealtimeMeshNaniteMesh NaniteMesh = FRealtimeMeshNaniteMesh::CreateFromRawMeshOptimized(
    Vertices, Normals, UVChannels, Colors, Triangles);

FRealtimeMeshNaniteLODBuildSettings LODSettings;
LODSettings.MaxLODs           = 10;     // hard ceiling on LOD count
LODSettings.TargetPartitionSize = 8;    // clusters per partition
LODSettings.LODReductionFactor  = 0.5f; // each LOD targets half the triangles

BuildHierarchicalLODs(NaniteMesh, LODSettings);
FRealtimeMeshNaniteMesh::BuildLODAwareHierarchy(NaniteMesh);   // NOT BuildMinimalHierarchy

FRealtimeMeshNaniteBuildSettings BuildSettings;
FRealtimeMeshNaniteResourcesPtr Resources = BuildRealtimeNaniteMesh(NaniteMesh, BuildSettings);
// ...then apply it as in step 4.
```

`FRealtimeMeshNaniteLODBuildSettings` also exposes the simplifier controls: `LODReductionFactor`, `MinClustersToContinue`, `ProgressFloor` (stop when a pass is no longer meaningfully reducing cluster count), `PositionWeldEpsilon` (welding across cluster seams), and the attribute error weights `NormalWeight` (default `0.5`), `UVWeight` (`0.1`), and `ColorWeight` (`0.0`).

`ARealtimeMeshExample_Nanite_HierarchicalLODs` (`Source/RealtimeMeshNaniteExamples/`) runs this exact pipeline on a dense UV sphere and logs the resulting LOD pyramid, with cluster counts, triangle counts, and the error band per level, so you can see what came out.

## Meshes that change often

Nanite resources are immutable once built. A mesh that changes every frame means rebuilding and reapplying the resources each time. There is no in-place edit path.

RMC's resource sharing and copy-on-write proxy design stop it from re-cloning data on every proxy rebuild, but the encode itself still runs each time. So a full rebuild every frame is only reasonable for modest mesh sizes.

`ARealtimeMeshExample_Nanite_FrequentUpdates` stress tests this, rebuilding and reapplying at several different frequencies and tracking the timings.

## Building clusters by hand

For complete control you can author clusters directly on a `FRealtimeMeshNaniteMesh`, adding `FRealtimeMeshNaniteCluster` entries and filling in their vertices, normals, colors, UVs, indices, and per-material ranges yourself, instead of going through `CreateFromRawMesh`.

This is very much an advanced path. `ARealtimeMeshExample_Nanite_ManualClusters` has a worked example building multi-cluster and multi-material-range meshes.

## Debug console variables

The shared Nanite builder registers a couple of developer console variables, in non-Shipping builds only. These are builder diagnostics, not rendering toggles:

- `r.TriAxisNanite.RMC.Audits` turns on read-only diagnostic passes in the builder, doing decode round-trip and conservation checks. Purely diagnostic, produces nothing the encoder consumes. Off by default.
- `r.TriAxisNanite.RMC.ISPC` turns on ISPC-optimized cluster building, where ISPC is compiled in. Off by default.

## Example actors

All six live in `Source/RealtimeMeshNaniteExamples/` as C++ actors you can drop into a level:

- **`ARealtimeMeshExample_Nanite_BasicRawData`** is the core pipeline: build Nanite meshes from raw vertex and triangle arrays (a low-poly sphere and a high-poly torus) and apply them.
- **`ARealtimeMeshExample_Nanite_HierarchicalLODs`** runs the full LOD DAG pipeline and logs the LOD pyramid.
- **`ARealtimeMeshExample_Nanite_LODInspector`** builds the LOD pyramid and extracts each level into its own mesh side by side, so you can inspect what the simplifier did at each level.
- **`ARealtimeMeshExample_Nanite_FrequentUpdates`** rebuilds and reapplies frame by frame at several frequencies, with timings.
- **`ARealtimeMeshExample_Nanite_ManualClusters`** hand-authors clusters and material ranges, bypassing `CreateFromRawMesh`.
- **`ARealtimeMeshExample_Nanite_PrebuiltData`** copies prebuilt Nanite resources straight off a `StaticMesh` asset onto a Realtime Mesh with `FRealtimeMeshNaniteResources::CreateFromCopy`, bypassing the build pipeline entirely. Handy for working out whether a rendering problem is in the component or the builder.
