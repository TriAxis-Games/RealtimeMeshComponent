---
title: Mesh Types
description: The four URealtimeMesh classes (Simple, Procedural, Dynamic, and Constructed) and how to choose between them.
---

A `URealtimeMeshComponent` renders whatever `URealtimeMesh` object you give it. RMC ships four to choose from, each suited to a different way of getting geometry in. You pick one, attach it to the component, then drive it with that class's API.

You attach a mesh with `InitializeRealtimeMesh` on the component:

```cpp
// Component is a URealtimeMeshComponent
URealtimeMeshSimple* Mesh = Component->InitializeRealtimeMesh<URealtimeMeshSimple>();
```

That one choice determines everything downstream: how you supply vertices, how you update them, and whether a CPU copy of the mesh is kept around. This page helps you pick. Each type then has its own page with the full API.

## The four types

| Type | Class | Module | You give it | Pick it when |
| --- | --- | --- | --- | --- |
| **Simple** | `URealtimeMeshSimple` | `RealtimeMeshComponent` | A `StreamSet` you fill with a builder | You want full control and the best update paths. This is the recommended default. |
| **Procedural** | `URealtimeMeshProcedural` | `RealtimeMeshComponent` | Parallel `TArray`s, via `CreateMeshSection` | You are moving a project off `UProceduralMeshComponent` and want a near find-and-replace port. |
| **Dynamic** | `URealtimeMeshDynamic` | `RealtimeMeshExt` | A `UDynamicMesh` or `FDynamicMesh3` | You already work in Geometry Scripting. |
| **Constructed** | `URealtimeMeshConstructed` | `RealtimeMeshExt` | A generator plus some parameters | Geometry is produced on demand from parameters, with no CPU copy kept. |

Simple and Procedural live in the core module, so they are always available. Dynamic and Constructed live in `RealtimeMeshExt`, which you have to add to your `.Build.cs`. Their pages explain how.

## Choosing

**Start with Simple.** `URealtimeMeshSimple` is the reference type and what the rest of these docs assume. You build a [StreamSet](../keyconcepts/streams/) with the [mesh builder](../keyconcepts/mesh-local-builder/), hand it to `CreateBufferSet`, and later change it with `UpdateBufferSet` or the in-place fast paths. It exposes everything RMC can do: flexible vertex formats, automatic sections from poly groups, LODs, and the cheapest possible per-frame updates. See [Simple](./realtime-mesh-simple/).

**Choose Procedural to port existing PMC code.** `URealtimeMeshProcedural` mirrors `UProceduralMeshComponent`'s Blueprint surface, right down to `CreateMeshSection` and `UpdateMeshSection` taking parallel arrays of positions, triangles, normals, UVs, colors, and tangents. It is the fastest way to get a PMC project running on RMC. Once you are up, you can move the parts that need it over to Simple. See [Procedural](./realtime-mesh-procedural/).

**Choose Dynamic if you live in Geometry Scripting.** `URealtimeMeshDynamic` keeps a `UDynamicMesh` as its source of truth, which makes RMC a mostly drop-in replacement for `UDynamicMeshComponent`. Edit with Geometry Script nodes or the GeometryProcessing API and RMC converts it for rendering and collision. See [Dynamic](./realtime-mesh-dynamic/).

**Choose Constructed for geometry that is a function of parameters.** `URealtimeMeshConstructed` holds a *generator* and some per-mesh parameters, and regenerates its geometry on demand. There is no CPU copy of the mesh kept around, which matters when you have hundreds of them. This is what the spatial streaming system is built on. See [Constructed](./realtime-mesh-constructed/).

## What they have in common

All four are `URealtimeMesh` classes, so everything in [Key Concepts](../keyconcepts/) and [Component Core](../component-core/) applies whichever you pick. Material slots, buffer sets and sections, LODs, and collision are all configured the same way.

The differences are entirely in *how geometry goes in* and *what update paths you get back out*.
