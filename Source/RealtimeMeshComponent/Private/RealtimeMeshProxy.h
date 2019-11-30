// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshSectionProxy.h"


/**
 *
 */
class FRealtimeMeshProxy
{
	ERHIFeatureLevel::Type FeatureLevel;

	TArray<TSharedPtr<FRealtimeMeshLODProxy>, TInlineAllocator<REALTIMEMESH_MAXLODS>> LODs;

public:
	FRealtimeMeshProxy(ERHIFeatureLevel::Type InFeatureLevel);
	~FRealtimeMeshProxy();

	ERHIFeatureLevel::Type GetFeatureLevel() const { return FeatureLevel; }

	void ConfigureLOD_GameThread(uint8 LODIndex, const FRealtimeMeshLODProperties& InProperties);
	void ClearLOD_GameThread(uint8 LODIndex);

	void CreateSection_GameThread(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& InProperties);
	void UpdateSectionProperties_GameThread(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& InProperties);
	void UpdateSection_GameThread(uint8 LODIndex, int32 SectionId, const TSharedPtr<FRealtimeMeshRenderableMeshData>& MeshData);
	void ClearSection_GameThread(uint8 LODIndex, int32 SectionId);
	void RemoveSection_GameThread(uint8 LODIndex, int32 SectionId);

	void ConfigureLOD_RenderThread(uint8 LODIndex, const FRealtimeMeshLODProperties& InProperties);
	void ClearLOD_RenderThread(uint8 LODIndex);

	void CreateSection_RenderThread(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& InProperties);
	void UpdateSectionProperties_RenderThread(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& InProperties);
	void UpdateSection_RenderThread(uint8 LODIndex, int32 SectionId, const TSharedPtr<FRealtimeMeshRenderableMeshData>& MeshData);
	void ClearSection_RenderThread(uint8 LODIndex, int32 SectionId);
	void RemoveSection_RenderThread(uint8 LODIndex, int32 SectionId);




	TArray<TSharedPtr<FRealtimeMeshLODProxy>, TInlineAllocator<REALTIMEMESH_MAXLODS>>& GetLODs() { return LODs; }

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

