---
title: Migration Notes
description: Renames, moved headers, and behaviour changes to be aware of when upgrading an existing project to RMC v5.4.
---

This page covers what can affect an existing project when you upgrade to v5.4: renames, moved headers, and behaviour changes. New features are in the [changelog](/realtime-mesh/changelog/) instead.

Most of this list is mechanical. Renames come with back-compat aliases, and include paths that moved still resolve in their short form. The one to read carefully is the **replication change**, because it is silent unless you know to look for it.

v5.4 also raises the minimum engine version. The supported range is now **UE 5.5 through 5.8**.

## Section groups are now buffer sets

The types and functions that used to say *section group* now say **buffer set**, which is a better description of what the thing actually is: a set of vertex and index buffers shared by one or more sections.

Nothing about the data changed. Serialization is by content, so saved assets, Blueprint graphs, and replicated data are all unaffected.

| Was | Now |
| --- | --- |
| `FRealtimeMeshSectionGroupKey` | `FRealtimeMeshBufferSetKey` |
| `FRealtimeMeshSectionGroupConfig` | `FRealtimeMeshBufferSetConfig` |
| `MakeSectionGroupKey*` | `MakeBufferSetKeyUnique` / `Indexed` / `Named` |
| `CreateSectionGroup[Unique]`, `UpdateSectionGroup`, `GetSectionGroup`, `ProcessSectionGroup` | `CreateBufferSet[Unique]`, `UpdateBufferSet`, `GetBufferSet`, `ProcessBufferSet` |
| `GetSectionGroups`, `GetSectionsInGroup`, `RemoveSectionGroup` | `GetBufferSets`, `GetSectionsInBufferSet`, `RemoveBufferSet` |

**Nothing breaks today.** The old C++ names are still there as aliases, marked `UE_DEPRECATED`. The old Blueprint nodes still exist and still run, they just show a deprecation warning. `StructRedirects` handle the two struct renames for existing assets.

Update at your own pace. The deprecated names will be removed in a future release.

One include path moved with the rename: `Core/RealtimeMeshSectionGroupConfig.h` is now `Core/RealtimeMeshBufferSetConfig.h`.

The internal `RealtimeMesh::` data layer and the generator interface (`FRealtimeMeshGenerator::GetSectionGroup`) keep the older spelling for now, so you will still run into it if you write a custom generator.

## Include paths: the Interface folder is gone

The core module's `Public/Interface/Core/` folder, a leftover from an older linkage scheme, has been flattened into `Public/Core/`.

**Short-form includes are unaffected.** `#include "Core/RealtimeMeshDataStream.h"` resolves exactly as it did before. Only long-form `Interface/Core/...` includes break, and the fix is to drop the `Interface/` prefix.

The same flattening happened in the extension modules:

* `RealtimeMeshExt`: `Interface/Ext/RealtimeMeshFactoryCommon.h` is now `Factory/RealtimeMeshFactoryCommon.h`.
* `RealtimeMeshSpatial`: `Interface/Spatial/RealtimeMeshSpatialValidChunkProvider.h` is now at the module's public root, as `RealtimeMeshSpatialValidChunkProvider.h`.

`RealtimeMeshInterfaceFwd.h` is renamed to `Core/RealtimeMeshCoreFwd.h`, with the same contents.

## RealtimeMeshAlgo.h has been split up

The grab-bag `Mesh/RealtimeMeshAlgo.h` is gone. The `RealtimeMeshAlgo` namespace is unchanged, so your call sites compile as they are. Only the includes move:

* `Mesh/RealtimeMeshPolyGroupUtils.h` has `OrganizeTrianglesByPolygonGroup` and `GetStreamRangesFromPolyGroups[DepthOnly]`.
* `Mesh/RealtimeMeshTangentUtils.h` has `GenerateTangents`.
* `Mesh/RealtimeMeshStreamSetUtils.h` has `ComputeBounds`.

A few helpers with no callers anywhere were removed outright: `ArePolygonGroupIndicesOptimal`, `PropagateTriangleSegmentsToPolygonGroups`, and the stream-to-stream `GatherSegmentsFromPolygonGroupIndices` overloads.

## The StaticMesh converter moved into the core module

`URealtimeMeshStaticMeshConverter` moved from `RealtimeMeshExt` into the core `RealtimeMeshComponent` module. That makes the editor's "Create Static Mesh" button and the Blueprint conversion nodes available in the free Core distribution too.

The include path (`RealtimeMeshStaticMeshConverter.h`) is unchanged, and CoreRedirects handle the module move for existing Blueprint graphs. If `RealtimeMeshExt` was only in your dependencies for this converter, you can drop it.

The DynamicMesh converter stays in `RealtimeMeshExt`.

## Raw meshoptimizer includes are no longer supported

If any of your code includes `MeshOptimizer/meshoptimizer.h` directly, switch to the canonical `RealtimeMeshMeshOptimizer.h` include.

The underlying library is vendored with a symbol prefix, and only the canonical header is guaranteed to resolve against it.

## Nanite debug console variables renamed

The builder's developer console variables moved from `r.RealtimeMesh.Nanite.*` to `r.TriAxisNanite.RMC.*`.

Update any project config, console command bindings, or automation scripts that use the old names.

## Actor replication is now opt-in

**This is the one to watch for.** Previously, `ARealtimeMeshActor::BeginPlay` force-enabled replication on every Realtime Mesh actor by calling `SetReplicates(true)` unconditionally.

It now respects whatever `bReplicates` you configured. The remote role and physics replication mode are only wired up when `bReplicates` is already `true` on the authority.

If your project relied on the old automatic behaviour, this changes things silently:

* If your Realtime Mesh actors need to replicate, set `bReplicates = true` explicitly, in your class defaults, your Blueprint, or the placed actor's details panel, just like any other actor.
* Actors that never needed replication are no longer forced into the replication graph, which is a straight win.

## Distance field and Lumen card generators removed

`URealtimeMeshDistanceFieldGeneration` and `URealtimeMeshCardRepresentationGenerator`, formerly in `RealtimeMeshExt`, have been removed. They spent an interim period as deprecated shims that funnelled into the shared SDF core. That shim layer is now gone, and calls to the old classes no longer compile.

**Use these instead:**

* `URealtimeMeshSDFLibrary` for the high-level "generate and apply to a mesh" flows.
* The `RealtimeMesh::SDF` API in `RealtimeMeshSDF` directly: `GenerateDistanceFieldCPU` plus `PackToDistanceFieldVolumeData` for distance fields, `GenerateCardRepresentationCPU` for Lumen cards, and their `...Async` variants.

Some behaviour notes when moving over:

* Output is **equivalent, not bit-for-bit identical**. The same `FDistanceFieldVolumeData` and `FMeshCardsBuildData` formats come out, but exact values can differ. Do not rely on reproducing prior output byte for byte.
* There is no pre-built BVH input any more. The core always builds its own internally.
* Card generation derives bounds from the mesh, with no distance field input, and returns **Failure** rather than a zero-card Success when no cards can be produced.
* The old `bUsePointQuery` and `bMultiThreadedGeneration` style options have no equivalent. Generation is always BVH based and internally parallelised.

## RealtimeMeshNoExportTypes.h removed

The key, config, and collision structs and enums this file used to mirror are now reflected directly at their native declarations in the `Core/*.h` headers.

Struct paths and reflected fields are unchanged, so existing Blueprint assets load exactly as before. If your C++ includes `RealtimeMeshNoExportTypes.h` directly, include the relevant `Core/*.h` header instead. Most code already gets these transitively.

This also fixes a class of Blueprint memory corruption caused by the old mirrors registering structs at the wrong size.

## Corrected default for bForceOpaque

The Blueprint and editor default shown for `FRealtimeMeshSectionConfig::bForceOpaque` used to display as `true`, even though the native C++ default has always been `false`. That was a drift between the native struct and its editor-reflected mirror, and it is now fixed. The displayed default is `false`, matching the native default and matching what has always actually happened at runtime.

Runtime behaviour does **not** change. If a Blueprint graph read the previously displayed `true` default and depended on it, set the value explicitly now.
