// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshComponentProxy.h"

#include "MaterialDomain.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RealtimeMeshComponent.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"
#include "PrimitiveSceneProxy.h"
#include "UnrealEngine.h"
#include "SceneManagement.h"
#include "RayTracingInstance.h"
#include "RealtimeMeshComponentModule.h"
#include "RealtimeMeshSceneViewExtension.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshDebugVertexFactory.h"
#include "Data/RealtimeMeshShared.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialDomain.h"
#include "Materials/MaterialRenderProxy.h"
#include "SceneInterface.h"

DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponentSceneProxy - Create Mesh Batch"), STAT_RealtimeMeshComponentSceneProxy_CreateMeshBatch, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponentSceneProxy - Get Dynamic Mesh Elements"), STAT_RealtimeMeshComponentSceneProxy_GetDynamicMeshElements, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponentSceneProxy - Draw Static Mesh Elements"), STAT_RealtimeMeshComponentSceneProxy_DrawStaticMeshElements, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponentSceneProxy - Get Dynamic Ray Tracing Instances"), STAT_RealtimeMeshComponentSceneProxy_GetDynamicRayTracingInstances,
                   STATGROUP_RealtimeMesh);

static TAutoConsoleVariable<int32> CVarRayTracingRealtimeMesh(
	TEXT("r.RayTracing.Geometry.RealtimeMeshes"),
	1,
	TEXT("Include realtime meshes in ray tracing effects (default = 1 (realtime meshes enabled in ray tracing))"));

// Debug visualization console variables
TAutoConsoleVariable<int32> CVarRealtimeMeshShowNormals(
	TEXT("r.RealtimeMesh.ShowNormals"),
	0,
	TEXT("Show normals for realtime meshes (0 = off, 1 = on)"));

TAutoConsoleVariable<int32> CVarRealtimeMeshShowTangents(
	TEXT("r.RealtimeMesh.ShowTangents"),
	0,
	TEXT("Show tangents for realtime meshes (0 = off, 1 = on)"));

TAutoConsoleVariable<int32> CVarRealtimeMeshShowBinormals(
	TEXT("r.RealtimeMesh.ShowBinormals"),
	0,
	TEXT("Show binormals for realtime meshes (0 = off, 1 = on)"));

static TAutoConsoleVariable<int32> CVarRealtimeMeshShowVertexColors(
	TEXT("r.RealtimeMesh.ShowVertexColors"),
	0,
	TEXT("Show vertex colors for realtime meshes (0 = off, 1 = on)"));

TAutoConsoleVariable<float> CVarRealtimeMeshDebugLineLength(
	TEXT("r.RealtimeMesh.DebugLineLength"),
	5.0f,
	TEXT("Length of debug lines for normals/tangents/binormals"));

namespace RealtimeMesh
{	
	FRealtimeMeshComponentSceneProxy::FRealtimeMeshComponentSceneProxy(URealtimeMeshComponent* Component, const FRealtimeMeshProxyRef& InRealtimeMeshProxy)
		: FPrimitiveSceneProxy(Component)
		  , RealtimeMeshProxy(InRealtimeMeshProxy)
		  , BodySetup(Component->GetBodySetup())
		  , bAnyMaterialUsesDithering(false)
	{
		check(Component->GetRealtimeMesh() != nullptr);

		for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); MaterialIndex++)
		{
			UMaterialInterface* Mat = Component->GetMaterial(MaterialIndex);
			if (Mat == nullptr)
			{
				Mat = UMaterial::GetDefaultMaterial(MD_Surface);
			}
			MaterialMap.SetMaterial(MaterialIndex, Mat->GetRenderProxy());
			MaterialMap.SetMaterialSupportsDither(MaterialIndex, Mat->IsDitheredLODTransition());
			MaterialRelevance |= Mat->GetRelevance_Concurrent(GetScene().GetFeatureLevel());
			bAnyMaterialUsesDithering = Mat->IsDitheredLODTransition();
		}

		// Disable shadow casting if no section has it enabled.
		bCastDynamicShadow = true;
		bCastStaticShadow = true;

		const auto FeatureLevel = GetScene().GetFeatureLevel();

		// We always use local vertex factory, which gets its primitive data from GPUScene, so we can skip expensive primitive uniform buffer updates
		bVFRequiresPrimitiveUniformBuffer = !UseGPUScene(GMaxRHIShaderPlatform, FeatureLevel);
		bStaticElementsAlwaysUseProxyPrimitiveUniformBuffer = true;
		bVerifyUsedMaterials = false;
		
		bSupportsDistanceFieldRepresentation = MaterialRelevance.bOpaque && !MaterialRelevance.bUsesSingleLayerWaterMaterial && RealtimeMeshProxy->HasDistanceFieldData();
		
		bCastsDynamicIndirectShadow = Component->bCastDynamicShadow && Component->CastShadow && Component->Mobility != EComponentMobility::Static;
		DynamicIndirectShadowMinVisibility = 0.1f;


#if RHI_RAYTRACING
		bSupportsRayTracing = true; //InRealtimeMeshProxy->HasRayTracingGeometry()
		//bDynamicRayTracingGeometry = false;
#endif

		if (MaterialRelevance.bOpaque && !MaterialRelevance.bUsesSingleLayerWaterMaterial)
		{
			UpdateVisibleInLumenScene();
		}		
	}

	FRealtimeMeshComponentSceneProxy::~FRealtimeMeshComponentSceneProxy()
	{
	}

	void FRealtimeMeshComponentSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList)
	{
		MeshReferencingHandle = RealtimeMeshProxy->GetReferencingHandle();		
		RealtimeMeshProxy->ProcessCommands(RHICmdList);
		FPrimitiveSceneProxy::CreateRenderThreadResources(RHICmdList);
	}

	bool FRealtimeMeshComponentSceneProxy::CanBeOccluded() const
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	SIZE_T FRealtimeMeshComponentSceneProxy::GetTypeHash() const
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FPrimitiveViewRelevance FRealtimeMeshComponentSceneProxy::GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);

		// Check if debug visualization is enabled to ensure we have dynamic relevance for debug drawing
		const bool bShowNormals = CVarRealtimeMeshShowNormals.GetValueOnRenderThread() != 0;
		const bool bShowTangents = CVarRealtimeMeshShowTangents.GetValueOnRenderThread() != 0;
		const bool bShowBinormals = CVarRealtimeMeshShowBinormals.GetValueOnRenderThread() != 0;
		const bool bShowVertexColors = CVarRealtimeMeshShowVertexColors.GetValueOnRenderThread() != 0;
		const bool bDebugVisualizationActive = bShowNormals || bShowTangents || bShowBinormals || bShowVertexColors;
		
		const bool bForceDynamicPath = IsRichView(*View->Family) || IsSelected() || View->Family->EngineShowFlags.Wireframe;

		Result.bStaticRelevance = !bForceDynamicPath && RealtimeMeshProxy->GetDrawMask().IsSet(ERealtimeMeshDrawMask::DrawStatic);
		Result.bDynamicRelevance = bForceDynamicPath || RealtimeMeshProxy->GetDrawMask().IsSet(ERealtimeMeshDrawMask::DrawDynamic) || bDebugVisualizationActive;

		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
		Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;
		return Result;
	}

	inline void SetupMeshBatchForRuntimeVirtualTexture(FMeshBatch& MeshBatch)
	{
		MeshBatch.CastShadow = 0;
		MeshBatch.bUseAsOccluder = 0;
		MeshBatch.bUseForDepthPass = 0;
		MeshBatch.bUseForMaterial = 0;
		MeshBatch.bDitheredLODTransition = 0;
		MeshBatch.bRenderToVirtualTexture = 1;
	}

	
	void FRealtimeMeshComponentSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
	{
		SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponentSceneProxy_DrawStaticMeshElements);
	
		// Walk active LODs
		for (auto LodIt = RealtimeMeshProxy->GetActiveStaticLODMaskIter(); LodIt; ++LodIt)
		{
			const FRealtimeMeshLODProxy* LOD = *LodIt;
			check(LOD->GetDrawMask().ShouldRenderStaticPath());
			
			FLODMask LODMask;
			LODMask.SetLOD(LodIt.GetIndex());

			const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeRangeForLOD(LodIt.GetIndex());

			// Walk all section groups within the LOD
			for (auto SectionGroupIt = LOD->GetActiveSectionGroupMaskIter(); SectionGroupIt; ++SectionGroupIt)
			{				
				const FRealtimeMeshSectionGroupProxy* SectionGroup = *SectionGroupIt;
				if (!SectionGroup->GetDrawMask().ShouldRenderStaticPath())
				{
					continue;
				}

				auto VertexFactory = SectionGroup->GetVertexFactory();
				check(VertexFactory && VertexFactory.IsValid() && VertexFactory->IsInitialized());

				check(SectionGroup->GetStream(FRealtimeMeshStreams::Triangles)->IsResourceInitialized());

				for (auto SectionIt = SectionGroup->GetActiveSectionMaskIter(); SectionIt; ++SectionIt)
				{
					const FRealtimeMeshSectionProxy* Section = *SectionIt;

					FMaterialRenderProxy* MaterialProxy = MaterialMap.GetMaterial(Section->GetMaterialSlot());
					
					FMeshBatch MeshBatch;

					MeshBatch.MaterialRenderProxy = MaterialProxy? MaterialProxy : UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
					MeshBatch.bWireframe = false;

					// Let SectionGroup do initial setup
					bool bIsValid = SectionGroup->InitializeMeshBatch(MeshBatch, StaticResources, IsLocalToWorldDeterminantNegative(), false);

					// Let Section finish setup
					bIsValid = bIsValid && Section->InitializeMeshBatch(MeshBatch, GetUniformBuffer());

					check(MeshBatch.VertexFactory == VertexFactory.Get());
					check(MeshBatch.VertexFactory && MeshBatch.VertexFactory->IsInitialized());

					if (bIsValid)
					{						
						// TODO: Should this check material?
						MeshBatch.bDitheredLODTransition &= bAnyMaterialUsesDithering && !IsMovable() && LODMask.IsDithered() &&
							MaterialMap.GetMaterialSupportsDither(Section->GetMaterialSlot());
						MeshBatch.CastShadow &= bCastDynamicShadow;
#if RHI_RAYTRACING
						MeshBatch.CastRayTracedShadow &= bCastDynamicShadow;
#endif

						auto& BatchElement = MeshBatch.Elements[0];

						// Setup LOD screen sizes
						BatchElement.MinScreenSize = LODScreenSizes.GetLowerBoundValue();
						BatchElement.MaxScreenSize = LODScreenSizes.GetUpperBoundValue();

						//check(MeshBatch.Validate(this, GetScene().GetFeatureLevel()));

						if (RuntimeVirtualTextureMaterialTypes.Num() > 0)
						{
							// Runtime virtual texture mesh elements.
							FMeshBatch RVTMeshBatch(MeshBatch);
							SetupMeshBatchForRuntimeVirtualTexture(RVTMeshBatch);
							for (ERuntimeVirtualTextureMaterialType MaterialType : RuntimeVirtualTextureMaterialTypes)
							{
								RVTMeshBatch.RuntimeVirtualTextureMaterialType = (uint32)MaterialType;
								PDI->DrawMesh(RVTMeshBatch, LODScreenSizes.GetLowerBoundValue());
							}
						}

						PDI->DrawMesh(MeshBatch, LODScreenSizes.GetLowerBoundValue());						
					}
				}
			}			
		}

	}

	void FRealtimeMeshComponentSceneProxy::DrawDebugVectorsDynamic(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
	{
		const float LineLength = CVarRealtimeMeshDebugLineLength.GetValueOnRenderThread();
		const bool bShowNormals = CVarRealtimeMeshShowNormals.GetValueOnRenderThread() != 0;
		const bool bShowTangents = CVarRealtimeMeshShowTangents.GetValueOnRenderThread() != 0;
		const bool bShowBinormals = CVarRealtimeMeshShowBinormals.GetValueOnRenderThread() != 0;
		// Create debug modes bitmask (only TBN, vertex colors handled separately)
		uint32 DebugMode = 0;
		if (bShowNormals) DebugMode |= FRealtimeMeshDebugVertexFactory::Normals;
		if (bShowTangents) DebugMode |= FRealtimeMeshDebugVertexFactory::Tangents;
		if (bShowBinormals) DebugMode |= FRealtimeMeshDebugVertexFactory::Binormals;

		// Use vertex color material that respects interpolated vertex colors
		FMaterialRenderProxy* DebugMaterial = nullptr;
		
		// First try the engine's vertex color material which should respect vertex colors
		if (GEngine->VertexColorViewModeMaterial_ColorOnly)
		{
			DebugMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly->GetRenderProxy();
		}
		// Fallback to unlit vertex color material
		else if (GEngine->VertexColorMaterial)
		{
			DebugMaterial = GEngine->VertexColorMaterial->GetRenderProxy();
		}
		// Last resort fallback
		else 
		{
			DebugMaterial = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
				FLinearColor(0.0f, 0.8f, 1.0f)
			);
			Collector.RegisterOneFrameMaterialProxy(DebugMaterial);
		}		

		// Walk active LODs
		for (auto LodIt = RealtimeMeshProxy->GetActiveStaticLODMaskIter(); LodIt; ++LodIt)
		{
			const FRealtimeMeshLODProxy* LOD = *LodIt;

			// Walk all section groups within the LOD
			for (auto SectionGroupIt = LOD->GetActiveSectionGroupMaskIter(); SectionGroupIt; ++SectionGroupIt)
			{
				const FRealtimeMeshSectionGroupProxy* SectionGroup = *SectionGroupIt;

				// Skip if we don't have the required streams
				if (!SectionGroup->GetStream(FRealtimeMeshStreams::Position) ||
					!SectionGroup->GetStream(FRealtimeMeshStreams::Tangents))
				{
					continue;
				}

				// Get or create cached debug vertex factory
				TSharedPtr<FRealtimeMeshDebugVertexFactory> DebugVertexFactory = GetOrCreateDebugVertexFactory(SectionGroup, DebugMode, LineLength, Collector.GetRHICommandList());
				
				// Skip if vertex factory is not valid
				if (!DebugVertexFactory.IsValid() || !DebugVertexFactory->IsInitialized() || DebugVertexFactory->GetValidRange().NumVertices() == 0)
				{
					continue;
				}
				
				// Create debug mesh batches for each view
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						// Pre-calculate if we have anything to render
						FRealtimeMeshStreamRange ValidRange = DebugVertexFactory->GetValidRange();
						uint32 ActiveDebugModes = 0;
						if (bShowNormals) ActiveDebugModes++;
						if (bShowTangents) ActiveDebugModes++;
						if (bShowBinormals) ActiveDebugModes++;
						
						// Only proceed if we have vertices and debug modes active
						if (ValidRange.NumVertices() > 0 && ActiveDebugModes > 0 && DebugVertexFactory->IsInitialized())
						{
							// Calculate lines: multiply by number of active debug channels
							// Each vertex gets one line per active debug mode
							uint32 NumLines = ValidRange.NumVertices() * ActiveDebugModes;
							
							// Only create mesh batch if we have valid primitives
							if (NumLines > 0)
							{
								// Create mesh batch for debug lines
								FMeshBatch& DebugMeshBatch = Collector.AllocateMesh();
								DebugMeshBatch.MaterialRenderProxy = DebugMaterial;
								DebugMeshBatch.VertexFactory = DebugVertexFactory.Get();
								DebugMeshBatch.Type = PT_LineList;
								DebugMeshBatch.DepthPriorityGroup = SDPG_World;
								DebugMeshBatch.bCanApplyViewModeOverrides = false;
								DebugMeshBatch.bUseWireframeSelectionColoring = false;
								DebugMeshBatch.bWireframe = false;

								// Set up mesh batch element
								FMeshBatchElement& BatchElement = DebugMeshBatch.Elements[0];
								
								// Use our debug index buffer for line pairs
								FRealtimeMeshResourceReferenceList DebugResources;
								bool bDepthOnly = false;
								bool bMatrixInverted = false;
								BatchElement.IndexBuffer = &DebugVertexFactory->GetIndexBuffer(bDepthOnly, bMatrixInverted, DebugResources);
								BatchElement.FirstIndex = 0;
								BatchElement.NumPrimitives = NumLines;
								BatchElement.MinVertexIndex = 0;
								BatchElement.MaxVertexIndex = ValidRange.NumVertices() - 1; // Max vertex in the vertex buffer
								
								// Initialize other required fields
								BatchElement.BaseVertexIndex = 0;
								BatchElement.NumInstances = 1;
								BatchElement.InstancedLODIndex = 0;
								BatchElement.InstancedLODRange = 0;
								
								// Set up primitive uniform buffer (required for GPU Scene)
								BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

								if (BatchElement.NumPrimitives > 0)
								{									
									// Add the debug mesh batch
									Collector.AddMesh(ViewIndex, DebugMeshBatch);
								}
							}
						}
					}
				}
			}
		}
	}

	void FRealtimeMeshComponentSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
	                                                              FMeshElementCollector& Collector) const
	{
		SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponentSceneProxy_GetDynamicMeshElements);
	
		// Set up wireframe material (if needed)
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;
		
		// Check if we should show vertex colors via material swap
		const bool bShowVertexColors = CVarRealtimeMeshShowVertexColors.GetValueOnRenderThread() != 0;

		/*FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
			                                                            FLinearColor(0.0f, 0.16f, 1.0f));
			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}*/


		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FSceneView* View = Views[ViewIndex];
			const bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || bWireframe || IsSelected();

			if (IsShown(View) && (VisibilityMap & (1 << ViewIndex)))
			{
				FFrozenSceneViewMatricesGuard FrozenMatricesGuard(*const_cast<FSceneView*>(Views[ViewIndex]));
				FLODMask LODMask = GetLODMask(View);

				// Walk active LODs
				for (auto LodIt = bForceDynamicPath? RealtimeMeshProxy->GetActiveLODMaskIter() : RealtimeMeshProxy->GetActiveDynamicLODMaskIter(); LodIt; ++LodIt)
				{
					if (LODMask.ContainsLOD(LodIt.GetIndex()))
					{
						const FRealtimeMeshLODProxy* LOD = *LodIt;
						check(LOD->GetDrawMask().ShouldRenderDynamicPath() || (bForceDynamicPath && LOD->GetDrawMask().ShouldRenderStaticPath()));
						
						const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeRangeForLOD(LodIt.GetIndex());

						// Walk all section groups within the LOD
						for (auto SectionGroupIt = bForceDynamicPath? LOD->GetActiveSectionGroupMaskIter() : LOD->GetActiveSectionGroupMaskIter(); SectionGroupIt; ++SectionGroupIt)
						{
							const FRealtimeMeshSectionGroupProxy* SectionGroup = *SectionGroupIt;
							if (!bForceDynamicPath && !SectionGroup->GetDrawMask().ShouldRenderDynamicPath())
							{
								continue;
							}

							auto VertexFactory = SectionGroup->GetVertexFactory();
							check(VertexFactory && VertexFactory.IsValid() && VertexFactory->IsInitialized());

							for (auto SectionIt = bForceDynamicPath? SectionGroup->GetActiveSectionMaskIter() : SectionGroup->GetActiveSectionMaskIter(); SectionIt; ++SectionIt)
							{
								const FRealtimeMeshSectionProxy* Section = *SectionIt;

								FMaterialRenderProxy* MaterialProxy = MaterialMap.GetMaterial(Section->GetMaterialSlot());
								
								//const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

								if (bWireframe)
								{
									MaterialProxy = new FColoredMaterialRenderProxy(GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
																								FLinearColor(0.0f, 0.16f, 1.0f));
									Collector.RegisterOneFrameMaterialProxy(MaterialProxy);
								}

								FMeshBatch& MeshBatch = Collector.AllocateMesh();

								MeshBatch.MaterialRenderProxy = MaterialProxy? MaterialProxy : UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
								
								// Override with vertex color material if debug mode is enabled
								if (bShowVertexColors)
								{
									UMaterialInterface* VertexColorMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly;
									if (VertexColorMaterial)
									{
										MeshBatch.MaterialRenderProxy = VertexColorMaterial->GetRenderProxy();
									}
								}
								
								MeshBatch.bWireframe = bWireframe;

								// Let SectionGroup do initial setup
								FRealtimeMeshResourceReferenceList DynamicResources;
								bool bIsValid = SectionGroup->InitializeMeshBatch(MeshBatch, DynamicResources, IsLocalToWorldDeterminantNegative(), false);

								// Let Section finish setup
								bIsValid = bIsValid && Section->InitializeMeshBatch(MeshBatch, GetUniformBuffer());

								check(MeshBatch.VertexFactory && MeshBatch.VertexFactory->IsInitialized());
								check(MeshBatch.Elements[0].IndexBuffer && MeshBatch.Elements[0].IndexBuffer->IsInitialized());
								
								if (bIsValid)
								{						
									// TODO: Should this check material?
									MeshBatch.bDitheredLODTransition &= bAnyMaterialUsesDithering && !IsMovable() && LODMask.IsDithered() &&
										MaterialMap.GetMaterialSupportsDither(Section->GetMaterialSlot());
									MeshBatch.CastShadow &= bCastDynamicShadow;
#if RHI_RAYTRACING
									MeshBatch.CastRayTracedShadow &= bCastDynamicShadow;
#endif

									auto& BatchElement = MeshBatch.Elements[0];

									// Setup LOD screen sizes
									BatchElement.MinScreenSize = LODScreenSizes.GetLowerBoundValue();
									BatchElement.MaxScreenSize = LODScreenSizes.GetUpperBoundValue();
									
									Collector.AddMesh(ViewIndex, MeshBatch);
								}
							}
						}		
					}
				}
			}
		}
		
		// Debug rendering for normals, tangents, and binormals
		const bool bShowNormals = CVarRealtimeMeshShowNormals.GetValueOnRenderThread() != 0;
		const bool bShowTangents = CVarRealtimeMeshShowTangents.GetValueOnRenderThread() != 0;
		const bool bShowBinormals = CVarRealtimeMeshShowBinormals.GetValueOnRenderThread() != 0;

		if (bShowNormals || bShowTangents || bShowBinormals)
		{
			
			DrawDebugVectorsDynamic(Views, ViewFamily, VisibilityMap, Collector);
		}
		
		// Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				// Draw simple collision as wireframe if 'show collision', and collision is enabled, and we are not using the complex as the simple
				if (ViewFamily.EngineShowFlags.Collision && IsCollisionEnabled() && BodySetup && BodySetup->GetCollisionTraceFlag() !=
					ECollisionTraceFlag::CTF_UseComplexAsSimple)
				{
					FTransform GeomTransform(GetLocalToWorld());
					BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255), IsSelected(), IsHovered()).ToFColor(true), NULL, false, false,
					                              DrawsVelocity(), ViewIndex, Collector);
				}

				// Render bounds
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
		}
#endif
	}

	void FRealtimeMeshComponentSceneProxy::GetDistanceFieldAtlasData(const FDistanceFieldVolumeData*& OutDistanceFieldData, float& SelfShadowBias) const
	{
		OutDistanceFieldData = RealtimeMeshProxy->GetDistanceFieldData();
		SelfShadowBias = DistanceFieldSelfShadowBias;
	}

#if RMC_ENGINE_BELOW_5_6
	void FRealtimeMeshComponentSceneProxy::GetDistanceFieldInstanceData(TArray<FRenderTransform>& InstanceLocalToPrimitiveTransforms) const
	{
		check(InstanceLocalToPrimitiveTransforms.IsEmpty());

		if (RealtimeMeshProxy->HasDistanceFieldData())
		{
			InstanceLocalToPrimitiveTransforms.Add(FRenderTransform::Identity);
		}
	}
#endif

	bool FRealtimeMeshComponentSceneProxy::HasDistanceFieldRepresentation() const
	{
		bool bCastsDS = CastsDynamicShadow();
		bool bAffectsDFLighting = AffectsDistanceFieldLighting();
		bool bHasDF = RealtimeMeshProxy->HasDistanceFieldData();
		
		return bCastsDS && bAffectsDFLighting && bHasDF;
	}

	bool FRealtimeMeshComponentSceneProxy::HasDynamicIndirectShadowCasterRepresentation() const
	{
		return bCastsDynamicIndirectShadow && HasDistanceFieldRepresentation();
	}

	const FCardRepresentationData* FRealtimeMeshComponentSceneProxy::GetMeshCardRepresentation() const
	{
		return RealtimeMeshProxy->GetCardRepresentation();
	}


#if RHI_RAYTRACING
	bool FRealtimeMeshComponentSceneProxy::IsRayTracingStaticRelevant() const
	{
		// Still some work to finish for static ray tracing to behave correctly as it's initialized on the GT before
		// the 
		return false && RealtimeMeshProxy->GetDrawMask().CanRenderInStaticRayTracing();
	}
	

	TArray<FRayTracingGeometry*> FRealtimeMeshComponentSceneProxy::GetStaticRayTracingGeometries() const
	{
		if (!CVarRayTracingRealtimeMesh.GetValueOnAnyThread())
		{
			return { };
		}
		
		if (IsRayTracingAllowed() && bSupportsRayTracing)
		{
			TArray<FRayTracingGeometry*> RayTracingGeometries;
			RayTracingGeometries.SetNum(RealtimeMeshProxy->GetNumLODs());
			for (int32 LODIndex = 0; LODIndex < RealtimeMeshProxy->GetNumLODs(); LODIndex++)
			{
				RayTracingGeometries[LODIndex] = RealtimeMeshProxy->GetLOD(LODIndex)->GetStaticRayTracingGeometry();
			}

			const int32 IndexOfFirstNull = RayTracingGeometries.IndexOfByPredicate([](const FRayTracingGeometry* RayTracingGeometry)
				{ return !RayTracingGeometry || !RayTracingGeometry->IsValid(); });

			// TODO: Should we inject blank ones or duplicate a valid one instead of doing this?
			// We strip to valid range with no nulls.
			if (IndexOfFirstNull >= 0 && IndexOfFirstNull < RayTracingGeometries.Num())
			{
				RayTracingGeometries.SetNum(IndexOfFirstNull);
			}
			
			return MoveTemp(RayTracingGeometries);
		}
		return {};
	}
	

#if RMC_ENGINE_ABOVE_5_5
	void FRealtimeMeshComponentSceneProxy::GetDynamicRayTracingInstances(class FRayTracingInstanceCollector& Collector)
#else
	void FRealtimeMeshComponentSceneProxy::GetDynamicRayTracingInstances(struct FRayTracingMaterialGatheringContext& Context,
	                                                                     TArray<struct FRayTracingInstance>& OutRayTracingInstances)
#endif
	{
		SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponentSceneProxy_GetDynamicRayTracingInstances);

		if (!CVarRayTracingRealtimeMesh.GetValueOnRenderThread())
		{
			return;
		}

#if RMC_ENGINE_ABOVE_5_5
		const uint32 LODIndex = FMath::Max(GetLOD(Collector.GetReferenceView()), (int32)GetCurrentFirstLODIdx_RenderThread());
#else
		const uint32 LODIndex = FMath::Max(GetLOD(Context.ReferenceView), (int32)GetCurrentFirstLODIdx_RenderThread());
#endif

		auto LOD = RealtimeMeshProxy->GetLOD(LODIndex);
		if (!LOD || !LOD->GetDrawMask().ShouldRenderInRayTracing())
		{
			return;
		}
		
		TMap<const FRayTracingGeometry*, int32> CurrentRayTracingGeometries;		

		const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex);

		// Walk all section groups within the LOD
		for (auto SectionGroupIt = LOD->GetActiveSectionGroupMaskIter(); SectionGroupIt; ++SectionGroupIt)
		{			
			const FRealtimeMeshSectionGroupProxy* SectionGroup = *SectionGroupIt;
			check(SectionGroup->GetDrawMask().ShouldRenderDynamicPath() || LOD->GetDrawMask().ShouldRenderStaticPath());

			if (!SectionGroup->GetDrawMask().ShouldRenderInRayTracing())
			{
				continue;
			}

			auto VertexFactory = SectionGroup->GetVertexFactory();
			check(VertexFactory && VertexFactory.IsValid() && VertexFactory->IsInitialized());

			const FRayTracingGeometry* RayTracingGeometry = SectionGroup->GetRayTracingGeometry();
			
			check(RayTracingGeometry->Initializer.TotalPrimitiveCount > 0);
			check(RayTracingGeometry->Initializer.IndexBuffer.IsValid());
			
#if RMC_ENGINE_ABOVE_5_5
			checkf(RayTracingGeometry->GetRHI(), TEXT("Ray tracing instance must have a valid geometry."));
			FRayTracingInstance RayTracingInstance;
#else
			checkf(RayTracingGeometry->RayTracingGeometryRHI, TEXT("Ray tracing instance must have a valid geometry."));
			FRayTracingInstance& RayTracingInstance = OutRayTracingInstances.AddDefaulted_GetRef();
#endif
			
			RayTracingInstance.Geometry = RayTracingGeometry;
			RayTracingInstance.InstanceTransforms.Add(GetLocalToWorld());
			
			if (RayTracingGeometry->IsValid() && RayTracingGeometry->IsInitialized())
			{
				for (auto SectionIt = SectionGroup->GetActiveSectionMaskIter(); SectionIt; ++SectionIt)
				{
					const FRealtimeMeshSectionProxy* Section = *SectionIt;
					check(Section->GetDrawMask().ShouldRenderDynamicPath() || Section->GetDrawMask().ShouldRenderStaticPath());

					FMaterialRenderProxy* MaterialProxy = MaterialMap.GetMaterial(Section->GetMaterialSlot());

					FMeshBatch MeshBatch;

					MeshBatch.MaterialRenderProxy = MaterialProxy? MaterialProxy : UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
					MeshBatch.bWireframe = false;

					// Let SectionGroup do initial setup
					FRealtimeMeshResourceReferenceList DynamicResources;					
					bool bIsValid = SectionGroup->InitializeMeshBatch(MeshBatch, DynamicResources, IsLocalToWorldDeterminantNegative(), false);

					// Let Section finish setup
					bIsValid = bIsValid && Section->InitializeMeshBatch(MeshBatch, GetUniformBuffer());

					if (bIsValid)
					{						
						// TODO: Should this check material?
						MeshBatch.bDitheredLODTransition &= false;
						MeshBatch.CastShadow &= bCastDynamicShadow;
#if RMC_ENGINE_ABOVE_5_5
						MeshBatch.CastShadow &= IsShadowCast(Collector.GetReferenceView());
#endif
						MeshBatch.CastRayTracedShadow &= bCastDynamicShadow;

						auto& BatchElement = MeshBatch.Elements[0];

						// Setup LOD screen sizes
						BatchElement.MinScreenSize = LODScreenSizes.GetLowerBoundValue();
						BatchElement.MaxScreenSize = LODScreenSizes.GetUpperBoundValue();

						MeshBatch.SegmentIndex = RayTracingInstance.Materials.Num();
						RayTracingInstance.Materials.Add(MeshBatch);
					}
				}
			}
			
#if RMC_ENGINE_ABOVE_5_5				
			Collector.AddRayTracingInstance(MoveTemp(RayTracingInstance));
#endif
		}
	}
#endif

	int8 FRealtimeMeshComponentSceneProxy::GetCurrentFirstLOD() const
	{
		return RealtimeMeshProxy->GetFirstLODIndex();
	}

	int8 FRealtimeMeshComponentSceneProxy::ComputeTemporalStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale,
	                                                                    int32 SampleIndex) const
	{
		const int32 NumLODs = REALTIME_MESH_MAX_LODS;

		const float ScreenRadiusSquared = ComputeBoundsScreenRadiusSquared(Origin, SphereRadius, View.GetTemporalLODOrigin(SampleIndex), View.ViewMatrices.GetProjectionMatrix())
			* FactorScale * FactorScale * View.LODDistanceFactor * View.LODDistanceFactor;

		// Walk backwards and return the first matching LOD
		for (int32 LODIndex = NumLODs - 1; LODIndex >= 0; --LODIndex)
		{
			const float LODSScreenSizeSquared = FMath::Square(RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex).GetLowerBoundValue() * 0.5f);
			if (LODSScreenSizeSquared > ScreenRadiusSquared)
			{
				return LODIndex;
			}
		}

		return MinLOD;
	}

	int8 FRealtimeMeshComponentSceneProxy::ComputeStaticMeshLOD(const FVector4& Origin, const float SphereRadius, const FSceneView& View, int32 MinLOD, float FactorScale) const
	{
		const FSceneView& LODView = GetLODView(View);
		const float ScreenRadiusSquared = ComputeBoundsScreenRadiusSquared(Origin, SphereRadius, LODView) * FactorScale * FactorScale * LODView.LODDistanceFactor * LODView.
			LODDistanceFactor;

		// Walk backwards and return the first matching LOD
		for (int32 LODIndex = RealtimeMeshProxy->GetNumLODs() - 1; LODIndex >= 0; --LODIndex)
		{
			const float LODSScreenSizeSquared = FMath::Square(RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex).GetLowerBoundValue() * 0.5f);
			if (LODSScreenSizeSquared > ScreenRadiusSquared)
			{
 				return FMath::Max(LODIndex, MinLOD);
			}
		}

		return MinLOD;
	}


	FLODMask FRealtimeMeshComponentSceneProxy::GetLODMask(const FSceneView* View) const
	{
		FLODMask Result;

		if (View->DrawDynamicFlags & EDrawDynamicFlags::ForceLowestLOD)
		{
			Result.SetLOD(RealtimeMeshProxy->GetLastLODIndex());
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
			bool bUseDithered = RealtimeMeshProxy->GetLastLODIndex() != INDEX_NONE && bAnyMaterialUsesDithering;

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

		return Result;
	}

	int32 FRealtimeMeshComponentSceneProxy::GetLOD(const FSceneView* View) const
	{
		const FBoxSphereBounds& ProxyBounds = GetBounds();
		FCachedSystemScalabilityCVars CachedSystemScalabilityCVars = GetCachedScalabilityCVars();

		float InvScreenSizeScale = (CachedSystemScalabilityCVars.StaticMeshLODDistanceScale != 0.f) ? (1.0f / CachedSystemScalabilityCVars.StaticMeshLODDistanceScale) : 1.0f;

		return ComputeStaticMeshLOD(ProxyBounds.Origin, ProxyBounds.SphereRadius, *View, 0, InvScreenSizeScale);
	}

	uint32 FRealtimeMeshComponentSceneProxy::GetMemoryFootprint() const
	{
		return (sizeof(*this) + GetAllocatedSize());
	}

	SIZE_T FRealtimeMeshComponentSceneProxy::GetAllocatedSize() const
	{
		return (FPrimitiveSceneProxy::GetAllocatedSize());
	}

	TSharedPtr<FRealtimeMeshDebugVertexFactory> FRealtimeMeshComponentSceneProxy::GetOrCreateDebugVertexFactory(const FRealtimeMeshSectionGroupProxy* SectionGroup, uint32 DebugMode, float LineLength, FRHICommandList& RHICmdList) const
	{
		// Check if we already have a cached vertex factory for this section group
		if (TSharedPtr<FRealtimeMeshDebugVertexFactory>* ExistingVF = DebugVertexFactoryCache.Find(SectionGroup))
		{
			if (ExistingVF->IsValid())
			{
				// Update debug mode and line length in case they changed
				(*ExistingVF)->SetDebugMode(DebugMode);
				(*ExistingVF)->SetLineLength(LineLength);
				return *ExistingVF;
			}
		}

		// Create new debug vertex factory
		TSharedPtr<FRealtimeMeshDebugVertexFactory> DebugVertexFactory = MakeShared<FRealtimeMeshDebugVertexFactory>(GetScene().GetFeatureLevel());
		DebugVertexFactory->SetDebugMode(DebugMode);
		DebugVertexFactory->SetLineLength(LineLength);

		// Get the buffers from the section group and initialize the debug vertex factory
		TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>> BufferMap;
		
		// Collect required streams
		if (auto PositionBuffer = SectionGroup->GetStream(FRealtimeMeshStreams::Position))
		{
			BufferMap.Add(FRealtimeMeshStreams::Position, PositionBuffer);
		}
		if (auto TangentBuffer = SectionGroup->GetStream(FRealtimeMeshStreams::Tangents))
		{
			BufferMap.Add(FRealtimeMeshStreams::Tangents, TangentBuffer);
		}
		if (auto ColorBuffer = SectionGroup->GetStream(FRealtimeMeshStreams::Color))
		{
			BufferMap.Add(FRealtimeMeshStreams::Color, ColorBuffer);
		}

		// Initialize the debug vertex factory
		DebugVertexFactory->Initialize(RHICmdList, BufferMap);

		// Cache it for future use
		DebugVertexFactoryCache.Add(SectionGroup, DebugVertexFactory);

		return DebugVertexFactory;
	}
}

#undef RMC_LOG_VERBOSE
