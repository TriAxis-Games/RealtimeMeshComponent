---
title: Spatial Streaming
description: Load and unload mesh chunks around the player with the RealtimeMeshSpatial quadtree and LOD streaming state machine.
---

For worlds too big to keep in memory all at once, like procedural terrain or endless grids, you want to stream mesh *chunks* in and out around the player.

The **`RealtimeMeshSpatial`** module does exactly that. A world subsystem tracks where your streaming sources are, and a per-actor manager runs a quadtree and LOD state machine that fires four events as chunks load, become visible, hide, and unload.

You provide the geometry. The module decides *which* chunks should exist right now, and *when*.

It pairs naturally with [custom providers](../custom-providers/), where a single shared generator produces the geometry for every chunk and the per-chunk variation lives in the descriptor.

Add the module to your `Build.cs`:

```csharp
PrivateDependencyModuleNames.AddRange(new string[] {
    "RealtimeMeshComponent",
    "RealtimeMeshExt",       // generator plus URealtimeMeshConstructed
    "RealtimeMeshSpatial",   // streaming manager and subsystem
});
```

The worked example is `ARealtimeMeshSpatialStreamingActor`, in `Source/RealtimeMeshExamples/Public/RealtimeMeshSpatialStreamingActor.h` with its implementation in `Source/RealtimeMeshExamples/Private/RealtimeMeshSpatialStreamingActor.cpp`. It streams a plane field around the player using the plane generator, and we follow it throughout this page.

## The pieces

* **`URealtimeMeshSpatialStreamingSubsystem`** is a `UTickableWorldSubsystem`, one per world. It holds the registered streaming *sources* and *managers*, and ticks every manager each frame. You never create it, the engine does. It ticks in the editor and while paused, so streaming keeps working in PIE and in the editor viewport.

* **`URealtimeMeshSpatialStreamingManager`** is a `UObject` you create and own, usually on your streaming actor. It runs the quadtree and LOD traversal and calls your event handlers. You initialise it once and it registers itself with the subsystem.

* **Streaming sources** are the points streaming radiates out from, usually the player. The subsystem adds one automatically for the player Character in PIE. For a custom pawn you add a `URealtimeMeshSpatialStreamingSourceComponent`.

* **A structure provider** is your implementation of `IRealtimeMeshSpatialStreamingStructureProvider`. It defines the shape of the grid: where it sits, how coarse it can get, whether streaming is 2D or 3D, and which chunks are valid.

## How a chunk is identified

Every event is keyed by a chunk location, which is the unit of streaming:

```cpp
// Source/RealtimeMeshSpatial/Public/RealtimeMeshSpatialComponentLocation.h
struct FRealtimeMeshSpatialComponentLocation
{
    FInt64Vector Location;   // integer grid coordinates
    int32 LOD;               // quadtree level, 0 is finest
};
```

It hashes and compares on `(Location, LOD)` together, so it works directly as a `TMap` key. That is how the example keeps track of which component it made for each live chunk.

## The four lifecycle events

You drive everything through four delegates on `FRealtimeMeshSpatialStreamingStateInitParams`:

```cpp
// Source/RealtimeMeshSpatial/Public/RealtimeMeshSpatialStreamingState.h
struct FRealtimeMeshSpatialStreamingStateInitParams
{
    TSharedPtr<RealtimeMesh::IRealtimeMeshSpatialStreamingStructureProvider> ChunkProvider;

    TDelegate<void(const FRealtimeMeshSpatialComponentLocation&)> OnActivateCell;
    TDelegate<void(const FRealtimeMeshSpatialComponentLocation&)> OnDeactivateCell;
    TDelegate<TFuture<bool>(const FRealtimeMeshSpatialComponentLocation&,
                            RealtimeMesh::FRealtimeMeshCancellationToken)> OnLoadCell;
    TDelegate<void(const FRealtimeMeshSpatialComponentLocation&)> OnUnloadCell;
};
```

A chunk's life goes like this:

1. **`OnLoadCell`** fires when the chunk comes into streaming range. This is the only *async* event: it returns a `TFuture<bool>` so you can build the geometry off the game thread and report success or failure when you are done. It also gets a `FRealtimeMeshCancellationToken`, and if the chunk leaves range before your work finishes you should honour that token and abandon it. Create the mesh here, but keep it hidden.
2. **`OnActivateCell`** fires when the loaded chunk is close enough to be seen. Show it.
3. **`OnDeactivateCell`** fires when it moves out of the visible band but is still loaded. Hide it, do not destroy it, because it may come back cheaply.
4. **`OnUnloadCell`** fires when it leaves streaming range entirely. Destroy its resources.

Splitting load from activate is what lets the manager build geometry slightly outside the visible radius, so chunks pop into view already finished rather than appearing as they build.

## Defining the grid

The manager needs to know the shape of your world. Implement `IRealtimeMeshSpatialStreamingStructureProvider`:

```cpp
// Source/RealtimeMeshSpatial/Public/RealtimeMeshSpatialValidChunkProvider.h
class IRealtimeMeshSpatialStreamingStructureProvider
{
protected:
    int32 MaxLOD;                       // coarsest LOD the grid streams to
    uint32 bShouldUse3dStreaming : 1;   // false is a 2D quadtree, true is a 3D octree
public:
    int32 GetMaxLOD() const;
    bool ShouldUse3dStreaming() const;

    virtual bool IsCellValid(const FRealtimeMeshSpatialComponentLocation& CellLocation) const = 0;
    virtual FTransform GetGridTransform() const = 0;
};
```

Set `MaxLOD` and `bShouldUse3dStreaming` in your constructor. `IsCellValid` lets you carve holes or bound the world, by returning `false` for chunks that should not exist. `GetGridTransform` places and orients the grid in world space.

The example's provider is about as simple as it gets, an unbounded 2D grid anchored at the actor:

```cpp
class FExampleChunkProvider : public IRealtimeMeshSpatialStreamingStructureProvider
{
public:
    FExampleChunkProvider(int32 InMaxLOD, const FTransform& InGridTransform)
        : GridTransform(InGridTransform)
    {
        MaxLOD = InMaxLOD;
        bShouldUse3dStreaming = false;
    }
    virtual bool IsCellValid(const FRealtimeMeshSpatialComponentLocation& Cell) const override { return true; }
    virtual FTransform GetGridTransform() const override { return GridTransform; }
private:
    FTransform GridTransform;
};
```

> **Get your sizes to agree.** The manager's base chunk size (`64` units) and your generator's per-chunk size have to match, so that each generated patch exactly fills its streaming chunk. The example pins both to a shared `GBaseChunkSize = 64` constant. A chunk at LOD *n* covers `BaseChunkSize << n` units. If these drift apart you get visible seams or overlaps, and it is not obvious why.

## Wiring it up

The manager's control surface:

```cpp
// URealtimeMeshSpatialStreamingManager
void Initialize(const FRealtimeMeshSpatialStreamingStateInitParams& InitParams, bool bAutoStart = true);
void Start();
void Stop();
void Reset();
bool IsStreamingStable() const;   // nothing pending and nothing changed last frame
```

`Initialize` takes your parameters and, when `bAutoStart` is true, registers with the subsystem so it starts ticking immediately. `Start` and `Stop` toggle that registration, and `Reset` tears everything down.

`IsStreamingStable()` returns true once every pending load and unload has settled, which is useful for gating teleports, screenshots, or "world is ready" logic.

From the example's `BeginPlay`:

```cpp
void ARealtimeMeshSpatialStreamingActor::BeginPlay()
{
    Super::BeginPlay();

    // Resolve the shared generator from the config picked in the editor.
    Generator = FRealtimeMeshProviderRegistry::Get().CreateGenerator(ProviderConfig);
    if (!Generator.IsValid())
    {
        return;
    }

    ChunkProvider = MakeShared<FExampleChunkProvider>(MaxLOD, GetActorTransform());
    Manager = NewObject<URealtimeMeshSpatialStreamingManager>(this);

    FRealtimeMeshSpatialStreamingStateInitParams Init;
    Init.ChunkProvider = ChunkProvider;
    Init.OnLoadCell.BindUObject(this, &ARealtimeMeshSpatialStreamingActor::OnLoadCell);
    Init.OnActivateCell.BindUObject(this, &ARealtimeMeshSpatialStreamingActor::OnActivateCell);
    Init.OnDeactivateCell.BindUObject(this, &ARealtimeMeshSpatialStreamingActor::OnDeactivateCell);
    Init.OnUnloadCell.BindUObject(this, &ARealtimeMeshSpatialStreamingActor::OnUnloadCell);

    // bAutoStart registers with the streaming subsystem, which then ticks and drives us.
    Manager->Initialize(Init, /*bAutoStart*/ true);
}
```

And tearing down cleanly in `EndPlay`. Reset the manager first, then destroy anything still alive:

```cpp
void ARealtimeMeshSpatialStreamingActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Manager)
    {
        Manager->Reset();
        Manager = nullptr;
    }
    for (auto& Pair : CellComponents)
    {
        if (Pair.Value) { Pair.Value->DestroyComponent(); }
    }
    CellComponents.Empty();
    Super::EndPlay(EndPlayReason);
}
```

## Handling the events

### Load: build the mesh, return a future

`OnLoadCell` creates a `URealtimeMeshComponent`, positions it at the chunk corner, initialises a `URealtimeMeshConstructed`, binds the shared generator with a per-chunk descriptor, and kicks off `Regenerate`. It returns that future mapped to `bool`.

The component starts hidden. Activation is what makes it visible.

```cpp
TFuture<bool> ARealtimeMeshSpatialStreamingActor::OnLoadCell(
    const FRealtimeMeshSpatialComponentLocation& Cell, FRealtimeMeshCancellationToken CancellationToken)
{
    const int64 ChunkSize = GBaseChunkSize << Cell.LOD;
    const FVector Corner(double(Cell.Location.X) * ChunkSize, double(Cell.Location.Y) * ChunkSize, 0.0);

    URealtimeMeshComponent* Component = NewObject<URealtimeMeshComponent>(this);
    Component->RegisterComponent();
    Component->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    Component->SetRelativeLocation(Corner);
    Component->SetVisibility(false);              // shown later, in OnActivateCell
    CellComponents.Add(Cell, Component);

    URealtimeMeshConstructed* Mesh = Component->InitializeRealtimeMesh<URealtimeMeshConstructed>();
    if (!Mesh)
    {
        return MakeFulfilledPromise<bool>(false).GetFuture();
    }

    // Per-chunk descriptor. Same generator, different chunk.
    FRealtimeMeshPlaneCellDescriptor Descriptor;
    Descriptor.CellLocation = FIntPoint(int32(Cell.Location.X), int32(Cell.Location.Y));
    Descriptor.LOD = Cell.LOD;
    Descriptor.CellSize = FVector2f(float(GBaseChunkSize), float(GBaseChunkSize));
    Descriptor.CellHeight = CellHeight;

    Mesh->SetGenerator(Generator.ToSharedRef(), FInstancedStruct::Make(Descriptor));

    return Mesh->Regenerate(FRealtimeMeshDirtyFlags::AllDirty())
        .Next([](ERealtimeMeshProxyUpdateStatus) { return true; });
}
```

Note the shape of this: **one generator, many descriptors**. Every chunk shares the generator resolved back in `BeginPlay`, and only the descriptor differs. That is what makes streaming hundreds of chunks at once cheap.

### Activate and deactivate: just visibility

```cpp
void ARealtimeMeshSpatialStreamingActor::OnActivateCell(const FRealtimeMeshSpatialComponentLocation& Cell)
{
    if (URealtimeMeshComponent* Component = CellComponents.FindRef(Cell))
    {
        Component->SetVisibility(true, true);
    }
}

void ARealtimeMeshSpatialStreamingActor::OnDeactivateCell(const FRealtimeMeshSpatialComponentLocation& Cell)
{
    if (URealtimeMeshComponent* Component = CellComponents.FindRef(Cell))
    {
        Component->SetVisibility(false, true);
    }
}
```

### Unload: destroy

```cpp
void ARealtimeMeshSpatialStreamingActor::OnUnloadCell(const FRealtimeMeshSpatialComponentLocation& Cell)
{
    if (URealtimeMeshComponent* Component = CellComponents.FindRef(Cell))
    {
        Component->DestroyComponent();
        CellComponents.Remove(Cell);
    }
}
```

## Streaming sources

Streaming radiates out from *sources*. In PIE the subsystem adds one for the player Character automatically, so the plane field simply follows the player with no setup at all.

If your pawn is **not** a Character, attach a `URealtimeMeshSpatialStreamingSourceComponent` to it:

```cpp
// Source/RealtimeMeshSpatial/Public/RealtimeMeshSpatialStreamingSourceComponent.h
UCLASS(Meta = (BlueprintSpawnableComponent))
class URealtimeMeshSpatialStreamingSourceComponent : public UActorComponent,
                                                     public IRealtimeMeshSpatialStreamingSourceProvider
{
public:
    UFUNCTION(BlueprintCallable, Category = "Streaming") void EnableStreamingSource();
    UFUNCTION(BlueprintCallable, Category = "Streaming") void DisableStreamingSource();
    UFUNCTION(BlueprintPure,     Category = "Streaming") bool IsStreamingSourceEnabled() const;

    // True once every manager in this world has settled its loads and unloads.
    UFUNCTION(BlueprintCallable, Category = "Streaming") bool IsStreamingStable() const;
};
```

The component registers itself with the subsystem in `OnRegister` and reports the pawn's position as a streaming source each frame. Everything on it is Blueprint callable, including `IsStreamingStable()`, which you can use to hold gameplay back until the world around the player has finished building.

Under the hood a source carries some tuning knobs, in `FRealtimeMeshSpatialStreamingSource`: `MaxVisibilityDistance`, `LODFalloff`, `LODVisibilityThreshold`, and `StreamingDistanceMultiplier` control how far chunks stream and how aggressively detail drops off with distance.
