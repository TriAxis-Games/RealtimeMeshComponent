---
title: Structure
description: The Realtime Mesh object model, from the actor in your level down to the render thread proxies that own the GPU buffers.
---

Several pieces work together to render and simulate a Realtime Mesh. This page walks the whole thing top to bottom. It is worth reading once, because the rest of this section (sections, materials, LODs, updates, collision) makes much more sense afterwards.

## ARealtimeMeshActor

`ARealtimeMeshActor` is the Realtime Mesh equivalent of `AStaticMeshActor`: a convenient base class for an actor that displays one. It owns a single `URealtimeMeshComponent` as its root, and adds a bit of editor plumbing so your mesh does not rebuild constantly while you drag the actor around.

It does very little beyond that. You are free to ignore it entirely and add a `URealtimeMeshComponent` to your own actor, or put several of them on one actor.

Worth knowing about (see `Public/RealtimeMeshActor.h`):

- `GetRealtimeMeshComponent()` gets you the component it owns.
- `bDeferGeneration` moves mesh generation out of the construction script and into the `OnGenerateMesh` event. Much kinder to editor performance when generation is expensive.
- `bFrozen` stops automatic rebuilds. You can still change the mesh by calling functions directly.
- `bResetOnRebuild` clears the mesh before each rebuild.
- `MakeStream`, `MakeStreamSet`, and `MakeMeshBuilder` are Blueprint helpers for building geometry without C++.

> The example actors in `Source/RealtimeMeshExamples/` all derive from `ARealtimeMeshExampleActor`, which derives from `ARealtimeMeshActor`. They are the reference for everything in this section.

## URealtimeMeshComponent

`URealtimeMeshComponent` is the smallest useful piece. It is a `UMeshComponent`, the same base class `UStaticMeshComponent` uses, so it slots into all the normal component machinery: transforms, materials, collision responses, visibility. You can put one or more on any actor.

The component holds almost no geometry itself. It holds a pointer to a `URealtimeMesh` and forwards rendering, bounds, and collision to it:

```cpp
// Create and attach a mesh in one call. The templated version casts the result for you.
URealtimeMeshSimple* RealtimeMesh =
    GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
```

Key members (see `Public/RealtimeMeshComponent.h`):

- `InitializeRealtimeMesh(TSubclassOf<URealtimeMesh>)` and `InitializeRealtimeMesh<MeshType>()` create a new mesh of that class, owned by the component.
- `SetRealtimeMesh(URealtimeMesh*)` assigns an existing mesh. This is how you **share** one mesh between components.
- `GetRealtimeMesh()` and `GetRealtimeMeshAs<T>()` get it back.
- `KeepMomentumOnCollisionUpdate` and `bUpdateNavigationOnCollisionUpdate` control collision update behaviour, covered in [Collision](../collision/).

The `RealtimeMesh` pointer is a replicated property, so which mesh a component is using replicates. Whether the actor replicates at all is opt-in, which is a change from older versions. See the [Migration Notes](../../migration/).

## URealtimeMesh

`URealtimeMesh` is the object that actually holds your mesh. It owns the material slots, manages the collision body, exposes the LOD API, and can be **shared** by several components and therefore several actors. Sharing means many instances render the same geometry without duplicating a single byte of it.

`URealtimeMesh` is abstract, so you always work with one of the concrete classes:

```
URealtimeMesh                        (abstract base)
└── URealtimeMeshManaged             (abstract, "we handle the data lifecycle for you")
    ├── URealtimeMeshSimple          (core module)
    ├── URealtimeMeshProcedural      (core module)
    └── URealtimeMeshDynamic         (RealtimeMeshExt module)

URealtimeMesh
└── URealtimeMeshConstructed         (RealtimeMeshExt, generator based, NOT Managed)
```

The **Managed** tier in the middle is where most of the shared functionality lives: collision, Nanite, distance field and Lumen card storage, section visibility and shadow controls, and removing sections and buffer sets. Its three leaves only add their own way of getting geometry in:

1. **`URealtimeMeshSimple`** is the "hand it a StreamSet and forget about it" mesh. Closest in feel to `ProceduralMeshComponent`, and the class used throughout this section.
2. **`URealtimeMeshProcedural`** matches `ProceduralMeshComponent`'s API almost exactly, for projects moving over from PMC.
3. **`URealtimeMeshDynamic`** takes a `UDynamicMesh` from Geometry Scripting, so you can author with GeometryScript and render with RMC.

`URealtimeMeshConstructed` sits off to the side. It derives straight from `URealtimeMesh` rather than from Managed, and builds its geometry on demand from a generator instead of storing it.

You can subclass `URealtimeMesh` yourself, or derive from any of the concrete classes.

For a proper look at each one and when to reach for it, see [Mesh Types](../../mesh-types/).

### The FRealtimeMesh data layer

Behind every `URealtimeMesh` there is a plain C++ object in the `RealtimeMesh::` namespace called `FRealtimeMesh`, and that is what does the real work. It holds the mesh hierarchy, talks to the render thread, and provides the rendering and collision data. The `UObject` is a thin Blueprint-friendly wrapper over the top.

You can reach it with `GetMesh()`, or with a typed accessor:

```cpp
using namespace RealtimeMesh;
TSharedRef<FRealtimeMeshSimple> Data = RealtimeMesh->GetMeshData(); // Simple's typed accessor
```

Almost everything you need is on the `UObject` API, so you rarely have to touch this directly. It is worth knowing it exists because the hierarchy below lives inside it.

### FRealtimeMeshMaterialSlot

Just like a material slot on a Static Mesh, `FRealtimeMeshMaterialSlot` names one of the materials the mesh uses. Each has a `SlotName` and a `Material`. Sections store the *index* of the slot they draw with, not the material itself. See [Materials](../materials/).

## The mesh hierarchy: LOD, buffer set, section

Inside the data layer, geometry is organised as a three level tree. Each level exists on the game thread and is mirrored on the render thread.

### FRealtimeMeshLOD

The top level. A LOD holds however many buffer sets and sections should render when that level of detail is active.

Its configuration (`FRealtimeMeshLODConfig`) carries the `ScreenSize` at which it kicks in, and a `bIsVisible` flag. LODs are completely independent of each other, so a detailed LOD might use many separate sections while a cheap distant LOD collapses down to one. See [Levels of Detail](../lods/).

### FRealtimeMeshBufferSet

The middle level. A buffer set owns a group of vertex and index buffers (the streams), and any number of sections can draw out of it. Sharing one buffer set between sections both saves memory and cuts down on render state changes, which is faster.

> **Older name:** buffer sets used to be called *section groups*, and you will still see that spelling in older projects and in the internal `RealtimeMesh::` data layer. `FRealtimeMeshSectionGroup`, `FRealtimeMeshSectionGroupKey`, and `FRealtimeMeshSectionGroupConfig` are deprecated aliases that still compile. See the [Migration Notes](../../migration/).

### FRealtimeMeshSection

The bottom level. A section takes a *range* of its buffer set's vertices and indices, pairs it with a material slot and some render settings, and draws it.

Because several sections can draw out of the same buffer set, you can render the same data in different ways. One section might draw a simplified subset of the index buffer for shadows while another draws the full thing for the visible mesh.

- The section's **stream range** (`FRealtimeMeshStreamRange`) is the start and end of the vertices and indices it draws.
- The section's **config** (`FRealtimeMeshSectionConfig`) sets its material slot, visibility, shadow casting, and a couple of advanced flags.

One thing that catches people out: the **draw type** (Static or Dynamic) belongs to the *buffer set* (`FRealtimeMeshBufferSetConfig`), not the individual section, because it controls how the buffers are allocated. [Buffer Sets & Sections](../sections/) covers all of this properly.

## The render thread mirror

Like other render proxies in Unreal, a Realtime Mesh keeps a second copy of its state on the render thread. This is what stops the game thread and the render thread fighting over the same data: the game thread changes its own objects and hands the render thread an updated copy, rather than the two sharing live state.

Every game thread object has a render thread counterpart:

| Game thread | Render thread |
| --- | --- |
| `FRealtimeMesh` | `FRealtimeMeshProxy` |
| `FRealtimeMeshLOD` | `FRealtimeMeshLODProxy` |
| `FRealtimeMeshBufferSet` | `FRealtimeMeshBufferSetProxy` |
| `FRealtimeMeshSection` | `FRealtimeMeshSectionProxy` |
| `URealtimeMeshComponent` | `FRealtimeMeshComponentSceneProxy` |

`FRealtimeMeshProxy` owns the GPU data and works with the renderer to draw the mesh. `FRealtimeMeshBufferSetProxy` owns the actual vertex and index buffers, vertex factories, and ray tracing data. `FRealtimeMeshSectionProxy` copies the section configuration so the render thread can set up draw calls. `FRealtimeMeshComponentSceneProxy` is the component's scene proxy and links back to `FRealtimeMeshProxy`.

You will almost never touch any of these. They matter because they explain the update model. A **Static** buffer set has to rebuild its proxy to change geometry, while a **Dynamic** one can have its buffers updated in place. That single distinction drives everything in [Updating Mesh Data](../updating-mesh-data/).
