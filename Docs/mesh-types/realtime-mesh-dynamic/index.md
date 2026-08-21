---
title: RealtimeMesh Dynamic
description: URealtimeMeshDynamic, a UDynamicMesh-backed type that makes RMC a drop-in for UDynamicMeshComponent and works with Geometry Scripting.
---

`URealtimeMeshDynamic` keeps a `UDynamicMesh` (an `FDynamicMesh3`) as its source of truth. Whenever that mesh changes, it converts it into RMC streams for rendering and collision, making one section per material ID.

That makes RMC a mostly drop-in replacement for `UDynamicMeshComponent`. If you already author geometry with Geometry Script nodes or the GeometryProcessing library, this is the type you want.

Header: `Source/RealtimeMeshExt/Public/RealtimeMeshDynamic.h`. This type lives in the **`RealtimeMeshExt`** module, so check the dependencies below.

## Build dependencies

`URealtimeMeshDynamic` is not in the core module. Add these to your module's `.Build.cs` under `PrivateDependencyModuleNames`, which is what the examples module does:

```csharp
"RealtimeMeshExt",     // the Dynamic type itself
"GeometryCore",        // UE::Geometry::FDynamicMesh3
"GeometryFramework",   // UDynamicMesh
```

See `Source/RealtimeMeshExamples/RealtimeMeshExamples.Build.cs` for a working reference.

## Getting a mesh in

```cpp
#include "RealtimeMeshDynamic.h"
#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
using namespace UE::Geometry;

URealtimeMeshDynamic* RealtimeMesh = Component->InitializeRealtimeMesh<URealtimeMeshDynamic>();

// One material slot per triangle group the converter will produce.
RealtimeMesh->SetNumMaterials(2);

// Move an FDynamicMesh3 in. SetMesh converts it and rebuilds the render data.
FDynamicMesh3 Mesh = BuildMyMesh();
RealtimeMesh->SetMesh(MoveTemp(Mesh));
```

In Blueprint, use the **Initialize Realtime Mesh Dynamic** node.

The converter makes one section per triangle group (material ID), so enable triangle groups on your `FDynamicMesh3` and size your material set to match. `ARealtimeMeshExample_Dynamic_Basic` (`Source/RealtimeMeshExamples/Private/Dynamic/RealtimeMeshExample_Dynamic_Basic.cpp`) hands a two-group grid to `SetMesh`.

### Three ways in

- `SetMesh(FDynamicMesh3&& MoveMesh)` moves an `FDynamicMesh3` into the internal mesh and rebuilds. C++ only.
- `SetDynamicMesh(UDynamicMesh* NewMesh)` replaces the owned `UDynamicMesh` and takes ownership of it. Blueprint callable.
- `GetDynamicMesh()` returns the owned `UDynamicMesh`, creating it if needed, which you can then drive with Geometry Script nodes.

## Editing the mesh

`EditMesh` hands you the internal `FDynamicMesh3` to change, then updates the render data according to the mode you pass:

```cpp
RealtimeMesh->EditMesh([](FDynamicMesh3& Mesh)
{
    for (const int32 Vid : Mesh.VertexIndicesItr())
    {
        FVector3d P = Mesh.GetVertex(Vid);
        P.Z = /* ... */;
        Mesh.SetVertex(Vid, P);
    }
}, ERealtimeMeshDynamicRenderUpdateMode::FastUpdate);
```

`ERealtimeMeshDynamicRenderUpdateMode` mirrors `EDynamicMeshComponentRenderUpdateMode`:

- `NoUpdate` leaves the render data alone.
- `FullUpdate` rebuilds everything. Use it after changing topology, materials, or attribute topology.
- `FastUpdate` only updates vertex attributes, and falls back to a full rebuild if the topology changed anyway.

Doing an initial full build and then animating with `FastUpdate` is what `ARealtimeMeshExample_Dynamic_Edit` (`Source/RealtimeMeshExamples/Private/Dynamic/RealtimeMeshExample_Dynamic_Edit.cpp`) does every frame.

### Reading and transforming

- `ProcessMesh([](const FDynamicMesh3&){ ... })` reads the internal mesh under its lock.
- `GetMeshPtr()` returns a raw `FDynamicMesh3*`, const and non-const. Prefer `GetDynamicMesh()` or `EditMesh()`.
- `ApplyTransform(const FTransform&, bool bInvert)` transforms positions and normals in place.

## Telling it what changed

If you change the mesh some other way, for example through Geometry Script operating on `GetDynamicMesh()`, tell the component what happened so it re-streams as little as possible:

- `NotifyMeshModified()` does a full rebuild. Use it after topology, material, or attribute topology changes. The Blueprint node is called **Notify Mesh Updated**, and `NotifyMeshUpdated()` is a C++ alias for the same thing.
- `NotifyMeshVertexAttributesModified(bPositions, bNormals, bUVs, bColors)` is the faster path when only vertex attributes changed and the topology did not. The Blueprint node is **Notify Vertex Attributes Updated**.
- `FastNotifyPositionsUpdated(bNormals, bColors, bUVs)`, `FastNotifyVertexAttributesUpdated(...)`, `FastNotifyColorsUpdated()`, and `FastNotifyUVsUpdated()` are targeted C++ entry points.

## Materials, tangents, and shading

- `ConfigureMaterialSet(const TArray<UMaterialInterface*>&, bDeleteExtraSlots)` sets the whole material set. `SetNumMaterials(N)` and `ValidateMaterialSlots(...)` manage the slot count.
- `SetTangentsType(ERealtimeMeshDynamicTangentsMode)` picks between `NoTangents`, `AutoCalculated` (worked out from positions, normals, and UVs), or `ExternallyProvided` (use the tangents already on the mesh's attribute set). `ExternallyProvided` is the default.
- Conversion-time conveniences: `SetColorOverrideMode` (`None`, `VertexColors`, `Polygroups`, or `Constant`), `SetConstantOverrideColor`, `SetVertexColorSpaceTransformMode`, and best-effort `SetEnableFlatShading`.
- `SetTwoSided(bool)` is **advisory only** here. RMC has no per-section two-sided flag, so assign a two-sided material if you want two-sided rendering.

## Collision

Dynamic supports whole-mesh complex collision plus simple shapes:

- `EnableComplexAsSimpleCollision()` and `SetComplexAsSimpleCollisionEnabled(bEnabled, bImmediateUpdate)`.
- `SetSimpleCollisionShapes(const FKAggregateGeom&, bUpdateCollision)`, `ClearSimpleCollisionShapes(...)`, and `GetSimpleCollisionShapes()`.
- `SetDeferredCollisionUpdatesEnabled(...)` and `UpdateCollision(bOnlyIfPending)` batch up cook work.
- `bEnableComplexCollision`, `CollisionType`, `bDeferCollisionUpdates`, and `bUseAsyncCooking` are all editable in the details panel.

## Using it with Geometry Scripting

Because the source of truth is a `UDynamicMesh`, the whole Geometry Script library works on this type directly.

Get the mesh with `GetDynamicMesh()`, run your Geometry Script nodes against it, and let the change notification flow through. The node graph's edits mark the mesh dirty and the component reconverts.

From C++, if you want finer control, wrap your edits in `EditMesh(...)` with the right `ERealtimeMeshDynamicRenderUpdateMode`.

## What isn't supported

`URealtimeMeshDynamic` covers the common `UDynamicMeshComponent` surface, but not the proxy-internal features: secondary triangle buffers, render decomposition, render mesh post-processors, live triangle color and vertex remap functions, wireframe overlay, override and secondary render materials, and the raytracing and draw path toggles.

As noted above, `bTwoSided` is a material property here, and flat shading is best effort.
