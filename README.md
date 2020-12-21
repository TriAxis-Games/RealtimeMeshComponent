**For information on installation, usage and everything else, [please read the Wiki](https://runtimemesh.koderz.io/)**
**Basic examples of the features of the RMC can be found [here!](https://github.com/KoderzUnreal/RuntimeMeshComponent-Examples)**
The Runtime Mesh Component, or RMC for short, is a component designed specifically to support rendering and collision on meshes generated at runtime. This could be anything from voxel engines like Minecraft, to custom model viewers, or supporting user loaded models. It has numerous different features to support most of the normal rendering needs of a game, as well as ability to support both static collision for things such as terrain, as well as dynamic collision for things that need to be able to move and bounce around!
Upgrading from the PMC to the RMC isn't difficult. Visit the [Wiki](https://github.com/Koderz/UE4RuntimeMeshComponent/wiki/) to find out more!
*List of features for V4: (Those with asterisks not yet completed, or tested)*
* Full support for variable mesh configurations
* Variable mesh formats, allowing for tradeoff in needed features and memory/performance overhead
* High or normal precision Texture Coordinate (UV) channels
* High or normal precision normals/tangents
* LOD Support (Both static and dynamic draw paths functional, with dithering support)
* Automatic Normal/Tangent calculation possible
* Navigation Mesh Support
* RMC <-> StaticMesh conversions (RMC -> SMC only works in editor due to engine limits)
* Configurable Render Paths, (Quicker Update, Slower Render) (Slower Update, Quicker Render)
* No mesh data stored internally, much lower memory footprint, mesh data can be cached by provider if wanted
* Material Slots, just like Static Mesh Component
* Visibility/Shadowing are configurable per section.
* Collision separate from rendering (collision can use render mesh automatically with use of provider)
* Tessellation Support

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
