// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshComponentProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RealtimeMeshComponent.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"
#include "PrimitiveSceneProxy.h"
#include "UnrealEngine.h"
#include "SceneManagement.h"
#include "RayTracingInstance.h"
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

#define RMC_LOG_VERBOSE(Format, ...) \
	//UE_LOG(RealtimeMeshLog, Verbose, TEXT("[RMCSP:%d Mesh:%d Thread:%d]: " Format), GetUniqueID(), (RealtimeMeshProxy? RealtimeMeshProxy->GetMeshID() : -1), FPlatformTLS::GetCurrentThreadId(), ##__VA_ARGS__);

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
			Materials.Add(MaterialIndex, MakeTuple(Mat->GetRenderProxy(), Mat->IsDitheredLODTransition()));
			MaterialRelevance |= Mat->GetRelevance(GetScene().GetFeatureLevel());
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
	}

	FRealtimeMeshComponentSceneProxy::~FRealtimeMeshComponentSceneProxy()
	{
		check(true);
	}

	bool FRealtimeMeshComponentSceneProxy::CanBeOccluded() const
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	void FRealtimeMeshComponentSceneProxy::CreateRenderThreadResources()
	{
		FPrimitiveSceneProxy::CreateRenderThreadResources();
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

		const auto ValidLODRange = RealtimeMeshProxy->GetValidLODRange();


		if (!ValidLODRange.IsEmpty())
		{
			FLODMask LODMask;

			for (uint8 LODIndex = ValidLODRange.GetLowerBoundValue(); LODIndex <= ValidLODRange.GetUpperBoundValue(); LODIndex++)
			{
				const auto& LOD = RealtimeMeshProxy->GetLOD(FRealtimeMeshLODKey(LODIndex));

				bool bCastRayTracedShadow = false; //IsShadowCast(Context.ReferenceView);

				if (LOD.IsValid() && LOD->GetDrawMask().IsSet(ERealtimeMeshDrawMask::DrawStatic))
				{
					LODMask.SetLOD(LODIndex);

					const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeLimits(LODIndex);

					FMeshBatch MeshBatch;
					FRealtimeMeshBatchCreationParams Params
					{
						[this](const TSharedRef<FRenderResource>& Resource) { InUseBuffers.Add(Resource); },
						[&MeshBatch]()-> FMeshBatch& {
							MeshBatch = FMeshBatch();
							return MeshBatch;
						},
#if RHI_RAYTRACING
						[&PDI](const FMeshBatch& Batch, float MinScreenSize, const FRayTracingGeometry*) { PDI->DrawMesh(Batch, MinScreenSize); },
#else
							[&PDI](const FMeshBatch& Batch, float MinScreenSize) { PDI->DrawMesh(Batch, MinScreenSize); },					
#endif
						GetUniformBuffer(),
						LODScreenSizes,
						LODMask,
						IsMovable(),
						IsLocalToWorldDeterminantNegative(),
						bCastRayTracedShadow
					};

					RealtimeMeshProxy->CreateMeshBatches(LODIndex, Params, Materials, nullptr, ERealtimeMeshSectionDrawType::Static, false);
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

		FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
			                                                            FLinearColor(0.0f, 0.16f, 1.0f));
			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		const TRange<int8> ValidLODRange = RealtimeMeshProxy->GetValidLODRange();


		check(!RealtimeMeshProxy->GetDrawMask().HasAnyFlags() || !ValidLODRange.IsEmpty());


		check(ValidLODRange.IsEmpty() || (ValidLODRange.GetLowerBound().IsInclusive() && ValidLODRange.GetUpperBound().IsInclusive()));

		if (!ValidLODRange.IsEmpty())
		{
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				const FSceneView* View = Views[ViewIndex];
				const bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || bWireframe || IsSelected();

				if (IsShown(View) && (VisibilityMap & (1 << ViewIndex)))
				{
					FFrozenSceneViewMatricesGuard FrozenMatricesGuard(*const_cast<FSceneView*>(Views[ViewIndex]));

					FLODMask LODMask = GetLODMask(View);

					for (uint8 LODIndex = ValidLODRange.GetLowerBoundValue(); LODIndex <= ValidLODRange.GetUpperBoundValue(); LODIndex++)
					{
						const auto& LOD = RealtimeMeshProxy->GetLOD(FRealtimeMeshLODKey(LODIndex));

						bool bCastRayTracedShadow = false; //IsShadowCast(Context.ReferenceView);

						if ((LOD.IsValid() && LOD->GetDrawMask().IsSet(ERealtimeMeshDrawMask::DrawDynamic)) ||
							(bForceDynamicPath && LOD->GetDrawMask().IsSet(ERealtimeMeshDrawMask::DrawStatic)))
						{
							LODMask.SetLOD(LODIndex);

							const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeLimits(LODIndex);

							FRealtimeMeshBatchCreationParams Params
							{
								[](const TSharedRef<FRenderResource>&)
								{
								},
								[&Collector]()-> FMeshBatch& { return Collector.AllocateMesh(); },
#if RHI_RAYTRACING
								[&Collector, ViewIndex](FMeshBatch& Batch, float, const FRayTracingGeometry*) { Collector.AddMesh(ViewIndex, Batch); },
#else
								[&Collector, ViewIndex](FMeshBatch& Batch, float) { Collector.AddMesh(ViewIndex, Batch); },
#endif
								GetUniformBuffer(),
								LODScreenSizes,
								LODMask,
								IsMovable(),
								IsLocalToWorldDeterminantNegative(),
								bCastRayTracedShadow
							};

							RealtimeMeshProxy->CreateMeshBatches(LODIndex, Params, Materials, WireframeMaterialInstance, ERealtimeMeshSectionDrawType::Dynamic, bForceDynamicPath);
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
	}


#if RHI_RAYTRACING
	void FRealtimeMeshComponentSceneProxy::GetDynamicRayTracingInstances(struct FRayTracingMaterialGatheringContext& Context,
	                                                                     TArray<struct FRayTracingInstance>& OutRayTracingInstances)
	{
		SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponentSceneProxy_GetDynamicRayTracingInstances);

		// TODO: Should this use any LOD determination logic? Or always use a specific LOD?
		const int32 LODIndex = 0;

		if (RealtimeMeshProxy->GetDrawMask().HasAnyFlags())
		{
			if (auto LOD = RealtimeMeshProxy->GetLOD(LODIndex))
			{
				if (LOD.IsValid() && LOD->GetDrawMask().IsAnySet(ERealtimeMeshDrawMask::DrawDynamic | ERealtimeMeshDrawMask::DrawStatic))
				{
					FLODMask LODMask;
					LODMask.SetLOD(LODIndex);

					const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeLimits(LODIndex);

					TMap<const FRayTracingGeometry*, int32> CurrentRayTracingGeometries;
					
					FMeshBatch MeshBatch;
					FRealtimeMeshBatchCreationParams Params
					{
						[](const TSharedRef<FRenderResource>&)
						{
						},
						[MeshBatch = &MeshBatch]()-> FMeshBatch& {
							*MeshBatch = FMeshBatch();
							return *MeshBatch;
						},
						[&OutRayTracingInstances, &CurrentRayTracingGeometries, LocalToWorld = GetLocalToWorld()](
								FMeshBatch& Batch, float MinScreenSize, const FRayTracingGeometry* RayTracingGeometry)
						{
							if (RayTracingGeometry->IsValid())
							{
								check(RayTracingGeometry->Initializer.TotalPrimitiveCount > 0);
								check(RayTracingGeometry->Initializer.IndexBuffer.IsValid());
								checkf(RayTracingGeometry->RayTracingGeometryRHI, TEXT("Ray tracing instance must have a valid geometry."));
								
								FRayTracingInstance* RayTracingInstance;
								if (const auto* RayTracingInstanceIndex = CurrentRayTracingGeometries.Find(RayTracingGeometry))
								{
									RayTracingInstance = &OutRayTracingInstances[*RayTracingInstanceIndex];
								}
								else
								{
									RayTracingInstance = &OutRayTracingInstances.AddDefaulted_GetRef();
									CurrentRayTracingGeometries.Add(RayTracingGeometry, OutRayTracingInstances.Num() - 1);
									
									RayTracingInstance->Geometry = RayTracingGeometry;
									RayTracingInstance->InstanceTransforms.Add(LocalToWorld);
								}
								Batch.SegmentIndex = RayTracingInstance->Materials.Num();

								RayTracingInstance->Materials.Add(Batch);
							}
						},
						GetUniformBuffer(),
						LODScreenSizes,
						LODMask,
						IsMovable(),
						IsLocalToWorldDeterminantNegative(),
						IsShadowCast(Context.ReferenceView)
					};

					RealtimeMeshProxy->CreateMeshBatches(LODIndex, Params, Materials, nullptr, ERealtimeMeshSectionDrawType::Dynamic, true /* bForceDynamicPath */);

#if RMC_ENGINE_BELOW_5_2
					for (auto& RayTracingInstance : OutRayTracingInstances)
					{
						RayTracingInstance.BuildInstanceMaskAndFlags(GetScene().GetFeatureLevel());
					}
#endif
				}
			}
		}

		check(true);
	}
#endif // RHI_RAYTRACING

	int8 FRealtimeMeshComponentSceneProxy::GetCurrentFirstLOD() const
	{
		return RealtimeMeshProxy->GetValidLODRange().GetLowerBoundValue();
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
			if (FMath::Square(RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex).GetLowerBoundValue() * 0.5f) > ScreenRadiusSquared)
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
			if (FMath::Square(RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex).GetLowerBoundValue() * 0.5f) > ScreenRadiusSquared)
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
			Result.SetLOD(RealtimeMeshProxy->GetValidLODRange().GetUpperBoundValue());
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
			bool bUseDithered = RealtimeMeshProxy->GetValidLODRange().GetUpperBoundValue() != INDEX_NONE && bAnyMaterialUsesDithering;

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

	FMaterialRenderProxy* FRealtimeMeshComponentSceneProxy::GetMaterialSlot(int32 MaterialSlotId) const
	{
		const TTuple<FMaterialRenderProxy*, bool>* Mat = Materials.Find(MaterialSlotId);
		if (Mat != nullptr && Mat->Get<0>() != nullptr)
		{
			return Mat->Get<0>();
		}

		return UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
	}
}

#undef RMC_LOG_VERBOSE
