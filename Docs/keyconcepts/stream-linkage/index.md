---
title: Stream Linkage
description: Tie several streams together so that resizing one resizes them all.
---

Your positions, normals, UVs, and colors all need to stay the same length. Add a vertex and you need a new row in every one of them.

A stream linkage (`FRealtimeMeshStreamLinkage`) does that bookkeeping. Bind a group of [streams](../streams/) to one, and any operation that changes the size of one of them changes all of them. You add a vertex once, then fill in only the attributes you care about. The rest get a default value.

Two practical notes. A linkage holds raw pointers to the streams it binds, so those streams have to outlive it. And it cannot be copied or moved, so bind it where it sits. When it is destroyed it unbinds everything cleanly.

Most of the time you will not create one by hand, because the [local mesh builder](../mesh-local-builder/) sets one up internally to keep its vertex streams in sync. This page is for when you are managing your own streams.

## Creating a stream linkage

```cpp
using namespace RealtimeMesh;

// Our position stream.
FRealtimeMeshStream PositionStream = FRealtimeMeshStream::Create<FVector3f>(FRealtimeMeshStreams::Position);

// Our tangents stream, at normal precision.
FRealtimeMeshStream TangentsStream =
    FRealtimeMeshStream::Create<FRealtimeMeshTangentsNormalPrecision>(FRealtimeMeshStreams::Tangents);

// The linkage that will tie them together.
FRealtimeMeshStreamLinkage VerticesLinkage;

// Bind positions, with a default row value of zero.
// We can build the default directly because we know the type.
VerticesLinkage.BindStream(PositionStream, FRealtimeMeshStreamDefaultRowValue::Create(FVector3f::ZeroVector));

// Bind tangents. Here we pass the value we want plus the stream's storage layout, so the
// conversion to packed form happens once and the result is copied into each new row.
VerticesLinkage.BindStream(TangentsStream, FRealtimeMeshStreamDefaultRowValue::Create(
    FRealtimeMeshTangentsNormalPrecision(FVector3f::ZAxisVector, FVector3f::XAxisVector),
    TangentsStream.GetLayout()));

// Builders make the streams easier to work with.
TRealtimeMeshStreamBuilder<FVector3f> PositionBuilder(PositionStream);
TRealtimeMeshStreamBuilder<FRealtimeMeshTangentsHighPrecision, FRealtimeMeshTangentsNormalPrecision> TangentsBuilder(TangentsStream);

// Adding to positions also grows tangents to match, filled with the default.
// It doesn't matter which linked stream you call Add on, they all resize together.
int32 Index = PositionBuilder.Add(FVector3f(1, 0, 0));

// Now set the tangent for that row by index.
// Do NOT call Add on the tangents builder to set this value. That would add another
// row to every linked stream instead of filling the one you just created.
TangentsBuilder[Index] = FRealtimeMeshTangentsHighPrecision(FVector3f(0, 1, 0), FVector3f(0, 0, 1));
```

That last comment is the one mistake everyone makes once. Add on one stream, index into the others.

## Adding many rows at once

If you know up front how many rows you need, the batch API is dramatically faster than adding them one at a time. It works out the growth once and applies it to every stream in a single pass:

```cpp
// Add 100 rows to every linked stream, filled with each stream's default value.
// Returns the index of the first new row.
int32 Start = VerticesLinkage.AddRowsZeroed(100);

// Or add 100 uninitialised rows, if you're about to write every one of them anyway.
int32 StartRaw = VerticesLinkage.AddRowsUninitialized(100);

// Force every linked stream to an exact row count, growing or shrinking as needed.
VerticesLinkage.SetNumRows(64);

// Remove a run of rows from every linked stream at once.
VerticesLinkage.RemoveRows(/* StartIdx */ 10, /* Count */ 5);

// Reserve capacity across every linked stream.
VerticesLinkage.ReserveRows(256);
```
