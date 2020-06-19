# Runtime Mesh Component for Unreal Engine 4
---
### Here you will find the current version of the RMC. At present it only supports UE4.23 and newer.  
### If you require support for UE4.20-4.22 you'll need to use the v4 release found here: https://github.com/KoderzUnreal/RuntimeMeshComponent/releases/tag/v4.0

---
**Join us on [Discord](https://discord.gg/KGvBBTv)**

**Basic examples of the features of the RMC can be found [here!](https://github.com/Koderz/RuntimeMeshComponent-Examples)**
**A more advanced example project can be found [here!](https://github.com/Moddingear/RMC-4.21-Example)**

**This project is a labor of love, but unfortunately love doesn't pay the bills!
If you've found this project useful, please consider supporting the development!
[You can support the project here!](https://github.com/Koderz/RuntimeMeshComponent/wiki/Support-the-development!)**

---

The Runtime Mesh Component, or RMC for short, is a component designed specifically to support rendering and collision on meshes generated at runtime. This could be anything from voxel engines like Minecraft, to custom model viewers, or supporting user loaded models. It has numerous different features to support most of the normal rendering needs of a game, as well as ability to support both static collision for things such as terrain, as well as dynamic collision for things that need to be able to move and bounce around!

The RMC is very similar in purpose to the ProceduralMeshComponent or CustomMeshComponent currently found in UE4, but far surpasses both in features, and efficiency! Both the Procedural Mesh Component and Custom Mesh Component are memory heavy, the Procedural Mesh Component is inefficient both in its rendering path as well as update logic, and the Custom Mesh Component is very lacking in any real features.

Version 4 is a total redesign of the component, to better support the wide variety of cases people have used it for, and more in the future. With this it has departed being backward compatible or cross compatible with the Procedural Mesh Component, but for that it has gained a massive amount of customizability, as well as compartmentalization of logic to better increase efficiency. 

Upgrading from the PMC to the RMC isn't difficult. Visit the [Wiki](https://github.com/Koderz/UE4RuntimeMeshComponent/wiki/) to find out more!

---

*List of features for V4: (Those with asterisks not yet completed, or tested)*
* Full support for variable mesh configurations
* Up to 8 Texture Coordinate (UV) channels
* High or normal precision Texture Coordinate (UV) channels
* High or normal precision normals/tangents
* LOD Support (Both static and dynamic draw paths functional, with dithering support)
* Automatic Normal/Tangent calculation possible
* Navigation Mesh Support
* RMC <-> StaticMesh conversions (RMC -> SMC only works in editor due to engine limits)
* Configurable Render Paths, (Quicker Update, Slower Render) (Slower Update, Quicker Render)
* No store mesh data internally, much lower memory footprint, mesh data can be cached by provider if wanted
* Material Slots, just like Static Mesh Component
* Visibility/Shadowing are configurable per section.
* Collision separate from rendering (collision can use render mesh automatically with use of provider)
* Tessellation Support

---

For information on installation, usage and everything else, [please read the Wiki](https://github.com/Koderz/UE4RuntimeMeshComponent/wiki/)



**Supported Engine Versions:**
v4.1 supports engine versions 4.23+
v4.0 supports engine versions 4.20+
v3.0 supports engine versions 4.17+
v2.0 supports engine versions 4.12+
v1.2 supports engine versions 4.10+

*The Runtime Mesh Component should support all UE4 platforms.*
*Collision MAY NOT be available on some platforms (HTML5)*
