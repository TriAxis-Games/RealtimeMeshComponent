---
title: Converting Dynamic & Procedural Meshes
description: One-shot conversion between a RealtimeMesh and Geometry Scripting's UDynamicMesh, plus how RMC relates to UProceduralMeshComponent.
---

`URealtimeMeshDynamicMeshConverter` copies geometry between Geometry Scripting's `UDynamicMesh` (`UE::Geometry::FDynamicMesh3`) and RMC. It lives in the `RealtimeMeshExt` module (see the [section overview](../) for the `Build.cs` dependency).

This is a **one-shot** conversion. It takes a snapshot of the geometry, once.

If what you actually want is a Realtime Mesh that *keeps itself in sync* with a `UDynamicMesh` as you edit it, you want [RealtimeMeshDynamic](../../mesh-types/realtime-mesh-dynamic/) instead. Different tool, different job.

This page also covers [ProceduralMeshComponent](#proceduralmeshcomponent) at the bottom.

## DynamicMesh to RealtimeMesh

At the stream set level, `CopyStreamSetFromDynamicMesh` reads a `FDynamicMesh3` into a stream set:

```cpp
#include "RealtimeMeshDynamicMeshConverter.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

FRealtimeMeshStreamSet StreamSet;

FStreamSetDynamicMeshConversionOptions Options;
Options.bWantNormals     = true;
Options.bWantTangents    = true;
Options.bWantUVs         = true;
Options.bWantVertexColors = true;
Options.bWantMaterialIDs = true;   // dynamic mesh material IDs become poly groups

URealtimeMeshDynamicMeshConverter::CopyStreamSetFromDynamicMesh(SourceDynamicMesh3, StreamSet, Options);

URealtimeMeshSimple* Mesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("FromDynamicMesh"));
Mesh->CreateBufferSet(GroupKey, MoveTemp(StreamSet));
```

The Blueprint node **Copy Realtime Mesh From Dynamic Mesh** does the whole thing in one call. It takes a `UDynamicMesh`, a target `URealtimeMeshSimple`, and a `FRealtimeMeshDynamicMeshConversionOptions` that also carries the target buffer set key, and gives you the mesh back with Success and Failure pins.

### Filtering and remapping (C++ only)

`FStreamSetDynamicMeshConversionOptions` has two `TFunction` hooks that let you transform the mesh as it is read. These are C++ only, since they are not `UPROPERTY`s, so they are available on the stream set functions but not from Blueprint:

```cpp
FStreamSetDynamicMeshConversionOptions Options;

// Drop triangles you don't want in the output.
Options.TriangleFilterFunction =
    [](const UE::Geometry::FDynamicMesh3& Mesh, int32 TriangleID)
    {
        return ShouldKeep(Mesh, TriangleID);   // return false to skip this triangle
    };

// Change each triangle's poly group, for example collapsing many groups into a few sections.
Options.PolyGroupRemapFunction =
    [](const UE::Geometry::FDynamicMesh3& Mesh, int32 TriangleID)
    {
        return RemapGroup(Mesh, TriangleID);   // returns the poly group to emit
    };
```

There is also `bAutoSortPolyGroupsIfNecessary`, on by default, which reorders triangles so each poly group ends up contiguous. RMC needs that to split them into sections cleanly.

## RealtimeMesh to DynamicMesh

`CopyStreamSetToDynamicMesh` writes a stream set into a `FDynamicMesh3`:

```cpp
UE::Geometry::FDynamicMesh3 OutMesh;

FStreamSetDynamicMeshConversionOptions Options;
const bool bAppend = false;   // true appends to OutMesh instead of replacing it

URealtimeMeshDynamicMeshConverter::CopyStreamSetToDynamicMesh(StreamSet, OutMesh, Options, bAppend);
```

The Blueprint node **Copy Realtime Mesh To Dynamic Mesh** takes a `URealtimeMeshSimple` and a target `UDynamicMesh`.

That final `bShouldAppendToExisting` parameter on the C++ functions lets you accumulate several conversions into one dynamic mesh. `CopyStreamSetFromDynamicMesh` can also tell you where each poly group ended up, through its optional `OutGroupRanges` map (`TMap<int32, FRealtimeMeshStreamRange>`).

## Blueprint nodes at a glance

| Node | Direction |
|------|-----------|
| `Copy Stream Set From Dynamic Mesh` | DynamicMesh to stream set |
| `Copy Stream Set To Dynamic Mesh` | stream set to DynamicMesh |
| `Copy Realtime Mesh From Dynamic Mesh` | DynamicMesh to RealtimeMesh |
| `Copy Realtime Mesh To Dynamic Mesh` | RealtimeMesh to DynamicMesh |

All four have Success and Failure execution pins.

## ProceduralMeshComponent

There is **no converter** that reads an existing `UProceduralMeshComponent`'s sections into a stream set, the way the static mesh and dynamic mesh converters do. That is deliberate.

RMC's answer to PMC is a **drop-in replacement component** instead. `URealtimeMeshProcedural`, in the core `RealtimeMeshComponent` module rather than `RealtimeMeshExt`, mirrors `UProceduralMeshComponent`'s Blueprint surface: `CreateMeshSection`, `CreateMeshSection_LinearColor`, `UpdateMeshSection`, `GetMeshSection`, `ClearMeshSection`, `ClearAllMeshSections`, `SetMeshSectionVisible`, `GetNumSections`, `AddCollisionConvexMesh`, and the rest.

So PMC content migrates by swapping the component out, not by converting mesh data at runtime.

`URealtimeMeshProceduralMeshLibrary` fills in the rest with the familiar geometry helpers (`GenerateBoxMesh`, `CalculateTangentsForMesh`, `ConvertQuadToTriangles`, `CreateGridMeshTriangles`, `CreateGridMeshWelded`, `CreateGridMeshSplit`, `GetSectionFromProceduralMesh`) mirroring `UKismetProceduralMeshLibrary`. They all produce the same plain CPU arrays `URealtimeMeshProcedural` expects.

For the full API and a migration checklist, see [RealtimeMeshProcedural](../../mesh-types/realtime-mesh-procedural/).

If you genuinely need the *geometry* out of a PMC as a stream set, read its section arrays with PMC's own API (or `GetSectionFromProceduralMesh` for a `URealtimeMeshProcedural`) and feed those arrays into a `TRealtimeMeshBuilderLocal`. That is the general [fill-a-stream-set pattern](../loading-meshes/#loading-other-formats).

## Related

- [RealtimeMeshDynamic](../../mesh-types/realtime-mesh-dynamic/) for live mirroring of a `UDynamicMesh`, rather than the one-shot conversion here.
- [RealtimeMeshProcedural](../../mesh-types/realtime-mesh-procedural/) for the PMC drop-in replacement.
- [Mesh Optimization](../mesh-optimization/) for cleaning up a converted stream set before rendering.
