---
title: Levels of Detail
description: Adding and configuring LODs on a Realtime Mesh. Screen sizes, visibility, per-LOD buffer sets, and removing LODs.
---

Every Realtime Mesh has at least one LOD, LOD 0. You can add more so the renderer swaps to cheaper geometry as the mesh gets smaller on screen.

LODs are completely independent of each other. Each holds its own buffer sets and sections, so a detailed LOD can be organised entirely differently from a distant one.

This page covers the LOD API on `URealtimeMesh`. For where LODs sit in the object model, see [Structure](../structure/).

The example throughout is `ARealtimeMeshExample_Simple_LODs` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_LODs.cpp`), which builds four LODs as differently rotated and coloured boxes so you can actually see the switches happen.

## LOD config

`FRealtimeMeshLODConfig` (in `Public/Core/RealtimeMeshLODConfig.h`) is small:

```cpp
struct FRealtimeMeshLODConfig
{
    bool  bIsVisible;   // default true
    float ScreenSize;   // default 0.0
};
```

- **`ScreenSize`** is how much of the screen the mesh has to fill for this LOD to be used, assuming no more detailed LOD is already active. It is a fraction of the screen, so higher values keep a LOD active when the object is bigger, and each successive LOD wants a smaller number than the one before it.
- **`bIsVisible`** turns the whole LOD off.

The constructor takes the screen size, so `FRealtimeMeshLODConfig(0.5f)` is all you usually need.

## Adding and configuring LODs

LOD 0 always exists. Tighten its screen size so the lower LODs get a chance to show, then add the rest with progressively smaller values:

```cpp
URealtimeMeshSimple* RealtimeMesh =
    GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");

// LOD 0 already exists, so reconfigure rather than add.
RealtimeMesh->UpdateLODConfig(0, FRealtimeMeshLODConfig(0.75f));

for (int32 LODIndex = 0; LODIndex < 4; LODIndex++)
{
    // Add LODs 1 to 3 with shrinking screen sizes: 0.5, 0.25, 0.125.
    if (LODIndex > 0)
    {
        RealtimeMesh->AddLOD(FRealtimeMeshLODConfig(FMath::Pow(0.5f, LODIndex)));
    }

    // ...build this LOD's geometry (below)...
}
```

The calls you need, all Blueprint callable (see `Public/RealtimeMesh.h`):

- `FRealtimeMeshLODKey AddLOD(const FRealtimeMeshLODConfig& Config)` appends a LOD and returns its key.
- `void UpdateLODConfig(FRealtimeMeshLODKey LODKey, const FRealtimeMeshLODConfig& Config)` changes an existing one. This is how you configure LOD 0, since you cannot add it.
- `void RemoveTrailingLOD()` removes the last LOD. Call it repeatedly to shorten the chain. There is no remove-by-index, because LODs have to stay contiguous from 0.
- `TArray<FRealtimeMeshLODKey> GetLODs()` lists what you currently have.

## Geometry per LOD

Buffer sets are keyed by LOD, so you fill each LOD by creating buffer sets against that LOD's key. In the example, each LOD gets its own box:

```cpp
// Buffer sets are keyed per LOD.
const FRealtimeMeshBufferSetKey GroupKey =
    FRealtimeMeshBufferSetKey::Create(LODIndex, FName("Box"));

FRealtimeMeshStreamSet StreamSet;
URealtimeMeshBasicShapeTools::AppendBoxMesh(
    StreamSet, FVector3f(100.0f, 100.0f, 100.0f), Transforms[LODIndex], 0, Colors[LODIndex]);

RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);
RealtimeMesh->UpdateSectionConfig(
    FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0),
    FRealtimeMeshSectionConfig(0));
```

Because each LOD's buffer sets are independent, nothing forces them to match. A detailed LOD might use several buffer sets and sections while a cheap one collapses to a single section. `GetBufferSets(LODKey)` lists what a given LOD has.

See [Buffer Sets & Sections](../sections/) for that API and [Materials](../materials/) for slot setup.

> Choosing and blending LODs at draw time is a rendering concern, covered in [Rendering](../../rendering/). This page is only about authoring the data.
