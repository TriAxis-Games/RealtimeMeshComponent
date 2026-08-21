---
title: Examples
description: A tour of every example actor that ships with the plugin, from a single triangle up to runtime Nanite.
---

Every example is a self-contained C++ actor. Drop one into a level and it renders immediately, because most of them build their mesh in `OnConstruction`.

The Simple, Procedural, Dynamic, and Constructed examples live in the `RealtimeMeshExamples` module. The Nanite ones live in a separate, editor-only `RealtimeMeshNaniteExamples` module. See [Installation: Examples](../installation/examples/) for where they are and how to make plugin content visible.

## Simple (URealtimeMeshSimple)

The general purpose, CPU-authored type. Roughly in order from beginner to advanced.

* **`ARealtimeMeshExample_Simple_HelloTriangle`** is the canonical first example: one two-sided triangle built with `TRealtimeMeshBuilderLocal` and a single buffer set. **Start here.** The [Quickstart](../quickstart/) walks through this exact example.
* **`ARealtimeMeshExample_Simple_ComponentSetup`** wires up a `URealtimeMeshComponent` by hand on a plain `AActor`, for when you are not subclassing `ARealtimeMeshActor`.
* **`ARealtimeMeshExample_Simple_StreamBuilders`** builds the same triangle with the lower-level per-stream `TRealtimeMeshStreamBuilder` instead of the all-in-one builder.
* **`ARealtimeMeshExample_Simple_MultipleUVs`** is the same triangle again, this time with two UV channels per vertex.
* **`ARealtimeMeshExample_Simple_VertexColors`** shows four ways to author per-vertex color: a rainbow grid, an RGB corner-coloured box, a radial gradient disc, and a height gradient cylinder.
* **`ARealtimeMeshExample_Simple_BasicShapes`** generates boxes with `URealtimeMeshBasicShapeTools` rather than placing vertices by hand.
* **`ARealtimeMeshExample_Simple_MultipleSections`** packs several boxes into one buffer set using poly groups, each with its own material slot. This is the idiomatic way to render one mesh with several materials.
* **`ARealtimeMeshExample_Simple_LODs`** builds four LODs, each a differently rotated and coloured box, showing `AddLOD` and per-LOD screen size setup.
* **`ARealtimeMeshExample_Simple_Collision`** shows both collision approaches: an explicit simple box shape, and complex-as-simple collision that follows the rendered triangles.
* **`ARealtimeMeshExample_Simple_HighPoly`** is a subdividable high-poly sphere using 32-bit indices, for stress testing large buffers.
* **`ARealtimeMeshExample_Simple_DynamicUpdate`** ripples a grid every frame by rebuilding and re-pushing a `StreamSet` through `UpdateBufferSet`. The simplest "mesh that changes over time" pattern.
* **`ARealtimeMeshExample_Simple_FastUpdate`** does the same thing far more cheaply, using the in-place paths `EditMeshInPlace` and `EditMeshInPlaceRanged` on a Dynamic buffer set, including a partial update that only touches the middle band.
* **`ARealtimeMeshExample_Simple_AsyncBuild`** builds an expensive `StreamSet` on a worker thread and commits it on the game thread with `DoOnAsyncThread` and `ContinueOnGameThread`. This is the recommended pattern for heavy procedural geometry.
* **`ARealtimeMeshExample_Simple_ObjLoad`** loads a mesh from an OBJ file at runtime.
* **`ARealtimeMeshExample_Simple_StaticMeshConvert`** converts a `UStaticMesh` into a `URealtimeMeshSimple` at runtime, defaulting to the engine's basic cube.
* **`ARealtimeMeshExample_Simple_DistanceField`** builds a box, asks the `RealtimeMeshSDF` module for a signed distance field, and visualises a slice of it with debug draw.
* **`ARealtimeMeshExample_Simple_LumenSupport`** applies a generated distance field and Lumen card representation so the mesh lights correctly under Lumen GI.
* **`ARealtimeMeshExample_Simple_DistanceFieldProfile`** and **`ARealtimeMeshExample_Simple_CardProfile`** profile the CPU and GPU distance field and Lumen card generators on a high-poly sphere, and log the timings.
* **`ARealtimeMeshExample_Simple_ComputeIndirect`** and **`ARealtimeMeshExample_Simple_ComputeWave`** cover GPU-driven indirect draws and per-frame compute shader mesh deformation. Both need a real GPU, so they do not work under `-NullRHI`.

## Procedural (URealtimeMeshProcedural)

The `ProceduralMeshComponent` parity type, for near drop-in PMC migration.

* **`ARealtimeMeshExample_Procedural_Basic`** creates a box the PMC way, with parallel `TArray`s of positions, triangles, normals, UVs, and colors through `CreateMeshSection`.
* **`ARealtimeMeshExample_Procedural_Update`** creates a grid once and then animates only its vertex positions every frame with `UpdateMeshSection`, which is PMC's partial update path.

## Dynamic (URealtimeMeshDynamic)

The `UDynamicMeshComponent` replacement, backed by an owned `FDynamicMesh3`.

* **`ARealtimeMeshExample_Dynamic_Basic`** builds an `FDynamicMesh3` with two triangle groups and hands it over with `SetMesh`. The converter turns each group into its own section and material slot.
* **`ARealtimeMeshExample_Dynamic_Edit`** edits the owned `UDynamicMesh` in place each frame using the fast vertex update path, `EditMesh` with `FastUpdate`.

## Generators and spatial streaming

* **`ARealtimeMeshConstructedActor`** shows the data-driven generator path: a `URealtimeMeshConstructed` whose geometry is produced on demand by a generator resolved from a config struct, with no CPU copy of the mesh kept.
* **`ARealtimeMeshSpatialStreamingActor`** is an end-to-end demo of the spatial streaming state machine, driving the generator path and loading and unloading chunks as the player moves.

## Nanite (RealtimeMeshNaniteExamples module, editor only)

* **`ARealtimeMeshExample_Nanite_BasicRawData`** is the core pipeline from raw vertex and triangle arrays: `CreateFromRawMesh`, `BuildMinimalHierarchy`, `BuildRealtimeNaniteMesh`, `SetNaniteResources`. It does a low-poly sphere, a high-poly torus, and a dense tessellated plane.
* **`ARealtimeMeshExample_Nanite_ManualClusters`** builds Nanite clusters by hand: a multi-cluster box with one cluster per face, and a single cluster with several material ranges.
* **`ARealtimeMeshExample_Nanite_HierarchicalLODs`** runs the full LOD DAG pipeline on a dense UV sphere and logs the LOD pyramid. You can toggle it against the single-LOD path for an A/B comparison.
* **`ARealtimeMeshExample_Nanite_LODInspector`** builds the full LOD pyramid for a sphere and lays each level out side by side, so you can see what the simplifier did at each step.
* **`ARealtimeMeshExample_Nanite_FrequentUpdates`** stress tests frame-by-frame Nanite updates at three rates: every frame, every 5 frames, and every 30 frames.
* **`ARealtimeMeshExample_Nanite_PrebuiltData`** copies prebuilt Nanite resources from a `UStaticMesh` asset straight onto a Realtime Mesh component, bypassing RMC's own build pipeline. Useful for working out whether a rendering problem is in the component or in the builder.

## Where to start

New to the plugin? Open `ARealtimeMeshExample_Simple_HelloTriangle` alongside the [Quickstart](../quickstart/), then branch out by mesh type. [Mesh Types](../mesh-types/) compares Simple, Procedural, Dynamic, and Constructed.
