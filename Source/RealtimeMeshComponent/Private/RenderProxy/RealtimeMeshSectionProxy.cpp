// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	FRealtimeMeshSectionProxy::FRealtimeMeshSectionProxy(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshProxyRef& InProxy,
		FRealtimeMeshSectionKey InKey, const FRealtimeMeshSectionProxyInitializationParametersRef& InInitParams)
		: ClassFactory(InClassFactory)
		, ProxyWeak(InProxy)
		, Key(InKey)
		, Config(InInitParams->Config)
		, StreamRange(InInitParams->StreamRange)
		, bIsStateDirty(true)
	{
	}

	FRealtimeMeshSectionProxy::~FRealtimeMeshSectionProxy()
	{
		check(IsInRenderingThread());
		Reset();
	}

	void FRealtimeMeshSectionProxy::UpdateConfig(const FRealtimeMeshSectionConfig& NewConfig)
	{
		Config = NewConfig;
		MarkStateDirty();
	}

	void FRealtimeMeshSectionProxy::UpdateStreamRange(const FRealtimeMeshStreamRange& InStreamRange)
	{
		StreamRange = InStreamRange;
		MarkStateDirty();
	}

#if RHI_RAYTRACING
	bool FRealtimeMeshSectionProxy::CreateMeshBatch(const FRealtimeMeshBatchCreationParams& Params, const FRealtimeMeshVertexFactoryRef& VertexFactory,
		const FMaterialRenderProxy* Material, bool bIsWireframe, bool bSupportsDithering, const FRayTracingGeometry* RayTracingGeometry) const
#else
	bool FRealtimeMeshSectionProxy::CreateMeshBatch(const FRealtimeMeshBatchCreationParams& Params, const FRealtimeMeshVertexFactoryRef& VertexFactory,
		const FMaterialRenderProxy* Material, bool bIsWireframe, bool bSupportsDithering) const
#endif
	{
		if (!VertexFactory->GatherVertexBufferResources(Params.ResourceSubmitter))
		{
			return false;
		}
		
		FMeshBatch& MeshBatch = Params.BatchAllocator();
		MeshBatch.LODIndex = FRealtimeMeshKeyHelpers::GetLODIndex(Key);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		MeshBatch.VisualizeLODIndex = MeshBatch.LODIndex;
#endif

		MeshBatch.SegmentIndex = FRealtimeMeshKeyHelpers::GetSectionGroupIndex(Key);
		MeshBatch.DepthPriorityGroup = SDPG_World;
		MeshBatch.bCanApplyViewModeOverrides = false;

		MeshBatch.bDitheredLODTransition = !Params.bIsMovable && Params.LODMask.IsDithered() && bSupportsDithering;
		MeshBatch.bWireframe = bIsWireframe;

		ensure(Material);
		MeshBatch.MaterialRenderProxy = Material;
		MeshBatch.ReverseCulling = Params.bIsLocalToWorldDeterminantNegative;

		bool bDepthOnly = false;
		bool bMatrixInverted = false;

		Params.ResourceSubmitter(VertexFactory);
		MeshBatch.VertexFactory = &VertexFactory.Get();
		MeshBatch.Type = VertexFactory->GetPrimitiveType();

		MeshBatch.CastShadow = DrawMask.ShouldRenderShadow();
		MeshBatch.CastRayTracedShadow = Params.bCastRayTracedShadow;

		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
		BatchElement.UserIndex = FRealtimeMeshKeyHelpers::GetSectionIndex(Key);
	
		BatchElement.PrimitiveUniformBuffer = Params.UniformBuffer;
		BatchElement.IndexBuffer = &VertexFactory->GetIndexBuffer(bDepthOnly, bMatrixInverted, Params.ResourceSubmitter);
		BatchElement.FirstIndex = StreamRange.GetMinIndex();
		BatchElement.NumPrimitives = StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE);
		BatchElement.MinVertexIndex = StreamRange.GetMinVertex();
		BatchElement.MaxVertexIndex = StreamRange.GetMaxVertex();

		BatchElement.MinScreenSize = Params.ScreenSizeLimits.GetLowerBoundValue();
		BatchElement.MaxScreenSize = Params.ScreenSizeLimits.GetUpperBoundValue();

#if RHI_RAYTRACING
		Params.BatchSubmitter(MeshBatch, Params.ScreenSizeLimits.GetLowerBoundValue(), RayTracingGeometry);
#else
		Params.BatchSubmitter(MeshBatch, Params.ScreenSizeLimits.GetLowerBoundValue());
#endif
		
		return true;
	}

	void FRealtimeMeshSectionProxy::MarkStateDirty()
	{
		bIsStateDirty = true;
	}

	bool FRealtimeMeshSectionProxy::HandleUpdates(bool bShouldForceUpdate)
	{
		if (bIsStateDirty || bShouldForceUpdate)
		{
			bIsStateDirty = false;

			// First evaluate whether we have valid mesh data to render			
			bool bHasValidMeshData = StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE) > 0 && StreamRange.NumVertices() > 0;
			if (bHasValidMeshData)
			{
				// Flip it here so if we don't get this series for whatever reason we're invalid after.
				bHasValidMeshData = false;
				if (const FRealtimeMeshProxyPtr Proxy = ProxyWeak.Pin())
				{
					if (const FRealtimeMeshLODProxyPtr LOD = Proxy->GetLOD(Key.GetLODKey()))
					{
						if (const FRealtimeMeshSectionGroupProxyPtr SectionGroup = LOD->GetSectionGroup(Key.GetSectionGroupKey()))
						{
							bHasValidMeshData = SectionGroup->GetVertexFactory()->IsValidStreamRange(StreamRange);
						}
					}
				}
			}
			
			FRealtimeMeshDrawMask NewDrawMask;

			// Then build the draw mask if it is valid
			if (bHasValidMeshData)
			{
				if (Config.bIsVisible)
				{					
					if (Config.bIsMainPassRenderable)
					{
						NewDrawMask.SetFlag(ERealtimeMeshDrawMask::DrawMainPass);
					}

					if (Config.bCastsShadow)
					{
						NewDrawMask.SetFlag(ERealtimeMeshDrawMask::DrawShadowPass);
					}
				}

				if (NewDrawMask.HasAnyFlags())
				{
					NewDrawMask.SetFlag(Config.DrawType == ERealtimeMeshSectionDrawType::Static? ERealtimeMeshDrawMask::DrawStatic : ERealtimeMeshDrawMask::DrawDynamic);
				}
			}

			const bool bStateChanged = DrawMask != NewDrawMask;
			DrawMask = NewDrawMask;
			return bStateChanged;
		}
		return false;
	}

	void FRealtimeMeshSectionProxy::Reset()
	{
		Config = FRealtimeMeshSectionConfig();
		StreamRange = FRealtimeMeshStreamRange();
		DrawMask = FRealtimeMeshDrawMask();
		bIsStateDirty = false;
	}

	void FRealtimeMeshSectionProxy::OnStreamsUpdated(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams,
		const TArray<FRealtimeMeshStreamKey>& RemovedStreams)
	{
		MarkStateDirty();
	}
}
