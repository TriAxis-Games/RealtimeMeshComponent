---
title: Mesh Tools
description: Load meshes at runtime, convert between RMC and StaticMesh, DynamicMesh, or ProceduralMesh, and clean up mesh data with meshoptimizer.
---

These are the tools for getting geometry *into* and *out of* a Realtime Mesh, and for tidying it up before you render it.

You will want them whenever the mesh data does not come from your own generator: files on disk, existing `UStaticMesh` assets, a Geometry Scripting `UDynamicMesh`, or a `UProceduralMeshComponent` you are migrating away from.

## What you need in your Build.cs

The OBJ loader, the DynamicMesh converter, and the data optimizer live in the **`RealtimeMeshExt`** module. To use any of them from C++:

```csharp
PublicDependencyModuleNames.AddRange(new[]
{
    "RealtimeMeshComponent",
    "RealtimeMeshExt",   // OBJ loader, DynamicMesh converter, optimizer
});
```

The StaticMesh converter, the `URealtimeMeshProcedural` component, and its helper library are all in the core `RealtimeMeshComponent` module, so they need no extra dependency.

## Everything speaks StreamSet

Every tool here works in terms of a `FRealtimeMeshStreamSet`, or its Blueprint wrapper `URealtimeMeshStreamSet`.

A stream set is just the raw vertex and index data: positions, tangents, texcoords, colors, triangles, poly groups. It is not yet a renderable mesh, it is the ingredients.

The pattern is always the same:

1. **Fill a stream set**, from a file, a converter, or your own [builder](../keyconcepts/).
2. *(Optional)* **Optimize it** in place. See [Mesh Optimization](./mesh-optimization/).
3. **Hand it to a mesh** with `URealtimeMeshSimple::CreateBufferSet(GroupKey, MoveTemp(StreamSet))`.

Because a stream set is plain CPU data, steps 1 and 2 can run on a worker thread. Only the final `CreateBufferSet` has to be on the game thread. See [Key Concepts](../keyconcepts/) for the fundamentals.

## What's here

- **[Loading Meshes](./loading-meshes/)** covers loading OBJ files at runtime with `URealtimeMeshObjLoader`, synchronously or in the background, plus the general recipe for wiring up any other file format.
- **[Converting Static Meshes](./converting-static-meshes/)** covers copying geometry between a `UStaticMesh` asset and a Realtime Mesh, in either direction, with `URealtimeMeshStaticMeshConverter` (core module).
- **[Converting Dynamic Meshes](./converting-dynamic-meshes/)** covers one-shot conversion to and from Geometry Scripting's `UDynamicMesh` with `URealtimeMeshDynamicMeshConverter`, plus how RMC relates to `UProceduralMeshComponent`.
- **[Mesh Optimization](./mesh-optimization/)** covers reindexing, cache optimisation, overdraw reduction, and vertex fetch improvements with the meshoptimizer-backed `URealtimeMeshDataOptimizer`.

## Related

If what you want is a mesh that *stays* mirrored to another live mesh, rather than converted once, you want a mesh type rather than a converter:

- [RealtimeMeshDynamic](../mesh-types/realtime-mesh-dynamic/) continuously mirrors a `UDynamicMesh`.
- [RealtimeMeshProcedural](../mesh-types/realtime-mesh-procedural/) is a drop-in replacement for `UProceduralMeshComponent` with the same Blueprint nodes.
