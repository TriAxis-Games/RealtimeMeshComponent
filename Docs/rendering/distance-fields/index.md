---
title: Distance Fields
description: Generate a signed distance field for a RealtimeMesh at runtime, on the GPU or CPU, so Lumen, distance field shadows, and DFAO work on dynamic geometry.
---

A mesh distance field is a volume that stores, for every point in a grid, how far away the mesh surface is. Unreal uses it for Lumen global illumination, distance field soft shadows, and distance field ambient occlusion.

Static meshes get theirs at cook time. A Realtime Mesh has to build one at runtime, from whatever geometry it is currently holding.

RMC generates distance fields with the standalone `RealtimeMeshSDF` module and applies them through `RealtimeMeshExt`. There are three ways in, from most convenient to most controllable.

## What you need first

- **Generate Mesh Distance Fields** has to be enabled in Project Settings, under Engine, Rendering. Without it the engine allocates no distance field atlas and anything you apply is quietly ignored.
- The GPU generation path needs a real rendering context. It does not run under `-NullRHI`. There is a CPU path for headless and background use.

## The easy way: URealtimeMeshSDFLibrary

`URealtimeMeshSDFLibrary` (in `RealtimeMeshExt`, header `RealtimeMeshSDFLibrary.h`) is the one-call option. It reads a buffer set's geometry, generates the field on the GPU, packs it into the engine's format, and applies it through the standard `SetDistanceField` path, so Lumen, distance field shadows, and DFAO all pick it up.

Generation is explicit and on demand. Nothing happens until you ask.

It works on a **buffer set**, identified by an `FRealtimeMeshBufferSetKey`. Every entry point exists as a C++ `Async` function returning a `TFuture`, and as a Blueprint callable function that reports completion through a callback.

```cpp
#include "RealtimeMeshSDFLibrary.h"

// Mesh is a URealtimeMeshSimple*, Key identifies the buffer set to convert.
URealtimeMeshSDFLibrary::GenerateAndApplyDistanceFieldAsync(Mesh, Key, /*ResolutionScale*/ 1.0f)
    .Next([](ERealtimeMeshProxyUpdateStatus Status)
    {
        UE_LOG(LogTemp, Log, TEXT("Distance field applied (status %d)."), static_cast<int32>(Status));
    });
```

`ResolutionScale` is the engine's `DistanceFieldResolutionScale`, where 1 is default detail and higher means finer. The field is sized the same way the engine sizes its own, and gets the exterior band added automatically, so it matches a baked static mesh distance field at the same scale.

### Skipping the readback

If the mesh has already rendered at least once, the `...FromResident` variants skip reading the geometry back from the GPU and re-uploading it. The field is built straight from the vertex and index buffers already sitting in GPU memory:

```cpp
URealtimeMeshSDFLibrary::GenerateAndApplyDistanceFieldFromResidentAsync(Mesh, Key, 1.0f);
```

This needs the mesh's render proxy to exist, meaning it must have been rendered, and it needs manual vertex fetch support. It uses the mesh's local bounds plus `BoundsPadding`.

Both variants also have Blueprint callable and non-`Async` forms (`GenerateAndApplyDistanceField` and `GenerateAndApplyDistanceFieldFromResident`) that report completion through a `FRealtimeMeshManagedCompletionCallback`.

## Applying data you already have

If you already have a distance field volume, from your own pipeline or from the low-level generator below, apply it directly. On `URealtimeMeshSimple`:

```cpp
FRealtimeMeshDistanceField DistanceField(VolumeData);   // from an FDistanceFieldVolumeData
Mesh->SetDistanceField(MoveTemp(DistanceField));
// ...or Mesh->ClearDistanceField(); to remove it.
```

`FRealtimeMeshDistanceField` (header `Mesh/RealtimeMeshDistanceField.h`) is RMC's serializable wrapper around the engine's sparse distance field volume. It constructs from an `FDistanceFieldVolumeData` and can hand rendering data back.

`SetDistanceField` returns a `TFuture<ERealtimeMeshProxyUpdateStatus>`, has a callback version, and has a matching `ClearDistanceField`.

## The low-level generator: RealtimeMesh::SDF

The `RealtimeMeshSDF` module generates the raw volume, with no dependency on Realtime Mesh at all. Its public headers (`RealtimeMeshSDFGenerator.h`, `RealtimeMeshSDFGenerate.h`, and others) alias a shared core into the `RealtimeMesh::SDF` namespace. Always write the `RealtimeMesh::SDF::` spelling. The examples do `using namespace RealtimeMesh;` and then write `SDF::`.

You feed it a `FSdfMeshInputCPU` (positions plus a flat `uint32` index list) and a `FSdfBuildParams`, and get back a `FSdfVolume`:

```cpp
#include "RealtimeMeshSDFGenerator.h"

using namespace RealtimeMesh;

SDF::FSdfBuildParams Params;
Params.ResolutionMode = SDF::ESdfResolutionMode::Auto;  // engine-matching sizing plus exterior band
Params.ResolutionScale = 1.0f;                          // in Auto mode, the DistanceFieldResolutionScale
Params.bComputeSign = true;                             // false gives an unsigned distance field
Params.Precision = SDF::ESdfPrecision::Float32;

SDF::FSdfMeshInputCPU Input;
Input.Positions = Positions;   // TArrayView<const FVector3f>
Input.Indices   = Indices;     // TArrayView<const uint32>, 3 per triangle

// GPU path. Returns immediately, the future completes after readback.
SDF::GenerateDistanceField(Input, Params).Next([](SDF::FSdfVolume Volume)
{
    if (Volume.IsValidVolume()) { /* pack and apply, or inspect */ }
});
```

The generation entry points, all in `RealtimeMesh::SDF`:

- `GenerateDistanceField(FSdfMeshInputCPU, FSdfBuildParams)` is the GPU path, returning a `TFuture<FSdfVolume>`. The portable choice for any mesh source.
- `GenerateDistanceFieldCPU(...)` runs entirely on the CPU with no rendering context, synchronously. Call it from a worker thread for background or asset load use. `GenerateDistanceFieldCPUAsync(...)` wraps it on the thread pool.
- `GenerateDistanceField(FSdfMeshInputGPU, ...)` builds from buffers already on the GPU, with no re-upload.
- `Generate(FSdfMeshInputCPU, FSdfGenerateRequest)` (header `RealtimeMeshSDFGenerate.h`) is the unified request. Ask for a distance field, or cards, or both, choosing CPU or GPU. When you ask for both, the SDF volume is built once and shared.

On failure every variant still completes, but with an invalid volume. Check `FSdfVolume::IsValidVolume()`.

### FSdfBuildParams settings that matter

- `ResolutionMode`. `Auto` (the default) works out the resolution from mesh size the same way Unreal does, and adds the exterior band. `Explicit` uses exactly the `Resolution` and `Bounds` you give it.
- `ResolutionScale`, `VoxelDensity`, and `MaxResolution` are the Auto mode sizing knobs. `ResolutionScale` is the engine's `DistanceFieldResolutionScale`.
- `Resolution` is the exact voxel grid size, used only in `Explicit` mode.
- `Bounds` and `BoundsPadding` let you supply explicit world space bounds, or leave `Bounds` invalid to fit them automatically and pad.
- `bComputeSign`. Set false for an unsigned field.
- `Precision`. `Float32` (default) or `Float16` storage.

Once you have a volume, pack it into engine data with `SDF::PackToDistanceFieldVolumeData(Volume, MeshBounds, OutData)` (header `RealtimeMeshSDFPacker.h`), wrap it in a `FRealtimeMeshDistanceField`, and apply it as above. `URealtimeMeshSDFLibrary` does all of that for you.

### GPU or CPU?

- **GPU** (`GenerateDistanceField`) is much faster and the right choice for anything interactive, but it needs a real rendering context and cannot run under `-NullRHI`.
- **CPU** (`GenerateDistanceFieldCPU`) is self-contained and needs no rendering context, which makes it ideal for background threads and asset loading. It builds an LBVH and uses a band-limited Fast Winding Number for the sign, so it is far quicker than a naive per-voxel generator, but still heavier than the GPU path on large volumes.

## Removed API

The old `URealtimeMeshDistanceFieldGeneration` API (`RealtimeMeshDistanceFieldGenerator.h`) has been removed.

Use `URealtimeMeshSDFLibrary::GenerateAndApplyDistanceField[Async]` instead, or `RealtimeMesh::SDF::GenerateDistanceFieldCPU` plus `PackToDistanceFieldVolumeData` if you need the raw path. Output is equivalent to the old generator but not bit-for-bit identical.

## Examples

- **`ARealtimeMeshExample_Simple_DistanceField`** (`Source/RealtimeMeshExamples/Private/Simple/`) builds a box, generates a GPU distance field with the low-level `SDF::GenerateDistanceField`, draws a colour-coded slice (red inside, green outside), and can apply the field, the cards, or the full Lumen setup through `URealtimeMeshSDFLibrary`.
- **`ARealtimeMeshExample_Simple_DistanceFieldProfile`** sweeps through grid resolutions and profiles CPU versus GPU generation time.
