---
title: Streams
description: The core data type in RMC. What streams hold, how to create one, and the helpers you get.
---

A stream is one column of mesh data. All the positions live in one stream, all the normals in another, all the triangle indices in a third.

Technically, `FRealtimeMeshStream` holds a single data type such as `FVector3f` or `FColor`, and each row can hold between 1 and 8 of them. That row-of-several arrangement is what lets a single texcoord stream carry all 8 UV channels at once.

You can work with a stream directly, but it is rarely the nicest way. Most of the time you will wrap it in a [stream builder](../stream-builder/) and keep it in a [stream set](../stream-set/).

Streams live in the `RealtimeMesh` namespace, declared in `Core/RealtimeMeshDataStream.h`.

## Stream keys

Every stream is identified by an `FRealtimeMeshStreamKey`: a name, plus whether it is `Vertex` or `Index` data (`ERealtimeMeshStreamType`).

You rarely need to spell these out. The ones RMC knows about are collected on `FRealtimeMeshStreams`:

* `FRealtimeMeshStreams::Position` (Vertex) is the vertex positions
* `FRealtimeMeshStreams::PositionPrev` (Vertex) holds last frame's positions. Adding it switches on motion vectors for deforming meshes.
* `FRealtimeMeshStreams::Tangents` (Vertex) holds both the normal (Tangent-Z) and the tangent (Tangent-X)
* `FRealtimeMeshStreams::TexCoords` (Vertex) holds 1 to 8 UV channels
* `FRealtimeMeshStreams::Color` (Vertex) is vertex color
* `FRealtimeMeshStreams::Triangles` (Index) is the triangle index list
* `FRealtimeMeshStreams::DepthOnlyTriangles` (Index) is an optional simplified index list used for shadow and depth passes
* `FRealtimeMeshStreams::PolyGroups` (Index) holds one group number per triangle
* `FRealtimeMeshStreams::DepthOnlyPolyGroups` (Index) does the same for the depth-only triangles

There are a few more on `FRealtimeMeshStreams` (`ReversedTriangles`, `ReversedDepthOnlyTriangles`, and the `PolyGroupSegments` variants) but those are mostly internal. The list above is what you will actually touch.

## Creating a stream

```cpp
using namespace RealtimeMesh;

// A stream with the key `Vertex:Position`, holding FVector3f.
FRealtimeMeshStream PositionStream = FRealtimeMeshStream::Create<FVector3f>(FRealtimeMeshStreams::Position);
```

## Adding data in bulk

```cpp
// Say you already have an array of positions from somewhere.
TArray<FVector3f> IncomingData;

// Append copies the whole thing in one go.
PositionStream.Append(IncomingData);
```

`Append` accepts a `TArray`, a `TArrayView`, an initializer list, or another `FRealtimeMeshStream`, so whatever shape your source data is in, it will probably just work.

## Other helpers

```cpp
// Fill 20 rows starting at index 10 with a value.
PositionStream.FillRange(10, 20, FVector3f(0, 0, 1));

// Zero 20 rows starting at index 10.
PositionStream.ZeroRange(10, 20);

// Reserve space for 128 rows up front, to avoid repeated reallocation.
PositionStream.Reserve(128);
```

There is more on the stream if you need it: `Num`, `SetNumUninitialized` and `SetNumZeroed`, `AddUninitialized` and `AddZeroed`, `RemoveAt`, `Empty`, `Shrink`, and `ConvertTo` for changing the stored format. For everyday mesh building though, reach for a [stream builder](../stream-builder/) instead.
