// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshSectionProxy.h"
#if RMC_ENGINE_ABOVE_5_2
#include "PrimitiveSceneProxy.h"
#endif

class UBodySetup;
class URealtimeMeshComponent;

namespace RealtimeMesh
{
	/** Runtime mesh scene proxy */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshComponentSceneProxy : public FPrimitiveSceneProxy
	{
	private:
		// THis is the proxy we're rendering
		FRealtimeMeshProxyRef RealtimeMeshProxy;

		// All the in use materials
		TMap<int32, TTuple<FMaterialRenderProxy*, bool>> Materials;

		// Reference all the in-use buffers so that as long as this proxy is around these buffers will be too. 
		// This is meant only for statically drawn sections. Dynamically drawn sections can update safely in place.
		// Static sections get new buffers on each update.
		TArray<TSharedRef<FRenderResource>> InUseBuffers;

		// Reference to the body setup for rendering.
		UBodySetup* BodySetup;

		// Store the combined material relevance.
		FMaterialRelevance MaterialRelevance;

		bool bAnyMaterialUsesDithering;

	public:
		/*Constructor, copies the whole mesh data to feed to UE */
		FRealtimeMeshComponentSceneProxy(URealtimeMeshComponent* Component, const FRealtimeMeshProxyRef& InRealtimeMeshProxy);

		virtual ~FRealtimeMeshComponentSceneProxy() override;

		virtual bool CanBeOccluded() const override;

		virtual int32 GetLOD(const FSceneView* View) const override;

		virtual uint32 GetMemoryFootprint(void) const override;


		virtual SIZE_T GetTypeHash() const override;

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

		virtual void CreateRenderThreadResources() override;

		virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
		                                    FMeshElementCollector& Collector) const override;


#if RHI_RAYTRACING
		virtual bool IsRayTracingRelevant() const override { return true; }
		virtual bool IsRayTracingStaticRelevant() const { return false; }

		/** Gathers dynamic ray tracing instances from this proxy. */
		virtual void GetDynamicRayTracingInstances(struct FRayTracingMaterialGatheringContext& Context, TArray<struct FRayTracingInstance>& OutRayTracingInstances);

#endif // RHI_RAYTRACING

	protected:
		SIZE_T GetAllocatedSize(void) const;

		FMaterialRenderProxy* GetMaterialSlot(int32 MaterialSlotId) const;


		// void CreateMeshBatch(TArray<TSharedRef<FRealtimeMeshRenderingBuffers>>* InUseBuffers, FMeshBatch& MeshBatch, const FRealtimeMeshLODProxy& LOD, const FLODMask& LODMask, const TScreenSizeLimits& ScreenSizeLimits,
		// 	int32 SectionID, const FMaterialRenderProxy* WireframeMaterial, const FMaterialRenderProxy* SectionMaterial, bool bSupportsDithering) const;


		int8 GetCurrentFirstLOD() const;

		int8 ComputeTemporalStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale, int32 SampleIndex) const;
		int8 ComputeStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale) const;
		FLODMask GetLODMask(const FSceneView* View) const;
	};
}
