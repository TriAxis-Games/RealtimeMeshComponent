# UE4 Runtime Mesh Component

**Examples project can be found here https://github.com/Koderz/UE4RuntimeMeshComponentExamples**


**The RuntimeMeshComponent, or RMC for short, is a component designed specifically to support rendering and collision on meshes generated at runtime. This could be anything from voxel engines like Minecraft, to custom model viewers, or just supporting loading user models for things like modding. It has numerous different features to support most of the normal rendering needs of a game, as well as ability to support both static collision for things such as terrain, as well as dynamic collision for things that need to be able to move and bounce around!**

**Now, the RMC is very similar in purpose to the ProceduralMeshComponent or CustomMeshComponent currently found in UE4, but it far surpasses both in features, and efficiency! It on average uses half the memory of the ProceduralMeshComponent, while also being more efficient to render, and far faster to update mesh data. This is shown by the ability to update a 600k+ vertex mesh in real time! The RMC is also nearly 100% compatible with the ProceduralMeshComponent, while adding many features above what the PMC offers.**

This branch is a *beta* version of the RMC. It is a near total rewrite of the core, to better support the upcoming features. This also means there has been some breaking changes in the API, but most should be easy to update for. There's been quite a few major changes to the core.

First, and most important, the base structure of the RMC has changed by being broken into 3 pieces. These are FRuntimeMeshData, URuntimeMesh and URuntimeMeshComponent.

The primary is URuntimeMesh, which is much like a UStaticMesh. It can be shared between multiple URuntimeMeshComponents (or other components in the future) and also separates the creation/maintenance of the GPU buffers from the URMC  so that even static section updates don't require sending every sections buffer data to the GPU again. This means static sections are now more efficient to update as well. Multi static section RMCs will notice a substantial change in update efficiency.
The URuntimeMeshComponent is now just meant to link the URuntimeMesh to an actor and doesn't carry any mesh data around internal to it, so you can have multiple URuntimeMeshComponents bound to a single URuntimeMesh and they all update with the single URuntimeMesh.
Now FRuntimeMeshData is where all the actual mesh data and section configuration is stored. It is owned by a TSharedRef internal to the URuntimeMesh. The reason for separating the data out of the URuntimeMesh is simple, FRuntimeMeshData is not a UObject so if used correctly you can now update the mesh data directly from any thread and the RMC will take care of the threading issues internally.

The next major change is that sections can have between 1 and 3 buffers now. This allows for separating pieces of data to be able to update smaller sections of the data at a time. Having positions in their own buffer tends to be a good idea for shadowing. Then you can have most of the rest (normal/tangent/UVs/Colors) in the second buffer or move, for example, the color channel to the third buffer to allow for things like mesh painting without updating the mesh structure.

*Current list of features*
* Full support for async collision cooking (See below for a known problem with UE4 regarding this)
* Configurable 1-3 buffers (Allows for more efficient updates/rendering)
* Brand new normal/tangent calculation utility that is several orders of magnitude faster

* High precision normals support 
* Tessellation support 
* Navigation mesh support 
* Fully configurable vertex structure 
* Ability to save individual sections or the entire RMC to disk 
* RMC <-> StaticMesHComponent conversions.  SMC -> RMC at runtime or in editor.  RMC -> SMC in editor.  
* Multiple UV channel support (up to 8 channels) 
* Static render path for meshes that don't update frequently, this provides a slightly faster rendering performance.
* Collision only mesh sections.
* 50%+ memory reduction over the ProceduralMeshComponent and CustomMeshComponent
* Visibility and shadowing are configurable per section.
* Separate vertex positions for cases of only needing to update the position.
* Collision has lower overhead compared to ProceduralMeshComponent


** The RMC fully supports the cooking speed improvements of UE4.14 and UE4.17 including async cooking. As of right now, the RMC is forced to always use async cooking in a shipping build due to an engine bug which I've submitted a fix to Epic for**


For information on installation, usage and everything else, [please read the Wiki](https://github.com/Koderz/UE4RuntimeMeshComponent/wiki/)

** Some features that I'm looking into now that the core has been rewritten **
* LOD (Probably with dithering support)
* Dithered transitions for mesh updates
* Instancing support (Probably ISMC style unless there's enough demand for HISMC style support)
* Mesh replication


**Supported Engine Versions:**
v1.2 supports engine versions 4.10+
v2.0 supports engine versions 4.12+
v3.0 supports engine versions 4.17+

*The Runtime Mesh Component should support all UE4 platforms.*
*Collision MAY NOT be available on some platforms (HTML5)*
