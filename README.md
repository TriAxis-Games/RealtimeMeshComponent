# Runtime Mesh Component for Unreal Engine 4
---
### Here you will find the current version of the RMC. At present it only supports UE4.23 and newer.  
### If you want to use RMC with UE5, use branch RMC4.5-dev as it is retrocompatible with providers written for UE4 and works in 5.1+. It won't work with DX12 on 5.1 though (the garbage collector on DX12 in UE5.1 is broken, your game will crash after some time).
### Branch RMC4-UE5 is mostly outdated now but still works on 5.0.
### If you require support for UE4.20-4.22 you'll need to use the v4 release found here: https://github.com/KoderzUnreal/RuntimeMeshComponent/releases/tag/v4.0
---
# Note that to use RMCv4, the plugin must reside withing the plugins folder of your game, and that folder must be named RuntimeMeshComponent

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

Features:
* Full Collision Support, both static triangle mesh and dynamic moving objects
* Variable mesh formats, allowing for tradeoff in needed features and memory/performance overhead
* Up to 8 Texture Coordinate (UV) channels, Normal (float16) or High precision (float32), supports engine feature for high precision normals
* LOD Support, alowing engine maximum of 8 LOD levels and full dithering support
* Full NavMesh support
* Tessellation support, including generation
* Full threading support, both internally managing threads and allowing for your external threading safely around the garbage collector.
* Async collision updates. As collision can be slow to update, the RMC can offload it from the game thread.
* Separate collision from rendering
* StaticMesh conversion in editor.
* Tangent generation utility
* Mesh Slicer (mostly broken)



**Supported Engine Versions:**
v4.2 supports engine versions 4.23-4.27

*The Runtime Mesh Component should support all UE4 platforms.*
Some issues have been reported with Android
*Collision MAY NOT be available on some platforms (HTML5)*
