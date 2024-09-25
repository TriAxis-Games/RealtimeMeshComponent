// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshComponentProxy.h"
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
#if RMC_ENGINE_ABOVE_5_2
#include "MaterialDomain.h"
#include "Materials/MaterialRenderProxy.h"
#include "SceneInterface.h"
#endif

DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponentSceneProxy - Create Mesh Batch"), STAT_RealtimeMeshComponentSceneProxy_CreateMeshBatch, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponentSceneProxy - Get Dynamic Mesh Elements"), STAT_RealtimeMeshComponentSceneProxy_GetDynamicMeshElements, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponentSceneProxy - Draw Static Mesh Elements"), STAT_RealtimeMeshComponentSceneProxy_DrawStaticMeshElements, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponentSceneProxy - Get Dynamic Ray Tracing Instances"), STAT_RealtimeMeshComponentSceneProxy_GetDynamicRayTracingInstances,
                   STATGROUP_RealtimeMesh);


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
		//bSupportsRayTracing = true; //InRealtimeMeshProxy->HasRayTracingGeometry();
		//bDynamicRayTracingGeometry = false;

#if RMC_ENGINE_BELOW_5_4
#if RMC_ENGINE_ABOVE_5_2
		if (IsRayTracingAllowed() && bSupportsRayTracing)
#elif RMC_ENGINE_ABOVE_5_1
		if (IsRayTracingEnabled(GetScene().GetShaderPlatform()) && bSupportsRayTracing)
#else
		if (IsRayTracingEnabled() && bSupportsRayTracing)		
#endif
		{			
			RayTracingGeometries.SetNum(InRealtimeMeshProxy->GetNumLODs());
			for (int32 LODIndex = 0; LODIndex < RealtimeMeshProxy->GetNumLODs(); LODIndex++)
			{
				if (FRayTracingGeometry* RayTracingGeo = RealtimeMeshProxy->GetLOD(LODIndex)->GetStaticRayTracingGeometry())
				{
					RayTracingGeometries[LODIndex] = RayTracingGeo;
				}
			}
			
			/*
			const bool bWantsRayTracingWPO = MaterialRelevance.bUsesWorldPositionOffset && InComponent->bEvaluateWorldPositionOffsetInRayTracing;
			
			// r.RayTracing.Geometry.StaticMeshes.WPO is handled in the following way:
			// 0 - mark ray tracing geometry as dynamic but don't create any dynamic geometries since it won't be included in ray tracing scene
			// 1 - mark ray tracing geometry as dynamic and create dynamic geometries
			// 2 - don't mark ray tracing geometry as dynamic nor create any dynamic geometries since WPO evaluation is disabled

			// if r.RayTracing.Geometry.StaticMeshes.WPO == 2, WPO evaluation is disabled so don't need to mark geometry as dynamic
			if (bWantsRayTracingWPO && CVarRayTracingStaticMeshesWPO.GetValueOnAnyThread() != 2)
			{
				bDynamicRayTracingGeometry = true;

				// only need dynamic geometries when r.RayTracing.Geometry.StaticMeshes.WPO == 1
				bNeedsDynamicRayTracingGeometries = CVarRayTracingStaticMeshesWPO.GetValueOnAnyThread() == 1;
			}
			*/
		}
#endif
#endif

#if RMC_ENGINE_ABOVE_5_2
		if (MaterialRelevance.bOpaque && !MaterialRelevance.bUsesSingleLayerWaterMaterial)
		{
			UpdateVisibleInLumenScene();
		}
#endif
		
	}

	FRealtimeMeshComponentSceneProxy::~FRealtimeMeshComponentSceneProxy()
	{
		check(true);
	}

#if RMC_ENGINE_ABOVE_5_4
	void FRealtimeMeshComponentSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList)
	{
		MeshReferencingHandle = RealtimeMeshProxy->GetReferencingHandle();		
		RealtimeMeshProxy->ProcessCommands(RHICmdList);
		FPrimitiveSceneProxy::CreateRenderThreadResources(RHICmdList);
	}
#else
	void FRealtimeMeshComponentSceneProxy::CreateRenderThreadResources()
	{		
		RealtimeMeshProxy->ProcessCommands(GRHICommandList.GetImmediateCommandList());
		FPrimitiveSceneProxy::CreateRenderThreadResources();
	}
#endif

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

		const bool bForceDynamicPath = IsRichView(*View->Family) || IsSelected() || View->Family->EngineShowFlags.Wireframe;

		Result.bStaticRelevance = !bForceDynamicPath && RealtimeMeshProxy->GetDrawMask().IsSet(ERealtimeMeshDrawMask::DrawStatic);
		Result.bDynamicRelevance = bForceDynamicPath || RealtimeMeshProxy->GetDrawMask().IsSet(ERealtimeMeshDrawMask::DrawDynamic);

		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
		Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;
		return Result;
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
						MeshBatch.CastRayTracedShadow &= bCastDynamicShadow;

						auto& BatchElement = MeshBatch.Elements[0];

						// Setup LOD screen sizes
						BatchElement.MinScreenSize = LODScreenSizes.GetLowerBoundValue();
						BatchElement.MaxScreenSize = LODScreenSizes.GetUpperBoundValue();

						//check(MeshBatch.Validate(this, GetScene().GetFeatureLevel()));

						PDI->DrawMesh(MeshBatch, LODScreenSizes.GetLowerBoundValue());
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
									MeshBatch.CastRayTracedShadow &= bCastDynamicShadow;

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

	UE_DISABLE_OPTIMIZATION
	void FRealtimeMeshComponentSceneProxy::GetDistanceFieldAtlasData(const FDistanceFieldVolumeData*& OutDistanceFieldData, float& SelfShadowBias) const
	{
		OutDistanceFieldData = RealtimeMeshProxy->GetDistanceFieldData();
		SelfShadowBias = DistanceFieldSelfShadowBias;
	}

	void FRealtimeMeshComponentSceneProxy::GetDistanceFieldInstanceData(TArray<FRenderTransform>& InstanceLocalToPrimitiveTransforms) const
	{
		check(InstanceLocalToPrimitiveTransforms.IsEmpty());

		if (RealtimeMeshProxy->HasDistanceFieldData())
		{
			InstanceLocalToPrimitiveTransforms.Add(FRenderTransform::Identity);
		}
	}

	bool FRealtimeMeshComponentSceneProxy::HasDistanceFieldRepresentation() const
	{
		bool bCastsDS = CastsDynamicShadow();
		bool bAffectsDFLighting = AffectsDistanceFieldLighting();
		bool bHasDF = RealtimeMeshProxy->HasDistanceFieldData();
		
		return bCastsDS && bAffectsDFLighting && bHasDF;
	}
	UE_ENABLE_OPTIMIZATION

	bool FRealtimeMeshComponentSceneProxy::HasDynamicIndirectShadowCasterRepresentation() const
	{
		return bCastsDynamicIndirectShadow && HasDistanceFieldRepresentation();
	}

	const FCardRepresentationData* FRealtimeMeshComponentSceneProxy::GetMeshCardRepresentation() const
	{
		return RealtimeMeshProxy->GetCardRepresentation();
	}

	bool FRealtimeMeshComponentSceneProxy::HasRayTracingRepresentation() const
	{
		return false;
/*#if RHI_RAYTRACING
		return bSupportsRayTracing;
#else
		return false;
#endif*/
	}

#if RMC_ENGINE_ABOVE_5_4
	TArray<FRayTracingGeometry*> FRealtimeMeshComponentSceneProxy::GetStaticRayTracingGeometries() const
	{
/*#if RHI_RAYTRACING
		if (IsRayTracingAllowed() && bSupportsRayTracing)
		{
			TArray<FRayTracingGeometry*> RayTracingGeometries;
			RayTracingGeometries.SetNum(RealtimeMeshProxy->GetNumLODs());
			for (int32 LODIndex = 0; LODIndex < RealtimeMeshProxy->GetNumLODs(); LODIndex++)
			{
				RayTracingGeometries[LODIndex] = RealtimeMeshProxy->GetLOD(LODIndex)->GetStaticRayTracingGeometry();
			}

			const int32 IndexOfFirstNull = RayTracingGeometries.IndexOfByPredicate([](FRayTracingGeometry* RayTracingGeometry) { return !RayTracingGeometry; });

			// TODO: Should we inject blank ones or duplicate a valid one instead of doing this?
			// We strip to valid range with no nulls.
			if (IndexOfFirstNull >= 0 && IndexOfFirstNull < RayTracingGeometries.Num())
			{
				RayTracingGeometries.SetNum(IndexOfFirstNull);
			}
			
			return MoveTemp(RayTracingGeometries);
		}
#endif // RHI_RAYTRACING*/
		return {};
	}
#endif // RMC_ENGINE_ABOVE_5_4

#if RHI_RAYTRACING
	void FRealtimeMeshComponentSceneProxy::GetDynamicRayTracingInstances(struct FRayTracingMaterialGatheringContext& Context,
	                                                                     TArray<struct FRayTracingInstance>& OutRayTracingInstances)
	{
		SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponentSceneProxy_GetDynamicRayTracingInstances);

		/*const uint32 LODIndex = FMath::Max(GetLOD(Context.ReferenceView), (int32)GetCurrentFirstLODIdx_RenderThread());

		TMap<const FRayTracingGeometry*, int32> CurrentRayTracingGeometries;

		if (auto LOD = RealtimeMeshProxy->GetLOD(LODIndex))
		{
			check(LOD->GetDrawMask().ShouldRenderDynamicPath() || LOD->GetDrawMask().ShouldRenderStaticPath());
			
			const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex);

			// Walk all section groups within the LOD
			for (auto SectionGroupIt = LOD->GetActiveSectionGroupMaskIter(); SectionGroupIt; ++SectionGroupIt)
			{			
				const FRealtimeMeshSectionGroupProxy* SectionGroup = *SectionGroupIt;
				check(SectionGroup->GetDrawMask().ShouldRenderDynamicPath() || LOD->GetDrawMask().ShouldRenderStaticPath());

				if (LOD->GetStaticRayTracedSectionGroup() && LOD->GetStaticRayTracedSectionGroup().Get() == SectionGroup)
				{
					continue;
				}

				auto VertexFactory = SectionGroup->GetVertexFactory();
				check(VertexFactory && VertexFactory.IsValid() && VertexFactory->IsInitialized());

				const FRayTracingGeometry* RayTracingGeometry = SectionGroup->GetRayTracingGeometry();
				
				check(RayTracingGeometry->Initializer.TotalPrimitiveCount > 0);
				check(RayTracingGeometry->Initializer.IndexBuffer.IsValid());
				checkf(RayTracingGeometry->RayTracingGeometryRHI, TEXT("Ray tracing instance must have a valid geometry."));				

				auto& RayTracingInstance = OutRayTracingInstances.AddDefaulted_GetRef();
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
				
#if RMC_ENGINE_BELOW_5_2
				RayTracingInstance.BuildInstanceMaskAndFlags(GetScene().GetFeatureLevel());
#endif
			}		
		}*/
	}
#endif // RHI_RAYTRACING

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
}

#undef RMC_LOG_VERBOSE
