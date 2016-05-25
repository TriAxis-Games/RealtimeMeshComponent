// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RuntimeMeshComponent.h"

DECLARE_STATS_GROUP(TEXT("RuntimeMesh"), STATGROUP_RuntimeMesh, STATCAT_Advanced);

// Scene Proxy Profiling
DECLARE_CYCLE_STAT(TEXT("Apply Batch Update (RT)"), STAT_RuntimeMesh_ApplyBatchUpdate_RenderThread, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Create Section (RT)"), STAT_RuntimeMesh_CreateSection_RenderThread, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Update Section (RT)"), STAT_RuntimeMesh_UpdateSection_RenderThread, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Set Section Visibility (RT)"), STAT_RuntimeMesh_SetSectionVisibility_RenderThread, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Set Section Casts Shadow (RT)"), STAT_RuntimeMesh_SetSectionCastsShadow_RenderThread, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("On Transform Changed (RT)"), STAT_RuntimeMesh_OnTransformChanged, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Draw Static Elements (RT)"), STAT_RuntimeMesh_DrawStaticElements, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Get Dynamic Mesh Elements (RT)"), STAT_RuntimeMesh_GetDynamicMeshElements, STATGROUP_RuntimeMesh);

// RuntimeMeshComponent Profiling
DECLARE_CYCLE_STAT(TEXT("Update Local Bounds (GT)"), STAT_RuntimeMesh_UpdateLocalBounds, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Finish Create Section (GT)"), STAT_RuntimeMesh_FinishCreateSectionInternal, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Finish Update Section (GT)"), STAT_RuntimeMesh_FinishUpdateSectionInternal, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Clear Mesh Section (GT)"), STAT_RuntimeMesh_ClearMeshSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Set Mesh Collision Section (GT)"), STAT_RuntimeMesh_SetMeshCollisionSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Clear Mesh Collision Section (GT)"), STAT_RuntimeMesh_ClearMeshCollisionSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Clear All Mesh Collision Sections (GT)"), STAT_RuntimeMesh_ClearAllMeshCollisionSections, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Add Collision Convex Mesh (GT)"), STAT_RuntimeMesh_AddCollisionConvexMesh, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Clear Collision Convex Mesh (GT)"), STAT_RuntimeMesh_ClearCollisionConvexMeshes, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Set Collision Convex Meshes (GT)"), STAT_RuntimeMesh_SetCollisionConvexMeshes, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Create Scene Proxy (GT)"), STAT_RuntimeMesh_CreateSceneProxy, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Get Physics TriMesh Data (GT)"), STAT_RuntimeMesh_GetPhysicsTriMeshData, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Update Collision (GT)"), STAT_RuntimeMesh_UpdateCollision, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("Serialize"), STAT_RuntimeMesh_Serialize, STATGROUP_RuntimeMesh);



DECLARE_CYCLE_STAT(TEXT("CreateMeshSection<VertexType> (GT)"), STAT_RuntimeMesh_CreateMeshSection_VertexType, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("CreateMeshSection<VertexType> (Move) (GT)"), STAT_RuntimeMesh_CreateMeshSection_VertexType_Move, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("UpdateMeshSection<VertexType> (GT)"), STAT_RuntimeMesh_UpdateMeshSection_VertexType, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("UpdateMeshSection<VertexType> (Move) (GT)"), STAT_RuntimeMesh_UpdateMeshSection_VertexType_Move, STATGROUP_RuntimeMesh);