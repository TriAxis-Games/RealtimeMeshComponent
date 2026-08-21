---
title: RealtimeMesh Procedural
description: URealtimeMeshProcedural, a near drop-in migration path from UProceduralMeshComponent using the familiar parallel-array API.
---

`URealtimeMeshProcedural` copies `UProceduralMeshComponent`'s API closely enough that an existing PMC project can move over with mostly find-and-replace changes.

You supply geometry the same way you always did: parallel `TArray`s of positions, triangles, normals, UVs, colors, and tangents, through `CreateMeshSection` and `UpdateMeshSection`.

Behind the scenes, each PMC section index becomes its own buffer set with a single section and its own buffers. That matches PMC's behaviour where updating one section does not touch any of the others.

Header: `Source/RealtimeMeshComponent/Public/RealtimeMeshProcedural.h`.

> Once you are up and running, consider moving the sections that need it over to [`URealtimeMeshSimple`](../realtime-mesh-simple/). Procedural trades away flexible vertex formats, automatic poly group sections, and the cheapest update paths in exchange for PMC parity.

## Getting started

```cpp
#include "RealtimeMeshProcedural.h"

URealtimeMeshProcedural* RealtimeMesh = Component->InitializeRealtimeMesh<URealtimeMeshProcedural>();
```

In Blueprint, use the **Initialize Realtime Mesh Procedural** node.

## Creating a section

`CreateMeshSection` takes the parameter list you already know. It replaces whatever was at that index:

```cpp
TArray<FVector> Vertices;
TArray<int32> Triangles;
TArray<FVector> Normals;
TArray<FVector2D> UV0;
TArray<FColor> VertexColors;
TArray<FRealtimeMeshProceduralTangent> Tangents;
// ... fill the arrays ...

RealtimeMesh->CreateMeshSection(
    /*SectionIndex*/ 0,
    Vertices, Triangles, Normals, UV0, VertexColors, Tangents,
    /*bCreateCollision*/ true);
```

`ARealtimeMeshExample_Procedural_Basic` (`Source/RealtimeMeshExamples/Private/Procedural/RealtimeMeshExample_Procedural_Basic.cpp`) builds a box this way, using `URealtimeMeshProceduralMeshLibrary::GenerateBoxMesh` to fill the arrays. That library mirrors PMC's `UKismetProceduralMeshLibrary`.

### The LinearColor version

`CreateMeshSection_LinearColor` mirrors PMC's linear color version. It takes `FLinearColor` vertex colors and up to four UV channels:

```cpp
RealtimeMesh->CreateMeshSection_LinearColor(
    0, Vertices, Triangles, Normals,
    UV0, UV1, UV2, UV3,
    LinearVertexColors, Tangents,
    /*bCreateCollision*/ false,
    /*bSRGBConversion*/ false);
```

`bSRGBConversion` controls how your `FLinearColor` values get packed into the `FColor` vertex stream. It defaults to `false`, meaning no conversion, so values round-trip faithfully to the material's Vertex Color node. Pass `true` if you would rather the stored color match the swatch as displayed.

## Updating a section

`UpdateMeshSection` changes vertex attributes on a section that already exists. The shape has to stay the same, so the index count must match what you created.

The useful part: **any optional array can be left empty to leave that stream alone**. That is how you cheaply animate just the positions:

```cpp
// Positions only. Pass empty arrays for everything you're not touching.
const TArray<FVector> EmptyNormals;
const TArray<FVector2D> EmptyUVs;
const TArray<FColor> EmptyColors;
const TArray<FRealtimeMeshProceduralTangent> EmptyTangents;

RealtimeMesh->UpdateMeshSection(0, NewVertices, EmptyNormals, EmptyUVs, EmptyColors, EmptyTangents);
```

There is a matching `UpdateMeshSection_LinearColor` for the four-UV layout. `ARealtimeMeshExample_Procedural_Update` (`Source/RealtimeMeshExamples/Private/Procedural/RealtimeMeshExample_Procedural_Update.cpp`) creates a grid once and then animates only its positions every frame this way.

## Reading, clearing, and visibility

- `GetMeshSection(Index, Vertices, Triangles, Normals, UV0..UV3, VertexColors, Tangents)` reads a section back into CPU arrays. Returns `false` if there is nothing at that index. Note that tangents come back with `bFlipTangentY = false`, because the flip is baked into the stored basis when written and cannot be recovered afterwards.
- `ClearMeshSection(Index)` removes one section. `ClearAllMeshSections()` removes all of them. The index space is **not** compacted, so cleared indices become holes, matching PMC.
- `SetMeshSectionVisible(Index, bVisible)` and `IsMeshSectionVisible(Index)` toggle rendering.
- `GetNumSections()` returns the highest index ever allocated plus one, holes included. Again, matching PMC.

## Collision

- `CreateMeshSection`'s `bCreateCollision` flag gives that section per-triangle collision, as in PMC.
- `AddCollisionConvexMesh(ConvexVerts)` adds a convex hull to the simple collision shapes. `ClearCollisionConvexMeshes()` removes them.
- `GetUseComplexAsSimpleCollision()` and `SetUseComplexAsSimpleCollision(bool)` mirror PMC's `bUseComplexAsSimpleCollision`. Set it to `false` if you are supplying simple shapes and want the mesh simulated.

See [Collision](../../component-core/collision/) for the wider picture.

## Migrating from ProceduralMeshComponent

A practical checklist:

1. **Swap the component.** Replace `UProceduralMeshComponent` with `URealtimeMeshComponent`, then create the mesh with `InitializeRealtimeMesh<URealtimeMeshProcedural>()` in C++, or the **Initialize Realtime Mesh Procedural** node in Blueprint. Your section calls then go on the returned `URealtimeMeshProcedural`, not on the component.
2. **Swap the tangent type.** Replace `FProcMeshTangent` with `FRealtimeMeshProceduralTangent`. It has the same `TangentX` and `bFlipTangentY` fields and the same constructors, so this is purely a type name change. RMC defines its own so it does not have to depend on the ProceduralMeshComponent module.
3. **Keep your `CreateMeshSection` and `UpdateMeshSection` calls.** Parameter order matches, and so does the "empty array leaves the stream alone" behaviour. The `_LinearColor` versions keep the four-UV layout.
4. **Check your vertex colors.** RMC's `bSRGBConversion` defaults to `false`. If your PMC content relied on the engine's sRGB packing, pass `true`.
5. **Move your collision setup.** `bCreateCollision`, `AddCollisionConvexMesh`, `ClearCollisionConvexMeshes`, and the complex-as-simple getter and setter all map one to one.
6. **Swap the helper library.** Use `URealtimeMeshProceduralMeshLibrary` wherever you used `UKismetProceduralMeshLibrary`.

Some PMC extras are not exposed here, including secondary buffers and custom render paths. If you relied on those, that content belongs on [`URealtimeMeshSimple`](../realtime-mesh-simple/) instead.
