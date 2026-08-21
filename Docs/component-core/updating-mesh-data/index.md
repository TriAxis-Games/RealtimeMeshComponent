---
title: Updating Mesh Data
description: Every way to change a mesh after you've built it, from a full rebuild to poking bytes in a GPU buffer, plus reading data back and building off the game thread.
---

Once a buffer set exists there are several ways to change its geometry. They range from "throw the buffers away and upload everything again" to "change a handful of bytes in a buffer that is already on the GPU".

Picking the right one is the single biggest thing you can do for update performance. This page walks the whole range, using the shipped `URealtimeMeshSimple` examples.

The short version:

- Changing the **shape of the mesh** (different number of vertices or triangles) means a full rebuild with `UpdateBufferSet`.
- Changing **only values** (positions, colors, normals) on a **Dynamic** buffer set means `EditMeshInPlace` or `EditMeshInPlaceRanged`, which are far cheaper.

## Full rebuild: UpdateBufferSet

The simplest option. Rebuild the `StreamSet` and hand it back. This works on any buffer set, copes with the vertex count changing, and is the right default when the mesh is not changing every frame:

```cpp
FRealtimeMeshStreamSet StreamSet;
// ...rebuild the whole grid into StreamSet with a builder...

const FRealtimeMeshBufferSetKey GroupKey =
    FRealtimeMeshBufferSetKey::Create(0, FName("DynamicMesh"));

// Create the buffer set on the first call, then just update it afterwards.
if (RealtimeMesh->GetBufferSets(FRealtimeMeshLODKey(0)).Contains(GroupKey))
{
    RealtimeMesh->UpdateBufferSet(GroupKey, StreamSet);
}
else
{
    RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);
}
```

`UpdateBufferSet` takes the stream set either by const reference or by rvalue, so use `MoveTemp(StreamSet)` when you are done with it.

On a **Static** buffer set this rebuilds the render proxy, which is fine occasionally but is exactly why per-frame animation wants either a Dynamic buffer set or the in-place path below.

`ARealtimeMeshExample_Simple_DynamicUpdate` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_DynamicUpdate.cpp`) does this every frame on a rippling grid.

## In-place: EditMeshInPlace

When the shape stays fixed and you are only changing vertex *values*, you can edit the CPU data in place and upload just the streams you touched. No reallocation, no proxy rebuild.

This needs a **Dynamic** draw type, which you set when you create the buffer set (see [Buffer Sets & Sections](../sections/)):

```cpp
// Dynamic draw type means the buffers are allocated so they can be updated in place.
RealtimeMesh->CreateBufferSet(
    GroupKey, StreamSet,
    FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType::Dynamic));
```

`EditMeshInPlace` takes a callback that changes the `FRealtimeMeshStreamSet` and returns the set of streams it touched:

```cpp
RealtimeMesh->EditMeshInPlace(GroupKey,
    [Time, Width](FRealtimeMeshStreamSet& Streams) -> TSet<FRealtimeMeshStreamKey>
    {
        FRealtimeMeshStream* Positions = Streams.Find(FRealtimeMeshStreams::Position);
        if (!Positions)
        {
            return {};
        }

        TRealtimeMeshStreamBuilder<FVector3f, void> Position(*Positions);
        for (int32 Index = 0; Index < Position.Num(); ++Index)
        {
            FVector3f V = Position.GetValue(Index);
            V.Z = /* ...new height... */;
            Position.Set(Index, V);
        }

        // Tell RMC which streams changed, so only those get uploaded.
        return { FRealtimeMeshStreams::Position };
    });
```

Returning the right set matters. RMC uploads exactly what you name and nothing else. Streams that are not eligible for the fast path quietly fall back to a full update, so you get correct results either way.

### Narrowing it further with EditMeshInPlaceRanged

If you only touched part of a stream, `EditMeshInPlaceRanged` lets you say which part. For each changed stream you return the half-open element range `[lower, upper)` you actually wrote, and only that slice of the GPU buffer is updated:

```cpp
RealtimeMesh->EditMeshInPlaceRanged(GroupKey,
    [/* captures */](FRealtimeMeshStreamSet& Streams) -> TMap<FRealtimeMeshStreamKey, FInt32Range>
    {
        TMap<FRealtimeMeshStreamKey, FInt32Range> Updated;

        FRealtimeMeshStream* Positions = Streams.Find(FRealtimeMeshStreams::Position);
        if (!Positions) { return Updated; }

        TRealtimeMeshStreamBuilder<FVector3f, void> Position(*Positions);
        for (int32 Index = VertexStart; Index < VertexEnd; ++Index)
        {
            FVector3f V = Position.GetValue(Index);
            V.Z = /* ...new height... */;
            Position.Set(Index, V);
        }

        Updated.Add(FRealtimeMeshStreams::Position, FInt32Range(VertexStart, VertexEnd));
        return Updated;
    });
```

Everything outside the range you name is left exactly as it was.

Both patterns come from `ARealtimeMeshExample_Simple_FastUpdate` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_FastUpdate.cpp`), which animates a Dynamic grid. Its full-grid mode uses `EditMeshInPlace` and its middle-band mode uses `EditMeshInPlaceRanged`, so you can watch the top and bottom of the grid stay flat.

## Reading data back: ProcessMesh

To look at a buffer set's current CPU data without copying it out, use `ProcessMesh`. It calls your callback with the `FRealtimeMeshStreamSet` while the lock is held, so the reference cannot escape:

```cpp
RealtimeMesh->ProcessMesh(GroupKey, [](const FRealtimeMeshStreamSet& Streams)
{
    // Inspect Streams here. Do NOT keep the reference past this callback.
});
```

Two rules. Do not store the reference beyond the callback, and do not call anything that writes to the same mesh from inside it.

`URealtimeMeshSimple` also has `ProcessBufferSet`, which hands you the buffer set object rather than just its streams, under the same rules.

## Building off the game thread

Heavy procedural generation should not stall the game thread. A `FRealtimeMeshStreamSet` is pure CPU data with no `UObject` access, which makes it safe to build on a worker thread and then hop back to hand it over.

RMC ships helpers for exactly this, in `Core/RealtimeMeshFuture.h`, in the `RealtimeMesh` namespace:

```cpp
using namespace RealtimeMesh;

TWeakObjectPtr<URealtimeMeshSimple> WeakMesh(RealtimeMesh);

// 1) Build the expensive StreamSet off the game thread.
TFuture<FRealtimeMeshStreamSet> BuildFuture = DoOnAsyncThread([]()
{
    return BuildGridStreamSet(); // pure CPU, safe on a worker thread
});

// 2) Hop back to the game thread to create or update the buffer set.
ContinueOnGameThread(MoveTemp(BuildFuture),
    [WeakMesh](TFuture<FRealtimeMeshStreamSet>&& Result)
    {
        const FRealtimeMeshStreamSet& StreamSet = Result.Get();
        if (URealtimeMeshSimple* Mesh = WeakMesh.Get())
        {
            Mesh->CreateBufferSet(
                FRealtimeMeshBufferSetKey::Create(0, FName("AsyncMesh")), StreamSet);
        }
    });
```

This is the recommended pattern for anything expensive, and it is what `ARealtimeMeshExample_Simple_AsyncBuild` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_AsyncBuild.cpp`) demonstrates. Note it runs the build in `BeginPlay` rather than `OnConstruction`, so it does not restart every time you nudge the actor in the editor.

Capture with `TWeakObjectPtr` as shown. If the mesh or actor is destroyed while the build is still running, the callback then does nothing instead of crashing.

See [Advanced Topics](../../advanced/) for more on these helpers.

## What the return value is for

Every call that changes geometry (`CreateBufferSet`, `UpdateBufferSet`, `EditMeshInPlace` and its ranged version, `SetSectionVisibility`, and so on) returns a `TFuture<ERealtimeMeshProxyUpdateStatus>`. It completes once the render thread has caught up.

The status is one of:

- `Updated`, the proxy was updated.
- `NoUpdate`, nothing needed updating.
- `NoProxy`, there was no render proxy to update, usually because the mesh is not being rendered right now.

You can ignore it completely and the update still happens. Or you can wait on it to chain follow-up work:

```cpp
RealtimeMesh->UpdateBufferSet(GroupKey, MoveTemp(StreamSet))
    .Next([](ERealtimeMeshProxyUpdateStatus Status) { /* ...after the proxy updates... */ });
```

**In Blueprint** there are no futures. The same operations take an `OnComplete` callback pin (`FRealtimeMeshManagedCompletionCallback`) that fires with the same status once the update lands.
