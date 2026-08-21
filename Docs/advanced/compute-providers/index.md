---
title: Compute Providers
description: Drive a section's geometry from a GPU compute shader, using compute-writable buffer sets and IRealtimeMeshComputeProvider.
---

Sometimes you want geometry that changes *every frame*, like a rippling ocean, a GPU particle mesh, or a marching cubes surface, and you never want the game thread involved at all.

RMC's compute path lets you allocate a buffer set's GPU buffers so a compute shader can write to them, then hand them to a shader that rewrites them inside the frame's render graph. The data never leaves the GPU.

**This is a C++ only, render thread workflow.** There is no Blueprint surface. You will be writing RDG (Render Dependency Graph) passes, so you need `RenderCore` and `RHI` as dependencies. If you are not comfortable with render thread code, the [in-place update paths](../../component-core/updating-mesh-data/) get you a long way with far less machinery.

Add what you need to your `Build.cs`:

```csharp
PrivateDependencyModuleNames.AddRange(new string[] {
    "RealtimeMeshComponent",   // compute registration API and compute-writable buffer sets
    "RenderCore",              // RDG, ENQUEUE_RENDER_COMMAND
    "RHI",
    "RealtimeMeshCompute",     // the shipped providers, optional if you write your own
});
```

The registration API and `FRealtimeMeshComputeContext` live in the **core** `RealtimeMeshComponent` module, in `Compute/RealtimeMeshComputeProvider.h`. You only need `RealtimeMeshCompute` for the ready-made provider implementations. Writing your own only needs the core interface plus `RenderCore` and `RHI`.

Two shipped examples ground this page:

* `ARealtimeMeshExample_Simple_ComputeWave` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_ComputeWave.cpp`) is a per-frame wave that rewrites a grid's positions on the GPU.
* `ARealtimeMeshExample_Simple_ComputeIndirect` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_ComputeIndirect.cpp`) uses a GPU buffer to control how much of a mesh gets drawn.

## Step 1: create a compute-writable buffer set

The buffers a compute pass writes to have to be allocated for it up front. You ask for that with the `bComputeWritable` flag on `FRealtimeMeshBufferSetConfig`:

```cpp
// Source/RealtimeMeshComponent/Public/Core/RealtimeMeshBufferSetConfig.h
struct FRealtimeMeshBufferSetConfig
{
    ERealtimeMeshSectionDrawType DrawType;
    bool bComputeWritable;   // allocate GPU buffers with UAV support, for compute writes

    FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType InDrawType = ERealtimeMeshSectionDrawType::Static,
                                    bool bInComputeWritable = false);
};
```

`bComputeWritable` is independent of `DrawType`. For per-frame deformation you want **`Dynamic`**, so the section renders through the per-frame path, which also rebuilds the motion vector uniform each frame.

The wave example builds its grid, then creates the buffer set compute-writable and dynamic:

```cpp
// ARealtimeMeshExample_Simple_ComputeWave::OnConstruction
FRealtimeMeshStreamSet StreamSet;
TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
Builder.EnableTangents();
Builder.EnableTexCoords();
// ... add N*N vertices (order must match the shader's index: i = GY*N + GX) and the grid triangles ...

// Compute-writable and Dynamic: Position and Tangents get buffers the provider writes each frame.
const FRealtimeMeshBufferSetConfig Config(ERealtimeMeshSectionDrawType::Dynamic, /*bComputeWritable*/ true);
RealtimeMesh->CreateBufferSet(GetComputeWaveGroupKey(), StreamSet, Config);
```

The CPU-side streams you upload are the *initial* state, and they set the buffer sizes. The compute pass overwrites them on the GPU from there.

One thing to get right: the vertex ordering you build with has to match how your shader indexes vertices. Nothing checks this for you.

> Static geometry can be compute-writable too. The indirect example uses `FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType::Static, /*bComputeWritable*/ true)`, allocating the box with compute write support even though its indirect draw does not strictly need it, so that it exercises the same allocation path.

## Step 2: write a provider

A provider records the RDG passes that write the buffers. There is one method to implement:

```cpp
// Source/RealtimeMeshComponent/Public/Compute/RealtimeMeshComputeProvider.h
class IRealtimeMeshComputeProvider
{
public:
    virtual void BuildComputePasses(FRDGBuilder& GraphBuilder,
                                    const FRealtimeMeshComputeContext& Context) = 0;
};
```

It is called on the render thread, inside the frame's RDG graph. The `Context` is your window onto the section's buffers:

```cpp
class FRealtimeMeshComputeContext
{
public:
    FRDGBuilder& GetGraphBuilder() const;
    const FRealtimeMeshComputeFrameInfo& GetFrame() const;   // time, delta, frame counter

    int32 GetVertexCapacity() const;   // buffers are sized to these, you may write fewer
    int32 GetIndexCapacity() const;

    // The imported RDG buffer for a compute-writable stream, or nullptr if absent or not writable.
    FRDGBufferRef  GetStreamBuffer(const FRealtimeMeshStreamKey& Key) const;
    FRDGBufferUAVRef GetStreamUAV(const FRealtimeMeshStreamKey& Key, EPixelFormat Format) const;
    FRDGBufferSRVRef GetStreamSRV(const FRealtimeMeshStreamKey& Key, EPixelFormat Format) const;

    // Only valid when the registration set bUseIndirect.
    FRDGBufferRef    GetIndirectArgsBuffer() const;
    FRDGBufferUAVRef GetIndirectArgsUAV() const;
};
```

You ask for a stream by key (`FRealtimeMeshStreams::Position`, `::Tangents`, `::Triangles`, and so on) in whatever pixel format matches how you write it, then bind that into your shader parameters.

The driver has already imported these buffers into the graph, and it transitions them back to the states the base pass expects once your passes have run. All you do is record the compute.

The shipped `FRealtimeMeshWaveProvider` (`Source/RealtimeMeshCompute/Public/Providers/RealtimeMeshWaveProvider.h`) is a complete reference:

```cpp
class FRealtimeMeshWaveProvider : public IRealtimeMeshComputeProvider
{
public:
    int32 GridSize = 64;     // NxN vertices, must match the created mesh
    float Extent = 100.0f;   // half-size of the plane in local units
    float Amplitude = 20.0f;
    float Frequency = 0.05f;
    float Speed = 2.0f;

    virtual void BuildComputePasses(FRDGBuilder& GraphBuilder,
                                    const FRealtimeMeshComputeContext& Context) override;
};
```

Inside `BuildComputePasses` it grabs the Position buffer, optionally the Tangents buffer, checks the capacity, and dispatches a compute shader:

```cpp
void FRealtimeMeshWaveProvider::BuildComputePasses(FRDGBuilder& GraphBuilder, const FRealtimeMeshComputeContext& Context)
{
    FRDGBufferUAVRef PositionsUAV = Context.GetStreamUAV(FRealtimeMeshStreams::Position, PF_R32_FLOAT);
    if (!PositionsUAV) { return; }

    FRDGBufferUAVRef TangentsUAV = Context.GetStreamUAV(FRealtimeMeshStreams::Tangents, PF_R8G8B8A8_SNORM);

    const uint32 Total = uint32(GridSize) * uint32(GridSize);
    if (Context.GetVertexCapacity() < int32(Total)) { return; }   // never write past the buffer

    // ... fill FParameters (buffers plus GridSize/Extent/Amplitude/Frequency and the frame time), then:
    // FComputeShaderUtils::AddPass(GraphBuilder, ..., FIntVector(groups, 1, 1));
}
```

Two habits worth copying from it. It **bails out cleanly** if the stream it wants is not there, or if the section is smaller than the grid, since either mismatch would otherwise write out of bounds or leave stale data on screen. And it reads `Context.GetFrame().TimeSeconds` to animate, rather than keeping its own clock.

## Step 3: register it

Build a `FRealtimeMeshComputeRegistration` on the game thread and register it. The registration is where you bind the provider to a buffer set and decide when it runs:

```cpp
// Source/RealtimeMeshComponent/Public/Compute/RealtimeMeshComputeProvider.h
struct FRealtimeMeshComputeRegistration
{
    FRealtimeMeshWeakPtr Mesh;                 // URealtimeMesh::GetMesh(), the data object
    FRealtimeMeshBufferSetKey BufferSetKey;    // the compute-writable buffer set you're writing
    FRealtimeMeshSectionKey SectionKey;        // section to bind indirect args to (bUseIndirect only)

    FRealtimeMeshComputeProviderPtr Provider;

    ERealtimeMeshComputeTrigger Trigger = ERealtimeMeshComputeTrigger::OnDemand;
    float IntervalSeconds = 0.0f;              // for the TimeInterval trigger

    bool bUseIndirect = false;                 // allocate and bind an indirect args buffer
    uint32 IndirectArgsSizeBytes = 5 * sizeof(uint32);
};
```

The **trigger** sets the cadence:

* `ERealtimeMeshComputeTrigger::EveryFrame` runs every rendered frame. Use it for continuous animation.
* `ERealtimeMeshComputeTrigger::OnDemand` runs once when registered, then again each time you mark it dirty.
* `ERealtimeMeshComputeTrigger::TimeInterval` runs every `IntervalSeconds`.

The wave example registers for every frame, in `BeginPlay`:

```cpp
TSharedRef<FRealtimeMeshWaveProvider, ESPMode::ThreadSafe> Provider =
    MakeShared<FRealtimeMeshWaveProvider, ESPMode::ThreadSafe>();
Provider->GridSize = N;
Provider->Extent = Extent;
Provider->Amplitude = Amplitude;
Provider->Frequency = Frequency;
Provider->Speed = Speed;

FRealtimeMeshComputeRegistration Registration;
Registration.Mesh = RealtimeMesh->GetMesh();           // the FRealtimeMesh data object
Registration.BufferSetKey = GetComputeWaveGroupKey();  // the same key used in CreateBufferSet
Registration.Provider = Provider;
Registration.Trigger = ERealtimeMeshComputeTrigger::EveryFrame;
Registration.bUseIndirect = false;

ComputeHandle = RegisterRealtimeMeshComputeProvider(Registration);   // uint64 handle, 0 means failure
```

The three registration functions are free functions in the `RealtimeMesh::` namespace, and all three are safe to call from the game thread since they queue render commands internally:

```cpp
uint64 RegisterRealtimeMeshComputeProvider(const FRealtimeMeshComputeRegistration& Registration);
void   UnregisterRealtimeMeshComputeProvider(uint64 Handle);
void   MarkRealtimeMeshComputeProviderDirty(uint64 Handle);   // ask an OnDemand provider to run once
```

**Always store the handle and unregister in `EndPlay`:**

```cpp
void ARealtimeMeshExample_Simple_ComputeWave::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ComputeHandle != 0)
    {
        UnregisterRealtimeMeshComputeProvider(ComputeHandle);
        ComputeHandle = 0;
    }
    Super::EndPlay(EndPlayReason);
}
```

Use `MarkRealtimeMeshComputeProviderDirty(Handle)` with an `OnDemand` provider whenever its inputs change, for example after rebuilding an SDF volume. That triggers a single run without paying for a dispatch every frame.

## Motion vectors, for clean deformation

A surface that moves every frame ghosts and smears under TSR or TAA, because the renderer does not know where each vertex was last frame.

RMC wires this up for you **if the section has a `PositionPrev` stream**. Its presence turns on per-vertex velocity output, and the compute driver keeps it updated.

Seed it when you create the mesh, as a copy of `Position`:

```cpp
// ARealtimeMeshExample_Simple_ComputeWave::OnConstruction (behind bEnableMotionVectors)
if (const FRealtimeMeshStream* PositionStream = StreamSet.Find(FRealtimeMeshStreams::Position))
{
    FRealtimeMeshStream PrevStream(FRealtimeMeshStreams::PositionPrev, PositionStream->GetLayout());
    PrevStream.Append(*PositionStream);
    StreamSet.AddStream(MoveTemp(PrevStream));
}
```

`FRealtimeMeshComputeFrameInfo` carries a `FrameCounter` that has to match the view's frame counter for the previous-position path to engage. The driver handles that plumbing. All you have to do is include the `PositionPrev` stream and use the `Dynamic` draw type so the per-frame path runs.

Toggling the stream off is a quick way to see exactly how much ghosting you get without it.

## Indirect draws

An *indirect* draw reads its parameters (index count, instance count, offsets) from a GPU buffer rather than from the CPU. That means a compute pass can decide how much to draw. There are two ways in.

**Through a compute provider.** Set `bUseIndirect = true` on the registration. The driver then allocates an indirect args buffer, binds it to `SectionKey` so that section renders indirect, and exposes it to your provider through `Context.GetIndirectArgsBuffer()` and `GetIndirectArgsUAV()`. Your compute pass fills in the args. Size it with `IndirectArgsSizeBytes`, which defaults to `5 * sizeof(uint32)` to match `FRHIDrawIndexedIndirectParameters`.

**Directly on the section.** For static control without a per-frame provider, set the indirect args on the section proxy yourself. `ARealtimeMeshExample_Simple_ComputeIndirect` does this through a proxy update task, building an args buffer laid out as `IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation`:

```cpp
// Inside a FRealtimeMeshProxyUpdateBuilder section task (render thread):
TResourceArray<uint32> Init;
Init.Add(NumIndices);  // IndexCountPerInstance
Init.Add(1);           // InstanceCount
Init.Add(0);           // StartIndexLocation
Init.Add(0);           // BaseVertexLocation
Init.Add(0);           // StartInstanceLocation

FRHIBufferCreateDesc Desc = FRHIBufferCreateDesc::Create(
        TEXT("RMC_ExampleIndirectArgs"), Init.GetResourceDataSize(), sizeof(uint32),
        EBufferUsageFlags::Static | EBufferUsageFlags::DrawIndirect | EBufferUsageFlags::UnorderedAccess
        | EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer)
    .SetInitialState(ERHIAccess::IndirectArgs)
    .SetInitActionResourceArray(&Init);

FBufferRHIRef Args = RHICmdList.CreateBuffer(Desc);
Section.SetIndirectArgs(Args, 0);
// Section.ClearIndirectArgs() goes back to a normal CPU-counted draw.
```

The example toggles between the indirect buffer and the equivalent CPU stream range limit (`Section.UpdateStreamRange(...)`), so you can check both produce identical output.

Use `EBufferUsageFlags::DrawIndirect` combined with the other flags exactly as shown. A buffer created with only `DrawIndirect` has no defined alignment on some backends and will assert.

## Headless runs

The compute path executes on the GPU inside RDG. Headless runs with `-NullRHI` do not execute compute or draw passes at all, so this workflow needs a real rendering context, meaning PIE or a running game, before you will see anything.
