// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentProxy.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "TessellationRendering.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/Material.h"

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
			UE_LOG(LogRuntimeMesh, Warning, TEXT("RMCP: creating renderable section data. %d"), FPlatformTLS::GetCurrentThreadId());


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
	bool bRenderWireframe = WireframeMaterial != nullptr;
	bool bWantsAdjacency = !bRenderWireframe && RenderData.bWantsAdjacencyInfo;

	Section.CreateMeshBatch(MeshBatch, Section.CastsShadow(), bWantsAdjacency);

	MeshBatch.LODIndex = LODIndex;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	MeshBatch.VisualizeLODIndex = LODIndex;
#endif

	MeshBatch.bDitheredLODTransition = false; // !IsMovable() && Material->GetMaterialInterface()->IsDitheredLODTransition();



	MeshBatch.bWireframe = WireframeMaterial != nullptr;
	MeshBatch.MaterialRenderProxy = MeshBatch.bWireframe ? WireframeMaterial : Material;

	MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
	MeshBatch.DepthPriorityGroup = SDPG_World;
	MeshBatch.bCanApplyViewModeOverrides = false;


	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	//BatchElement.PrimitiveUniformBufferResource = GetUniformBuffer();

	BatchElement.MaxScreenSize = 1.0f;// RuntimeMeshProxy->GetScreenSize(LODIndex);
	BatchElement.MinScreenSize = 0.0f;// RuntimeMeshProxy->GetScreenSize(LODIndex + 1);
}

void FRuntimeMeshComponentSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
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

				UE_LOG(LogRuntimeMesh, Warning, TEXT("RMCP: Registering renderable section. %d"), FPlatformTLS::GetCurrentThreadId());
				FMaterialRenderProxy* Material = RenderData->Material->GetRenderProxy();

				FMeshBatch MeshBatch;
				CreateMeshBatch(MeshBatch, *Section, LODIndex, *RenderData, Material, nullptr);
				PDI->DrawMesh(MeshBatch, 0.0f);// RuntimeMeshProxy->GetScreenSize(LODIndex));


			}
		}
	}
}

void FRuntimeMeshComponentSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{

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



	auto& LODs = RuntimeMeshProxy->GetLODs();
	for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
	{
		auto& LOD = LODs[LODIndex];
		for (const auto& SectionEntry : LOD->GetSections())
		{
			auto& Section = SectionEntry.Value;
			auto* RenderData = SectionRenderData[LODIndex].Find(SectionEntry.Key);

			if (RenderData != nullptr && Section->ShouldRender())
			{
				// Add the mesh batch to every view it's visible in
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || Views[ViewIndex]->Family->EngineShowFlags.Wireframe || IsSelected() || !IsStaticPathAvailable();

						if (bForceDynamicPath || !Section->WantsToRenderInStaticPath())
						{
							FMaterialRenderProxy* Material = RenderData->Material->GetRenderProxy();

							FMeshBatch& MeshBatch = Collector.AllocateMesh();
							CreateMeshBatch(MeshBatch, *Section, LODIndex, *RenderData, Material, WireframeMaterialInstance);

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
				//BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255), IsSelected(), IsHovered()).ToFColor(true), NULL, false, false, UseEditorDepthTest(), ViewIndex, Collector);
			}

			// Render bounds
			RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
		}
	}
#endif
}
