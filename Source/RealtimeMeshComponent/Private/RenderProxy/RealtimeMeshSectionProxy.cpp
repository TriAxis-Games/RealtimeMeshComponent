// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	FRealtimeMeshSectionProxy::FRealtimeMeshSectionProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey InKey)
		: SharedResources(InSharedResources)
		, Key(InKey)
	{
	}

	FRealtimeMeshSectionProxy::~FRealtimeMeshSectionProxy()
	{
		check(IsInRenderingThread());
	}

	void FRealtimeMeshSectionProxy::UpdateConfig(const FRealtimeMeshSectionConfig& NewConfig)
	{
		if (Config != NewConfig)
		{
			Config = NewConfig;
		}
	}

	void FRealtimeMeshSectionProxy::UpdateStreamRange(const FRealtimeMeshStreamRange& NewStreamRange)
	{
		if (StreamRange != NewStreamRange)
		{
			StreamRange = NewStreamRange;
		}
	}

	bool FRealtimeMeshSectionProxy::InitializeMeshBatch(FMeshBatch& MeshBatch, FRHIUniformBuffer* PrimitiveUniformBuffer) const
	{
		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];

		BatchElement.PrimitiveUniformBuffer = PrimitiveUniformBuffer;
		//BatchElement.PrimitiveUniformBufferResource = nullptr;
		//BatchElement.LooseParametersUniformBuffer = nullptr;
		//BatchElement.IndexBuffer = nullptr; // &VertexFactory->GetIndexBuffer(bDepthOnly, bMatrixInverted, Params.ResourceSubmitter);
		//BatchElement.UserData = nullptr;
		//BatchElement.VertexFactoryUserData = nullptr;

		//BatchElement.IndirectArgsBuffer = nullptr;
		//BatchElement.IndirectArgsOffset = 0;

		BatchElement.FirstIndex = StreamRange.GetMinIndex();
		BatchElement.NumPrimitives = StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE);
		
		//BatchElement.NumInstances = 1;
		//BatchElement.BaseVertexIndex = 0;
		BatchElement.MinVertexIndex = StreamRange.GetMinVertex();
		BatchElement.MaxVertexIndex = StreamRange.GetMaxVertex();
		//BatchElement.UserIndex = -1;
		BatchElement.MinScreenSize = 0;
		BatchElement.MaxScreenSize = 1;

		BatchElement.InstancedLODIndex = 0;
		BatchElement.InstancedLODRange = 0;
		BatchElement.bUserDataIsColorVertexBuffer = false;
		BatchElement.bIsSplineProxy = false;
		BatchElement.bIsInstanceRuns = false;
		BatchElement.bForceInstanceCulling = false;
		BatchElement.bPreserveInstanceOrder = false;
#if RMC_ENGINE_ABOVE_5_4
		BatchElement.bFetchInstanceCountFromScene = false;
#endif
		
#if UE_ENABLE_DEBUG_DRAWING
		BatchElement.VisualizeElementIndex = INDEX_NONE;
#endif

		check(BatchElement.NumPrimitives <= (static_cast<const FRealtimeMeshIndexBuffer*>(BatchElement.IndexBuffer)->Num() - BatchElement.FirstIndex) / 3);
		check((int32)BatchElement.NumPrimitives <= StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE));
		check((int32)BatchElement.MaxVertexIndex <= StreamRange.GetMaxVertex());

		return true;
	}

	UE_DISABLE_OPTIMIZATION
	void FRealtimeMeshSectionProxy::UpdateCachedState(FRealtimeMeshSectionGroupProxy& ParentGroup)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionProxy::UpdateCachedState);

		// First evaluate whether we have valid mesh data to render			
		bool bHasValidMeshData = StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE) > 0 &&
			StreamRange.NumVertices() >= REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE;

		if (bHasValidMeshData)
		{
			// Flip it here so if we don't get this series for whatever reason we're invalid after.
			bHasValidMeshData = ParentGroup.GetVertexFactory().IsValid() && ParentGroup.GetVertexFactory()->IsValidStreamRange(StreamRange);
		}

		DrawMask = FRealtimeMeshDrawMask();

		// Then build the draw mask if it is valid
		if (bHasValidMeshData)
		{
			if (Config.bIsVisible)
			{
				if (Config.bIsMainPassRenderable)
				{
					DrawMask.SetFlag(ERealtimeMeshDrawMask::DrawMainPass);
				}

				if (Config.bCastsShadow)
				{
					DrawMask.SetFlag(ERealtimeMeshDrawMask::DrawShadowPass);
				}
			}
		}
	}
	UE_ENABLE_OPTIMIZATION


	void FRealtimeMeshSectionProxy::Reset()
	{
		Config = FRealtimeMeshSectionConfig();
		StreamRange = FRealtimeMeshStreamRange();
		DrawMask = FRealtimeMeshDrawMask();
	}
}
