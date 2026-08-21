---
title: Component & Core
description: The runtime objects behind a Realtime Mesh. Actor, component, mesh, LODs, buffer sets, sections, materials, updates, and collision.
---

This section covers the objects you work with day to day: the actor and component that put a mesh in your world, the `URealtimeMesh` object that holds the data, and the LOD, buffer set, and section hierarchy that organises the geometry. It also covers what you actually do with them: setting up materials and LODs, pushing geometry in, changing it later, and building collision.

If you have not read [Key Concepts](../keyconcepts/) yet, start there. This section assumes you are comfortable with **streams** and **stream builders**, and links to them rather than explaining them again.

For the concrete mesh classes (`URealtimeMeshSimple`, `URealtimeMeshProcedural`, and the rest), see [Mesh Types](../mesh-types/).

## Pages

- **[Structure](./structure/)** is the object model end to end: `ARealtimeMeshActor`, `URealtimeMeshComponent`, the mesh class hierarchy, the `FRealtimeMesh` data layer, the LOD, buffer set, and section hierarchy, and the render thread mirror.
- **[Buffer Sets & Sections](./sections/)** covers keys, configuration, draw types, stream ranges, and the automatic one-section-per-poly-group behaviour most meshes rely on.
- **[Materials](./materials/)** covers material slots on the mesh, how a section picks one, and how component level overrides fit in.
- **[Levels of Detail](./lods/)** covers adding and configuring LODs.
- **[Updating Mesh Data](./updating-mesh-data/)** covers everything from a full rebuild to the in-place GPU fast paths, reading data back, and building geometry off the game thread.
- **[Collision](./collision/)** covers simple shapes, per-triangle collision, custom collision geometry, and async cooking.
