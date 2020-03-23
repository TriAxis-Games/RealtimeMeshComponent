// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshSectionProxy.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"


/**
 *
 */
class FRuntimeMeshProxy
{
	ERHIFeatureLevel::Type FeatureLevel;

	TInlineLODArray<TSharedPtr<FRuntimeMeshLODProxy>> LODs;

	FThreadSafeBool IsQueuedForUpdate;
	TQueue<TFunction<void()>, EQueueMode::Mpsc> PendingUpdates;
public:
	FRuntimeMeshProxy(ERHIFeatureLevel::Type InFeatureLevel);
	~FRuntimeMeshProxy();

	ERHIFeatureLevel::Type GetFeatureLevel() const { return FeatureLevel; }

	int32 GetMaxLOD() 
	{
		return LODs.Num() - 1;
	}
	float GetScreenSize(int32 LODIndex);

	void QueueForUpdate();
	void FlushPendingUpdates();

	void InitializeLODs_GameThread(const TArray<FRuntimeMeshLODProperties>& InProperties);
	void ClearAllSectionsForLOD_GameThread(int32 LODIndex);
	void RemoveAllSectionsForLOD_GameThread(int32 LODIndex);

	void CreateOrUpdateSection_GameThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset);
	void UpdateSectionMesh_GameThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData);
	void ClearAllSections_GameThread(int32 LODIndex);
	void ClearSection_GameThread(int32 LODIndex, int32 SectionId);
	void RemoveAllSections_GameThread(int32 LODIndex);
	void RemoveSection_GameThread(int32 LODIndex, int32 SectionId);

	void InitializeLODs_RenderThread(const TArray<FRuntimeMeshLODProperties>& InProperties);
	void ClearAllSectionsForLOD_RenderThread(int32 LODIndex);
	void RemoveAllSectionsForLOD_RenderThread(int32 LODIndex);

	void CreateOrUpdateSection_RenderThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset);
	void UpdateSectionMesh_RenderThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData);
	void ClearAllSections_RenderThread(int32 LODIndex);
	void ClearSection_RenderThread(int32 LODIndex, int32 SectionId);
	void RemoveAllSections_RenderThread(int32 LODIndex);
	void RemoveSection_RenderThread(int32 LODIndex, int32 SectionId);


	TInlineLODArray<TSharedPtr<FRuntimeMeshLODProxy>>& GetLODs() { return LODs; }

	void CalculateViewRelevance(bool& bHasStaticSections, bool& bHasDynamicSections, bool& bHasShadowableSections)
	{
		check(IsInRenderingThread());
		bHasStaticSections = false;
		bHasDynamicSections = false;
		bHasShadowableSections = false;
		for (const auto& LOD : LODs)
		{
			if (LOD.IsValid())
			{
				bHasStaticSections |= LOD->HasAnyStaticPath();
				bHasDynamicSections |= LOD->HasAnyDynamicPath();
				bHasShadowableSections = LOD->HasAnyShadowCasters();
			}
		}
	}
private:

};

