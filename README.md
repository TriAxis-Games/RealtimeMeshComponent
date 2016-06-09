# UE4 Runtime Mesh Component

The Runtime Mesh Component allows for rendering runtime generated meshes as well as supporting collision with the runtime generated meshes. It is also nearly 100% compatible with the UE4 UProceduralMeshComponent

**Examples project can be found here https://github.com/Koderz/UE4RuntimeMeshComponentExamples**

**Improvements over UE4 UProceduralMeshComponent:**
* Frequently updating sections use faster update path and dynamic buffers
* Static objects use the static draw lists for lower overhead rendering
* Faster updates to sections for frame-by-frame update scenarios.
* 50%+ memory reduction. 
* Multiple UV channel support.
* Custom vertex structure support. 
* Corrected collision support, less cooking overhead. (More to come later)
* Collision only mesh sections.
* Shadow casting controllable per section.
* Serialization configurable to reduce map sizes when mesh saves aren't necessary.
* Sections can have separate vertex position buffer allowing for fastest possible updates when updating only the vertex position
