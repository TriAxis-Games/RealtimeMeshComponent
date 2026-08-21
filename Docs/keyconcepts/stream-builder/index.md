---
title: Stream Builder
description: Type-safe, array-like access to a stream, including builders that target a single channel of a multi-channel stream.
---

A [stream](../streams/) stores raw bytes. A stream builder wraps one and lets you treat it like a `TArray`, with `Add`, `RemoveAt`, `Append`, `Reserve`, and indexed access, all correctly typed.

It can also target part of a stream. If your texcoord stream holds four UV channels, a builder can address just one of them as though it were its own array.

There are two kinds, both aliases of the same underlying `TRealtimeMeshStreamBuilderBase` template in the `RealtimeMesh` namespace:

* `TRealtimeMeshStreamBuilder<AccessType, BufferType = AccessType>` works with a whole row.
* `TRealtimeMeshStridedStreamBuilder<AccessType, BufferType = AccessType>` works with one element within each row.

Both take two template parameters. **AccessType** is what you read and write in your own code. **BufferType** is what is actually stored in memory. When they differ, the builder converts between them for you, which is how you can work in comfortable `FVector3d` while the mesh stores compact `FVector3f`.

## Creating a stream builder

```cpp
using namespace RealtimeMesh;

// A stream with the key `Vertex:Position`, storing FVector3f.
FRealtimeMeshStream PositionStream = FRealtimeMeshStream::Create<FVector3f>(FRealtimeMeshStreams::Position);

// You know the stored type is FVector3f, so read and write it directly.
// Fastest option, since nothing is converted. Asserts if the types don't match.
TRealtimeMeshStreamBuilder<FVector3f> PositionBuilder(PositionStream);

// You'd rather work in FVector3d, but the stream stores FVector3f.
// There's a small conversion cost each way, but it's resolved at compile time,
// so it's as cheap as conversion gets.
TRealtimeMeshStreamBuilder<FVector3d, FVector3f> PositionBuilderConverting(PositionStream);

// You want FVector3d and you don't know what's stored (note the `void`).
// The conversion is worked out at runtime. The formats still have to be compatible:
// FVector3d, FVector3f, or a custom type with the same layout are all fine.
TRealtimeMeshStreamBuilder<FVector3d, void> PositionBuilderDynamic(PositionStream);
```

## Working with one channel at a time

A strided builder targets a single element inside each row. That lets you treat one UV channel as its own array. The second constructor argument is which element you want, counting from 0.

```cpp
// A texcoords stream with 4 channels, stored as packed half-precision floats.
FRealtimeMeshStream TexCoordsStream =
    FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 4>>(FRealtimeMeshStreams::TexCoords);

// A normal builder still sees all 4 channels at once.
TRealtimeMeshStreamBuilder<TRealtimeMeshTexCoords<FVector2DHalf, 4>> AllChannelsBuilder(TexCoordsStream);

// A strided builder sees only channel 1 (the second one), as an array of FVector2f,
// converting from the packed storage as it goes. It supports the same no-conversion,
// compile-time-conversion, and runtime-conversion variants as a normal builder.
TRealtimeMeshStridedStreamBuilder<FVector2f, FVector2DHalf> SecondChannelBuilder(TexCoordsStream, 1);
```

## Using a stream builder

```cpp
// Stream builders behave much like a TArray, plus a few extras.

// Add an element and get back its index.
int32 Index = PositionBuilder.Add(FVector3f(0, 0, 0));

// Build a row from its individual elements. Handy for multi-channel streams.
AllChannelsBuilder.Emplace({ FVector2f(0, 0), FVector2f(1, 1), FVector2f(2, 2), FVector2f(3, 3) });

// Reserve capacity up front.
PositionBuilder.Reserve(100);

// Append several at once.
PositionBuilder.Append({ FVector3f(0, 0, 0), FVector3f(1, 1, 1), FVector3f(2, 2, 2) });

// Overwrite an existing index.
PositionBuilder.Set(1, FVector3f(5, 5, 5));

// Remove element 1.
PositionBuilder.RemoveAt(1);

// Resize, filling any new rows with a value.
PositionBuilder.SetNumWithFill(128, FVector3f(0, 0, 1));
```

`ARealtimeMeshExample_Simple_StreamBuilders` (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_StreamBuilders.cpp`) builds a whole mesh this way, one stream at a time.

## When the format isn't known until runtime

Everything above is typed at compile time. Sometimes you cannot do that: the layout might come from Blueprint input, or you might need to pass a builder by reference into a function that is not a template.

For those cases use `FRealtimeMeshDynamicBuilder` (`Core/RealtimeMeshDynamicBuilder.h`). It is a non-templated builder over a stream set. You choose the format when you enable each stream, and each write pays a small cost to work out the layout:

```cpp
using namespace RealtimeMesh;

FRealtimeMeshStreamSet StreamSet;

// Creates the Position and Triangles streams straight away
// (FVector3f positions, uint16 indices).
FRealtimeMeshDynamicBuilder Builder(StreamSet);

// Turn on the optional streams, choosing storage types as you go. The no-argument
// versions default to FPackedNormal tangents and half-precision tex coords.
Builder.EnableTangents();
Builder.EnableTexCoords(/* NumChannels */ 1);
Builder.EnableColors();
Builder.EnablePolyGroups(GetRealtimeMeshDataElementType<uint16>());

// Add a vertex, then set its attributes by row index.
const int32 V0 = Builder.AddVertex(FVector3f(-50.0f, 0.0f, 0.0f));
Builder.SetTangents(V0, FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f));
Builder.SetColor(V0, FColor::Red);
Builder.SetTexCoord(V0, /* Channel */ 0, FVector2f(0.0f, 0.0f));

// Add a triangle, optionally tagging it with a poly group.
Builder.AddTriangle(V0, V1, V2, /* PolyGroup */ 0);
```

If you do know the format at compile time and you are in a hot loop, prefer the [local mesh builder](../mesh-local-builder/). Its accessors are matched to the format and have no indirection.
