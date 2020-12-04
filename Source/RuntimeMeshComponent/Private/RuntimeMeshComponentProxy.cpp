// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshComponentProxy.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshProxy.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"
#include "TessellationRendering.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/Material.h"
#include "UnrealEngine.h"
#include "SceneManagement.h"
#include "RayTracingDefinitions.h"
#include "RayTracingInstance.h"


DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponentSceneProxy - Create Mesh Batch"), STAT_RuntimeMeshComponentSceneProxy_CreateMeshBatch, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponentSceneProxy - Get Dynamic Mesh Elements"), STAT_RuntimeMeshComponentSceneProxy_GetDynamicMeshElements, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponentSceneProxy - Draw Static Mesh Elements"), STAT_RuntimeMeshComponentSceneProxy_DrawStaticMeshElements, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponentSceneProxy - Get Dynamic Ray Tracing Instances"), STAT_RuntimeMeshComponentSceneProxy_GetDynamicRayTracingInstances, STATGROUP_RuntimeMesh);

#define RMC_LOG_VERBOSE(Format, ...) \
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("[RMCSP:%d Mesh:%d Thread:%d]: " Format), GetUniqueID(), (RuntimeMeshProxy? RuntimeMeshProxy->GetMeshID() : -1), FPlatformTLS::GetCurrentThreadId(), ##__VA_ARGS__);

FRuntimeMeshComponentSceneProxy::FRuntimeMeshComponentSceneProxy(URuntimeMeshComponent* Component) 
	: FPrimitiveSceneProxy(Component)
	, BodySetup(Component->GetBodySetup())
	, bAnyMaterialUsesDithering(false)
{
	check(Component->GetRuntimeMesh() != nullptr);
	RMC_LOG_VERBOSE("Created");


	URuntimeMesh* Mesh = Component->GetRuntimeMesh();
	RuntimeMeshProxy = Mesh->GetRenderProxy(GetScene().GetFeatureLevel());

	// Fill the section render data
	for (int32 LODIndex = 0; LODIndex < Mesh->LODs.Num(); LODIndex++)
	{
		const FRuntimeMeshLOD& LOD = Mesh->LODs[LODIndex];

		for (const auto& Section : LOD.Sections)
		{
			const FRuntimeMeshSectionProperties& SectionProperties = Section.Value;
			int32 MaterialSlotIndex = SectionProperties.MaterialSlot;

			if (!Materials.Contains(MaterialSlotIndex))
			{
				UMaterialInterface* SlotMaterial = Component->GetMaterial(MaterialSlotIndex);
				if (!SlotMaterial)
				{
					SlotMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
				}
				Materials.Add(MaterialSlotIndex, SlotMaterial);
				MaterialRelevance |= SlotMaterial->GetRelevance(GetScene().GetFeatureLevel());
				bAnyMaterialUsesDithering |= SlotMaterial->IsDitheredLODTransition();
			}		
		}
	}   

	// Disable shadow casting if no section has it enabled.
	//bCastShadow = true;// bCastShadow&& bAnySectionCastsShadows;
	bCastDynamicShadow = true;// bCastDynamicShadow&& bCastShadow;


	const auto FeatureLevel = GetScene().GetFeatureLevel();

	// We always use local vertex factory, which gets its primitive data from GPUScene, so we can skip expensive primitive uniform buffer updates
	bVFRequiresPrimitiveUniformBuffer = !UseGPUScene(GMaxRHIShaderPlatform, FeatureLevel);
	bStaticElementsAlwaysUseProxyPrimitiveUniformBuffer = true;
	bVerifyUsedMaterials = false;

}

FRuntimeMeshComponentSceneProxy::~FRuntimeMeshComponentSceneProxy()
{
	RMC_LOG_VERBOSE("Destroyed");
}

void FRuntimeMeshComponentSceneProxy::CreateRenderThreadResources()
{
	// Make sure the proxy has been updated.
	RuntimeMeshProxy->FlushPendingUpdates();

	FPrimitiveSceneProxy::CreateRenderThreadResources();
}

FPrimitiveViewRelevance FRuntimeMeshComponentSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);

	#if ENGINE_MINOR_VERSION >= 26
		bool bForceDynamicPath = IsRichView(*View->Family) || IsSelected() || View->Family->EngineShowFlags.Wireframe;
	#else
		bool bForceDynamicPath = !IsStaticPathAvailable() || IsRichView(*View->Family) || IsSelected() || View->Family->EngineShowFlags.Wireframe;
	#endif
	Result.bStaticRelevance = !bForceDynamicPath && RuntimeMeshProxy->ShouldRenderStatic();
	Result.bDynamicRelevance = bForceDynamicPath || RuntimeMeshProxy->ShouldRenderDynamic();

	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 25
	Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;
#else
	Result.bVelocityRelevance = IsMovable() && Result.bOpaqueRelevance && Result.bRenderInMainPass;
#endif
	return Result;
}

void FRuntimeMeshComponentSceneProxy::CreateMeshBatch(FMeshBatch& MeshBatch, const FRuntimeMeshSectionProxy& Section, int32 LODIndex, int32 SectionId, UMaterialInterface* Material, FMaterialRenderProxy* WireframeMaterial, bool bForRayTracing) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponentSceneProxy_CreateMeshBatch);

	check(Section.CanRender());
	check(Section.Buffers->VertexFactory.IsInitialized());

	// Should we be rendering in wireframe?
	const bool bRenderWireframe = WireframeMaterial != nullptr;

	// Decide if we should be using adjacency information for this material
	const bool bWantsAdjacencyInfo = !bForRayTracing && !bRenderWireframe && RequiresAdjacencyInformation(Material, Section.Buffers->VertexFactory.GetType(), GetScene().GetFeatureLevel());
	check(!bWantsAdjacencyInfo || Section.bHasAdjacencyInfo);


	// No support for stateless dithered LOD transitions for movable meshes
	const bool bDitheredLODTransition = !IsMovable() && Material->IsDitheredLODTransition();


	const FRuntimeMeshIndexBuffer* CurrentIndexBuffer =
		(bWantsAdjacencyInfo ?
			&Section.Buffers->AdjacencyIndexBuffer :
		&Section.Buffers->IndexBuffer);

	int32 NumIndicesPerTriangle = bWantsAdjacencyInfo ? 12 : 3;
	int32 NumPrimitives = CurrentIndexBuffer->Num() / NumIndicesPerTriangle;

	FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy();

	MeshBatch.VertexFactory = &Section.Buffers->VertexFactory;
	MeshBatch.Type = bWantsAdjacencyInfo ? PT_12_ControlPointPatchList : PT_TriangleList;
	MeshBatch.CastShadow = Section.bCastsShadow;

	MeshBatch.LODIndex = LODIndex;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	MeshBatch.VisualizeLODIndex = LODIndex;
#endif

	MeshBatch.SegmentIndex = SectionId;

	MeshBatch.bDitheredLODTransition = !bForRayTracing && !IsMovable() && MaterialRenderProxy->GetMaterialInterface()->IsDitheredLODTransition();
	MeshBatch.bWireframe = !bForRayTracing && WireframeMaterial != nullptr;

	MeshBatch.MaterialRenderProxy = MeshBatch.bWireframe ? WireframeMaterial : MaterialRenderProxy;
	MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();

	MeshBatch.DepthPriorityGroup = SDPG_World;
	MeshBatch.bCanApplyViewModeOverrides = false;


	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
	BatchElement.IndexBuffer = CurrentIndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = NumPrimitives;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = Section.Buffers->PositionBuffer.Num() - 1;

	if (!bForRayTracing)
	{
		BatchElement.MaxScreenSize = RuntimeMeshProxy->GetScreenSize(LODIndex);
		BatchElement.MinScreenSize = RuntimeMeshProxy->GetScreenSize(LODIndex + 1);
	}
}

void FRuntimeMeshComponentSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponentSceneProxy_DrawStaticMeshElements);

	RMC_LOG_VERBOSE("DrawStaticElements called");

	if (RuntimeMeshProxy->HasValidLODs())
	{
		for (int32 LODIndex = RuntimeMeshProxy->GetMinLOD(); LODIndex <= RuntimeMeshProxy->GetMaxLOD(); LODIndex++)
		{
			FRuntimeMeshLODData& LOD = RuntimeMeshProxy->GetLOD(LODIndex);

			if (LOD.bShouldRenderStatic)
			{
				for (const auto& SectionEntry : LOD.Sections)
				{
					int32 SectionId = SectionEntry.Key;
					auto& Section = SectionEntry.Value;

					if (Section.ShouldRenderStaticPath())
					{
						UMaterialInterface* SectionMat = GetMaterialSlot(Section.MaterialSlot);
						check(SectionMat);

						FMeshBatch MeshBatch;
						MeshBatch.LODIndex = LODIndex;
						MeshBatch.SegmentIndex = SectionEntry.Key;

						CreateMeshBatch(MeshBatch, Section, LODIndex, SectionId, SectionMat, nullptr, false);
						PDI->DrawMesh(MeshBatch, RuntimeMeshProxy->GetScreenSize(LODIndex));

						// Here we add a reference to the buffers so that we can guarantee these stay around for the life of this proxy
						InUseBuffers.Add(Section.Buffers);
					}
				}
			}
		}
	}
}

void FRuntimeMeshComponentSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponentSceneProxy_GetDynamicMeshElements);

	// Set up wireframe material (if needed)
	const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

	FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
	if (bWireframe)
	{
		WireframeMaterialInstance = new FColoredMaterialRenderProxy(GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr, FLinearColor(0, 0.5f, 1.f));

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
	}

	if (RuntimeMeshProxy->HasValidLODs())
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FSceneView* View = Views[ViewIndex];

			#if ENGINE_MINOR_VERSION >= 26
				bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || Views[ViewIndex]->Family->EngineShowFlags.Wireframe || IsSelected();
			#else
				bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || Views[ViewIndex]->Family->EngineShowFlags.Wireframe || IsSelected() || !IsStaticPathAvailable();
			#endif

			if (IsShown(View) && (VisibilityMap & (1 << ViewIndex)))
			{
				FFrozenSceneViewMatricesGuard FrozenMatricesGuard(*const_cast<FSceneView*>(Views[ViewIndex]));

				FLODMask LODMask = GetLODMask(View);

				for (int32 LODIndex = RuntimeMeshProxy->GetMinLOD(); LODIndex <= RuntimeMeshProxy->GetMaxLOD(); LODIndex++)
				{
					FRuntimeMeshLODData& LOD = RuntimeMeshProxy->GetLOD(LODIndex);

					if (LODMask.ContainsLOD(LODIndex) && (LOD.bShouldRenderDynamic || bForceDynamicPath))
					{
						for (const auto& SectionEntry : LOD.Sections)
						{
							int32 SectionId = SectionEntry.Key;
							auto& Section = SectionEntry.Value;

							if (Section.ShouldRender() && (Section.ShouldRenderDynamicPath() || bForceDynamicPath))
							{
								UMaterialInterface* SectionMat = GetMaterialSlot(Section.MaterialSlot);
								check(SectionMat);

								FMeshBatch& MeshBatch = Collector.AllocateMesh();
								CreateMeshBatch(MeshBatch, Section, LODIndex, SectionId, SectionMat, WireframeMaterialInstance, false);
								MeshBatch.bDitheredLODTransition = LODMask.IsDithered();

								Collector.AddMesh(ViewIndex, MeshBatch);
							}
						}
					}
				}
			}
		}
		// Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				// Draw simple collision as wireframe if 'show collision', and collision is enabled, and we are not using the complex as the simple
				if (ViewFamily.EngineShowFlags.Collision && IsCollisionEnabled() && BodySetup && BodySetup->GetCollisionTraceFlag() != ECollisionTraceFlag::CTF_UseComplexAsSimple)
				{
					FTransform GeomTransform(GetLocalToWorld());
					BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255), IsSelected(), IsHovered()).ToFColor(true), NULL, false, false, DrawsVelocity(), ViewIndex, Collector);
				}

				// Render bounds
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
		}
#endif
	}
}


// This function was exposed on 4.24, previously it existed but wasn't public
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 24
 static float ComputeBoundsScreenRadiusSquared(const FVector4& BoundsOrigin, const float SphereRadius, const FVector4& ViewOrigin, const FMatrix& ProjMatrix)
 {
 	const float DistSqr = FVector::DistSquared(BoundsOrigin, ViewOrigin);
 
 	// Get projection multiple accounting for view scaling.
 	const float ScreenMultiple = FMath::Max(0.5f * ProjMatrix.M[0][0], 0.5f * ProjMatrix.M[1][1]);
 
 	// Calculate screen-space projected radius
 	return FMath::Square(ScreenMultiple * SphereRadius) / FMath::Max(1.0f, DistSqr);
 }
#endif


#if RHI_RAYTRACING
void FRuntimeMeshComponentSceneProxy::GetDynamicRayTracingInstances(struct FRayTracingMaterialGatheringContext& Context, TArray<struct FRayTracingInstance>& OutRayTracingInstances)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponentSceneProxy_GetDynamicRayTracingInstances);

	// TODO: Should this use any LOD determination logic? Or always use a specific LOD?
	int32 LODIndex = 0;
	
	if (RuntimeMeshProxy->HasValidLODs())
	{
		auto& LOD = RuntimeMeshProxy->GetLOD(LODIndex);

		for (const auto& SectionEntry : LOD.Sections)
		{
			int32 SectionId = SectionEntry.Key;
			auto& Section = SectionEntry.Value;

			if (Section.ShouldRenderDynamicPathRayTracing())
			{
				UMaterialInterface* SectionMat = GetMaterialSlot(Section.MaterialSlot);
				check(SectionMat);

				FRayTracingGeometry* SectionRayTracingGeometry = &Section.Buffers->RayTracingGeometry;

				if (SectionRayTracingGeometry->RayTracingGeometryRHI->IsValid())
				{
					check(SectionRayTracingGeometry->Initializer.TotalPrimitiveCount > 0);
					check(SectionRayTracingGeometry->Initializer.IndexBuffer.IsValid());

					FRayTracingInstance RayTracingInstance;
					RayTracingInstance.Geometry = SectionRayTracingGeometry;
					RayTracingInstance.InstanceTransforms.Add(GetLocalToWorld());

					FMeshBatch MeshBatch;
					CreateMeshBatch(MeshBatch, Section, LODIndex, SectionId, SectionMat, nullptr, true);


					MeshBatch.CastRayTracedShadow = IsShadowCast(Context.ReferenceView);


					RayTracingInstance.Materials.Add(MeshBatch);
					RayTracingInstance.BuildInstanceMaskAndFlags();
					OutRayTracingInstances.Add(RayTracingInstance);
				}
			}
		}
	}
}
#endif // RHI_RAYTRACING

 int8 FRuntimeMeshComponentSceneProxy::GetCurrentFirstLOD() const
 {
	 return RuntimeMeshProxy->GetMaxLOD();
 }

int8 FRuntimeMeshComponentSceneProxy::ComputeTemporalStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale, int32 SampleIndex) const
{
	const int32 NumLODs = RUNTIMEMESH_MAXLODS;

	const float ScreenRadiusSquared = ComputeBoundsScreenRadiusSquared(Origin, SphereRadius, View.GetTemporalLODOrigin(SampleIndex), View.ViewMatrices.GetProjectionMatrix())
		* FactorScale * FactorScale * View.LODDistanceFactor * View.LODDistanceFactor;

	// Walk backwards and return the first matching LOD
	for (int32 LODIndex = NumLODs - 1; LODIndex >= 0; --LODIndex)
	{
		if (FMath::Square(RuntimeMeshProxy->GetScreenSize(LODIndex) * 0.5f) > ScreenRadiusSquared)
		{
			return LODIndex;
		}
	}

	return MinLOD;
}

int8 FRuntimeMeshComponentSceneProxy::ComputeStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale) const
{
	const int32 NumLODs = RUNTIMEMESH_MAXLODS;
	const FSceneView& LODView = GetLODView(View);
	const float ScreenRadiusSquared = ComputeBoundsScreenRadiusSquared(Origin, SphereRadius, LODView) * FactorScale * FactorScale * LODView.LODDistanceFactor * LODView.LODDistanceFactor;

	// Walk backwards and return the first matching LOD
	for (int32 LODIndex = NumLODs - 1; LODIndex >= 0; --LODIndex)
	{
		if (FMath::Square(RuntimeMeshProxy->GetScreenSize(LODIndex) * 0.5f) > ScreenRadiusSquared)
		{
			return FMath::Max(LODIndex, MinLOD);
		}
	}

	return MinLOD;
}



FLODMask FRuntimeMeshComponentSceneProxy::GetLODMask(const FSceneView* View) const
{
	FLODMask Result;

	if (!RuntimeMeshProxy.IsValid())
	{
		RMC_LOG_VERBOSE("GetLODMask failed! No bound proxy.");
		Result.SetLOD(0);
	}
	else
	{
		if (View->DrawDynamicFlags & EDrawDynamicFlags::ForceLowestLOD)
		{
			Result.SetLOD(RuntimeMeshProxy->GetMaxLOD());
		}
#if WITH_EDITOR
		else if (View->Family && View->Family->EngineShowFlags.LOD == 0)
		{
			Result.SetLOD(0);
		}
#endif
		else
		{
			const FBoxSphereBounds& ProxyBounds = GetBounds();
			bool bUseDithered = RuntimeMeshProxy->GetMaxLOD() != INDEX_NONE && bAnyMaterialUsesDithering;

			FCachedSystemScalabilityCVars CachedSystemScalabilityCVars = GetCachedScalabilityCVars();

			float InvScreenSizeScale = (CachedSystemScalabilityCVars.StaticMeshLODDistanceScale != 0.f) ? (1.0f / CachedSystemScalabilityCVars.StaticMeshLODDistanceScale) : 1.0f;

			int32 ClampedMinLOD = 0;

			if (bUseDithered)
			{
				for (int32 Sample = 0; Sample < 2; Sample++)
				{
					Result.SetLODSample(ComputeTemporalStaticMeshLOD(ProxyBounds.Origin, ProxyBounds.SphereRadius, *View, ClampedMinLOD, InvScreenSizeScale, Sample), Sample);
				}
			}
			else
			{
				Result.SetLOD(ComputeStaticMeshLOD(ProxyBounds.Origin, ProxyBounds.SphereRadius, *View, ClampedMinLOD, InvScreenSizeScale));
			}
		}
	}

	return Result;
}

int32 FRuntimeMeshComponentSceneProxy::GetLOD(const FSceneView* View) const
{
	const FBoxSphereBounds& ProxyBounds = GetBounds();
	FCachedSystemScalabilityCVars CachedSystemScalabilityCVars = GetCachedScalabilityCVars();

	float InvScreenSizeScale = (CachedSystemScalabilityCVars.StaticMeshLODDistanceScale != 0.f) ? (1.0f / CachedSystemScalabilityCVars.StaticMeshLODDistanceScale) : 1.0f;

	return ComputeStaticMeshLOD(ProxyBounds.Origin, ProxyBounds.SphereRadius, *View, 0, InvScreenSizeScale);
}



#undef RMC_LOG_VERBOSE