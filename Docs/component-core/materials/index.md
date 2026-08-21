---
title: Materials
description: Material slots on a Realtime Mesh, how a section picks one, and how component level overrides fit in.
---

Materials on a Realtime Mesh work in two layers.

The `URealtimeMesh` object owns a list of **material slots**, each a name plus a material. Every section references one slot by its index. On top of that, because `URealtimeMeshComponent` is a `UMeshComponent`, the standard per-component material override system still applies.

This page covers both layers and how they resolve.

## Material slots on the mesh

A material slot is an `FRealtimeMeshMaterialSlot`: a `SlotName` and a `Material`. Sections do not hold a material directly. They store the *index* of the slot they draw with, in `FRealtimeMeshSectionConfig::MaterialSlot`.

Setting up slots:

```cpp
URealtimeMeshSimple* RealtimeMesh =
    GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

// Slot index 0, named "PrimaryMaterial", with an optional material asset.
RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial", SomeMaterialInterface);
```

`SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial = nullptr)` creates or replaces the slot at that index. The material is optional. The examples routinely register a named slot with no material at all, using `SetupMaterialSlot(0, "DefaultMaterial")`, and let it get assigned in the details panel later.

To size the list directly:

```cpp
RealtimeMesh->SetNumMaterialSlots(3);
```

Growing adds empty, unnamed slots. Shrinking drops the trailing ones and forgets their names.

One deliberate design choice worth knowing: if you shrink the list below a slot index that a live section still points at, that section falls back to the engine's default material rather than erroring. Things look wrong, but nothing breaks.

### Querying slots

The full slot API is available, and all of it is Blueprint callable (see `Public/RealtimeMesh.h`):

```cpp
int32                             Count  = RealtimeMesh->GetNumMaterials();
int32                             Index  = RealtimeMesh->GetMaterialIndex(FName("PrimaryMaterial")); // INDEX_NONE if missing
FName                             Name   = RealtimeMesh->GetMaterialSlotName(0);
bool                              bValid = RealtimeMesh->IsMaterialSlotNameValid(FName("PrimaryMaterial"));
FRealtimeMeshMaterialSlot         Slot   = RealtimeMesh->GetMaterialSlot(0);
UMaterialInterface*               Mat    = RealtimeMesh->GetMaterial(0); // nullptr if the index is invalid
TArray<FName>                     Names  = RealtimeMesh->GetMaterialSlotNames();
TArray<FRealtimeMeshMaterialSlot> Slots  = RealtimeMesh->GetMaterialSlots();
```

`GetMaterialIndex` is what lets you work with names instead of raw numbers. Resolve the name to an index once, then use that index in your section configs.

## How a section picks its material

A section renders with `FRealtimeMeshSectionConfig::MaterialSlot`, an index into the mesh's slot list.

As covered in [Buffer Sets & Sections](../sections/), sections created automatically for poly groups start with their material slot set to their poly group number. You change that with `UpdateSectionConfig`:

```cpp
// Point the first auto-created section at material slot 0.
RealtimeMesh->UpdateSectionConfig(
    FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0),
    FRealtimeMeshSectionConfig(0));
```

So the full path from a triangle to a material is: the section's `MaterialSlot` index, then the mesh's slot at that index, then that slot's `Material`. If the slot is empty or the index is out of range, you get the engine default material.

## Component level overrides

`URealtimeMeshComponent` is a `UMeshComponent`, so Unreal's normal per-component override system works here too. `SetMaterial(index, material)`, inherited from `UPrimitiveComponent`, stores an override on the component, exactly as it does on a static mesh component.

Resolution is layered. The component prefers its own override and falls back to the mesh's slot (from `Public/RealtimeMeshComponent.h`):

```cpp
// URealtimeMeshComponent::GetMaterial(ElementIndex):
//   1. If the component has an override at ElementIndex, use it.
//   2. Otherwise fall back to the mesh's slot: RealtimeMesh->GetMaterial(ElementIndex).
//   3. Otherwise nullptr.
```

`GetNumMaterials()` on the component returns whichever is larger, its own override count or the mesh's slot count, so overrides can extend past the slots the mesh declares. The component also forwards the name lookups (`GetMaterialIndex`, `GetMaterialSlotName`, `GetMaterialSlotNames`, `IsMaterialSlotNameValid`), which is what makes name-based access work through the component and in the details panel.

**In practice:** define your slots and their default materials on the mesh, and save `Component->SetMaterial(...)` for per-instance changes. The classic case is two components sharing one mesh but wanting different materials. An override on one component does not affect the shared mesh or anything else using it.
