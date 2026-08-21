// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	FRealtimeMeshSectionProxy::FRealtimeMeshSectionProxy(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshSectionKey InKey)
		: Context(InContext)
		, Key(InKey)
		, BufferSetIndex(INDEX_NONE)
		, bRangeChanged(false)
		, IndirectArgsOffset(0)
	{
	}

	FRealtimeMeshSectionProxy::~FRealtimeMeshSectionProxy()
	{
		check(IsInRenderingThread());
	}

	TSharedRef<FRealtimeMeshSectionProxy> FRealtimeMeshSectionProxy::Clone() const
	{
		return MakeShared<FRealtimeMeshSectionProxy>(*this);
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
			bRangeChanged = true;
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

		// GPU-driven indirect draw: NumPrimitives==0 tells the engine to source the index count
		// from IndirectArgsBuffer. The vertex/index range still bounds the draw to the allocated
		// capacity (StreamRange), the args buffer just decides how much of it is drawn this frame.
		const bool bIndirect = IndirectArgsBuffer.IsValid();
		BatchElement.IndirectArgsBuffer = bIndirect ? IndirectArgsBuffer.GetReference() : nullptr;
		BatchElement.IndirectArgsOffset = bIndirect ? IndirectArgsOffset : 0;

		BatchElement.FirstIndex = StreamRange.GetMinIndex();
		BatchElement.NumPrimitives = bIndirect ? 0 : StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE);

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
#if RMC_ENGINE_BELOW_5_8 // 5.8 removed FMeshBatchElement::bIsSplineProxy
		BatchElement.bIsSplineProxy = false;
#endif
		BatchElement.bIsInstanceRuns = false;
		BatchElement.bForceInstanceCulling = false;
		BatchElement.bPreserveInstanceOrder = false;
		BatchElement.bFetchInstanceCountFromScene = false;
		
#if UE_ENABLE_DEBUG_DRAWING
		BatchElement.VisualizeElementIndex = INDEX_NONE;
#endif

		check(BatchElement.NumPrimitives <= (static_cast<const FRealtimeMeshIndexBuffer*>(BatchElement.IndexBuffer)->Num() - BatchElement.FirstIndex) / 3);
		check((int32)BatchElement.NumPrimitives <= StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE));
		check((int32)BatchElement.MaxVertexIndex <= StreamRange.GetMaxVertex());

		return true;
	}

	void FRealtimeMeshSectionProxy::UpdateCachedState(FRealtimeMeshBufferSetProxy& ParentGroup)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionProxy::UpdateCachedState);

		bRangeChanged = false;

		bool bHasValidMeshData = StreamRange.NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE) > 0 &&
			StreamRange.NumVertices() >= REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE;

		if (bHasValidMeshData)
		{
			// The range must also be one the bound vertex factory can actually serve.
			bHasValidMeshData = ParentGroup.GetVertexFactory().IsValid() && ParentGroup.GetVertexFactory()->IsValidStreamRange(StreamRange);
		}

		DrawMask = FRealtimeMeshDrawMask();

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

	void FRealtimeMeshSectionProxy::Reset()
	{
		Config = FRealtimeMeshSectionConfig();
		StreamRange = FRealtimeMeshStreamRange();
		DrawMask = FRealtimeMeshDrawMask();
	}
}
