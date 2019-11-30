// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshData.h"
#include "RealtimeMeshSectionProxy.h"

class UBodySetup;
class URealtimeMeshComponent;

/** Runtime mesh scene proxy */
class FRealtimeMeshComponentSceneProxy : public FPrimitiveSceneProxy
{
private:
	struct FRealtimeMeshSectionRenderData
	{
		UMaterialInterface* Material;
		bool bWantsAdjacencyInfo;
	};


	FRealtimeMeshProxyPtr RealtimeMeshProxy;

	TArray<TMap<int32, FRealtimeMeshSectionRenderData>, TInlineAllocator<REALTIMEMESH_MAXLODS>> SectionRenderData;

	// Reference to the body setup for rendering.
	UBodySetup* BodySetup;

	FMaterialRelevance MaterialRelevance;

	bool bHasStaticSections;
	bool bHasDynamicSections;
	bool bHasShadowableSections;

public:

	/*Constructor, copies the whole mesh data to feed to UE */
	FRealtimeMeshComponentSceneProxy(URealtimeMeshComponent* Component);

	virtual ~FRealtimeMeshComponentSceneProxy();

	void CreateRenderThreadResources() override;

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	void CreateMeshBatch(FMeshBatch& MeshBatch, const FRealtimeMeshSectionProxy& Section, int32 LODIndex, const FRealtimeMeshSectionRenderData& RenderData, FMaterialRenderProxy* Material, FMaterialRenderProxy* WireframeMaterial) const;

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}

	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}
};