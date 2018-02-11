// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshSectionProxy.h"




FRuntimeMeshSectionProxy::FRuntimeMeshSectionProxy(FRuntimeMeshSectionCreationParamsPtr CreationData)
	: UpdateFrequency(CreationData->UpdateFrequency)
	, VertexFactory(this)
	, bIsVisible(CreationData->bIsVisible)
	, bCastsShadow(CreationData->bCastsShadow)
{
	check(IsInRenderingThread());
	check(FRuntimeMeshVertexStreamStructure::ValidTripleStream(CreationData->VertexBuffer0.VertexStructure,
		CreationData->VertexBuffer1.VertexStructure, CreationData->VertexBuffer2.VertexStructure));

	FLocalVertexFactory::FDataType DataType;
	AddBufferToVertexDataType(DataType, CreationData->VertexBuffer0.VertexStructure, &VertexBuffer0);
	AddBufferToVertexDataType(DataType, CreationData->VertexBuffer1.VertexStructure, &VertexBuffer1);
	AddBufferToVertexDataType(DataType, CreationData->VertexBuffer2.VertexStructure, &VertexBuffer2);


	VertexFactory.Init(DataType);
	VertexFactory.InitResource();

	VertexBuffer0.Reset(CreationData->VertexBuffer0.VertexStructure.CalculateStride(), CreationData->VertexBuffer0.NumVertices, UpdateFrequency);
	VertexBuffer0.SetData(CreationData->VertexBuffer0.Data);

	VertexBuffer1.Reset(CreationData->VertexBuffer1.VertexStructure.CalculateStride(), CreationData->VertexBuffer1.NumVertices, UpdateFrequency);
	VertexBuffer1.SetData(CreationData->VertexBuffer1.Data);

	VertexBuffer2.Reset(CreationData->VertexBuffer2.VertexStructure.CalculateStride(), CreationData->VertexBuffer2.NumVertices, UpdateFrequency);
	VertexBuffer2.SetData(CreationData->VertexBuffer2.Data);

	// 	bool bRequiresNullBuffer = RequiresNullBuffer(DataType);
	// 
	// 	if (bRequiresNullBuffer)
	// 	{
	// 		VertexNullBuffer.Reset(sizeof(FRuntimeMeshSectionNullBufferElement), 1, EUpdateFrequency::Infrequent);
	// 
	// 		TArray<uint8> NullVertex;
	// 		NullVertex.SetNum(sizeof(FRuntimeMeshSectionNullBufferElement));
	// 
	// 		// Set a single default element
	// 		*((FRuntimeMeshSectionNullBufferElement*)NullVertex.GetData()) = FRuntimeMeshSectionNullBufferElement();
	// 
	// 		VertexNullBuffer.SetData(NullVertex);
	// 	}

	IndexBuffer.Reset(CreationData->IndexBuffer.b32BitIndices ? 4 : 2, CreationData->IndexBuffer.NumIndices, UpdateFrequency);
	IndexBuffer.SetData(CreationData->IndexBuffer.Data);

	AdjacencyIndexBuffer.Reset(CreationData->IndexBuffer.b32BitIndices ? 4 : 2, CreationData->AdjacencyIndexBuffer.NumIndices, UpdateFrequency);
	AdjacencyIndexBuffer.SetData(CreationData->AdjacencyIndexBuffer.Data);
}

FRuntimeMeshSectionProxy::~FRuntimeMeshSectionProxy()
{
	check(IsInRenderingThread());

	VertexBuffer0.ReleaseResource();
	VertexBuffer1.ReleaseResource();
	VertexBuffer2.ReleaseResource();
	VertexNullBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
	AdjacencyIndexBuffer.ReleaseResource();
	VertexFactory.ReleaseResource();
}

bool FRuntimeMeshSectionProxy::ShouldRender()
{
	if (!bIsVisible)
		return false;

	if (IndexBuffer.Num() <= 0)
		return false;

	if (!VertexBuffer0.IsEnabled() || VertexBuffer0.Num() <= 0)
		return false;

	if (VertexBuffer1.IsEnabled() && (VertexBuffer1.Num() <= 0 || VertexBuffer0.Num() != VertexBuffer1.Num()))
		return false;

	if (VertexBuffer2.IsEnabled() && (VertexBuffer2.Num() <= 0 || VertexBuffer0.Num() != VertexBuffer2.Num()))
		return false;

	return true;
}

bool FRuntimeMeshSectionProxy::WantsToRenderInStaticPath() const
{
	return UpdateFrequency == EUpdateFrequency::Infrequent;
}

bool FRuntimeMeshSectionProxy::CastsShadow() const
{
	return bCastsShadow;
}


void FRuntimeMeshSectionProxy::CreateMeshBatch(FMeshBatch& MeshBatch, bool bWantsAdjacencyInfo)
{
	MeshBatch.VertexFactory = &VertexFactory;

	MeshBatch.Type = bWantsAdjacencyInfo ? PT_12_ControlPointPatchList : PT_TriangleList;

	MeshBatch.DepthPriorityGroup = SDPG_World;
	MeshBatch.CastShadow = bCastsShadow;

	// Make sure that if the material wants adjacency information, that you supply it
	check(!bWantsAdjacencyInfo || AdjacencyIndexBuffer.Num() > 0);

	FRuntimeMeshIndexBuffer* CurrentIndexBuffer = bWantsAdjacencyInfo ? &AdjacencyIndexBuffer : &IndexBuffer;

	int32 NumIndicesPerTriangle = bWantsAdjacencyInfo ? 12 : 3;
	int32 NumPrimitives = CurrentIndexBuffer->Num() / NumIndicesPerTriangle;

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.IndexBuffer = CurrentIndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = NumPrimitives;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = VertexBuffer0.Num() - 1;
}

void FRuntimeMeshSectionProxy::AddBufferToVertexDataType(FLocalVertexFactory::FDataType& DataType, const FRuntimeMeshVertexStreamStructure& Structure, FVertexBuffer* VertexBuffer)
{
	const auto AddComponent = [&VertexBuffer](FVertexStreamComponent& StreamComponent, const FRuntimeMeshVertexStreamStructureElement& Element)
	{
		if (Element.IsValid())
		{
			check(StreamComponent.Type == EVertexElementType::VET_None);
			StreamComponent = FVertexStreamComponent(VertexBuffer, Element.Offset, Element.Stride, Element.Type);
		}
	};

	AddComponent(DataType.PositionComponent, Structure.Position);
	AddComponent(DataType.TangentBasisComponents[1], Structure.Normal);
	AddComponent(DataType.TangentBasisComponents[0], Structure.Tangent);

	if (Structure.UVs.Num() > 0)
	{
		DataType.TextureCoordinates.SetNum(Structure.UVs.Num());
		for (int32 Index = 0; Index < Structure.UVs.Num(); Index++)
		{
			AddComponent(DataType.TextureCoordinates[Index], Structure.UVs[Index]);
		}
	}

	AddComponent(DataType.ColorComponent, Structure.Color);

}

bool FRuntimeMeshSectionProxy::RequiresNullBuffer(FLocalVertexFactory::FDataType& DataType)
{
	bool bNeededAnElement = false;

	if (DataType.TangentBasisComponents[1].Type == EVertexElementType::VET_None)
	{
		DataType.TangentBasisComponents[1] = FVertexStreamComponent(&VertexNullBuffer, STRUCT_OFFSET(FRuntimeMeshSectionNullBufferElement, Normal), 0, EVertexElementType::VET_PackedNormal);
		bNeededAnElement = true;
	}

	if (DataType.TangentBasisComponents[0].Type == EVertexElementType::VET_None)
	{
		DataType.TangentBasisComponents[0] = FVertexStreamComponent(&VertexNullBuffer, STRUCT_OFFSET(FRuntimeMeshSectionNullBufferElement, Tangent), 0, EVertexElementType::VET_PackedNormal);
		bNeededAnElement = true;
	}

	if (DataType.TextureCoordinates.Num() == 0 || DataType.TextureCoordinates[0].Type == EVertexElementType::VET_None)
	{
		DataType.TextureCoordinates.SetNum(1);
		DataType.TextureCoordinates[0] = FVertexStreamComponent(&VertexNullBuffer, STRUCT_OFFSET(FRuntimeMeshSectionNullBufferElement, UV0), 0, EVertexElementType::VET_Float2);
		bNeededAnElement = true;
	}

	if (DataType.ColorComponent.Type == EVertexElementType::VET_None)
	{
		DataType.ColorComponent = FVertexStreamComponent(&VertexNullBuffer, STRUCT_OFFSET(FRuntimeMeshSectionNullBufferElement, Color), 0, EVertexElementType::VET_Color);
		bNeededAnElement = true;
	}

	return bNeededAnElement;
}

void FRuntimeMeshSectionProxy::FinishUpdate_RenderThread(FRuntimeMeshSectionUpdateParamsPtr UpdateData)
{
	check(IsInRenderingThread());

	// Update vertex buffer 0
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::VertexBuffer0))
	{
		VertexBuffer0.SetNum(UpdateData->VertexBuffer0.NumVertices);
		VertexBuffer0.SetData(UpdateData->VertexBuffer0.Data);
	}

	// Update vertex buffer 1
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::VertexBuffer1))
	{
		VertexBuffer1.SetNum(UpdateData->VertexBuffer1.NumVertices);
		VertexBuffer1.SetData(UpdateData->VertexBuffer1.Data);
	}

	// Update vertex buffer 2
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::VertexBuffer2))
	{
		VertexBuffer2.SetNum(UpdateData->VertexBuffer2.NumVertices);
		VertexBuffer2.SetData(UpdateData->VertexBuffer2.Data);
	}

	// Update index buffer
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::IndexBuffer))
	{
		IndexBuffer.SetNum(UpdateData->IndexBuffer.NumIndices);
		IndexBuffer.SetData(UpdateData->IndexBuffer.Data);
	}

	// Update index buffer
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::AdjacencyIndexBuffer))
	{
		AdjacencyIndexBuffer.SetNum(UpdateData->AdjacencyIndexBuffer.NumIndices);
		AdjacencyIndexBuffer.SetData(UpdateData->AdjacencyIndexBuffer.Data);
	}
}

void FRuntimeMeshSectionProxy::FinishPropertyUpdate_RenderThread(FRuntimeMeshSectionPropertyUpdateParamsPtr UpdateData)
{
	// Copy visibility/shadow
	bIsVisible = UpdateData->bIsVisible;
	bCastsShadow = UpdateData->bCastsShadow;
}
