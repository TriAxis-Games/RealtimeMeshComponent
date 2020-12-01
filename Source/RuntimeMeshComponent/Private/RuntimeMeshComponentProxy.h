// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMesh.h"
#include "RuntimeMeshSectionProxy.h"
#include "Materials/Material.h"

class UBodySetup;
class URuntimeMeshComponent;

/** Runtime mesh scene proxy */
class FRuntimeMeshComponentSceneProxy : public FPrimitiveSceneProxy
{
private:
	// THis is the proxy we're rendering
	FRuntimeMeshProxyPtr RuntimeMeshProxy;

	// All the in use materials
	TMap<int32, UMaterialInterface*> Materials;

	// Reference all the in-use buffers so that as long as this proxy is around these buffers will be too. 
	// This is meant only for statically drawn sections. Dynamically drawn sections can update safely in place.
	// Static sections get new buffers on each update.
	TArray<TSharedPtr<FRuntimeMeshSectionProxyBuffers>> InUseBuffers;

	// Reference to the body setup for rendering.
	UBodySetup* BodySetup;

	// Store the combined material relevance.
	FMaterialRelevance MaterialRelevance;

	FRuntimeMeshObjectId<FRuntimeMeshComponentSceneProxy> ObjectId;

	bool bAnyMaterialUsesDithering;
public:

	/*Constructor, copies the whole mesh data to feed to UE */
	FRuntimeMeshComponentSceneProxy(URuntimeMeshComponent* Component);

	virtual ~FRuntimeMeshComponentSceneProxy();

	int32 GetUniqueID() const { return ObjectId.Get(); }

	UMaterialInterface* GetMaterialSlot(int32 MaterialSlotId) const
	{
		UMaterialInterface*const* Mat = Materials.Find(MaterialSlotId);
		if (Mat && *Mat)
		{
			return *Mat;
		}
		
		return UMaterial::GetDefaultMaterial(MD_Surface);
	}

	void CreateRenderThreadResources() override;

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	void CreateMeshBatch(FMeshBatch& MeshBatch, const FRuntimeMeshSectionProxy& Section, int32 LODIndex, int32 SectionId, UMaterialInterface* Material, FMaterialRenderProxy* WireframeMaterial, bool bForRayTracing) const;

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;


#if RHI_RAYTRACING
	virtual bool IsRayTracingRelevant() const { return true; }
	virtual bool IsRayTracingStaticRelevant() const { return false; }

	/** Gathers dynamic ray tracing instances from this proxy. */
	virtual void GetDynamicRayTracingInstances(struct FRayTracingMaterialGatheringContext& Context, TArray<struct FRayTracingInstance>& OutRayTracingInstances);

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