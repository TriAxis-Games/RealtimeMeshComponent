// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentProxy.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "TessellationRendering.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/Material.h"
#include "UnrealEngine.h"
#include "SceneManagement.h"


DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponentSceneProxy - Create Mesh Batch"), STAT_RuntimeMeshComponentSceneProxy_CreateMeshBatch, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponentSceneProxy - Get Dynamic Mesh Elements"), STAT_RuntimeMeshComponentSceneProxy_GetDynamicMeshElements, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponentSceneProxy - Draw Static Mesh Elements"), STAT_RuntimeMeshComponentSceneProxy_DrawStaticMeshElements, STATGROUP_RuntimeMesh);


FRuntimeMeshComponentSceneProxy::FRuntimeMeshComponentSceneProxy(URuntimeMeshComponent* Component) 
	: FPrimitiveSceneProxy(Component)
	, BodySetup(Component->GetBodySetup())
{
	check(Component->GetRuntimeMesh() != nullptr);


	FRuntimeMeshDataPtr RuntimeMeshData = Component->GetRuntimeMesh()->Data;

	RuntimeMeshProxy = Component->GetRuntimeMesh()->GetRenderProxy(GetScene().GetFeatureLevel());

	UMaterialInterface* DefaultMat = UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface);

	// Fill the section render data
	SectionRenderData.SetNum(RuntimeMesh_MAXLODS);
	for (int32 LODIndex = 0; LODIndex < RuntimeMeshData->LODs.Num(); LODIndex++)
	{
		const FRuntimeMeshLOD& LOD = RuntimeMeshData->LODs[LODIndex];

		for (const auto& Section : LOD.Sections)
		{
			const FRuntimeMeshSectionProperties& SectionProperties = Section.Value;

			auto& RenderData = SectionRenderData[LODIndex].Add(Section.Key);
			RenderData.Material = Component->GetMaterial(SectionProperties.MaterialSlot);
			if (RenderData.Material == nullptr)
			{
				RenderData.Material = DefaultMat;
			}
			RenderData.bWantsAdjacencyInfo = RequiresAdjacencyInformation(RenderData.Material, nullptr, GetScene().GetFeatureLevel());

			MaterialRelevance |= RenderData.Material->GetRelevance(GetScene().GetFeatureLevel());

		}
	}   

	// Disable shadow casting if no section has it enabled.
	//bCastShadow = true;// bCastShadow&& bAnySectionCastsShadows;
	bCastDynamicShadow = true;// bCastDynamicShadow&& bCastShadow;

	bStaticElementsAlwaysUseProxyPrimitiveUniformBuffer = true;
	// We always use local vertex factory, which gets its primitive data from GPUScene, so we can skip expensive primitive uniform buffer updates
	bVFRequiresPrimitiveUniformBuffer = !UseGPUScene(GMaxRHIShaderPlatform, RuntimeMeshProxy->GetFeatureLevel());

}

FRuntimeMeshComponentSceneProxy::~FRuntimeMeshComponentSceneProxy()
{

}

void FRuntimeMeshComponentSceneProxy::CreateRenderThreadResources()
{
	RuntimeMeshProxy->CalculateViewRelevance(bHasStaticSections, bHasDynamicSections, bHasShadowableSections);
	FPrimitiveSceneProxy::CreateRenderThreadResources();
}

FPrimitiveViewRelevance FRuntimeMeshComponentSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);

	bool bForceDynamicPath = !IsStaticPathAvailable() || IsRichView(*View->Family) || IsSelected() || View->Family->EngineShowFlags.Wireframe;
	Result.bStaticRelevance = !bForceDynamicPath && bHasStaticSections;
	Result.bDynamicRelevance = bForceDynamicPath || bHasDynamicSections;

	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	Result.bVelocityRelevance = IsMovable() && Result.bOpaqueRelevance && Result.bRenderInMainPass;
	return Result;
}

void FRuntimeMeshComponentSceneProxy::CreateMeshBatch(FMeshBatch& MeshBatch, const FRuntimeMeshSectionProxy& Section, int32 LODIndex, const FRuntimeMeshSectionRenderData& RenderData, FMaterialRenderProxy* Material, FMaterialRenderProxy* WireframeMaterial) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponentSceneProxy_CreateMeshBatch);

	bool bRenderWireframe = WireframeMaterial != nullptr;
	bool bWantsAdjacency = !bRenderWireframe && RenderData.bWantsAdjacencyInfo;

	Section.CreateMeshBatch(MeshBatch, Section.CastsShadow(), bWantsAdjacency);

	MeshBatch.LODIndex = LODIndex;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	MeshBatch.VisualizeLODIndex = LODIndex;
#endif

	MeshBatch.bDitheredLODTransition = !IsMovable() && Material->GetMaterialInterface()->IsDitheredLODTransition();
	MeshBatch.bWireframe = WireframeMaterial != nullptr;
	MeshBatch.MaterialRenderProxy = MeshBatch.bWireframe ? WireframeMaterial : Material;

	MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
	MeshBatch.DepthPriorityGroup = SDPG_World;
	MeshBatch.bCanApplyViewModeOverrides = false;


	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	//BatchElement.PrimitiveUniformBufferResource = GetUniformBuffer();

	BatchElement.MaxScreenSize = RuntimeMeshProxy->GetScreenSize(LODIndex);
	BatchElement.MinScreenSize = RuntimeMeshProxy->GetScreenSize(LODIndex + 1);

	//UE_LOG(LogRuntimeMesh, Warning, TEXT("Section Screen Size: %f - %f"), BatchElement.MaxScreenSize, BatchElement.MinScreenSize);
	return;
}

void FRuntimeMeshComponentSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponentSceneProxy_DrawStaticMeshElements);

	auto& LODs = RuntimeMeshProxy->GetLODs();
	for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
	{
		auto& LOD = LODs[LODIndex];
		for (const auto& SectionEntry : LOD->GetSections())
		{
			auto& Section = SectionEntry.Value;
			auto* RenderData = SectionRenderData[LODIndex].Find(SectionEntry.Key);

			if (RenderData != nullptr && Section->ShouldRender() && Section->WantsToRenderInStaticPath())
			{
				FMaterialRenderProxy* Material = RenderData->Material->GetRenderProxy();

				FMeshBatch MeshBatch;
				CreateMeshBatch(MeshBatch, *Section, LODIndex, *RenderData, Material, nullptr);
				PDI->DrawMesh(MeshBatch, RuntimeMeshProxy->GetScreenSize(LODIndex));

				//UE_LOG(LogRuntimeMesh, Warning, TEXT("Section Screen Size Max: %f"), RuntimeMeshProxy->GetScreenSize(LODIndex));
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
		WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
			FLinearColor(0, 0.5f, 1.f)
		);

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
	}


	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FSceneView* View = Views[ViewIndex];

		if (IsShown(View) && (VisibilityMap & (1 << ViewIndex)))
		{
			FFrozenSceneViewMatricesGuard FrozenMatricesGuard(*const_cast<FSceneView*>(Views[ViewIndex]));

			FLODMask LODMask = GetLODMask(View);

			auto& LODs = RuntimeMeshProxy->GetLODs();
			for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
			{
				if (LODMask.ContainsLOD(LODIndex))
				{
					auto& LOD = LODs[LODIndex];
					for (const auto& SectionEntry : LOD->GetSections())
					{
						auto& Section = SectionEntry.Value;
						auto* RenderData = SectionRenderData[LODIndex].Find(SectionEntry.Key);

						if (RenderData != nullptr && Section->ShouldRender())
						{
							bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || Views[ViewIndex]->Family->EngineShowFlags.Wireframe || IsSelected() || !IsStaticPathAvailable();

							if (bForceDynamicPath || !Section->WantsToRenderInStaticPath())
							{
								FMaterialRenderProxy* Material = RenderData->Material->GetRenderProxy();

								FMeshBatch& MeshBatch = Collector.AllocateMesh();
								CreateMeshBatch(MeshBatch, *Section, LODIndex, *RenderData, Material, WireframeMaterialInstance);
								MeshBatch.bDitheredLODTransition = LODMask.IsDithered();

								Collector.AddMesh(ViewIndex, MeshBatch);
							}
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
				//BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255), IsSelected(), IsHovered()).ToFColor(true), NULL, false, false, UseEditorDepthTest(), ViewIndex, Collector);
			}

			// Render bounds
			RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
		}
	}
#endif
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

int8 FRuntimeMeshComponentSceneProxy::ComputeTemporalStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale, int32 SampleIndex) const
{
	const int32 NumLODs = RuntimeMesh_MAXLODS;

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
	const int32 NumLODs = RuntimeMesh_MAXLODS;
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

// 	if (View->DrawDynamicFlags & EDrawDynamicFlags::ForceLowestLOD)
// 	{
// 		// Pin the LOD
// 	}
// 	else if (View->Family && View->Family->EngineShowFlags.LOD == 0)
// 	{
// 		// 
// 	}

	const FBoxSphereBounds& ProxyBounds = GetBounds();
	bool bUseDithered = false;
	if (RuntimeMeshProxy->GetMaxLOD() > 0)
	{
		// only dither if at least one section in LOD0 is dithered. Mixed dithering on sections won't work very well, but it makes an attempt
		const ERHIFeatureLevel::Type FeatureLevel = GetScene().GetFeatureLevel();

		TSharedPtr<FRuntimeMeshLODProxy> LOD = RuntimeMeshProxy->GetLODs()[0];

		const auto& LOD0Sections = SectionRenderData[0];

		for (const auto& Section : LOD0Sections)
		{
			if (Section.Value.Material->IsDitheredLODTransition())
			{
				bUseDithered = true;
				break;
			}
		}
	}

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

	return Result;
}