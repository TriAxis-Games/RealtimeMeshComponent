---
title: RealtimeMesh Constructed
description: URealtimeMeshConstructed, a generator-driven type that produces geometry on demand from parameters, with no CPU-side mesh stored.
---

`URealtimeMeshConstructed` does not keep a copy of your mesh. Instead it holds a **generator** plus a set of per-mesh parameters, and regenerates the geometry whenever it needs to.

The generator is stateless. Its whole job is to answer one question: "given these parameters, produce this part of a mesh." Because it holds no per-mesh state, a single generator can drive hundreds of meshes at once, on worker threads, safely.

This is what the spatial streaming system is built on, and it is the right choice when your geometry is a pure function of some parameters rather than something you author and keep.

Header: `Source/RealtimeMeshExt/Public/Factory/RealtimeMeshConstructed.h`. Like [Dynamic](../realtime-mesh-dynamic/), this type lives in the **`RealtimeMeshExt`** module, so add `"RealtimeMeshExt"` to your `.Build.cs` dependencies.

## How a generator works

A generator derives from `FRealtimeMeshGenerator` and overrides only the parts it produces. Two pieces of data travel *alongside* it rather than living inside it:

- **The config** is a `USTRUCT` deriving from `FRealtimeMeshProviderConfig`. This is the generator's fixed setup, things like grid resolution and UV scale. It doubles as the editor surface: an `FInstancedStruct` property whose type dropdown is filtered to provider configs, so a designer can pick and tune it without code.
- **The descriptor** is an `FInstancedStruct` carrying the identity of one particular mesh: which cell, what LOD, what seed. It gets passed in with every generation request, and that is what lets one generator serve many meshes.

Because the per-mesh state is in the descriptor (a plain value) and the config is copied by value, generation is safe to run off the game thread. The generator's phases (`GetStructure`, `GetSectionGroup`, and the optional `GetBounds`, `GetCollision`, and `GetNanite`) all run on whatever `AllowedThread()` says, which defaults to async.

> `GetSectionGroup` fills in one **buffer set**. The generator interface still uses the older `SectionGroup` spelling. Same thing, older name.

RMC ships a reference generator, a sine-height plane, in `Source/RealtimeMeshExt/Public/Factory/RealtimeMeshGeneratorPlane.h`:

```cpp
// The config: editor setup, shared across every mesh this generator drives.
USTRUCT(BlueprintType)
struct FRealtimeMeshPlaneProviderConfig : public FRealtimeMeshProviderConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, ...) FIntPoint  GridSize = FIntPoint(128, 128);
    UPROPERTY(EditAnywhere, ...) FVector2f  UVScale  = FVector2f(1.0f / 1.28f, 1.0f / 1.28f);
};

// The descriptor: which mesh this is, at what LOD.
USTRUCT(BlueprintType)
struct FRealtimeMeshPlaneCellDescriptor
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, ...) FIntPoint CellLocation = FIntPoint(0, 0);
    UPROPERTY(EditAnywhere, ...) int32     LOD          = 0;
    UPROPERTY(EditAnywhere, ...) FVector2f CellSize     = FVector2f(64.0f, 64.0f);
    UPROPERTY(EditAnywhere, ...) float     CellHeight   = 256.0f;
};
```

## Driving it from C++

Initialize the mesh, resolve a generator from your config, bind it with a descriptor, and regenerate:

```cpp
#include "Factory/RealtimeMeshConstructed.h"
#include "Factory/RealtimeMeshGeneratorPlane.h"
#include "Factory/RealtimeMeshProviderRegistry.h"
using namespace RealtimeMesh;

URealtimeMeshConstructed* Mesh = Component->InitializeRealtimeMesh<URealtimeMeshConstructed>();

// Pick a config and a descriptor. In a real actor these are usually FInstancedStruct
// properties edited in the details panel. Here we set them up in code.
FInstancedStruct ProviderConfig;
ProviderConfig.InitializeAs<FRealtimeMeshPlaneProviderConfig>();
FInstancedStruct Descriptor;
Descriptor.InitializeAs<FRealtimeMeshPlaneCellDescriptor>();

// Find the C++ generator registered for that config struct.
const FRealtimeMeshGeneratorPtr Generator = FRealtimeMeshProviderRegistry::Get().CreateGenerator(ProviderConfig);
if (Generator.IsValid())
{
    // Bind the generator and descriptor, then generate everything.
    Mesh->SetGenerator(Generator.ToSharedRef(), Descriptor);
    Mesh->Regenerate(FRealtimeMeshDirtyFlags::AllDirty());
}
```

`SetGenerator` stores the generator and descriptor on the mesh. `Regenerate(DirtyFlags, CancellationToken)` then regenerates only the parts whose dirty bit is set, so partial updates such as "just the bounds" stay cheap. It returns a `TFuture<ERealtimeMeshProxyUpdateStatus>`, and does nothing if no generator is bound.

Call `Regenerate` again any time the descriptor's inputs change. The mesh keeps hold of the generator, so there is no need to look it up again.

`ARealtimeMeshConstructedActor` (`Source/RealtimeMeshExamples/Public/RealtimeMeshConstructedActor.h` and the matching `.cpp`) does exactly this.

## Picking a generator in the editor

The example actor exposes the config and descriptor as `FInstancedStruct` properties, so a designer can pick and tune the generator without touching code:

```cpp
// Type dropdown filtered to registered provider configs, defaulting to the plane provider.
UPROPERTY(EditAnywhere, Category = "RealtimeMesh",
    meta = (BaseStruct = "/Script/RealtimeMeshExt.RealtimeMeshProviderConfig", ExcludeBaseStruct))
FInstancedStruct ProviderConfig;

// The per-mesh descriptor handed to the generator.
UPROPERTY(EditAnywhere, Category = "RealtimeMesh")
FInstancedStruct Descriptor;
```

That `meta = (BaseStruct = ..., ExcludeBaseStruct)` filter is what limits the dropdown to valid provider configs. When the actor is constructed it resolves the chosen config to its generator through `FRealtimeMeshProviderRegistry` and binds it, exactly like the C++ snippet above.

Point the `ProviderConfig` dropdown at a registered config, edit its fields, and the matching generator produces the geometry.

## Writing your own

To add a generator you need three things: a config `USTRUCT` deriving from `FRealtimeMeshProviderConfig`, a generator deriving from `FRealtimeMeshGenerator` (or from the convenience base `TRealtimeMeshProvider<TConfig>`, which stores a typed config for you), and a registration so the registry can find it.

Keep the phase methods thread safe and `const`, because one generator instance serves many meshes at the same time.

The full walkthrough is in [Custom Providers](../../advanced/custom-providers/).
