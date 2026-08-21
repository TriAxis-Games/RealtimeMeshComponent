---
title: Collision
description: Setting up collision on a Realtime Mesh. Simple shapes, per-triangle collision, custom collision geometry, async cooking, and the helper libraries.
---

A Realtime Mesh can do two kinds of collision, and it is worth being clear on the difference because picking the wrong one is the most common performance mistake here.

**Simple collision** uses basic shapes: spheres, boxes, capsules, convex hulls. Cheap to build and cheap to test against. This is what physics objects should use wherever possible.

**Complex collision** uses the actual triangles of your mesh. Exact, but much more expensive to prepare and to query. Right for terrain and static level geometry, wrong for a barrel you want to roll downhill.

The mesh owns the collision data and cooks it into a `UBodySetup`. The component then reports collision to the engine like any other primitive.

Everything here comes from `ARealtimeMeshExample_Simple_Collision` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_Collision.cpp`) and the types in `Public/Core/RealtimeMeshCollision.h`. The collision API lives on `URealtimeMeshManaged`, so it is available on Simple, Procedural, and Dynamic meshes.

## Collision settings

`FRealtimeMeshCollisionConfiguration` is the top level switch. Here is the whole thing, with defaults:

```cpp
struct FRealtimeMeshCollisionConfiguration
{
    bool bUseComplexAsSimpleCollision; // default true
    bool bUseAsyncCook;                // default true
    bool bShouldFastCookMeshes;        // default false
    bool bFlipNormals;                 // default false
    bool bDeformableMesh;              // default false
    bool bMergeAllMeshes;              // default false
};
```

- **`bUseComplexAsSimpleCollision`** means your rendered triangles also answer *simple* collision queries. Turn it **off** when you are supplying your own simple shapes and do not want per-triangle collision as well.
- **`bUseAsyncCook`** cooks collision on a worker thread instead of blocking the game thread. On by default, and you almost certainly want to leave it on.
- **`bShouldFastCookMeshes`** uses a faster, lower quality cook.
- **`bFlipNormals`** flips the winding of the collision mesh.
- **`bDeformableMesh`** hints that this collision changes often.
- **`bMergeAllMeshes`** merges all collision meshes into one during the cook.

Apply settings with `SetCollisionConfig`:

```cpp
FRealtimeMeshCollisionConfiguration CollisionConfig;
CollisionConfig.bUseComplexAsSimpleCollision = false; // simple shapes only
CollisionConfig.bUseAsyncCook = true;
RealtimeMesh->SetCollisionConfig(CollisionConfig);

FRealtimeMeshCollisionConfiguration Current = RealtimeMesh->GetCollisionConfig();
```

`SetCollisionConfig` returns a `TFuture<ERealtimeMeshCollisionUpdateResult>`, and has a Blueprint version taking a completion callback. See [Async cooking](#async-cooking-and-what-the-result-means) below.

## Simple shapes

`FRealtimeMeshSimpleGeometry` is a bundle of shape lists, one per kind:

```cpp
struct FRealtimeMeshSimpleGeometry
{
    FSimpleShapeSet<FRealtimeMeshCollisionSphere>         Spheres;
    FSimpleShapeSet<FRealtimeMeshCollisionBox>            Boxes;
    FSimpleShapeSet<FRealtimeMeshCollisionCapsule>        Capsules;
    FSimpleShapeSet<FRealtimeMeshCollisionTaperedCapsule> TaperedCapsules;
    FSimpleShapeSet<FRealtimeMeshCollisionConvex>         ConvexHulls;
};
```

Every shape has a `Name`, `Center`, `Rotation`, and `bContributesToMass` from its base `FRealtimeMeshCollisionShape`. On top of that:

- **Sphere** has `Radius`.
- **Box** has `Extents`. Careful here: these are **full dimensions**, not half-extents, because they map straight onto `FKBoxElem.X/Y/Z`. A box matching a mesh built with a half-extent of 250 needs `Extents` of 500.
- **Capsule** has `Radius` and `Length`.
- **Tapered capsule** has `RadiusA`, `RadiusB`, and `Length`.
- **Convex hull** takes a set of vertices via `SetVertices(...)`.

Build the geometry, add your shapes, and apply it with `SetSimpleGeometry`, which returns a `TFuture<ERealtimeMeshCollisionUpdateResult>` (Blueprint gets a callback version):

```cpp
// A 500x500x10 collision box, matching a rendered floor built with 250x250x5 half-extents.
FRealtimeMeshSimpleGeometry SimpleGeometry;
FRealtimeMeshCollisionBox CollisionBox;
CollisionBox.Extents = FVector(500.0f, 500.0f, 10.0f);
CollisionBox.Center = FVector::ZeroVector;
CollisionBox.Rotation = FRotator::ZeroRotator;
CollisionBox.bContributesToMass = true;
SimpleGeometry.Boxes.Add(CollisionBox);
RealtimeMesh->SetSimpleGeometry(SimpleGeometry);

FRealtimeMeshSimpleGeometry Current = RealtimeMesh->GetSimpleGeometry();
```

Simple shapes are dramatically cheaper than per-triangle collision. If a handful of primitives approximates your mesh well enough, use them, and set `bUseComplexAsSimpleCollision = false`. This is the `BuildSimpleCollision` path in the example.

Each `FSimpleShapeSet` supports `Add`, `Insert`, `Update`, `Remove` (by index or by name), `GetByIndex`, `GetByName`, and `GetIndexFromName`, so you can manage shapes by name if that suits you better.

## Complex collision from your rendered triangles

Instead of authoring separate collision geometry, you can promote the mesh's own triangles to collision. Two things need to be true:

1. `bUseComplexAsSimpleCollision = true` in the config, so complex geometry also answers simple queries.
2. Each section has to opt in with `bShouldCreateCollision`. A section only contributes its triangles when it is flagged.

You flag a section either when you create it or when you update its config:

```cpp
// When creating a section explicitly:
RealtimeMesh->CreateSection(SectionKey, FRealtimeMeshSectionConfig(0),
    StreamRange, /*bShouldCreateCollision=*/ true);

// Or when updating an existing section (including one created automatically):
RealtimeMesh->UpdateSectionConfig(SectionKey, FRealtimeMeshSectionConfig(0),
    /*bShouldCreateCollision=*/ true);
```

The example's `BuildComplexCollision` sets `bUseComplexAsSimpleCollision = true` and builds a stepped platform whose rendered triangles are also its collision surface, with no separate collision geometry authored at all.

> Complex collision is per-triangle, and both cooking and querying it cost real time. Save it for shapes that genuinely need it, such as arbitrary terrain, and lean on simple shapes for everything else.

## Custom collision geometry

Sometimes you want per-triangle collision that is *different* from what you render, typically a lower-poly proxy. Supply it as `FRealtimeMeshComplexGeometry`, which is a collection of `FRealtimeMeshCollisionMesh` objects:

```cpp
FRealtimeMeshCollisionMesh CollisionMesh;
CollisionMesh.Name = FName("Proxy");
CollisionMesh.SetVertices(MoveTemp(Vertices));      // TArray<FVector3f>
CollisionMesh.SetTriangles(MoveTemp(Triangles));    // TArray<TIndex3<int32>>
// optional: SetMaterials(...), SetTexCoords(...)

FRealtimeMeshComplexGeometry ComplexGeometry;
ComplexGeometry.Add(MoveTemp(CollisionMesh));
RealtimeMesh->SetCustomComplexMeshGeometry(MoveTemp(ComplexGeometry));
```

The full API on the mesh, all returning `TFuture<ERealtimeMeshCollisionUpdateResult>`:

- `SetCustomComplexMeshGeometry(...)`, by rvalue or const reference.
- `EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshComplexGeometry&)>)` changes what is already there.
- `ClearCustomComplexMeshGeometry()` removes it.
- `ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshComplexGeometry&)>)` reads it under lock. Do not keep the reference past the callback.
- `HasCustomComplexMeshGeometry()` tells you whether any is set.

```cpp
RealtimeMesh->EditCustomComplexMeshGeometry([](FRealtimeMeshComplexGeometry& Geometry)
{
    Geometry.GetByIndex(0).SetVertices(NewVertices);
});
```

`FRealtimeMeshCollisionMesh` caches its cooked result and only re-cooks when you actually change its vertices, triangles, materials, or UVs, so repeated edits only pay for what changed.

## Async cooking and what the result means

With `bUseAsyncCook` on, which is the default, collision cooks on a worker thread and gets applied when it finishes. The game thread never blocks waiting for it.

One consequence: a newer update can overtake one still cooking. So every collision call reports how it ended up, via `ERealtimeMeshCollisionUpdateResult`:

- `Updated`, the collision was cooked and applied.
- `Ignored`, a newer update superseded this one and it was dropped.
- `Error`, it failed.
- `Unknown`, the default unset value.

Wait on the future, or use the Blueprint callback, if you need to react:

```cpp
RealtimeMesh->SetSimpleGeometry(SimpleGeometry)
    .Next([](ERealtimeMeshCollisionUpdateResult Result) { /* ... */ });
```

### Component level settings

Two flags on `URealtimeMeshComponent` (see `Public/RealtimeMeshComponent.h`) control what happens when collision updates:

- **`KeepMomentumOnCollisionUpdate`** (default `false`) keeps the body's velocity through a collision rebuild instead of resetting it.
- **`bUpdateNavigationOnCollisionUpdate`** (default `true`) refreshes the navigation system on every collision update. **Turn this off for collision meshes that change frequently**, otherwise you will rebuild navmesh tiles continuously and wonder where your frame rate went. The navigation update is skipped automatically when the component cannot affect navigation anyway.

## Helper libraries

Two Blueprint function libraries help out here (`Public/RealtimeMeshCollisionLibrary.h`):

- **`URealtimeMeshCollisionTools`** has utilities: `FindCollisionUVRealtimeMesh` gets the UV at a hit location, `CookConvexHull` and `CookComplexMesh` cook a shape up front so it does not get cooked lazily later, and `AppendStreamsToCollisionMesh` fills an `FRealtimeMeshCollisionMesh` from a `StreamSet`, optionally just a range of triangles.
- **`URealtimeMeshSimpleGeometryFunctionLibrary`** gives you Blueprint friendly `Add`, `Insert`, `Get`, `Update`, and `Remove` (by index and by name) for each shape kind, operating on an `FRealtimeMeshSimpleGeometry`.

From C++ you can also get the UV at a hit directly on the mesh with `URealtimeMesh::CalcTexCoordAtLocation(...)`.
