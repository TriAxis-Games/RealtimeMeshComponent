---
title: Loading Meshes at Runtime
description: Load Wavefront OBJ files into a RealtimeMesh while your game is running, either on the spot or in the background.
---

RMC can load geometry from disk at runtime, which is how you support things like players importing their own models.

The built-in loader handles **Wavefront OBJ** files through `URealtimeMeshObjLoader`, in the `RealtimeMeshExt` module (see the [section overview](../) for the `Build.cs` dependency). It parses the OBJ into a `FRealtimeMeshStreamSet` and hands back the material definitions from the accompanying `.mtl` file, so you can build the actual Unreal materials yourself.

For any other format, the [general pattern](#loading-other-formats) at the bottom of this page applies: parse the file, fill a stream set, create a buffer set.

The working example is `ARealtimeMeshExample_Simple_ObjLoad` (`Source/RealtimeMeshExamples/.../Simple/`).

## What you get back

`URealtimeMeshObjLoader` parses the OBJ, removes duplicate vertices (matching on position, normal, and texcoord together), and fills a stream set with:

- **Position** and **Triangles**, always.
- **Tangents**, a normal and tangent basis seeded from the OBJ's normals and then regenerated. See the options below.
- **TexCoords**, UV channel 0, if the file has any.
- **Color**, vertex colors, if the file supplies them.
- **PolyGroups**, one per OBJ material index. After loading, the triangles are sorted by poly group, so each material becomes its own section when you create the buffer set. Faces with no material go into group `0`.

Only **triangle faces** are supported. If the loader hits an n-gon it fails with a message, so triangulate on export.

One thing to be aware of: materials come back as **data**, not as `UMaterialInterface` objects. Each `FRealtimeMeshOBJMaterial` carries the parsed MTL fields (diffuse, specular, and emission colors, roughness, metallic, IOR) and the texture references as `FRealtimeMeshOBJTextureInfo` with file paths and sampler options. Turning those into engine materials and assigning them to slots is up to you.

## Load options

`FRealtimeMeshOBJLoadOptions` controls the parse:

```cpp
FRealtimeMeshOBJLoadOptions Options;
Options.MaterialSearchPath   = TEXT("");   // where to look for the .mtl (defaults to "./")
Options.bReverseWinding      = false;      // flip triangle winding
Options.bGenerateTangents    = true;       // regenerate the tangent basis after loading
Options.bGenerateSmoothTangents = true;    // smooth rather than per-face tangents
```

If either tangent option is set, the loader runs RMC's tangent generator over the loaded data. That means you get a usable tangent basis even from an OBJ that only had normals, or none at all.

## Loading on the spot (C++)

The synchronous entry point fills a stream set directly. Parsing a large OBJ is not cheap, so on the game thread you should prefer the [async path](#loading-in-the-background-c) below. The synchronous call is fine on a worker thread, or for small files.

```cpp
#include "RealtimeMeshObjLoader.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

void AMyActor::LoadObj(const FString& FilePath)
{
    FRealtimeMeshStreamSet StreamSet;
    TArray<FRealtimeMeshOBJMaterial> Materials;

    FRealtimeMeshOBJLoadOptions Options;   // defaults are fine for most files

    const FRealtimeMeshOBJLoadResult Result =
        URealtimeMeshObjLoader::LoadStreamSetFromOBJFile(StreamSet, Materials, FilePath, Options);

    if (!Result.bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("OBJ load failed: %s"), *Result.Message);
        return;
    }

    URealtimeMeshSimple* Mesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

    // One material slot per OBJ material. Build your UMaterialInterface from the
    // FRealtimeMeshOBJMaterial fields (diffuse color, texture paths, and so on).
    for (int32 Index = 0; Index < Materials.Num(); ++Index)
    {
        Mesh->SetupMaterialSlot(Index, FName(*Materials[Index].Name));
    }

    // The stream set has poly groups, one per material, so creating the buffer set
    // makes one section per material automatically.
    const FRealtimeMeshBufferSetKey GroupKey =
        FRealtimeMeshBufferSetKey::Create(0, FName("LoadedOBJ"));

    Mesh->CreateBufferSet(GroupKey, MoveTemp(StreamSet));
}
```

On any parse error, `Result.bSuccess` is false and `Result.Message` explains what went wrong.

## Loading in the background (C++)

For anything loaded during gameplay, use the async entry point. It runs the whole parse, dedup, and tangent generation on the thread pool and hands back a `TFuture<FRealtimeMeshOBJLoadData>`. That payload holds the `Streams`, the `Materials`, and the `Result` together, and moves cheaply.

**Important:** the future completes on the worker thread that finished the job, not on the game thread. Marshal back yourself before touching any `UObject`, including the mesh component:

```cpp
using namespace RealtimeMesh;

URealtimeMeshObjLoader::LoadStreamSetFromOBJFileAsync(FilePath, FRealtimeMeshOBJLoadOptions())
    .Next([WeakThis = TWeakObjectPtr<AMyActor>(this)](FRealtimeMeshOBJLoadData Data)
    {
        // Still on the worker thread here, so hop to the game thread for the UObject work.
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Data = MoveTemp(Data)]() mutable
        {
            AMyActor* Self = WeakThis.Get();
            if (!Self || !Data.Result.bSuccess)
            {
                return;
            }

            URealtimeMeshSimple* Mesh =
                Self->GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

            for (int32 Index = 0; Index < Data.Materials.Num(); ++Index)
            {
                Mesh->SetupMaterialSlot(Index, FName(*Data.Materials[Index].Name));
            }

            const FRealtimeMeshBufferSetKey GroupKey =
                FRealtimeMeshBufferSetKey::Create(0, FName("LoadedOBJ"));

            Mesh->CreateBufferSet(GroupKey, MoveTemp(Data.Streams));
        });
    });
```

## Loading in the background (Blueprint)

`LoadStreamSetFromOBJFileAsync` also has a Blueprint version, and it handles the threading for you. You pass in a `URealtimeMeshStreamSet` to fill and hook up a completion event. The parse happens off the game thread, and the event fires **on the game thread** once the stream set is populated, so it is safe to do whatever you like from there.

```cpp
// The C++ shape of the Blueprint node. In a graph this is a call with an event pin.
URealtimeMeshObjLoader::LoadStreamSetFromOBJFileAsync(
    StreamSetObject,          // URealtimeMeshStreamSet* to fill (keep it alive until completion)
    FilePath,
    Options,
    OnComplete);              // fires with FRealtimeMeshOBJLoadResult + the material array
```

The event gives you a `FRealtimeMeshOBJLoadResult` and a `TArray<FRealtimeMeshOBJMaterial>`. By the time it fires the stream set you passed in is already filled, so create your buffer set from it right there.

Two things to watch: check `Result.bSuccess` before using the data, and keep the stream set object alive until the event fires. Storing it as a variable on your actor is enough.

## Loading other formats

There is no built-in loader for anything else, but nothing about the pipeline is OBJ specific. The loader above is just one way of filling a stream set. For any other format:

1. **Parse** the file into plain arrays: positions, indices, and whatever attributes it carries.
2. **Fill a stream set.** Either add streams directly with `StreamSet.AddStream<...>(FRealtimeMeshStreams::Position)` and friends, or use a `TRealtimeMeshBuilderLocal`, which handles the layout for you.
3. **Create the buffer set** with `CreateBufferSet(GroupKey, MoveTemp(StreamSet))`.

If the data is large, do steps 1 and 2 on a worker thread and only step 3 on the game thread, exactly like the async OBJ path above.

## Related

- [Mesh Optimization](../mesh-optimization/) runs the optimizer over your loaded stream set before you create the buffer set.
- [Key Concepts](../../keyconcepts/) covers stream sets, builders, and buffer sets.
