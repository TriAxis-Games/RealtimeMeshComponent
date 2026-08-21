---
title: Welcome to the Realtime Mesh Component
description: A fast, fully dynamic mesh component for Unreal Engine. Build geometry while your game is running, load it from files, or change it every frame.
---

Most meshes in Unreal are built ahead of time and shipped inside your project. The Realtime Mesh Component (RMC) is for everything else: geometry you create while the game is running, models your players load from their own files, terrain you generate procedurally, or a surface that changes shape every frame.

It scales from a single hand-built triangle all the way up to a streaming voxel world.

RMC is faster and uses less memory than Unreal's `ProceduralMeshComponent` and `UDynamicMeshComponent`. It can also convert to and from both of them, so if you already have a project built on one of those, you can move over without rewriting everything.

## Do I need to know C++?

Not to get started. A lot of RMC is available as Blueprint nodes: building mesh data, creating and updating geometry, setting up collision, and converting to and from Static Meshes. If you can wire up a Blueprint, you can build a mesh.

Some of the more specialised features are C++ only, mainly the compute shader and custom generator paths. Those pages say so at the top.

## What it can do

* **Collision** that works for both static level geometry and moving physics objects
* **Flexible vertex formats**, so you can trade off features against memory and speed
* **Up to 8 UV channels**, at normal or high precision
* **LODs**, up to Unreal's limit of 8 levels, with smooth dithered transitions
* **Full navmesh support**, so AI can path over geometry you generate
* **Async collision cooking**, which keeps the expensive part of collision off the game thread
* **Conversion** to and from Static Meshes (in editor and in game), ProceduralMeshComponent, and DynamicMesh
* **Runtime Nanite**, building real Nanite meshes from geometry you generate, with no cooking step. Works on an unmodified engine across the supported 5.5 to 5.8 range.
* **Runtime distance fields and Lumen cards**, so dynamic geometry lights correctly under Lumen
* **Spatial streaming**, loading and unloading chunks of world around the player, plus a compute shader path for geometry that lives entirely on the GPU

## How the plugin is put together

RMC ships as several modules so you only compile what you actually use:

* **RealtimeMeshComponent** is the core. Mesh data, rendering, the component itself, and collision. This is the one everybody needs.
* **RealtimeMeshExt** adds generators, converters, mesh loaders, and the distance field and Lumen card helpers.
* **RealtimeMeshSDF** is a standalone distance field and Lumen card generator.
* **RealtimeMeshNanite** builds Nanite meshes at runtime.
* **RealtimeMeshSpatial** streams mesh chunks in and out around the player.
* **RealtimeMeshCompute** drives geometry from compute shaders.
* **RealtimeMeshMeshOptimizer** is the shared mesh optimisation library the modules above use.

## Where to go next

* [Installation](./installation/) gets the plugin into your project.
* [Quickstart](./quickstart/) builds your first mesh, a single triangle, from scratch.
* [Key Concepts](./keyconcepts/) covers the handful of ideas everything else is built on.
* [Component Core](./component-core/) is the day to day API: materials, LODs, geometry, and collision.
* [Mesh Types](./mesh-types/) helps you pick the right mesh class for what you are building.
* [Examples](./examples/) tours every example actor that ships with the plugin.
