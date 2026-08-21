---
title: Converting Static Meshes
description: Copy geometry between UStaticMesh assets and a RealtimeMesh, in either direction, with URealtimeMeshStaticMeshConverter.
---

`URealtimeMeshStaticMeshConverter` copies geometry between a `UStaticMesh` asset and RMC. It works in both directions, and at two levels: raw stream sets, or a whole `URealtimeMeshSimple` complete with LODs and materials.

Unlike the other tools in this section, the static mesh converter lives in the **core** `RealtimeMeshComponent` module. That means it is available in both the Core and Pro distributions, with no extra `Build.cs` dependency.

The working example is `ARealtimeMeshExample_Simple_StaticMeshConvert` (`Source/RealtimeMeshExamples/.../Simple/`).

> **Editor versus runtime.** Reading *out of* a static mesh works at runtime, with one caveat covered below. Writing *into* one requires the editor, because Unreal's static mesh build pipeline is editor only. If you call the to-static-mesh functions in a packaged game they log a warning and fail.

## Static Mesh to RealtimeMesh

### Into a stream set

```cpp
#include "RealtimeMeshStaticMeshConverter.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

FRealtimeMeshStreamSet StreamSet;

FStreamSetStaticMeshConversionOptions Options;
Options.LODIndex          = 0;
Options.bWantTangents     = true;
Options.bWantUVs          = true;
Options.bWantVertexColors = true;
Options.bWantPolyGroups   = true;   // keeps each static mesh section as its own poly group

if (URealtimeMeshStaticMeshConverter::CopyStreamSetFromStaticMesh(SourceStaticMesh, StreamSet, Options))
{
    URealtimeMeshSimple* Mesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

    const FRealtimeMeshBufferSetKey GroupKey =
        FRealtimeMeshBufferSetKey::Create(0, FName("FromStaticMesh"));
    Mesh->CreateBufferSet(GroupKey, MoveTemp(StreamSet));
}
```

`CopyStreamSetFromStaticMesh` can read from two different places inside the asset, and picks the best one available:

- **Source data**, the editor's `MeshDescription`. Highest fidelity, but **editor only**. Used automatically in the editor when the asset has valid source models.
- **Render data**, the cooked LOD buffers. This is what exists **at runtime**. It has been through vertex splitting and optimisation during the build, so it is slightly lower fidelity, and it only works if the asset has **`Allow CPU Access` ticked**. Without that flag the buffers are not readable from the CPU and the copy fails with a warning.

So if you want to convert a static mesh at runtime, tick *Allow CPU Access* on the asset. In the editor it works either way. This one setting is the most common reason runtime conversion "just doesn't work".

### Into a whole RealtimeMesh

`CopyRealtimeMeshFromStaticMesh` is Blueprint callable and copies a range of LODs, the materials, and optionally the distance field and Lumen card data straight into a `URealtimeMeshSimple`:

```cpp
ERealtimeMeshOutcomePins Outcome;

FRealtimeMeshStaticMeshConversionOptions Options;
Options.MinLODIndex        = 0;
Options.MaxLODIndex        = REALTIME_MESH_MAX_LOD_INDEX;   // all LODs
Options.bWantsMaterials    = true;
Options.bWantsDistanceField = true;
Options.bWantsLumenCards   = true;

URealtimeMeshSimple* Mesh =
    URealtimeMeshStaticMeshConverter::CopyRealtimeMeshFromStaticMesh(
        SourceStaticMesh,
        GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>(),
        Options,
        Outcome);
```

`FRealtimeMeshStaticMeshConversionOptions` adds `MinLODIndex` and `MaxLODIndex` (which LODs to copy), plus `bWantsMaterials`, `bWantsDistanceField`, and `bWantsLumenCards`. The distance field and card data are copied from the source asset's render data when it is present.

There is a Nanite copy option in the header, but it is commented out. Nanite conversion is a separate RMC Pro feature and is not part of this converter.

`Outcome` is a Success or Failure execution pin in Blueprint. In C++ it is an out parameter you check against `ERealtimeMeshOutcomePins::Success`.

## RealtimeMesh to Static Mesh

This direction builds a real `UStaticMesh` asset, so it is **editor only**. Use it to bake generated geometry into an asset, for example saving out a runtime-generated mesh from an editor utility.

### From a stream set

```cpp
#if WITH_EDITOR
FStreamSetStaticMeshConversionOptions Options;

const bool bOk =
    URealtimeMeshStaticMeshConverter::CopyStreamSetToStaticMesh(StreamSet, TargetStaticMesh, Options);
#endif
```

The converter builds a `MeshDescription`, creates one polygon group per distinct poly group value (named `Material_<n>`), lets the build work out the tangent basis, and calls `Build()`. Your authored normals are kept, unless a stream did not have any.

### From a whole RealtimeMesh

`CopyRealtimeMeshToStaticMesh` is Blueprint callable and does the same for an entire `URealtimeMeshSimple`, flattening all its buffer sets into a single LOD0 mesh description with one polygon group per material slot:

```cpp
ERealtimeMeshOutcomePins Outcome;

UStaticMesh* Baked =
    URealtimeMeshStaticMeshConverter::CopyRealtimeMeshToStaticMesh(
        SourceRealtimeMesh,
        TargetStaticMeshAsset,
        FRealtimeMeshStaticMeshConversionOptions(),
        Outcome);
```

## Blueprint nodes at a glance

| Node | Direction | Notes |
|------|-----------|-------|
| `Copy Stream Set From Static Mesh` | Static Mesh to stream set | At runtime, needs *Allow CPU Access* |
| `Copy Stream Set To Static Mesh` | stream set to Static Mesh | Editor only |
| `Copy Realtime Mesh From Static Mesh` | Static Mesh to RealtimeMesh | Copies LODs, materials, distance field, cards |
| `Copy Realtime Mesh To Static Mesh` | RealtimeMesh to Static Mesh | Editor only |

All four have Success and Failure execution pins.

## Related

- [Mesh Optimization](../mesh-optimization/) can clean up the stream set after converting, before you create the buffer set.
- [Converting Dynamic Meshes](../converting-dynamic-meshes/) is the equivalent for Geometry Scripting's `UDynamicMesh`.
