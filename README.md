# Realtime Mesh Component (Core) for Unreal Engine 5

> This is the free, MIT-licensed **Core** edition of the Realtime Mesh Component.
> Looking for spatial streaming, runtime Nanite, mesh→SDF/Lumen cards, compute
> providers, replication, and more? Check out the
> [Pro edition](https://github.com/TriAxis-Games/RealtimeMeshComponent-Pro),
> available on [Fab](https://www.fab.com/).

**[Documentation](https://triaxis.games/realtime-mesh/)** · **[Discord](https://discord.gg/WCgffd3h6r)** · **[Issues](https://github.com/TriAxis-Games/RealtimeMeshComponent/issues)**

The Realtime Mesh Component (RMC) renders runtime-generated and runtime-modified
geometry in Unreal Engine 5. It is a high-performance replacement for the
ProceduralMeshComponent (PMC) and DynamicMeshComponent, built around an
efficient stream-based mesh data layer and a copy-on-write render proxy. It
scales from simply loading a model at runtime, through live mesh editing and
debug views, up to procedural generation of entire worlds.

RMC has been in active development for 8+ years with a community ranging from
individual developers to schools and Fortune 500 studios.

## What's included in Core

- `RealtimeMeshComponent` — the core runtime: stream-based mesh data, LODs,
  sections and section groups, materials, copy-on-write render proxy,
  collision, and full Blueprint + C++ APIs
- `RealtimeMeshExamples` — canonical usage examples (HelloTriangle, basic
  shapes, collision, async build, LODs, fast updates, and more)
- `RealtimeMeshEditor` — editor integration and details-panel tooling

## Pro edition adds

Mesh providers and the Constructed mesh system, dynamic mesh (UDynamicMesh)
integration, static-mesh/OBJ conversion, GPU + CPU mesh→SDF generation with
Lumen card support, compute-shader mesh generation, spatial streaming, a
runtime Nanite builder, and meshoptimizer-based mesh optimization — plus the
examples and tests for all of it.

## Installation

1. Clone or copy this repository into your project's `Plugins/` folder
   (e.g. `MyProject/Plugins/RealtimeMeshComponent/`), or install from
   [Fab](https://www.fab.com/).
2. Regenerate project files and build (C++ project required for source builds).
3. Enable the *Realtime Mesh Component* plugin in the editor if it isn't
   enabled automatically.

**Supported engine versions:** UE 5.5 – 5.8

## Getting started

The examples module is the best starting point — see
`Source/RealtimeMeshExamples/`, starting with
`RealtimeMeshExample_Simple_HelloTriangle`. Full guides, key concepts, and API
documentation live at [triaxis.games/realtime-mesh](https://triaxis.games/realtime-mesh/).

## Community & support

- [Discord](https://discord.gg/WCgffd3h6r) — active community support
- [Issue tracker](https://github.com/TriAxis-Games/RealtimeMeshComponent/issues)
- Contributions welcome — see [CONTRIBUTING.md](CONTRIBUTING.md)

## License

The Core edition is licensed under the [MIT License](LICENSE.txt).
Copyright © TriAxis Games, L.L.C.
