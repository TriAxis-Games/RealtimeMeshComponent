---
title: Local Mesh Builder
description: A helper for building meshes in the common vertex format. Vertices, tangents, UVs, colors, and triangles all in one place.
---

`TRealtimeMeshBuilderLocal` is the one you will use most.

Building a mesh by hand means juggling a [stream builder](../stream-builder/) per stream and a [linkage](../stream-linkage/) to keep them the same length. The local builder does all of that for you. It owns the position, tangent, tex coord, color, triangle, and poly group builders, keeps the vertex streams locked together, and gives you a single `AddVertex` and `AddTriangle` interface.

It is built around Unreal's local vertex factory, which is the format ordinary rendered meshes use. If you are making a normal mesh, this is the right tool.

You pick the storage formats through template parameters, and just like a stream builder it converts between what is stored and the friendlier types you write in your own code.

```cpp
template <
    typename IndexType = uint32,                   // uint16 or uint32
    typename TangentElementType = FPackedNormal,   // FPackedNormal or FPackedRGBA16N
    typename TexCoordElementType = FVector2DHalf,  // FVector2f or FVector2DHalf
    int32 NumTexCoords = 1,                        // how many UV channels, 1 to 8
    typename PolyGroupIndexType = uint16>          // poly group index storage
struct TRealtimeMeshBuilderLocal;
```

Smaller types mean less memory and faster rendering. `uint16` indices only address 65,536 vertices though, so use `uint32` for anything larger.

## Creating one

```cpp
using namespace RealtimeMesh;

// The builder works on a stream set. It creates the Position and Triangles streams
// straight away, and adds the optional ones as you enable them.
FRealtimeMeshStreamSet StreamSet;

// Pick your storage formats. Here: 16-bit indices, packed normals,
// half-precision UVs, one UV channel.
TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

// Choose which optional vertex data you want.
Builder.EnableTangents();
Builder.EnableTexCoords();
Builder.EnableColors();

// Poly groups let you pack several separately rendered pieces into one set of buffers
// by tagging each triangle.
Builder.EnablePolyGroups();

// Add a vertex, then set its attributes in a chain. AddVertex returns a small handle
// that converts to the new vertex index, so you can capture the index directly.
const int32 V0 = Builder.AddVertex(FVector3f(-50.0f, 0.0f, 0.0f))
    .SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
    .SetColor(FColor::Red)
    .SetTexCoord(FVector2f(0.0f, 0.0f));

const int32 V1 = Builder.AddVertex(FVector3f(0.0f, 0.0f, 100.0f))
    .SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
    .SetColor(FColor::Green)
    .SetTexCoord(FVector2f(0.5f, 1.0f));

const int32 V2 = Builder.AddVertex(FVector3f(50.0f, 0.0f, 0.0f))
    .SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
    .SetColor(FColor::Blue)
    .SetTexCoord(FVector2f(1.0f, 0.0f));

// Make a triangle from the three vertex indices, optionally tagging it with a poly group.
Builder.AddTriangle(V0, V1, V2, 0);
```

This is exactly the pattern in `ARealtimeMeshExample_Simple_HelloTriangle` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_HelloTriangle.cpp`), the shortest working mesh in the plugin.

## What else it can do

Beyond `AddVertex` and `AddTriangle`, you get what you would expect:

* **Ask what's enabled** with `HasTangents`, `HasTexCoords`, `HasVertexColors`, `HasPolyGroups`, `HasDepthOnlyTriangles`, and `NumTexCoordChannels`.
* **Reserve or resize** with `ReserveNumVertices`, `ReserveAdditionalVertices`, `SetNumVertices`, and the matching triangle versions. Worth doing if you know your vertex count up front.
* **Edit an existing vertex** with `EditVertex(Index)`, or set one attribute at a time with `SetPosition`, `SetNormalAndTangent`, `SetColor`, `SetTexCoord`, and friends.
* **Add a simplified shadow mesh** with `EnableDepthOnlyTriangles()` and `AddDepthOnlyTriangle`.

If your mesh format is not known until runtime, use `FRealtimeMeshDynamicBuilder` instead. See [Stream Builder](../stream-builder/).

## Motion vectors, and why your deforming mesh looks smeary

If a mesh changes shape every frame, for example one driven by a compute shader, it will ghost and smear under TSR or TAA. That is because the renderer has no idea where each vertex was last frame, so it cannot work out how fast anything is moving.

Adding a `PositionPrev` stream alongside `Position` fixes it. Its presence switches on per-vertex motion vectors and the surface stays clean.

`PositionPrev` is not part of the vertex format the builder manages, so you add it to the stream set yourself, as a copy of `Position`. RMC and the compute driver keep it updated from there:

```cpp
using namespace RealtimeMesh;

// After building your vertices into StreamSet, mirror Position into a PositionPrev stream.
if (const FRealtimeMeshStream* PositionStream = StreamSet.Find(FRealtimeMeshStreams::Position))
{
    FRealtimeMeshStream PrevStream(FRealtimeMeshStreams::PositionPrev, PositionStream->GetLayout());
    PrevStream.Append(*PositionStream);
    StreamSet.AddStream(MoveTemp(PrevStream));
}
```

`ARealtimeMeshExample_Simple_ComputeWave` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_ComputeWave.cpp`) lets you toggle the `PositionPrev` stream on and off, which is a quick way to see the ghosting for yourself.
