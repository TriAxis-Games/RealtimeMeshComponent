---
title: Buffer Sets & Sections
description: Creating and configuring buffer sets and sections. Keys, draw types, stream ranges, automatic sections per poly group, and visibility.
---

Inside each LOD, geometry lives in two levels. **Buffer sets** own the vertex and index data. **Sections** draw a slice of that data with a particular material and set of render settings.

This page covers how you address them, how you configure them, and the automatic section behaviour that most meshes rely on. For where they sit in the bigger picture, see [Structure](../structure/).

Everything here is shown against `URealtimeMeshSimple`, but the same keys and config types work on the other Managed meshes.

## Keys: how you refer to things

Buffer sets and sections are addressed by value-type keys rather than pointers. Keys are stable, cheap to copy, and safe to hold on to, which pointers into a live mesh would not be. Every key is scoped to a LOD.

`FRealtimeMeshBufferSetKey` identifies a buffer set:

```cpp
const FRealtimeMeshLODKey LODKey(0);

// By name. This is the form used throughout the examples.
FRealtimeMeshBufferSetKey GroupA = FRealtimeMeshBufferSetKey::Create(LODKey, FName("Floor"));

// By explicit slot index, much like a static mesh material slot.
FRealtimeMeshBufferSetKey GroupB = FRealtimeMeshBufferSetKey::Create(LODKey, 0, FName("Terrain"));

// A guaranteed-unique one, when you don't care what it's called.
FRealtimeMeshBufferSetKey GroupC = FRealtimeMeshBufferSetKey::CreateUnique(LODKey);
```

A key's identity is the LOD index and slot index together. The name is friendly metadata and does **not** affect equality. The name-based version works out a stable slot index from the name, so both spellings can address the same buffer set.

`FRealtimeMeshSectionKey` identifies a section inside a buffer set:

```cpp
// The key for the automatically created section of poly group N. You'll use this one most.
FRealtimeMeshSectionKey PolyGroup0 = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);

// Or address a section by name, or ask for a unique one.
FRealtimeMeshSectionKey Named  = FRealtimeMeshSectionKey::Create(GroupKey, FName("Visible"));
FRealtimeMeshSectionKey Unique = FRealtimeMeshSectionKey::CreateUnique(GroupKey);
```

`CreateForPolyGroup` is the important one, because automatically created sections use exactly these keys. That is how you reach in afterwards and reconfigure them.

## Creating a buffer set

On `URealtimeMeshSimple` you create a buffer set and hand it geometry as a `FRealtimeMeshStreamSet`:

```cpp
FRealtimeMeshStreamSet StreamSet;
// ...build the mesh into StreamSet with a builder (see Key Concepts)...

const FRealtimeMeshBufferSetKey GroupKey =
    FRealtimeMeshBufferSetKey::Create(FRealtimeMeshLODKey(0), FName("Box"));

RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);
```

`CreateBufferSet` has versions that take the stream set by const reference or by rvalue (use `MoveTemp(StreamSet)` when you are finished with it), plus a config argument and a `bShouldAutoCreateSectionsForPolyGroups` flag that defaults to `true`. See `Public/RealtimeMeshSimple.h`.

A buffer set used to be called a *section group*, so the deprecated `CreateSectionGroup` and `UpdateSectionGroup` names still work if you are updating older code. See the [Migration Notes](../../migration/).

> For building the `StreamSet` in the first place, see [Key Concepts](../../keyconcepts/). This page assumes you already have one.

## Buffer set config

`FRealtimeMeshBufferSetConfig` (in `Public/Core/RealtimeMeshBufferSetConfig.h`) controls how the buffers are allocated and updated:

```cpp
struct FRealtimeMeshBufferSetConfig
{
    ERealtimeMeshSectionDrawType DrawType;  // Static (default) or Dynamic
    bool bComputeWritable;                  // default false
};
```

**DrawType is the one that matters.**

- **`Static`** is cheaper to render, but changing the geometry means rebuilding the render proxy for every component using this mesh. Right for geometry that rarely or never changes after you build it.
- **`Dynamic`** costs slightly more per frame, but the buffers can be updated in place without that rebuild. Choose this for anything you animate or edit often. It is also what makes the in-place fast paths available, covered in [Updating Mesh Data](../updating-mesh-data/).

If you are not sure, start with Static. Switch to Dynamic when you find yourself changing the mesh every frame.

**`bComputeWritable`** allocates the GPU buffers so a compute shader can write straight into them. Since the geometry is then changed on the GPU rather than re-uploaded from the CPU, it behaves like Dynamic in that no proxy rebuild is needed. It is independent of `DrawType`.

To make a buffer set updatable in place, pass a Dynamic config when you create it:

```cpp
RealtimeMesh->CreateBufferSet(
    GroupKey, StreamSet,
    FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType::Dynamic));
```

See `ARealtimeMeshExample_Simple_FastUpdate` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_FastUpdate.cpp`).

## Sections created for you

By default (`bShouldAutoCreateSectionsForPolyGroups == true`) you do not create sections by hand at all.

When you create or update a buffer set, RMC looks at the poly group stream in your `StreamSet` and makes one section per group, keyed with `FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, PolyGroupIndex)`. If there is no poly group stream, you get a single section covering everything.

Each of those sections starts with its material slot set to its own poly group number. To point poly group 0 at material slot 0 instead, reconfigure it:

```cpp
const FRealtimeMeshBufferSetKey GroupKey =
    FRealtimeMeshBufferSetKey::Create(0, FName("Box"));
RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);

// Reach the auto-created section for poly group 0 and set its config.
RealtimeMesh->UpdateSectionConfig(
    FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0),
    FRealtimeMeshSectionConfig(0));   // material slot 0
```

`ARealtimeMeshExample_Simple_LODs` and `ARealtimeMeshExample_Simple_Collision` both do exactly this.

You can turn the behaviour off per buffer set:

```cpp
RealtimeMesh->SetShouldAutoCreateSectionsForPolyGroups(GroupKey, false);
bool bAuto = RealtimeMesh->ShouldAutoCreateSectionsForPolygonGroups(GroupKey);
```

With it off, you create sections yourself using the API below.

## Section config

`FRealtimeMeshSectionConfig` (in `Public/Core/RealtimeMeshSectionConfig.h`) is the per-section render configuration:

```cpp
struct FRealtimeMeshSectionConfig
{
    int32 MaterialSlot;          // which material slot to draw with (default 0)
    bool  bIsVisible;            // default true
    bool  bCastsShadow;          // default true
    bool  bIsMainPassRenderable; // advanced, default true
    bool  bForceOpaque;          // advanced, default false
};
```

`MaterialSlot` picks which of the mesh's material slots this section renders with. See [Materials](../materials/). The constructor takes the slot index, so `FRealtimeMeshSectionConfig(2)` gives you a visible, shadow casting section on slot 2.

### Creating and updating sections by hand

On any Managed mesh (Simple, Procedural, or Dynamic) you can create a section explicitly against a stream range and config:

```cpp
RealtimeMesh->CreateSection(
    SectionKey,
    FRealtimeMeshSectionConfig(0),
    StreamRange,
    /*bShouldCreateCollision=*/ false);

RealtimeMesh->UpdateSectionConfig(SectionKey, FRealtimeMeshSectionConfig(1), /*bShouldCreateCollision=*/ false);
RealtimeMesh->UpdateSectionRange(SectionKey, NewStreamRange);
```

`bShouldCreateCollision` marks this section's triangles for inclusion in per-triangle collision. See [Collision](../collision/).

### Stream ranges

A section draws part of its buffer set, described by `FRealtimeMeshStreamRange`: a `Vertices` range and an `Indices` range, both `FInt32Range`.

Sections created for you get the full range. You only set one explicitly when you want a section to draw a *subset*, such as a shadow-only section over part of the index buffer. `UpdateSectionRange` changes it later.

## Visibility and shadows

You can flip visibility and shadow casting without rebuilding the whole config. These return a `TFuture<ERealtimeMeshProxyUpdateStatus>`, and there are Blueprint versions that take a completion callback instead:

```cpp
RealtimeMesh->SetSectionVisibility(SectionKey, false);
RealtimeMesh->SetSectionCastShadow(SectionKey, true);

bool bVisible = RealtimeMesh->IsSectionVisible(SectionKey);
bool bShadow  = RealtimeMesh->IsSectionCastingShadow(SectionKey);
FRealtimeMeshSectionConfig Config = RealtimeMesh->GetSectionConfig(SectionKey);
```

[Updating Mesh Data](../updating-mesh-data/) explains what that `TFuture` is for.

## Listing and removing

```cpp
TArray<FRealtimeMeshBufferSetKey> BufferSets = RealtimeMesh->GetBufferSets(FRealtimeMeshLODKey(0));
TArray<FRealtimeMeshSectionKey>   Sections   = RealtimeMesh->GetSectionsInBufferSet(GroupKey);

RealtimeMesh->RemoveSection(SectionKey);     // returns a TFuture, callback version also available
RealtimeMesh->RemoveBufferSet(GroupKey);     // removes the buffer set and its streams
```

`GetBufferSets` is how the dynamic update example decides whether to create a buffer set or update the one that is already there. See `ARealtimeMeshExample_Simple_DynamicUpdate` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_DynamicUpdate.cpp`).
