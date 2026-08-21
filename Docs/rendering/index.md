---
title: Advanced Rendering
description: Runtime Nanite, distance fields, and Lumen cards for a RealtimeMesh. What each one does and what it needs to work.
---

Unreal normally builds several extra GPU representations of a mesh when you import it: Nanite cluster data, a distance field, Lumen cards. Those are baked at cook time, which is fine for static assets and useless for geometry that did not exist until a moment ago.

RMC can generate all three **at runtime**, from geometry you create or change on the fly. That lets dynamic and procedural meshes take part in features that normally require cooked content.

This section covers all three. Each is optional and lives in its own module, so you only pay for what you use.

## Runtime Nanite

Build a Nanite-ready mesh from runtime or procedural vertex data, with no cooking step. You get the full LOD DAG, cluster hierarchy, and per-page streaming, and the result renders through Unreal's normal Nanite pipeline exactly like a cooked Nanite static mesh.

- **Module:** `RealtimeMeshNanite`
- **Requires:** nothing special on UE 5.5 to 5.8, it works on the unmodified engine. The TriAxis provider engine fork adds a more efficient streaming path. See the tier breakdown on the [Nanite](./nanite/) page.
- **Start here:** [Runtime Nanite](./nanite/)

## Mesh distance fields

Generate a signed distance field for a buffer set, on the GPU or the CPU, and apply it to the mesh.

Once applied, everything that consumes a mesh distance field starts working: Lumen global illumination, distance field shadows, and distance field ambient occlusion.

- **Modules:** `RealtimeMeshSDF` (the generator) plus `RealtimeMeshExt` (the `URealtimeMeshSDFLibrary` bridge)
- **Requires:** the project setting **Generate Mesh Distance Fields** switched on. The GPU path needs a real rendering context, though there is a CPU path for headless and background use.
- **Start here:** [Distance Fields](./distance-fields/)

## Lumen cards

Generate a Lumen card representation, which is the set of oriented planes Lumen uses to build its surface cache, and apply it so Lumen can light the mesh properly.

Without one, a dynamic mesh gets no surface cache coverage and will look plainly wrong under Lumen GI.

- **Modules:** `RealtimeMeshSDF` plus `RealtimeMeshExt`
- **Requires:** Lumen GI switched on, and mesh distance fields switched on, because Lumen needs both. Cards and the distance field are usually generated together.
- **Start here:** [Lumen Cards](./lumen-cards/)

## Which of these do I need?

**Your dynamic mesh looks wrong under Lumen GI.** You need *both* a distance field and a card representation. `URealtimeMeshSDFLibrary` has a single call that generates and applies both. See [Lumen Cards](./lumen-cards/).

**You want distance field shadows or DFAO, but not Lumen.** A distance field on its own is enough. See [Distance Fields](./distance-fields/).

**You are rendering huge amounts of high-poly runtime geometry.** Use [Runtime Nanite](./nanite/). It is completely independent of the distance field and Lumen paths, with its own LOD and streaming.
