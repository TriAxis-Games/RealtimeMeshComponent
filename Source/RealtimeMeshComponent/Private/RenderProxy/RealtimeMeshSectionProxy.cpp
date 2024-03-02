// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	FRealtimeMeshSectionProxy::FRealtimeMeshSectionProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey InKey)
		: SharedResources(InSharedResources)
		  , Key(InKey)
		  , bIsStateDirty(true)
	{
	}

	FRealtimeMeshSectionProxy::~FRealtimeMeshSectionProxy()
	{
		check(IsInRenderingThread());
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

	bool FRealtimeMeshSectionProxy::CreateMeshBatch(
		const FRealtimeMeshBatchCreationParams& Params,
		const FRealtimeMeshVertexFactoryRef& VertexFactory,
		const FMaterialRenderProxy* Material,
		bool bIsWireframe,
		bool bSupportsDithering
#if RHI_RAYTRACING
		, const FRayTracingGeometry* RayTracingGeometry
#endif
	) const
	{
		if (!VertexFactory->GatherVertexBufferResources(Params.ResourceSubmitter))
		{
			return false;
		}

		FMeshBatch& MeshBatch = Params.BatchAllocator();
		MeshBatch.LODIndex = Key.LOD();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		MeshBatch.VisualizeLODIndex = MeshBatch.LODIndex;
#endif

		// TODO: Map section index down
		MeshBatch.SegmentIndex = 0;
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
#if RHI_RAYTRACING
		MeshBatch.CastRayTracedShadow = Params.bCastRayTracedShadow;
#endif

		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
		//BatchElement.UserIndex = Key;
		
		BatchElement.PrimitiveUniformBuffer = Params.UniformBuffer;
		BatchElement.IndexBuffer = &VertexFactory->GetIndexBuffer(bDepthOnly, bMatrixInverted, Params.ResourceSubmitter);
		BatchElement.FirstIndex = StreamRange.GetMinIndex();
		BatchElement.NumPrimitives = StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE);
		BatchElement.MinVertexIndex = StreamRange.GetMinVertex();
		BatchElement.MaxVertexIndex = StreamRange.GetMaxVertex();

		check(BatchElement.NumPrimitives <= (static_cast<const FRealtimeMeshIndexBuffer*>(BatchElement.IndexBuffer)->Num() - BatchElement.FirstIndex) / 3);
		check((int32)BatchElement.NumPrimitives <= StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE))
		check((int32)BatchElement.MaxVertexIndex <= StreamRange.GetMaxVertex())

		BatchElement.MinScreenSize = Params.ScreenSizeLimits.GetLowerBoundValue();
		BatchElement.MaxScreenSize = Params.ScreenSizeLimits.GetUpperBoundValue();

#if RHI_RAYTRACING
		Params.BatchSubmitter(MeshBatch, Params.ScreenSizeLimits.GetLowerBoundValue(), RayTracingGeometry);
#else
		Params.BatchSubmitter(MeshBatch, Params.ScreenSizeLimits.GetLowerBoundValue());
#endif

		return true;
	}

	bool FRealtimeMeshSectionProxy::UpdateCachedState(bool bShouldForceUpdate, FRealtimeMeshSectionGroupProxy& ParentGroup)
	{
		if (!bIsStateDirty && !bShouldForceUpdate)
		{
			return false;
		}

		// First evaluate whether we have valid mesh data to render			
		bool bHasValidMeshData = StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE) > 0 &&
			StreamRange.NumVertices() >= REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE;

		if (bHasValidMeshData)
		{
			// Flip it here so if we don't get this series for whatever reason we're invalid after.
			bHasValidMeshData = ParentGroup.GetVertexFactory()->IsValidStreamRange(StreamRange);
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
				NewDrawMask.SetFlag(Config.DrawType == ERealtimeMeshSectionDrawType::Static ? ERealtimeMeshDrawMask::DrawStatic : ERealtimeMeshDrawMask::DrawDynamic);
			}
		}

		const bool bStateChanged = DrawMask != NewDrawMask;
		DrawMask = NewDrawMask;
		bIsStateDirty = false;
		return bStateChanged;
	}

	void FRealtimeMeshSectionProxy::MarkStateDirty()
	{
		bIsStateDirty = true;
	}

	void FRealtimeMeshSectionProxy::Reset()
	{
		Config = FRealtimeMeshSectionConfig();
		StreamRange = FRealtimeMeshStreamRange();
		DrawMask = FRealtimeMeshDrawMask();
		bIsStateDirty = true;
	}
}
