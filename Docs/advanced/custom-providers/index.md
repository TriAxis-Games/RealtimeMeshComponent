---
title: Custom Providers
description: Write a reusable, thread-safe mesh generator with FRealtimeMeshGenerator and drive a Constructed mesh from it.
---

A *generator* answers one question: "given this descriptor, produce the requested parts of a mesh."

It holds no per-mesh state and no `UObject` references. That is the whole design, and it is what lets one configured generator be shared across many meshes and called concurrently on worker threads. It is why generators underpin both one-off procedural meshes and the [spatial streaming](../spatial-streaming/) system, where hundreds of chunks share a single generator instance.

Generators live in the **`RealtimeMeshExt`** module. Add it to `PrivateDependencyModuleNames` in your `Build.cs`:

```csharp
PrivateDependencyModuleNames.AddRange(new string[] {
    "RealtimeMeshComponent",
    "RealtimeMeshExt",   // FRealtimeMeshGenerator, URealtimeMeshConstructed
});
```

Everything here is in the `RealtimeMesh::` namespace. The worked example is `FRealtimeMeshPlaneGenerator`, a procedural sine-height plane, in `Source/RealtimeMeshExt/Public/Factory/RealtimeMeshGeneratorPlane.h` with its implementation in the matching `Private/Factory/RealtimeMeshGeneratorPlane.cpp`. We will walk through it phase by phase.

## The two halves of a generator

A generator keeps *fixed configuration* separate from *per-mesh identity*:

* **Config** is set once per configured generator and shared by every mesh it drives: grid resolution, UV scale, material choices. It doubles as the editor surface, since a designer picks and edits it through an `FInstancedStruct` dropdown.
* **Descriptor** is the identity of one particular mesh, supplied at generation time: cell coordinates, LOD, seed. It travels *in the request* rather than sitting in a map inside the generator.

Because neither the generator nor its config holds mutable shared state, the generator is safe to run off the game thread. Keep it that way. Do not cache per-mesh state on the generator between calls.

## The base class and its phases

Every generator derives from `FRealtimeMeshGenerator`. You override only the parts you produce and the rest default to producing nothing. `GetStructure()` and `GetSectionGroup()` are pure virtual, because a generator that produces no geometry would be pointless. Bounds, collision, and Nanite are all optional.

> **A note on naming.** The generator interface still spells buffer sets as *section groups*, in `GetSectionGroup` and `FRealtimeMeshSectionGroupData`, even though the rest of the API has moved to `BufferSet`. They are the same thing: a set of shared vertex and index buffers.

```cpp
// Source/RealtimeMeshExt/Public/Factory/RealtimeMeshGenerator.h
class FRealtimeMeshGenerator : public TSharedFromThis<FRealtimeMeshGenerator>
{
public:
    // Which thread(s) the phases may run on. Async by default.
    virtual ERealtimeMeshThreadType AllowedThread() const { return ERealtimeMeshThreadType::AsyncThread; }

    // Work out shared data once per update, for the phases below to read. Optional.
    virtual void PreUpdate(const FRealtimeMeshGenerationRequest& Request,
                           FRealtimeMeshGenerationScratch& OutScratch) const {}

    // The LOD and buffer set layout for this mesh. (required)
    virtual FRealtimeMeshStructure GetStructure(const FRealtimeMeshGenerationRequest& Request,
                                                const FRealtimeMeshGenerationScratch& Scratch) const = 0;

    // Fill the streams and sections for one buffer set. Called once per out-of-date buffer set. (required)
    virtual void GetSectionGroup(const FRealtimeMeshGenerationRequest& Request,
                                 const FRealtimeMeshGenerationScratch& Scratch,
                                 const FRealtimeMeshBufferSetKey& SectionGroupKey,
                                 FRealtimeMeshSectionGroupData& OutData) const = 0;

    // Mesh bounds. Default: unset, meaning existing bounds are left alone.
    virtual TOptional<FBoxSphereBounds3f> GetBounds(const FRealtimeMeshGenerationRequest& Request,
                                                    const FRealtimeMeshGenerationScratch& Scratch) const;

    // Collision geometry. Default: none.
    virtual void GetCollision(const FRealtimeMeshGenerationRequest& Request,
                              const FRealtimeMeshGenerationScratch& Scratch,
                              FRealtimeMeshCollisionInfo& OutCollision) const {}

    // Nanite resources. Default: none.
    virtual FRealtimeMeshNaniteResourcesPtr GetNanite(const FRealtimeMeshGenerationRequest& Request,
                                                      const FRealtimeMeshGenerationScratch& Scratch) const { return nullptr; }
};
```

The orchestrator (`FRealtimeMeshConstructed::Construct`) only calls the phases whose dirty bit is set, so partial updates stay cheap. Regenerating just the bounds does not re-run geometry.

### What's in a request

```cpp
struct FRealtimeMeshGenerationRequest
{
    FInstancedStruct Descriptor;              // per-mesh identity (your descriptor struct)
    FRealtimeMeshDirtyFlags DirtyFlags;       // which parts to regenerate
    FRealtimeMeshCancellationToken CancellationToken;

    template <typename T> const T& Get() const;    // Request.Get<FMyDescriptor>()
    template <typename T> bool IsDescriptorOfType() const;
};
```

`Get<T>()` unwraps the descriptor into your concrete struct.

`PreUpdate()` is where you compute anything several phases need, for example a sampled density field, and stash it in the `FRealtimeMeshGenerationScratch`. Every phase then reads it from there, so a full update computes it once rather than once per phase.

## Threading

`AllowedThread()` decides where the phases run. The default, `ERealtimeMeshThreadType::AsyncThread`, means **all** of them (`PreUpdate`, `GetStructure`, `GetSectionGroup`, `GetBounds`, `GetCollision`, `GetNanite`) run on a worker thread.

That is why the phase methods are `const`, and why the generator must not hold mutable shared state. One instance serves many meshes at the same time.

If a phase genuinely has to touch game-thread-only state, override `AllowedThread()` to return `ERealtimeMeshThreadType::GameThread`. You give up all the concurrency, though, so prefer keeping everything async and passing what you need through the descriptor instead. The values are flags (`RenderThread`, `GameThread`, `AsyncThread`, `Any`) defined in `Core/RealtimeMeshFuture.h`.

Whatever `AllowedThread()` says, the final commit of generated data onto the mesh always happens on the game thread. The orchestrator handles that for you.

## The convenience base: TRealtimeMeshProvider

Most generators do not want to unwrap the config `FInstancedStruct` by hand. `TRealtimeMeshProvider<TConfigType>` stores a typed, value-copied config and gives you `GetConfig()`:

```cpp
template <typename TConfigType>
class TRealtimeMeshProvider : public FRealtimeMeshGenerator
{
public:
    using FConfig = TConfigType;
    explicit TRealtimeMeshProvider(const TConfigType& InConfig) : Config(InConfig) {}
    const TConfigType& GetConfig() const { return Config; }
protected:
    TConfigType Config;
};
```

The copy by value is exactly what makes the generator safe off the game thread, since there is no shared mutable reference back to a `UObject`.

Your config struct must derive from `FRealtimeMeshProviderConfig`, a `USTRUCT`. That is what marks it as a provider config and lets the editor filter its dropdown to valid choices.

## Worked example: the plane generator

### 1. The config and descriptor

From `RealtimeMeshGeneratorPlane.h`. The config is the editor surface. The descriptor is the per-chunk identity that the streaming layer, or a one-off caller, supplies.

```cpp
USTRUCT(BlueprintType)
struct FRealtimeMeshPlaneProviderConfig : public FRealtimeMeshProviderConfig
{
    GENERATED_BODY()

    // Number of quads per side of the grid.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh|Plane")
    FIntPoint GridSize = FIntPoint(128, 128);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh|Plane")
    FVector2f UVScale = FVector2f(1.0f / 1.28f, 1.0f / 1.28f);
};

USTRUCT(BlueprintType)
struct FRealtimeMeshPlaneCellDescriptor
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh|Plane")
    FIntPoint CellLocation = FIntPoint(0, 0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh|Plane")
    int32 LOD = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh|Plane")
    FVector2f CellSize = FVector2f(64.0f, 64.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh|Plane")
    float CellHeight = 256.0f;
};
```

Both have to be `USTRUCT`s. That is a hard requirement, because they travel as `FInstancedStruct` payloads across the worker thread boundary.

### 2. The generator class

```cpp
namespace RealtimeMesh
{
    class FRealtimeMeshPlaneGenerator : public TRealtimeMeshProvider<FRealtimeMeshPlaneProviderConfig>
    {
        static const FRealtimeMeshBufferSetKey MainSectionGroup;
        static const FRealtimeMeshSectionKey MainSection;

    public:
        using TRealtimeMeshProvider::TRealtimeMeshProvider;   // inherit the (const FConfig&) constructor

        virtual FRealtimeMeshStructure GetStructure(const FRealtimeMeshGenerationRequest& Request,
                                                    const FRealtimeMeshGenerationScratch& Scratch) const override;
        virtual void GetSectionGroup(const FRealtimeMeshGenerationRequest& Request,
                                     const FRealtimeMeshGenerationScratch& Scratch,
                                     const FRealtimeMeshBufferSetKey& SectionGroupKey,
                                     FRealtimeMeshSectionGroupData& OutData) const override;
        virtual TOptional<FBoxSphereBounds3f> GetBounds(const FRealtimeMeshGenerationRequest& Request,
                                                        const FRealtimeMeshGenerationScratch& Scratch) const override;
    };
}
```

It overrides only the three phases it needs. The plane keys everything to one buffer set in one LOD, declared as static members.

### 3. GetStructure, the layout

Return the LOD and buffer set skeleton. The plane uses a single LOD with a single buffer set:

```cpp
FRealtimeMeshStructure FRealtimeMeshPlaneGenerator::GetStructure(
    const FRealtimeMeshGenerationRequest& Request, const FRealtimeMeshGenerationScratch& Scratch) const
{
    FRealtimeMeshStructure Structure;
    Structure.LODs.SetNum(1);
    Structure.LODs[0].SectionGroups.Add(MainSectionGroup);
    return Structure;
}
```

### 4. GetSectionGroup, the geometry

This is where the mesh actually gets built, into `OutData`. It runs once per out-of-date buffer set.

You unwrap the descriptor with `Request.Get<T>()`, then build streams with the usual [stream builders](../../keyconcepts/). Here that means a `TRealtimeMeshBuilderLocal` writing straight into `OutData.Streams`:

```cpp
void FRealtimeMeshPlaneGenerator::GetSectionGroup(
    const FRealtimeMeshGenerationRequest& Request, const FRealtimeMeshGenerationScratch& Scratch,
    const FRealtimeMeshBufferSetKey& SectionGroupKey, FRealtimeMeshSectionGroupData& OutData) const
{
    const FRealtimeMeshPlaneCellDescriptor& Cell = Request.Get<FRealtimeMeshPlaneCellDescriptor>();
    const FIntPoint GridSize = Config.GridSize;                       // from the config
    const FVector2f Size = Cell.CellSize * FMath::Pow(2.0f, static_cast<float>(Cell.LOD));

    OutData.DrawType = ERealtimeMeshSectionDrawType::Static;
    OutData.bReplaceAllExistingStreams = true;
    OutData.bAutoCreatePolygroupSections = true;
    OutData.SectionConfigs.Add(MainSection, FRealtimeMeshSectionConfig());

    TRealtimeMeshBuilderLocal Builder(OutData.Streams);
    Builder.EnableTangents();
    Builder.EnablePolyGroups();
    Builder.EnableTexCoords();

    // ... sample a sine-height field over the (GridSize+1)^2 grid, adding a vertex per sample ...
    Builder.AddVertex(FVector3f(X * GridSpacing.X, Y * GridSpacing.Y, Height))
        .SetTexCoord(UVOffset + FVector2f(X, Y) * UVStepSize)
        .SetTangents(Tangent, Binormal, Normal);

    // ... then two triangles per quad ...
    Builder.AddTriangle(BaseIndex, BaseIndexNextRow + 1, BaseIndex + 1);
    Builder.AddTriangle(BaseIndex, BaseIndexNextRow, BaseIndexNextRow + 1);
}
```

Notice how the config and the descriptor divide the work. The config (`GridSize`, `UVScale`) sets the resolution, shared by every chunk. The descriptor (`CellLocation`, `LOD`, `CellSize`, `CellHeight`) places and sizes this one.

The `FRealtimeMeshSectionGroupData` fields (`DrawType`, `bReplaceAllExistingStreams`, `bAutoCreatePolygroupSections`, `SectionConfigs`) control how the streams you built get merged into the mesh.

### 5. GetBounds, cheap and separate

Bounds are their own phase so they can be recalculated without rebuilding any geometry. Return an unset `TOptional` to leave the existing bounds alone:

```cpp
TOptional<FBoxSphereBounds3f> FRealtimeMeshPlaneGenerator::GetBounds(
    const FRealtimeMeshGenerationRequest& Request, const FRealtimeMeshGenerationScratch& Scratch) const
{
    const FRealtimeMeshPlaneCellDescriptor& Cell = Request.Get<FRealtimeMeshPlaneCellDescriptor>();
    const FVector2f Size = Cell.CellSize * FMath::Pow(2.0f, static_cast<float>(Cell.LOD));
    const FVector3f Extent = FVector3f(Size.X, Size.Y, Cell.CellHeight);
    return FBoxSphereBounds3f(FBox3f(FVector3f::ZeroVector, Extent));
}
```

## Registering a generator

Registration is what connects a config struct type to the generator that handles it. Once registered, picking that config struct in the editor resolves to your generator with no extra wiring.

Add one line at file scope inside `namespace RealtimeMesh` in the generator's `.cpp`:

```cpp
// In RealtimeMeshGeneratorPlane.cpp
namespace RealtimeMesh
{
    RMC_REGISTER_PROVIDER(FRealtimeMeshPlaneGenerator, FRealtimeMeshPlaneProviderConfig)
    // ... phase implementations ...
}
```

`RMC_REGISTER_PROVIDER(GeneratorType, ConfigType)` sets up a static registration with `FRealtimeMeshProviderRegistry` once the object system is ready. Registration is deliberately deferred past static initialisation so that calling `StaticStruct()` is safe.

`GeneratorType` needs a constructor taking `const ConfigType&`, which `TRealtimeMeshProvider<ConfigType>` gives you for free.

To build a generator from a config at runtime, ask the registry:

```cpp
FInstancedStruct ProviderConfig = /* picked in the editor, or FInstancedStruct::Make(MyConfig) */;
FRealtimeMeshGeneratorPtr Generator =
    FRealtimeMeshProviderRegistry::Get().CreateGenerator(ProviderConfig);
if (!Generator.IsValid())
{
    // that config type wasn't registered
}
```

`CreateGenerator` returns null if the config type is not registered or the struct is invalid, so always check.

### Exposing the config in the editor

Declare an `FInstancedStruct` property with the base struct meta, so the type dropdown only offers registered provider configs:

```cpp
UPROPERTY(EditAnywhere, Category = "RealtimeMesh",
    meta = (BaseStruct = "/Script/RealtimeMeshExt.RealtimeMeshProviderConfig", ExcludeBaseStruct))
FInstancedStruct ProviderConfig;
```

## Using a generator from a Constructed mesh

`URealtimeMeshConstructed` is the mesh type that holds a generator plus a per-mesh descriptor and regenerates itself on demand. Bind the pair with `SetGenerator`, then call `Regenerate` with the parts you want rebuilt:

```cpp
URealtimeMeshConstructed* Mesh = Component->InitializeRealtimeMesh<URealtimeMeshConstructed>();

FRealtimeMeshPlaneCellDescriptor Descriptor;
Descriptor.CellLocation = FIntPoint(0, 0);
Descriptor.LOD = 0;
Descriptor.CellSize = FVector2f(64.0f, 64.0f);
Descriptor.CellHeight = 256.0f;

// Bind one shared generator plus this mesh's own descriptor.
Mesh->SetGenerator(Generator.ToSharedRef(), FInstancedStruct::Make(Descriptor));

// Generate. AllDirty() marks structure, Nanite, bounds, and optionally collision as needing work.
Mesh->Regenerate(RealtimeMesh::FRealtimeMeshDirtyFlags::AllDirty())
    .Next([](ERealtimeMeshProxyUpdateStatus Status) { /* done, on the game thread */ });
```

`Regenerate` returns a `TFuture<ERealtimeMeshProxyUpdateStatus>` that completes once the render proxy update lands.

To regenerate only part of the mesh later, say after the descriptor changes, build a narrower `FRealtimeMeshDirtyFlags` such as `MarkBoundsDirty()` or `MarkSectionGroupDirty(Key)` instead of `AllDirty()`. Only those phases then run. Change the descriptor first with `SetDescriptor(FInstancedStruct::Make(NewDescriptor))`.

For a complete running example across many meshes at once, see the [spatial streaming](../spatial-streaming/) page. Its example actor creates one `URealtimeMeshConstructed` per streamed chunk and drives them all from a single shared generator.
