# Runtime Mesh Component for Unreal Engine 4
---
### Here you will find the current version of the RMC. At present it only supports UE4.23 and newer.  
### If you require support for UE4.20-4.22 you'll need to use the v4 release found here: https://github.com/KoderzUnreal/RuntimeMeshComponent/releases/tag/v4.0
---

**For information on installation, usage and everything else, [please read the docs!](https://runtimemesh.koderz.io/)**

---
**Join us on [Discord](https://discord.gg/KGvBBTv)**

**Basic examples of the features of the RMC can be found [here!](https://github.com/TriAxis-Games/RuntimeMeshComponent-Examples)**
**A more advanced example project can be found [here!](https://github.com/Moddingear/Hexagons)**

**This project is a labor of love, but unfortunately love doesn't pay the bills!
If you've found this project useful, please consider supporting the development!
[You can support the project here!](https://github.com/Koderz/RuntimeMeshComponent/wiki/Support-the-development!)**

---

The RuntimeMeshComponent or more commonly known as RMC, is a replacement to the ProceduralMeshComponent (aka PMC) found in UE4. The RMC is much more efficient, and carries many more features, while allowing for a much more fine-grained approach for advanced use cases, while being simple to use just like the PMC. It can handle any use case from simply loading models at runtime, to debug views, to modification of existing models all the way up to procedural generation of entire worlds!

The RMC has been around for 4+ years and has an active community of users from individuals, to schools, to Fortune 500 companies, with many released projects. You can also find active support in our Discord server here: https://discord.gg/KGvBBTv

---

Features (Community Edition):
* Full Collision Support, both static triangle mesh and dynamic moving objects
* Variable mesh formats, allowing for tradeoff in needed features and memory/performance overhead
* Up to 8 Texture Coordinate (UV) channels
* Normal or High preicision Texture Coordinate (UV) channels
* Normal or High preicision texture coordinates, supports engine feature for high precision normals
* LOD Support, alowing engine maximum of 8 LOD levels and full dithering support
* Full NavMesh support
* Tessellation support, including generation
* Full threading support, both internally managing threads and allowing for your external threading safely around the garbage collector.
* Async collision updates. As collision can be slow to update, the RMC can offload it from the game thread.
* Separate collision from rendering
* StaticMesh conversion in game and editor.
* Tangent generation utility
* Mesh Slicer


Features (Premium Edition):
* All features found in RMC-Community!
* Additional Reversed Indices buffer for inverted views or inverted transforms like negative scaling.
* Depth Only Indices, allow for a separate index buffer for depth prepass and shadows for improved performance
* Distance Field support. Supports engine features like DF Shadows, and DF Ambient Occlusion, and material distance queries, and Niagara collision
* Distance Field generation on CPU. RMC can generate the DF for you on the CPU, GPU implementation coming later.
* Model loading of many common formats including obj, stl, fbx, x, 3ds, dae, and more through Assimp
* Optimization-ReIndex, can reindex the mesh to remove redundant vertices.
* Optimization-VertexOrder, Forsyth algorithm to optimize the order of vertices to improve caching efficiency
* Optimization-Overdraw, Reducing overdraw of a mesh by rearranging triangles
* Optimization-VertexFetch, Improve memory coherency by ordering vertex buffer to more efficiently work with triangle order
* Texture loading support for .jpg/.jpeg, .png, .tga, .bmp, .psd, .gif
* Texture mipmap generation support


Improvements over PMC:
* 50-90% Lower memory useage than PMC
* 30-100% lower render thread cpu time
* Static draw path for maximum rendering performance
* Dynamic Draw path for efficient frequent updates.



**Supported Engine Versions:**
v4.1 supports engine versions 4.23+
v4.0 supports engine versions 4.20+
v3.0 supports engine versions 4.17+
v2.0 supports engine versions 4.12+
v1.2 supports engine versions 4.10+

*The Runtime Mesh Component should support all UE4 platforms.*
*Collision MAY NOT be available on some platforms (HTML5)*
