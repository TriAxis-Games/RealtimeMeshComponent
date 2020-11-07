// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshSectionProxy.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"


/**
 *
 */
class FRuntimeMeshProxy : public TSharedFromThis<FRuntimeMeshProxy, ESPMode::ThreadSafe>
{
	TInlineLODArray<FRuntimeMeshLODData> LODs;



	FThreadSafeBool IsQueuedForUpdate;
	TQueue<TFunction<void()>, EQueueMode::Mpsc> PendingUpdates;


	uint32 bShouldRender : 1;
	uint32 bShouldRenderStatic : 1;
	uint32 bShouldRenderDynamic : 1;
	uint32 bShouldRenderShadow : 1;

	int8 MinAvailableLOD;
	int8 MaxAvailableLOD;

	int8 CurrentForcedLOD;

	FRuntimeMeshObjectId<FRuntimeMeshProxy> ObjectId;

	uint32 ParentMeshId;
public:
	FRuntimeMeshProxy(uint32 InParentMeshId);
	~FRuntimeMeshProxy();

	int32 GetUniqueID() const { return ObjectId.Get(); }
	int32 GetMeshID() const { return ParentMeshId; }

	bool ShouldRender() const { return bShouldRender; }
	bool ShouldRenderStatic() const { return bShouldRenderStatic; }
	bool ShouldRenderDynamic() const { return bShouldRenderDynamic; }
	bool ShouldRenderShadow() const { return bShouldRenderShadow; }

	int32 GetMinLOD() const { return MinAvailableLOD; }
	int32 GetMaxLOD() const { return MaxAvailableLOD; }

	bool HasValidLODs() const { return MinAvailableLOD != INDEX_NONE && MaxAvailableLOD != INDEX_NONE && MaxAvailableLOD >= MinAvailableLOD; }

	int32 GetForcedLOD() const { return CurrentForcedLOD; }

	FRuntimeMeshLODData& GetLOD(int32 LODIndex) { return LODs[LODIndex]; }

	float GetScreenSize(int32 LODIndex) const;


	void QueueForUpdate();
	void FlushPendingUpdates();


	void ResetProxy_GameThread();

	void InitializeLODs_GameThread(const TArray<FRuntimeMeshLODProperties>& InProperties);
	void ClearAllSectionsForLOD_GameThread(int32 LODIndex);
	void RemoveAllSectionsForLOD_GameThread(int32 LODIndex);

	void CreateOrUpdateSection_GameThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset);
	void SetSectionsForLOD_GameThread(int32 LODIndex, const TMap<int32, FRuntimeMeshSectionProperties>& InProperties, bool bShouldReset);
	void UpdateSectionMesh_GameThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshSectionUpdateData>& MeshData);
	void UpdateMultipleSectionsMesh_GameThread(int32 LODIndex, const TMap<int32, TSharedPtr<FRuntimeMeshSectionUpdateData>>& MeshData);
	void ClearSection_GameThread(int32 LODIndex, int32 SectionId);
	void RemoveSection_GameThread(int32 LODIndex, int32 SectionId);


	void ResetProxy_RenderThread();

	void InitializeLODs_RenderThread(const TArray<FRuntimeMeshLODProperties>& InProperties);
	void ClearAllSectionsForLOD_RenderThread(int32 LODIndex);
	void RemoveAllSectionsForLOD_RenderThread(int32 LODIndex);

	void CreateOrUpdateSection_RenderThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset);
	void SetSectionsForLOD_RenderThread(int32 LODIndex, const TMap<int32, FRuntimeMeshSectionProperties>& InProperties, bool bShouldReset);
	void UpdateSectionMesh_RenderThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshSectionUpdateData>& MeshData);
	void UpdateMultipleSectionsMesh_RenderThread(int32 LODIndex, const TMap<int32, TSharedPtr<FRuntimeMeshSectionUpdateData>>& MeshData);
	void ClearSection_RenderThread(int32 LODIndex, int32 SectionId);
	void RemoveSection_RenderThread(int32 LODIndex, int32 SectionId);


	void UpdateRenderState();

	void ClearSection(FRuntimeMeshSectionProxy& Section);

	void ApplyMeshToSection(int32 LODIndex, int32 SectionId, FRuntimeMeshSectionProxy& Section, FRuntimeMeshSectionUpdateData&& MeshData);

};

