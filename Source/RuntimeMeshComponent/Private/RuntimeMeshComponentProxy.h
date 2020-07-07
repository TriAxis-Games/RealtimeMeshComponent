// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMesh.h"
#include "RuntimeMeshSectionProxy.h"

class UBodySetup;
class URuntimeMeshComponent;

/** Runtime mesh scene proxy */
class FRuntimeMeshComponentSceneProxy : public FPrimitiveSceneProxy
{
private:
	struct FRuntimeMeshSectionRenderData
	{
		UMaterialInterface* Material;
		bool bWantsAdjacencyInfo;
	};


	// THis is the proxy we're rendering
	FRuntimeMeshProxyPtr RuntimeMeshProxy;

	// Extra section data, mostly material data after being combined with override materials on the component
	TArray<TMap<int32, FRuntimeMeshSectionRenderData>, TInlineAllocator<RUNTIMEMESH_MAXLODS>> SectionRenderData;

	// Reference all the in-use buffers so that as long as this proxy is around these buffers will be too. 
	// This is meant only for statically drawn sections. Dynamically drawn sections can update safely in place.
	// Static sections get new buffers on each update.
	TArray<TSharedPtr<FRuntimeMeshSectionProxyBuffers>> InUseBuffers;

	// Reference to the body setup for rendering.
	UBodySetup* BodySetup;

	// Store the combined material relevance.
	FMaterialRelevance MaterialRelevance;

	FRuntimeMeshObjectId<FRuntimeMeshComponentSceneProxy> ObjectId;
public:

	/*Constructor, copies the whole mesh data to feed to UE */
	FRuntimeMeshComponentSceneProxy(URuntimeMeshComponent* Component);

	virtual ~FRuntimeMeshComponentSceneProxy();

	int32 GetUniqueID() const { return ObjectId.Get(); }

	void CreateRenderThreadResources() override;

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	void CreateMeshBatch(FMeshBatch& MeshBatch, const FRuntimeMeshSectionProxy& Section, int32 LODIndex, const FRuntimeMeshSectionRenderData& RenderData, FMaterialRenderProxy* Material, FMaterialRenderProxy* WireframeMaterial) const;

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

#if RHI_RAYTRACING
	virtual bool IsRayTracingRelevant() const { return false; }
	virtual bool IsRayTracingStaticRelevant() const { return false; }

	/** Gathers dynamic ray tracing instances from this proxy. */
	//virtual void GetDynamicRayTracingInstances(struct FRayTracingMaterialGatheringContext& Context, TArray<struct FRayTracingInstance>& OutRayTracingInstances);

#endif // RHI_RAYTRACING

	int8 GetCurrentFirstLOD() const;

	int8 ComputeTemporalStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale, int32 SampleIndex) const;
	int8 ComputeStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale) const;
	FLODMask GetLODMask(const FSceneView* View) const;

	virtual int32 GetLOD(const FSceneView* View) const;

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