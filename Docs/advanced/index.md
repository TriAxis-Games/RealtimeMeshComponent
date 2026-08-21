---
title: Advanced Topics
description: Write custom mesh generators, stream chunks of world around the player, replicate geometry, and drive meshes from the GPU.
---

Once you are comfortable with the [core component API](../component-core/) and the [mesh types](../mesh-types/), the extension modules open up four larger workflows.

Each lives in its own module, so you only compile what you use. Every page here walks through the real shipped API and grounds its examples in an actor from the `RealtimeMeshExamples` module, so you can open the source and run it yourself.

## In this section

* **[Custom Providers](./custom-providers/)** covers writing a mesh generator against `FRealtimeMeshGenerator`, in the `RealtimeMeshExt` module. A generator is stateless and thread safe, so one instance can drive many meshes at once on worker threads. You feed it a per-mesh descriptor and it produces whichever parts are out of date: structure, geometry, bounds, collision, Nanite. Also covers the `TRealtimeMeshProvider<TConfig>` convenience base, registering a generator so designers can pick it in the editor, and using one from `URealtimeMeshConstructed`.

* **[Spatial Streaming](./spatial-streaming/)** covers loading and unloading mesh chunks around the player, using the `RealtimeMeshSpatial` module. A world subsystem tracks streaming sources (the player is added for you) and drives a quadtree and LOD state machine that fires four events per chunk: load, activate, deactivate, unload. This is what you want for procedural terrain and any world too large to keep in memory at once.

* **[Compute Providers](./compute-providers/)** covers driving a section's geometry entirely from a GPU compute shader, in the `RealtimeMeshCompute` module. You create a compute-writable buffer set, implement `IRealtimeMeshComputeProvider::BuildComputePasses`, and register it to run every frame, on demand, or on a timer. Also covers motion vectors for clean deformation under TSR and TAA, and GPU indirect draws. **This path is C++ only.**

* **[Mesh Replication](./mesh-replication/)** covers replicating mesh geometry from server to clients automatically, when the optional UnrealNet plugin is installed. Meshes stream coarsest LOD first so clients see something immediately and it sharpens up as data arrives. Late joiners catch up on their own. Beyond enabling the plugins and replicating the actor, there is nothing to configure.

## Before you start

These pages assume you already know how to:

* Create a mesh with `InitializeRealtimeMesh<T>()` on a `URealtimeMeshComponent`. See [Component Core](../component-core/).
* Build streams with the builders, particularly `TRealtimeMeshBuilderLocal`. See [Key Concepts](../keyconcepts/).
* Work with buffer sets and sections. See [Component Core](../component-core/).

The provider and spatial streaming pages both build on `URealtimeMeshConstructed`, so read the [Constructed mesh type](../mesh-types/realtime-mesh-constructed/) first if you have not already.
