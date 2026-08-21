---
title: Lumen Cards
description: Generate a Lumen card representation for a RealtimeMesh at runtime, so Lumen's surface cache can light dynamic geometry properly.
---

Lumen does not light your mesh directly. It builds a **surface cache**, and it fills that cache using the mesh's **card representation**: a small set of oriented planes that roughly approximate the surface.

Static meshes get their cards at cook time. A Realtime Mesh has none until you generate one, and with no cards Lumen has nothing to capture. The mesh ends up missing or plainly wrong under Lumen GI. If your procedural geometry looks unlit or strangely dark, this is usually why.

RMC generates cards with the standalone `RealtimeMeshSDF` module and applies them through `RealtimeMeshExt`, mirroring the [distance field](../distance-fields/) path.

In practice you almost always want both a distance field and cards, because Lumen needs both, so the recommended path generates them together.

## What you need first

- **Lumen GI** enabled, in Project Settings under Engine, Rendering, Global Illumination.
- **Generate Mesh Distance Fields** enabled. Lumen consumes both the distance field and the cards. Cards on their own, with no distance field, are not enough to light the mesh.
- The GPU card path needs a real rendering context and does not run under `-NullRHI`. There is a CPU path for background use.

## The easy way: URealtimeMeshSDFLibrary

`URealtimeMeshSDFLibrary` (in `RealtimeMeshExt`, header `RealtimeMeshSDFLibrary.h`) generates a card representation on the GPU and applies it so Lumen's surface cache uses it.

It works on a buffer set, identified by an `FRealtimeMeshBufferSetKey`. Every entry point has a C++ `Async` form returning a `TFuture`, and a Blueprint callable form with a completion callback.

```cpp
#include "RealtimeMeshSDFLibrary.h"

// Mesh is a URealtimeMeshSimple*, Key identifies the buffer set.
URealtimeMeshSDFLibrary::GenerateAndApplyCardsAsync(Mesh, Key, /*MaxCards*/ 12, /*VolumeResolutionScale*/ 1.0f)
    .Next([](ERealtimeMeshProxyUpdateStatus Status)
    {
        UE_LOG(LogTemp, Log, TEXT("Lumen cards applied (status %d)."), static_cast<int32>(Status));
    });
```

`GenerateAndApplyCardsFromResidentAsync` builds the cards from the render proxy's buffers already on the GPU, with no geometry re-upload. Same resident fast path as the distance field library.

### Doing everything Lumen needs in one call

Since Lumen wants both representations, `GenerateAndApplyLumenFromResidentAsync` builds and applies **the distance field and the cards together**, from resident GPU buffers, completing once both are in place:

```cpp
URealtimeMeshSDFLibrary::GenerateAndApplyLumenFromResidentAsync(
        Mesh, Key, /*ResolutionScale*/ 1.0f, /*MaxCards*/ 12)
    .Next([](ERealtimeMeshProxyUpdateStatus Status)
    {
        UE_LOG(LogTemp, Log, TEXT("Full Lumen (DF + cards) applied (status %d)."), static_cast<int32>(Status));
    });
```

**This is the call you want** if your goal is simply "make my dynamic mesh light correctly under Lumen". One call, and the SDF volume gets built once and shared between the distance field and the cards rather than computed twice.

Each of these also has a Blueprint callable and non-`Async` form reporting completion through a `FRealtimeMeshManagedCompletionCallback`.

## Applying data you already have

If you already have a card representation, apply it directly. On `URealtimeMeshSimple`:

```cpp
FRealtimeMeshCardRepresentation Cards(CardData);   // from an FCardRepresentationData
Mesh->SetCardRepresentation(MoveTemp(Cards));
// ...or Mesh->ClearCardRepresentation(); to remove it.
```

`FRealtimeMeshCardRepresentation` (header `Mesh/RealtimeMeshCardRepresentation.h`) constructs from the engine's `FCardRepresentationData`. `SetCardRepresentation` returns a `TFuture<ERealtimeMeshProxyUpdateStatus>`, has a callback version, and has a matching `ClearCardRepresentation`.

## The low-level generator: RealtimeMesh::SDF

The `RealtimeMeshSDF` module generates the raw card representation with no dependency on Realtime Mesh. As with the distance field API, the public headers (`RealtimeMeshSDFCards.h` and others) alias a shared core into `RealtimeMesh::SDF`. Always write the `RealtimeMesh::SDF::` spelling.

```cpp
#include "RealtimeMeshSDFCards.h"

using namespace RealtimeMesh;

SDF::FSdfCardBuildParams Params;
Params.MaxCards = 12;
Params.Source   = SDF::ESdfCardSource::RayCast;   // precise, needs the triangle mesh

SDF::FSdfMeshInputCPU Input;
Input.Positions = Positions;   // TArrayView<const FVector3f>
Input.Indices   = Indices;     // TArrayView<const uint32>

// CPU: synchronous and needs no rendering context. Call it from a worker thread.
SDF::FSdfCardRepresentation Rep = SDF::GenerateCardRepresentationCPU(Input, Params);

// GPU: SDF and surfels on the GPU, clustering on the CPU. Returns a future.
SDF::GenerateCardRepresentationGPU(Input, Params).Next([](SDF::FSdfCardRepresentation Rep) { /* ... */ });
```

The card entry points, all in `RealtimeMesh::SDF`:

- `GenerateCardRepresentationCPU(FSdfMeshInputCPU, FSdfCardBuildParams)` is synchronous and needs no rendering context. `GenerateCardRepresentationCPUAsync` wraps it on the thread pool.
- `GenerateCardRepresentationGPU(...)` is the hybrid GPU build, returning a `TFuture<FSdfCardRepresentation>`. There is a resident-GPU-buffers version too.
- `GenerateCardRepresentationFromVolume(FSdfVolume, FSdfCardBuildParams)` derives cards from an existing SDF volume, so one distance field feeds both Lumen representations.

The result is a `FSdfCardRepresentation`. Check `IsValid()`, then `ToMeshCardsBuildData()` converts it into the engine `FMeshCardsBuildData` that a card representation expects.

### FSdfCardBuildParams fields that matter

- `MaxCards` (default `12`) is the most cards emitted across all six directions.
- `Source` (`ESdfCardSource`) decides how the surface samples that drive card placement are produced:
  - `RayCast` (the default) casts rays through the mesh BVH for real triangle hits and a visibility test. Precise, and it mirrors how the engine builds its own cards. It needs the triangle mesh. Use this for quality.
  - `DistanceField` derives samples from a signed distance field instead, using zero crossings and gradient normals. Much faster, and it only needs the volume, but it is softer on thin features. Good for runtime and streaming.
- `VolumeResolutionScale` is the `DistanceFieldResolutionScale` used to size the volume, for the versions that build one internally.
- `bMultiThreaded`, `MaxVoxelsPerDim`, `TargetVoxelSize`, `TargetNumSurfels`, `NumSurfelSamples`, and `NumVisibilityRays` are grid and sampling tuning. The defaults mirror the engine.

> Note: the GPU and resident-buffer paths always derive cards from the distance field on the GPU, so `Source` and the ray cast sampling parameters are ignored there.

## Removed API

The old `URealtimeMeshCardRepresentationGenerator` (`RealtimeMeshCardRepresentationGenerator.h`) has been removed. Use `URealtimeMeshSDFLibrary`, or `RealtimeMesh::SDF::GenerateCardRepresentationCPU`.

Output is equivalent to the old generator but not bit-for-bit identical. The new core builds its own BVH and derives bounds from the mesh.

## Examples

- **`ARealtimeMeshExample_Simple_LumenSupport`** is the one to start with. It generates a distance field *and* a card representation and applies both through `URealtimeMeshSDFLibrary`, so Lumen lights the mesh correctly. If your goal is a dynamic mesh that looks right under Lumen GI, read this one.
- **`ARealtimeMeshExample_Simple_CardProfile`** (`Source/RealtimeMeshExamples/Private/Simple/`) profiles card generation, comparing the `RayCast` and `DistanceField` sources and the CPU and GPU paths.
- **`ARealtimeMeshExample_Simple_DistanceField`** also exercises the full Lumen path. With cards and resident buffers enabled it calls `GenerateAndApplyLumenFromResidentAsync`.
