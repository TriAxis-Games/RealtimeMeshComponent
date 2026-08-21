---
title: RealtimeMesh Simple
description: "URealtimeMeshSimple, the StreamSet-driven mesh type: create and update buffer sets, read data back, and use it from Blueprint."
---

`URealtimeMeshSimple` is the main RMC mesh type and the one to reach for unless another [mesh type](../) obviously fits better.

You build a [StreamSet](../../keyconcepts/streams/) with the [mesh builder](../../keyconcepts/mesh-local-builder/), hand it over, and RMC turns it into GPU buffers, sections, and optionally collision. It exposes RMC's whole feature set: any vertex format you like, automatic sections from poly groups, [LODs](../../component-core/lods/), and the cheapest update paths available.

Header: `Source/RealtimeMeshComponent/Public/RealtimeMeshSimple.h`.

## Getting started

Attach a Simple mesh to the component and keep the pointer:

```cpp
#include "RealtimeMeshSimple.h"
using namespace RealtimeMesh;

URealtimeMeshSimple* RealtimeMesh = Component->InitializeRealtimeMesh<URealtimeMeshSimple>();

// Material slots are addressed by index from your sections.
RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
```

For the shortest complete example, see `ARealtimeMeshExample_Simple_HelloTriangle` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_HelloTriangle.cpp`).

## Creating a buffer set

A **buffer set** owns a group of shared buffers: positions, tangents, texcoords, triangles, and so on. The **sections** inside it each reference a range of those buffers and carry the material and draw settings.

You address a buffer set with an `FRealtimeMeshBufferSetKey`:

```cpp
// Build the geometry into a StreamSet.
FRealtimeMeshStreamSet StreamSet;
TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
Builder.EnableTangents();
Builder.EnableTexCoords();
Builder.EnableColors();
// ... add vertices and triangles ...

// Key it into LOD 0 with a name of your choosing.
const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("MyMesh"));

// Hand the StreamSet over.
RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);
```

`CreateBufferSet` returns a `TFuture<ERealtimeMeshProxyUpdateStatus>` that completes once the render thread has caught up. Ignore it for fire-and-forget updates, or chain onto it if you need to know when the mesh is actually on screen.

### Two ways to pass the StreamSet

Both `CreateBufferSet` and `UpdateBufferSet` come in two flavours:

```cpp
// const& : RMC copies your StreamSet. You keep it and can reuse it.
CreateBufferSet(const FRealtimeMeshBufferSetKey&, const FRealtimeMeshStreamSet& MeshData, ...);

// rvalue : RMC takes ownership, no copy. Use MoveTemp when you're done with it.
CreateBufferSet(const FRealtimeMeshBufferSetKey&, FRealtimeMeshStreamSet&& MeshData, ...);
```

If you built the StreamSet purely for this call, use the move version. It skips a full copy of your vertex data, which is significant on large meshes:

```cpp
RealtimeMesh->CreateBufferSet(GroupKey, MoveTemp(StreamSet));
```

There is also a no-data version, `CreateBufferSet(GroupKey, InConfig, bShouldAutoCreateSectionsForPolyGroups)`, which makes an empty buffer set for you to fill in later.

## Sections from poly groups, made for you

If your StreamSet has **poly groups** (you tagged each triangle with a group number through the builder), creating the buffer set makes one section per group automatically. You do not declare sections at all.

This is controlled by `bShouldAutoCreateSectionsForPolyGroups`, which defaults to `true`.

The section keys are derived from the buffer set key and the group number, so you can find them afterwards and point each at a material slot:

```cpp
Builder.EnablePolyGroups();
Builder.AddTriangle(V0, V1, V2, /*PolyGroup*/ 0);   // becomes the section for group 0
Builder.AddTriangle(V2, V1, V0, /*PolyGroup*/ 1);   // becomes the section for group 1

RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);

// Point each one at a material slot.
const FRealtimeMeshSectionKey Section0 = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
const FRealtimeMeshSectionKey Section1 = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);
RealtimeMesh->UpdateSectionConfig(Section0, FRealtimeMeshSectionConfig(0));
RealtimeMesh->UpdateSectionConfig(Section1, FRealtimeMeshSectionConfig(1));
```

You can toggle the behaviour per buffer set at runtime with `SetShouldAutoCreateSectionsForPolyGroups(GroupKey, bNewValue)` and check it with `ShouldAutoCreateSectionsForPolygonGroups(GroupKey)`.

`ARealtimeMeshExample_Simple_MultipleSections` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_MultipleSections.cpp`) packs several poly group sections into one buffer set, each on its own material slot.

## Updating a buffer set

To replace the geometry, build a new StreamSet and call `UpdateBufferSet`, which has the same two flavours:

```cpp
if (RealtimeMesh->GetBufferSets(FRealtimeMeshLODKey(0)).Contains(GroupKey))
{
    RealtimeMesh->UpdateBufferSet(GroupKey, StreamSet);   // or MoveTemp(StreamSet)
}
else
{
    RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);
}
```

That create-or-update pattern is what `ARealtimeMeshExample_Simple_DynamicUpdate` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_DynamicUpdate.cpp`) does to ripple a grid every frame.

For per-frame vertex animation though, rebuilding the whole StreamSet is more work than you need. See the fast paths below.

## Editing in place

When only vertex values change and the shape stays fixed, you can edit the CPU data in place and upload only the buffers you touched. No reallocation, no render proxy rebuild:

```cpp
RealtimeMesh->EditMeshInPlace(GroupKey,
    [](FRealtimeMeshStreamSet& Streams) -> TSet<FRealtimeMeshStreamKey>
    {
        FRealtimeMeshStream* Positions = Streams.Find(FRealtimeMeshStreams::Position);
        // ... change Positions in place ...
        return { FRealtimeMeshStreams::Position };   // say which streams you changed
    });
```

`EditMeshInPlaceRanged` goes one step further. Your callback returns each changed stream mapped to the half-open element range `[lower, upper)` it touched, and RMC only uploads that slice of the GPU buffer.

Both need the buffer set to use the **Dynamic** draw type, so create it with `FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType::Dynamic)`. Anything that is not eligible for the fast path falls back to a normal update, so your results are always correct either way.

`ARealtimeMeshExample_Simple_FastUpdate` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_FastUpdate.cpp`) demonstrates both. For the whole picture on update strategies, see [Updating Mesh Data](../../component-core/updating-mesh-data/).

## Reading data back

`ProcessMesh` calls your callback with a buffer set's StreamSet while the lock is held, so you can inspect the data without it escaping:

```cpp
RealtimeMesh->ProcessMesh(GroupKey, [](const FRealtimeMeshStreamSet& Streams)
{
    // Read positions, indices, and so on. Don't stash the reference.
});
```

If the buffer set does not exist, the callback simply is not called. There is a matching `ProcessBufferSet` that hands you the buffer set object rather than just its streams.

## Building on a background thread

Building a StreamSet is pure CPU work with no `UObject` access, so it is safe to do on a worker thread and commit on the game thread. RMC ships helpers for exactly that:

```cpp
// 1) Build the expensive StreamSet off the game thread.
TFuture<FRealtimeMeshStreamSet> BuildFuture = DoOnAsyncThread([]()
{
    return BuildGridStreamSet();   // your own CPU-only builder function
});

// 2) Hop back to the game thread to commit it.
ContinueOnGameThread(MoveTemp(BuildFuture),
    [WeakMesh](TFuture<FRealtimeMeshStreamSet>&& Result)
    {
        if (URealtimeMeshSimple* Mesh = WeakMesh.Get())
        {
            Mesh->CreateBufferSet(FRealtimeMeshBufferSetKey::Create(0, FName("AsyncMesh")), Result.Get());
        }
    });
```

The helpers are in `Core/RealtimeMeshFuture.h`, and `ARealtimeMeshExample_Simple_AsyncBuild` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_AsyncBuild.cpp`) shows the pattern in full.

## Using it from Blueprint

Simple is Blueprintable, and most of what is above is available without writing any C++.

Start with the **Initialize Realtime Mesh Simple** node, passing your owner. From there you work with a `URealtimeMeshStreamSet` object, which is the Blueprint-friendly wrapper around the same mesh data. The `ARealtimeMeshActor` helper nodes **Make Stream**, **Make Stream Set**, and **Make Mesh Builder** are how you build geometry in a graph.

The **Create Buffer Set** and **Update Buffer Set** nodes take that stream set object plus an **On Complete** pin, which fires when the update has actually landed. Blueprint has no futures, so this callback is how you find out an update finished:

```cpp
// The C++ signatures behind the Blueprint nodes:
void CreateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey,
                     URealtimeMeshStreamSet* MeshData,
                     const FRealtimeMeshManagedCompletionCallback& OnComplete);

// Generates a fresh unique key for you and returns it:
FRealtimeMeshBufferSetKey CreateBufferSetUnique(const FRealtimeMeshLODKey& LODKey,
                     URealtimeMeshStreamSet* MeshData,
                     const FRealtimeMeshManagedCompletionCallback& OnComplete);

void UpdateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey,
                     URealtimeMeshStreamSet* MeshData,
                     const FRealtimeMeshManagedCompletionCallback& OnComplete);
```

`Set Should Auto Create Sections For Poly Groups` and `Should Auto Create Sections For Polygon Groups` are exposed too.

> If you have older graphs using the **Create Section Group** or **Update Section Group** nodes, those still exist and still work. They just show a deprecation warning. The **Buffer Set** nodes are the same functions under a clearer name.
