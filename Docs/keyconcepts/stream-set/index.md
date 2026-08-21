---
title: Stream Set
description: A container for one or more streams. This is how you pass mesh data around.
---

A stream set (`FRealtimeMeshStreamSet`) is a collection of [streams](../streams/), stored as a map from [stream key](../streams/) to stream.

This is the currency of the whole plugin. When you hand geometry to a `URealtimeMeshSimple`, you hand it a stream set. When a loader or converter produces geometry, it produces a stream set. It can hold whatever combination of streams you need.

## Creating a basic stream set

```cpp
using namespace RealtimeMesh;

// Start with an empty set.
FRealtimeMeshStreamSet StreamSet;

// Add whatever streams you want. You can pass an explicit buffer layout...
FRealtimeMeshStream& PositionStream =
    StreamSet.AddStream(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>());

// ...or give the type as a template argument and let it work out the layout.
FRealtimeMeshStream& TangentsStream =
    StreamSet.AddStream<FRealtimeMeshTangentsNormalPrecision>(FRealtimeMeshStreams::Tangents);

// Look streams up by key. Find returns a pointer, null if it isn't there.
// FindChecked returns a reference and asserts if it's missing.
if (StreamSet.Contains(FRealtimeMeshStreams::Position))
{
    TRealtimeMeshStreamBuilder<FVector3f> PositionBuilder(StreamSet.FindChecked(FRealtimeMeshStreams::Position));
    // ...fill the stream through the builder...
}
```

To keep several streams the same length, put a [stream linkage](../stream-linkage/) over them. The linkage is a standalone object that you own, or more often that the [local mesh builder](../mesh-local-builder/) owns on your behalf.

## Copying and moving

Stream sets deliberately cannot be copied with `=`. Copying one duplicates every byte of mesh data, which can be very expensive, so RMC makes you ask for it explicitly:

```cpp
// Explicit copy constructor. Implicit copy and copy-assignment are disabled on purpose.
FRealtimeMeshStreamSet StreamSet2(StreamSet);
```

Moving is much cheaper. It hands over ownership of the internal data rather than duplicating it, and leaves the original empty:

```cpp
// Fast, because it moves ownership instead of duplicating the data.
FRealtimeMeshStreamSet StreamSet3 = MoveTemp(StreamSet);
```

Most functions that take a stream set, `CreateBufferSet` among them, either copy it or take ownership of it. So once you have finished building one, the usual move is to `MoveTemp` it in and let RMC keep it.
