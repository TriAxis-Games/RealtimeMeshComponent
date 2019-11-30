// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshSectionProxy.h"


/**
 *
 */
class FRuntimeMeshProxy
{
	ERHIFeatureLevel::Type FeatureLevel;

	TArray<TSharedPtr<FRuntimeMeshLODProxy>, TInlineAllocator<RuntimeMesh_MAXLODS>> LODs;

public:
	FRuntimeMeshProxy(ERHIFeatureLevel::Type InFeatureLevel);
	~FRuntimeMeshProxy();

	ERHIFeatureLevel::Type GetFeatureLevel() const { return FeatureLevel; }

	void ConfigureLOD_GameThread(uint8 LODIndex, const FRuntimeMeshLODProperties& InProperties);
	void ClearLOD_GameThread(uint8 LODIndex);

	void CreateSection_GameThread(uint8 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties);
	void UpdateSectionProperties_GameThread(uint8 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties);
	void UpdateSection_GameThread(uint8 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData);
	void ClearSection_GameThread(uint8 LODIndex, int32 SectionId);
	void RemoveSection_GameThread(uint8 LODIndex, int32 SectionId);

	void ConfigureLOD_RenderThread(uint8 LODIndex, const FRuntimeMeshLODProperties& InProperties);
	void ClearLOD_RenderThread(uint8 LODIndex);

	void CreateSection_RenderThread(uint8 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties);
	void UpdateSectionProperties_RenderThread(uint8 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties);
	void UpdateSection_RenderThread(uint8 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData);
	void ClearSection_RenderThread(uint8 LODIndex, int32 SectionId);
	void RemoveSection_RenderThread(uint8 LODIndex, int32 SectionId);




	TArray<TSharedPtr<FRuntimeMeshLODProxy>, TInlineAllocator<RuntimeMesh_MAXLODS>>& GetLODs() { return LODs; }

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

