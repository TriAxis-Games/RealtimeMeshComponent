---
title: Key Concepts
description: The data structures behind the RealtimeMeshComponent, and how you use them to build a mesh.
---

There are a handful of building blocks that everything else in RMC sits on top of. They are small, they fit together, and they turn up everywhere. Once these click, the rest of the API mostly explains itself.

Start with how a mesh is put together, then work down through the pieces you use to build one.

## Mesh structure

What a mesh actually is: vertices, the indices that join them into triangles, and the poly groups that let you split one mesh into separately rendered pieces. [Mesh structure](./meshes/)

## Streams

A stream is one column of mesh data. Positions in one stream, normals in another, triangle indices in a third. They are the core data type in RMC. [Learn about streams](./streams/)

## Stream builder

Streams store raw bytes. A stream builder wraps one up and lets you use it like a normal array, with the right type and no manual offset maths. [Learn about stream builders](./stream-builder/)

## Stream linkage

A linkage ties several streams together so that adding a vertex to one adds a matching row to all of them. It keeps positions, normals, UVs, and colors the same length without you having to think about it. [Learn about stream linkages](./stream-linkage/)

## Stream set

A stream set is a bag of streams, and it is how you pass mesh data around. When you hand geometry to a mesh, you hand it one of these. [Learn about stream sets](./stream-set/)

## Local mesh builder

`TRealtimeMeshBuilderLocal` wires all of the above together for the common case, giving you a simple `AddVertex` and `AddTriangle` interface. This is what most code should use. [Learn about the local mesh builder](./mesh-local-builder/)
