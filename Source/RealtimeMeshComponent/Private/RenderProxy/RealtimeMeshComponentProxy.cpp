// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

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
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderProxy/RealtimeMeshDebugVertexFactory.h"
#include "Data/RealtimeMeshShared.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialDomain.h"
#include "Materials/MaterialRenderProxy.h"
#include "SceneInterface.h"
#if RMC_ENGINE_ABOVE_5_6
#include "SceneView.h"
#endif

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
	FRealtimeMeshComponentSceneProxy::FRealtimeMeshComponentSceneProxy(URealtimeMeshComponent* Component, const TSharedRef<const FRealtimeMeshProxy>& InRealtimeMeshProxy)
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
#if RMC_ENGINE_ABOVE_5_7
			// 5.7 deprecated the ERHIFeatureLevel overload in favor of EShaderPlatform.
			MaterialRelevance |= Mat->GetRelevance_Concurrent(GetScene().GetShaderPlatform());
#else
			MaterialRelevance |= Mat->GetRelevance_Concurrent(GetScene().GetFeatureLevel());
#endif
			bAnyMaterialUsesDithering |= Mat->IsDitheredLODTransition();
		}

		// Disable shadow casting if no section has it enabled.
		bCastDynamicShadow = true;
		bCastStaticShadow = true;

		const auto FeatureLevel = GetScene().GetFeatureLevel();

		// We always use local vertex factory, which gets its primitive data from GPUScene, so we can skip expensive primitive uniform buffer updates
		bVFRequiresPrimitiveUniformBuffer = !UseGPUScene(GMaxRHIShaderPlatform, FeatureLevel);
		bStaticElementsAlwaysUseProxyPrimitiveUniformBuffer = true;
		bVerifyUsedMaterials = false;
		
		// If any section carries a PositionPrev stream the geometry deforms with a static transform, so
		// force velocity output (motion vectors) — otherwise TAA/TSR ghosts the moving surface.
		{
			bool bHasVelocityStreams = false;
			for (int32 LODIndex = 0; LODIndex < RealtimeMeshProxy->GetNumLODs() && !bHasVelocityStreams; ++LODIndex)
			{
				const FRealtimeMeshLODProxyConstPtr LOD = RealtimeMeshProxy->GetLOD(FRealtimeMeshLODKey(LODIndex));
				if (!LOD.IsValid())
				{
					continue;
				}
				for (const TCowPtr<FRealtimeMeshBufferSetProxy>& BufferSet : LOD->GetBufferSets())
				{
					if (BufferSet.IsValid() && BufferSet->GetStream(FRealtimeMeshStreams::PositionPrev).IsValid())
					{
						bHasVelocityStreams = true;
						break;
					}
				}
			}
			if (bHasVelocityStreams)
			{
				bAlwaysHasVelocity = true;
			}
		}

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
		// Nothing to drain — this scene proxy renders against the immutable
		// version captured at construction. Future updates publish new versions
		// that go to freshly-recreated scene proxies.
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

	
	// DUP-028: tail shared verbatim by DrawStaticElements and GetDynamicMeshElements
	// after each path builds its FMeshBatch. Only this side-effect-free suffix is
	// collapsed here; the preceding bDitheredLODTransition line stays inline at both
	// sites because its && chain short-circuits member calls (IsMovable(),
	// MaterialMap.GetMaterialSupportsDither(...)) whose lazy evaluation would be lost
	// if precomputed into helper arguments.
	inline void ApplySharedMeshBatchShadowAndScreenSize(FMeshBatch& MeshBatch, bool bCastDynamicShadow, const TRange<float>& LODScreenSizes)
	{
		MeshBatch.CastShadow &= bCastDynamicShadow;
#if RHI_RAYTRACING
		MeshBatch.CastRayTracedShadow &= bCastDynamicShadow;
#endif
		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
		BatchElement.MinScreenSize = LODScreenSizes.GetLowerBoundValue();
		BatchElement.MaxScreenSize = LODScreenSizes.GetUpperBoundValue();
	}

	void FRealtimeMeshComponentSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
	{
		SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponentSceneProxy_DrawStaticMeshElements);

		for (const int32 LODIndex : RealtimeMeshProxy->GetActiveStaticLODIndices())
		{
			const auto& LODSlot = RealtimeMeshProxy->GetLODs()[LODIndex];
			if (!LODSlot.IsValid())
			{
				continue;
			}
			const FRealtimeMeshLODProxy* LOD = LODSlot.Get();

			FLODMask LODMask;
			LODMask.SetLOD(LODIndex);

			const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex);
			const auto& BufferSets = LOD->GetSectionGroups();
			const auto& Sections = LOD->GetSections();

			for (const FRealtimeMeshRenderEntry& Entry : LOD->GetStaticRenderEntries())
			{
				const FRealtimeMeshBufferSetProxy* SectionGroup = BufferSets[Entry.BufferSetIndex].Get();
				const FRealtimeMeshSectionProxy* Section = Sections[Entry.SectionIndex].Get();

				const auto VertexFactory = SectionGroup->GetVertexFactory();
				check(VertexFactory && VertexFactory.IsValid() && VertexFactory->IsInitialized());

				FMaterialRenderProxy* MaterialProxy = MaterialMap.GetMaterial(Section->GetMaterialSlot());

				FMeshBatch MeshBatch;
				MeshBatch.MaterialRenderProxy = MaterialProxy ? MaterialProxy : UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
				MeshBatch.bWireframe = false;

				bool bIsValid = SectionGroup->InitializeMeshBatch(MeshBatch, StaticResources, IsLocalToWorldDeterminantNegative(), false);
				bIsValid = bIsValid && Section->InitializeMeshBatch(MeshBatch, GetUniformBuffer());

				check(MeshBatch.VertexFactory == VertexFactory.Get());
				check(MeshBatch.VertexFactory && MeshBatch.VertexFactory->IsInitialized());

				if (!bIsValid)
				{
					continue;
				}

				MeshBatch.bDitheredLODTransition &= bAnyMaterialUsesDithering && !IsMovable() && LODMask.IsDithered() &&
					MaterialMap.GetMaterialSupportsDither(Section->GetMaterialSlot());

				// DUP-028: shared shadow-flag + screen-size tail (see helper above).
				ApplySharedMeshBatchShadowAndScreenSize(MeshBatch, bCastDynamicShadow, LODScreenSizes);

				if (RuntimeVirtualTextureMaterialTypes.Num() > 0)
				{
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

	void FRealtimeMeshComponentSceneProxy::DrawDebugVectorsDynamic(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
	{
		const float LineLength = CVarRealtimeMeshDebugLineLength.GetValueOnRenderThread();
		const bool bShowNormals = CVarRealtimeMeshShowNormals.GetValueOnRenderThread() != 0;
		const bool bShowTangents = CVarRealtimeMeshShowTangents.GetValueOnRenderThread() != 0;
		const bool bShowBinormals = CVarRealtimeMeshShowBinormals.GetValueOnRenderThread() != 0;
		// TBN bitmask only; vertex colors are handled on their own path.
		uint32 DebugMode = 0;
		if (bShowNormals) DebugMode |= FRealtimeMeshDebugVertexFactory::Normals;
		if (bShowTangents) DebugMode |= FRealtimeMeshDebugVertexFactory::Tangents;
		if (bShowBinormals) DebugMode |= FRealtimeMeshDebugVertexFactory::Binormals;

		// Prefer a material that renders interpolated vertex colors, falling back to a wireframe-tinted proxy.
		FMaterialRenderProxy* DebugMaterial = nullptr;
		if (GEngine->VertexColorViewModeMaterial_ColorOnly)
		{
			DebugMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly->GetRenderProxy();
		}
		else if (GEngine->VertexColorMaterial)
		{
			DebugMaterial = GEngine->VertexColorMaterial->GetRenderProxy();
		}
		else 
		{
			DebugMaterial = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
				FLinearColor(0.0f, 0.8f, 1.0f)
			);
			Collector.RegisterOneFrameMaterialProxy(DebugMaterial);
		}		

		// Walk per-BS for the debug visualization paths — each BS is rendered once
		// with its own debug vertex factory, regardless of how many sections it has.
		for (const int32 LODIndex : RealtimeMeshProxy->GetActiveStaticLODIndices())
		{
			const auto& LODSlot = RealtimeMeshProxy->GetLODs()[LODIndex];
			if (!LODSlot.IsValid()) continue;
			const FRealtimeMeshLODProxy* LOD = LODSlot.Get();
			const auto& BufferSets = LOD->GetSectionGroups();

			for (const int32 BufferSetIndex : LOD->GetActiveBufferSetIndices())
			{
				const FRealtimeMeshBufferSetProxy* SectionGroup = BufferSets[BufferSetIndex].Get();

				if (!SectionGroup->GetStream(FRealtimeMeshStreams::Position) ||
					!SectionGroup->GetStream(FRealtimeMeshStreams::Tangents))
				{
					continue;
				}

				TSharedPtr<FRealtimeMeshDebugVertexFactory> DebugVertexFactory = GetOrCreateDebugVertexFactory(SectionGroup, DebugMode, LineLength, Collector.GetRHICommandList());

				if (!DebugVertexFactory.IsValid() || !DebugVertexFactory->IsInitialized() || DebugVertexFactory->GetValidRange().NumVertices() == 0)
				{
					continue;
				}

				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						FRealtimeMeshStreamRange ValidRange = DebugVertexFactory->GetValidRange();
						uint32 ActiveDebugModes = 0;
						if (bShowNormals) ActiveDebugModes++;
						if (bShowTangents) ActiveDebugModes++;
						if (bShowBinormals) ActiveDebugModes++;

						if (ValidRange.NumVertices() > 0 && ActiveDebugModes > 0 && DebugVertexFactory->IsInitialized())
						{
							// One line per vertex per active debug channel.
							uint32 NumLines = ValidRange.NumVertices() * ActiveDebugModes;

							// PROXY-F15: the shared debug line index buffer holds only
							// MaxDebugVertices * 2 = 65536 uint16 indices (i.e. MaxDebugVertices
							// line primitives). Drawing NumLines primitives reads NumLines * 2
							// indices from FirstIndex 0, so a mesh with > 32k verts (or > 16k with
							// two debug modes active) would walk off the end of the buffer. Clamp so
							// we never index past what's allocated — surplus verts just go
							// unvisualized rather than causing an OOB read.
							NumLines = FMath::Min(NumLines, FRealtimeMeshDebugLineIndexBuffer::MaxDebugVertices);

							if (NumLines > 0)
							{
								FMeshBatch& DebugMeshBatch = Collector.AllocateMesh();
								DebugMeshBatch.MaterialRenderProxy = DebugMaterial;
								DebugMeshBatch.VertexFactory = DebugVertexFactory.Get();
								DebugMeshBatch.Type = PT_LineList;
								DebugMeshBatch.DepthPriorityGroup = SDPG_World;
								DebugMeshBatch.bCanApplyViewModeOverrides = false;
								DebugMeshBatch.bUseWireframeSelectionColoring = false;
								DebugMeshBatch.bWireframe = false;

								FMeshBatchElement& BatchElement = DebugMeshBatch.Elements[0];

								FRealtimeMeshResourceReferenceList DebugResources;
								bool bDepthOnly = false;
								bool bMatrixInverted = false;
								BatchElement.IndexBuffer = &DebugVertexFactory->GetIndexBuffer(bDepthOnly, bMatrixInverted, DebugResources);
								BatchElement.FirstIndex = 0;
								BatchElement.NumPrimitives = NumLines;
								BatchElement.MinVertexIndex = 0;
								// PROXY-F15: MaxVertexIndex must be derived from the SAME clamped
								// NumLines that bounds the draw. The linear index buffer emits 2 verts
								// per line, so this draw references vertex indices 0..2*NumLines-1 and
								// MaxVertexIndex must be 2*NumLines-1 (not the source vertex count).
								// NumLines is guaranteed >= 1 here (inside the NumLines > 0 guard),
								// so 2*NumLines-1 cannot underflow.
								BatchElement.MaxVertexIndex = 2 * NumLines - 1; // Max vertex referenced by the clamped line draw

								BatchElement.BaseVertexIndex = 0;
								BatchElement.NumInstances = 1;
								BatchElement.InstancedLODIndex = 0;
								BatchElement.InstancedLODRange = 0;

								// Required for GPU Scene.
								BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

								if (BatchElement.NumPrimitives > 0)
								{									
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

		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;
		const bool bShowVertexColors = CVarRealtimeMeshShowVertexColors.GetValueOnRenderThread() != 0;

		// PROXY-F18: allocate the wireframe material proxy once per GetDynamicMeshElements
		// call (not per render entry per view). The collector takes ownership and frees it
		// at the end of the frame via RegisterOneFrameMaterialProxy.
		FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
			                                                            FLinearColor(0.0f, 0.16f, 1.0f));
			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		// PROXY-F19: pin buffer resources once per call rather than per render entry.
		// The scene proxy's snapshot ref (RealtimeMeshProxy, held for its full lifetime)
		// transitively pins each buffer set's Streams map — the strong owner of the
		// GPU buffers the vertex factory references weakly — for the entire draw, so
		// the list contents are redundant for lifetime; hoisting simply avoids the
		// per-entry TSet allocation. InitializeMeshBatch requires a mutable list arg,
		// so we keep one and reuse it.
		FRealtimeMeshResourceReferenceList DynamicResources;


		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FSceneView* View = Views[ViewIndex];
			const bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || bWireframe || IsSelected();

			if (!IsShown(View) || !(VisibilityMap & (1 << ViewIndex)))
			{
				continue;
			}

#if RMC_ENGINE_BELOW_5_7
			// FFrozenSceneViewMatricesGuard is deprecated and no longer used as of 5.7.
			FFrozenSceneViewMatricesGuard FrozenMatricesGuard(*const_cast<FSceneView*>(Views[ViewIndex]));
#endif
			FLODMask LODMask = GetLODMask(View);

			const TArrayView<const int32> LODIndices = bForceDynamicPath
				? RealtimeMeshProxy->GetActiveLODIndices()
				: RealtimeMeshProxy->GetActiveDynamicLODIndices();

			for (const int32 LODIndex : LODIndices)
			{
				if (!LODMask.ContainsLOD(LODIndex))
				{
					continue;
				}
				const auto& LODSlot = RealtimeMeshProxy->GetLODs()[LODIndex];
				if (!LODSlot.IsValid())
				{
					continue;
				}
				const FRealtimeMeshLODProxy* LOD = LODSlot.Get();

				const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex);
				const auto& BufferSets = LOD->GetSectionGroups();
				const auto& Sections = LOD->GetSections();
				const TArray<FRealtimeMeshRenderEntry>& Entries = bForceDynamicPath
					? LOD->GetAllRenderEntries()
					: LOD->GetDynamicRenderEntries();

				for (const FRealtimeMeshRenderEntry& Entry : Entries)
				{
					const FRealtimeMeshBufferSetProxy* SectionGroup = BufferSets[Entry.BufferSetIndex].Get();
					const FRealtimeMeshSectionProxy* Section = Sections[Entry.SectionIndex].Get();

					const auto VertexFactory = SectionGroup->GetVertexFactory();
					check(VertexFactory && VertexFactory.IsValid() && VertexFactory->IsInitialized());

					FMaterialRenderProxy* MaterialProxy = MaterialMap.GetMaterial(Section->GetMaterialSlot());

					if (bWireframe)
					{
						MaterialProxy = WireframeMaterialInstance;
					}

					FMeshBatch& MeshBatch = Collector.AllocateMesh();
					MeshBatch.MaterialRenderProxy = MaterialProxy ? MaterialProxy : UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();

					if (bShowVertexColors)
					{
						UMaterialInterface* VertexColorMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly;
						if (VertexColorMaterial)
						{
							MeshBatch.MaterialRenderProxy = VertexColorMaterial->GetRenderProxy();
						}
					}

					MeshBatch.bWireframe = bWireframe;

					// PROXY-F19: reuse the once-per-call DynamicResources list (see above).
					bool bIsValid = SectionGroup->InitializeMeshBatch(MeshBatch, DynamicResources, IsLocalToWorldDeterminantNegative(), false);
					bIsValid = bIsValid && Section->InitializeMeshBatch(MeshBatch, GetUniformBuffer());

					check(MeshBatch.VertexFactory && MeshBatch.VertexFactory->IsInitialized());
					check(MeshBatch.Elements[0].IndexBuffer && MeshBatch.Elements[0].IndexBuffer->IsInitialized());

					if (!bIsValid)
					{
						continue;
					}

					MeshBatch.bDitheredLODTransition &= bAnyMaterialUsesDithering && !IsMovable() && LODMask.IsDithered() &&
						MaterialMap.GetMaterialSupportsDither(Section->GetMaterialSlot());

					// DUP-028: shared shadow-flag + screen-size tail (see helper above).
					ApplySharedMeshBatchShadowAndScreenSize(MeshBatch, bCastDynamicShadow, LODScreenSizes);

					Collector.AddMesh(ViewIndex, MeshBatch);
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
		// PROXY-F24: static ray tracing is intentionally disabled. It is initialized on the
		// game thread before the proxy's draw mask is ready, so it does not yet behave
		// correctly. When that ordering is resolved this should return
		// RealtimeMeshProxy->GetDrawMask().CanRenderInStaticRayTracing().
		return false;
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

			// We strip to valid range with no nulls.
			if (IndexOfFirstNull >= 0 && IndexOfFirstNull < RayTracingGeometries.Num())
			{
				RayTracingGeometries.SetNum(IndexOfFirstNull);
			}
			
			return MoveTemp(RayTracingGeometries);
		}
		return {};
	}
	

	void FRealtimeMeshComponentSceneProxy::GetDynamicRayTracingInstances(class FRayTracingInstanceCollector& Collector)
	{
		SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponentSceneProxy_GetDynamicRayTracingInstances);

		if (!CVarRayTracingRealtimeMesh.GetValueOnRenderThread())
		{
			return;
		}

#if RMC_ENGINE_ABOVE_5_7
		// 5.7 moved ray tracing to per-view collection: GetReferenceView() is deprecated.
		// Derive the reference view from the first active view in the visibility map, the
		// same approach the engine's own scene proxies use.
		const TConstArrayView<const FSceneView*> RayTracingViews = Collector.GetViews();
		const uint32 RayTracingVisibilityMap = Collector.GetVisibilityMap();
		const int32 FirstActiveViewIndex = FMath::CountTrailingZeros(RayTracingVisibilityMap);
		checkf(RayTracingViews.IsValidIndex(FirstActiveViewIndex), TEXT("GetDynamicRayTracingInstances called with no active view."));
		const FSceneView* ReferenceView = RayTracingViews[FirstActiveViewIndex];
#else
		const FSceneView* ReferenceView = Collector.GetReferenceView();
#endif
		const uint32 LODIndex = FMath::Max(GetLOD(ReferenceView), (int32)GetCurrentFirstLODIdx_RenderThread());

		const auto& LODsView = RealtimeMeshProxy->GetLODs();
		if (!LODsView.IsValidIndex(LODIndex) || !LODsView[LODIndex].IsValid())
		{
			return;
		}
		const FRealtimeMeshLODProxy* LOD = LODsView[LODIndex].Get();
		if (!LOD->GetDrawMask().ShouldRenderInRayTracing())
		{
			return;
		}

		const auto LODScreenSizes = RealtimeMeshProxy->GetScreenSizeRangeForLOD(LODIndex);
		const auto& BufferSets = LOD->GetSectionGroups();
		const auto& Sections = LOD->GetSections();
		const TArray<FRealtimeMeshRenderEntry>& RTEntries = LOD->GetRayTracingRenderEntries();

		// PROXY-F19: pin buffer resources once per call rather than per render entry.
		// The snapshot ref keeps the buffer sets' Streams (strong owners) alive for the
		// whole draw, so the list is redundant for lifetime; hoisting it out of the loop
		// avoids the per-entry TSet allocation. InitializeMeshBatch needs a mutable arg.
		FRealtimeMeshResourceReferenceList DynamicResources;

		// Entries are emitted in BufferSet order by UpdateCachedState — scan and
		// group consecutive entries that share the same BufferSetIndex into one
		// FRayTracingInstance with all those sections as materials.
		int32 Cursor = 0;
		while (Cursor < RTEntries.Num())
		{
			const int32 GroupBufferSetIndex = RTEntries[Cursor].BufferSetIndex;
			const FRealtimeMeshBufferSetProxy* SectionGroup = BufferSets[GroupBufferSetIndex].Get();

			const auto VertexFactory = SectionGroup->GetVertexFactory();
			check(VertexFactory && VertexFactory.IsValid() && VertexFactory->IsInitialized());

			const FRayTracingGeometry* RayTracingGeometry = SectionGroup->GetRayTracingGeometry();
			check(RayTracingGeometry && RayTracingGeometry->Initializer.TotalPrimitiveCount > 0);
			check(RayTracingGeometry->Initializer.IndexBuffer.IsValid());

			checkf(RayTracingGeometry->GetRHI(), TEXT("Ray tracing instance must have a valid geometry."));
			FRayTracingInstance RayTracingInstance;
			RayTracingInstance.Geometry = RayTracingGeometry;
			RayTracingInstance.InstanceTransforms.Add(GetLocalToWorld());

			if (RayTracingGeometry->IsValid() && RayTracingGeometry->IsInitialized())
			{
				while (Cursor < RTEntries.Num() && RTEntries[Cursor].BufferSetIndex == GroupBufferSetIndex)
				{
					const FRealtimeMeshSectionProxy* Section = Sections[RTEntries[Cursor].SectionIndex].Get();

					FMaterialRenderProxy* MaterialProxy = MaterialMap.GetMaterial(Section->GetMaterialSlot());

					FMeshBatch MeshBatch;
					MeshBatch.MaterialRenderProxy = MaterialProxy ? MaterialProxy : UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
					MeshBatch.bWireframe = false;

					// PROXY-F19: reuse the once-per-call DynamicResources list (see above).
					bool bIsValid = SectionGroup->InitializeMeshBatch(MeshBatch, DynamicResources, IsLocalToWorldDeterminantNegative(), false);
					bIsValid = bIsValid && Section->InitializeMeshBatch(MeshBatch, GetUniformBuffer());

					if (bIsValid)
					{
						MeshBatch.bDitheredLODTransition &= false;
						MeshBatch.CastShadow &= bCastDynamicShadow;
						MeshBatch.CastShadow &= IsShadowCast(ReferenceView);
						MeshBatch.CastRayTracedShadow &= bCastDynamicShadow;

						auto& BatchElement = MeshBatch.Elements[0];
						BatchElement.MinScreenSize = LODScreenSizes.GetLowerBoundValue();
						BatchElement.MaxScreenSize = LODScreenSizes.GetUpperBoundValue();

						MeshBatch.SegmentIndex = RayTracingInstance.Materials.Num();
						RayTracingInstance.Materials.Add(MeshBatch);
					}
					++Cursor;
				}
			}
			else
			{
				// Skip this BS — advance past all its entries without emitting a batch.
				while (Cursor < RTEntries.Num() && RTEntries[Cursor].BufferSetIndex == GroupBufferSetIndex)
				{
					++Cursor;
				}
			}

			// Only emit an instance with valid geometry and at least one material —
			// an empty/mismatched Materials array trips engine asserts downstream.
			if (RayTracingInstance.Materials.Num() > 0)
			{
#if RMC_ENGINE_ABOVE_5_7
				// 5.7 per-view ray tracing API: emit the instance for every active view.
				for (int32 ViewIndex = 0; ViewIndex < RayTracingViews.Num(); ++ViewIndex)
				{
					if ((RayTracingVisibilityMap & (1u << ViewIndex)) != 0)
					{
						Collector.AddRayTracingInstance(ViewIndex, RayTracingInstance);
					}
				}
#else
				Collector.AddRayTracingInstance(MoveTemp(RayTracingInstance));
#endif
			}
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
		// PROXY-F21: iterate only actual LODs, matching ComputeStaticMeshLOD, rather than
		// the fixed REALTIME_MESH_MAX_LODS upper bound.
		const int32 NumLODs = RealtimeMeshProxy->GetNumLODs();

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

	TSharedPtr<FRealtimeMeshDebugVertexFactory> FRealtimeMeshComponentSceneProxy::GetOrCreateDebugVertexFactory(const FRealtimeMeshBufferSetProxy* SectionGroup, uint32 DebugMode, float LineLength, FRHICommandList& RHICmdList) const
	{
		// Check if we already have a cached vertex factory for this buffer set
		if (TSharedPtr<FRealtimeMeshDebugVertexFactory>* ExistingVF = DebugVertexFactoryCache.Find(SectionGroup))
		{
			if (ExistingVF->IsValid())
			{
				// PROXY-F16: the uniform buffer bakes the debug params from the CVars at
				// creation time. If the params changed since we cached this VF, refresh
				// the uniform buffer so toggling r.RealtimeMesh.ShowTangents (etc.) or the
				// line length actually takes effect — SetDebugMode/SetLineLength alone only
				// write members that nothing downstream consumes.
				if ((*ExistingVF)->GetDebugMode() != DebugMode || (*ExistingVF)->GetLineLength() != LineLength)
				{
					(*ExistingVF)->SetDebugMode(DebugMode);
					(*ExistingVF)->SetLineLength(LineLength);
					(*ExistingVF)->RefreshUniformBuffer();
				}
				return *ExistingVF;
			}
		}

		// Create new debug vertex factory
		TSharedPtr<FRealtimeMeshDebugVertexFactory> DebugVertexFactory = MakeShared<FRealtimeMeshDebugVertexFactory>(GetScene().GetFeatureLevel());
		DebugVertexFactory->SetDebugMode(DebugMode);
		DebugVertexFactory->SetLineLength(LineLength);

		// Get the buffers from the buffer set and initialize the debug vertex factory
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
