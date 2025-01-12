// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

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
		TSharedPtr<uint8> MeshReferencingHandle;

		// All the in use materials
		FRealtimeMeshMaterialProxyMap MaterialMap;

		// Reference all the in-use buffers so that as long as this proxy is around these buffers will be too. 
		// This is meant only for statically drawn sections. Dynamically drawn sections can update safely in place.
		// Static sections get new buffers on each update.
		FRealtimeMeshResourceReferenceList StaticResources;

		// Reference to the body setup for rendering.
		UBodySetup* BodySetup;

		// Store the combined material relevance.
		FMaterialRelevance MaterialRelevance;

		uint32 bAnyMaterialUsesDithering : 1;
		uint32 bSupportsRayTracing : 1;

	public:
		/*Constructor, copies the whole mesh data to feed to UE */
		FRealtimeMeshComponentSceneProxy(URealtimeMeshComponent* Component, const FRealtimeMeshProxyRef& InRealtimeMeshProxy);

		virtual ~FRealtimeMeshComponentSceneProxy() override;

#if RMC_ENGINE_ABOVE_5_4
		virtual void CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override;
#else
		virtual void CreateRenderThreadResources() override;
#endif

		virtual bool CanBeOccluded() const override;

		virtual int32 GetLOD(const FSceneView* View) const override;

		virtual uint32 GetMemoryFootprint(void) const override;


		virtual SIZE_T GetTypeHash() const override;

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

		virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;


		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
		                                    FMeshElementCollector& Collector) const override;

		virtual void GetDistanceFieldAtlasData(const FDistanceFieldVolumeData*& OutDistanceFieldData, float& SelfShadowBias) const override;
		virtual void GetDistanceFieldInstanceData(TArray<FRenderTransform>& InstanceLocalToPrimitiveTransforms) const override;
		virtual bool HasDistanceFieldRepresentation() const override;
		virtual bool HasDynamicIndirectShadowCasterRepresentation() const override;

		virtual const FCardRepresentationData* GetMeshCardRepresentation() const override;
		
#if RHI_RAYTRACING
		virtual bool IsRayTracingRelevant() const override { return true; }
		virtual bool HasRayTracingRepresentation() const override { return bSupportsRayTracing; }
		virtual bool IsRayTracingStaticRelevant() const override;

#if RMC_ENGINE_ABOVE_5_4
		virtual TArray<FRayTracingGeometry*> GetStaticRayTracingGeometries() const override;
#endif
		
		/** Gathers dynamic ray tracing instances from this proxy. */
#if RMC_ENGINE_ABOVE_5_5
		virtual void GetDynamicRayTracingInstances(class FRayTracingInstanceCollector& Collector) override;
#else
		virtual void GetDynamicRayTracingInstances(struct FRayTracingMaterialGatheringContext& Context, TArray<struct FRayTracingInstance>& OutRayTracingInstances) override;
#endif
#endif // RHI_RAYTRACING

	protected:
		SIZE_T GetAllocatedSize(void) const;

		int8 GetCurrentFirstLOD() const;

		int8 ComputeTemporalStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale, int32 SampleIndex) const;
		int8 ComputeStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale) const;
		FLODMask GetLODMask(const FSceneView* View) const;
	};
}
