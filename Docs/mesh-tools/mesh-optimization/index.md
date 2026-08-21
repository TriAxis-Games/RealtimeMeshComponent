---
title: Mesh Optimization
description: Reindex, cache-optimise, reduce overdraw, and improve vertex fetch on a stream set with the meshoptimizer-backed URealtimeMeshDataOptimizer.
---

Geometry you generate or load is rarely laid out the way a GPU would prefer. `URealtimeMeshDataOptimizer` runs the industry-standard [meshoptimizer](https://github.com/zeux/meshoptimizer) passes over a `FRealtimeMeshStreamSet` to fix that.

You normally run these once, on freshly generated or freshly loaded geometry, **before** handing the stream set to `CreateBufferSet`. The payoff is faster rendering every frame the mesh is on screen, and for the indexing pass, a smaller vertex buffer as well.

`URealtimeMeshDataOptimizer` lives in the `RealtimeMeshExt` module (see the [section overview](../) for the `Build.cs` dependency). All four passes change the stream set in place, and each has a C++ version taking `RealtimeMesh::FRealtimeMeshStreamSet&` and a Blueprint version taking `URealtimeMeshStreamSet*`.

## The four passes

### OptimizeMeshIndexing

Builds or rebuilds the triangle index buffer, removing duplicate vertices as it goes. If the stream set already has a Triangles stream it gets reindexed. If not, one is created by treating every successive group of three vertices as a triangle.

**Run this first.** It is what turns a pile of duplicated vertices into a compact indexed mesh, and the other passes assume an index buffer exists.

```cpp
URealtimeMeshDataOptimizer::OptimizeMeshIndexing(StreamSet);
```

### OptimizeVertexCache

Reorders the index buffer so the GPU reuses vertices it has already transformed, instead of transforming the same vertex several times. This is usually the single biggest win.

```cpp
URealtimeMeshDataOptimizer::OptimizeVertexCache(StreamSet,
    ERealtimeMeshOptimizationQuality::RenderingEfficiency);   // or ::GenerationSpeed
```

`ERealtimeMeshOptimizationQuality` has two values. `RenderingEfficiency` gives the best result and is the Blueprint default. `GenerationSpeed` is quicker to compute, which matters if you are optimising during gameplay and care about the hitch more than the last few percent.

### OptimizeOverdraw

Reorders triangles to reduce how often the GPU shades the same pixel twice.

The `Threshold` caps how much vertex cache efficiency it is allowed to give up in exchange. The default `1.01` means "sacrifice at most 1%". Higher values let it work harder on overdraw at the cost of the previous pass's gains.

```cpp
URealtimeMeshDataOptimizer::OptimizeOverdraw(StreamSet, /*Threshold=*/1.01f);
```

### OptimizeVertexFetch

Reorders the vertex buffer so vertices sit in memory roughly in the order the index buffer asks for them, which makes memory access more predictable.

**Run this last**, once the index order has settled.

```cpp
URealtimeMeshDataOptimizer::OptimizeVertexFetch(StreamSet);
```

## Run them in this order

The passes build on each other, so order matters. Follow the order meshoptimizer recommends:

1. **`OptimizeMeshIndexing`** builds and compacts the index buffer.
2. **`OptimizeVertexCache`** reorders indices for the GPU's vertex cache.
3. **`OptimizeOverdraw`** reorders triangles to cut overdraw, keeping most of step 2's gain.
4. **`OptimizeVertexFetch`** reorders vertices to match the final index order.

```cpp
#include "RealtimeMeshDataOptimizer.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

void AMyActor::BuildOptimizedMesh(FRealtimeMeshStreamSet&& StreamSet)
{
    // 1. Index the geometry. Removes duplicate vertices, ensures a triangle stream exists.
    URealtimeMeshDataOptimizer::OptimizeMeshIndexing(StreamSet);

    // 2. Vertex cache. The biggest win.
    URealtimeMeshDataOptimizer::OptimizeVertexCache(StreamSet,
        ERealtimeMeshOptimizationQuality::RenderingEfficiency);

    // 3. Overdraw, without giving up more than about 1% of the cache gain.
    URealtimeMeshDataOptimizer::OptimizeOverdraw(StreamSet, 1.01f);

    // 4. Vertex fetch. Last, once the index order is final.
    URealtimeMeshDataOptimizer::OptimizeVertexFetch(StreamSet);

    // Now hand the optimised stream set to the mesh.
    URealtimeMeshSimple* Mesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
    const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("Optimized"));
    Mesh->CreateBufferSet(GroupKey, MoveTemp(StreamSet));
}
```

Since the optimizer only touches CPU data, you can run all four passes on a worker thread and only call `CreateBufferSet` on the game thread.

## When it's worth it

- **Geometry you generate** without deduplicating vertices yourself. `OptimizeMeshIndexing` alone can shrink the vertex buffer substantially.
- **Geometry you load**, for example from the [OBJ loader](../loading-meshes/), before it goes live.
- **Any large mesh you render a lot.** The cache, overdraw, and fetch passes pay for themselves every frame it is visible.

For meshes you rebuild every frame, weigh the one-off cost against the per-frame saving. If you do optimise at runtime, consider `ERealtimeMeshOptimizationQuality::GenerationSpeed` for the vertex cache pass.

## Related

- [Loading Meshes](../loading-meshes/) to optimise right after loading.
- [Converting Static Meshes](../converting-static-meshes/) and [Dynamic Meshes](../converting-dynamic-meshes/) to optimise a converted stream set before creating its buffer set.
