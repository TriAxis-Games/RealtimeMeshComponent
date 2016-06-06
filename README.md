# UE4 Runtime Mesh Component

The Runtime Mesh Component allows for rendering runtime generated meshes as well as supporting collision with the runtime generated meshes. It is also nearly 100% compatible with the UE4 UProceduralMeshComponent

**Examples project can be found here https://github.com/Koderz/UE4RuntimeMeshComponentExamples**

**Improvements over UE4 UProceduralMeshComponent:**
* More efficient rendering.
* Static objects use the fastest available rendering path.
* Faster updates to sections for frame-by-frame update scenarios.
* Corrected collision support, less cooking overhead. (More to come later)
* 50%+ memory reduction.
* Custom vertex structure support.
* Multiple UV channel support.
* Collision only mesh sections.
* Shadow casting controllable per section.
* Serialization configurable to reduce map sizes when mesh saves aren't necessary.
