// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshSection.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "RuntimeMeshUpdateCommands.h"

template<typename Type>
struct FRuntimeMeshStreamAccessor
{
	const TArray<uint8>* Data;
	int32 Offset;
	int32 Stride;
public:
	FRuntimeMeshStreamAccessor(const TArray<uint8>* InData, int32 InOffset, int32 InStride)
		: Data(InData), Offset(InOffset), Stride(InStride)
	{
	}
	virtual ~FRuntimeMeshStreamAccessor() { }

	int32 Num() const { return Data->Num() / Stride; }

	Type& Get(int32 Index)
	{
		int32 StartPosition = (Index * Stride + Offset);
		return *((Type*)(&(*Data)[StartPosition]));
	}
};

// Helper for accessing position element within a vertex stream
struct FRuntimeMeshVertexStreamPositionAccessor : public FRuntimeMeshStreamAccessor<FVector>
{
public:
	FRuntimeMeshVertexStreamPositionAccessor(TArray<uint8>* InData, const FRuntimeMeshVertexStreamStructure& StreamStructure)
		: FRuntimeMeshStreamAccessor<FVector>(InData, StreamStructure.Position.Offset, StreamStructure.Position.Stride)
	{
		check(StreamStructure.Position.IsValid());
	}
};

struct FRuntimeMeshVertexStreamUVAccessor
{
	virtual ~FRuntimeMeshVertexStreamUVAccessor() { }

	virtual FVector2D GetUV(int32 Index) = 0;
	virtual int32 Num() = 0;
};

// Helper for accessing position element within a vertex stream
struct FRuntimeMeshVertexStreamUVFullPrecisionAccessor : public FRuntimeMeshStreamAccessor<FVector2D>, public FRuntimeMeshVertexStreamUVAccessor
{
public:
	FRuntimeMeshVertexStreamUVFullPrecisionAccessor(TArray<uint8>* InData, const FRuntimeMeshVertexStreamStructureElement& Element)
		: FRuntimeMeshStreamAccessor<FVector2D>(InData, Element.Offset, Element.Stride)
	{
		check(Element.IsValid());
	}

	virtual FVector2D GetUV(int32 Index) override
	{
		return Get(Index);
	}

	virtual int32 Num() override
	{
		return FRuntimeMeshStreamAccessor<FVector2D>::Num();
	}
};
struct FRuntimeMeshVertexStreamUVHalfPrecisionAccessor : public FRuntimeMeshStreamAccessor<FVector2DHalf>, public FRuntimeMeshVertexStreamUVAccessor
{
public:
	FRuntimeMeshVertexStreamUVHalfPrecisionAccessor(TArray<uint8>* InData, const FRuntimeMeshVertexStreamStructureElement& Element)
		: FRuntimeMeshStreamAccessor<FVector2DHalf>(InData, Element.Offset, Element.Stride)
	{
		check(Element.IsValid());
	}

	virtual FVector2D GetUV(int32 Index) override
	{
		return Get(Index);
	}

	virtual int32 Num() override
	{
		return FRuntimeMeshStreamAccessor<FVector2DHalf>::Num();
	}
};


void FRuntimeMeshSection::FSectionVertexBuffer::FillUpdateParams(FRuntimeMeshSectionVertexBufferParams& Params)
{
	Params.VertexStructure = VertexStructure;
	Params.Data = Data;
	Params.NumVertices = GetNumVertices();
}

void FRuntimeMeshSection::FSectionIndexBuffer::FillUpdateParams(FRuntimeMeshSectionIndexBufferParams& Params)
{
	Params.b32BitIndices = b32BitIndices;
	Params.Data = Data;
	Params.NumIndices = GetNumIndices();
}

FRuntimeMeshSection::FRuntimeMeshSection(const FRuntimeMeshVertexStreamStructure& InVertexStructure0, const FRuntimeMeshVertexStreamStructure& InVertexStructure1, 
	const FRuntimeMeshVertexStreamStructure& InVertexStructure2, bool b32BitIndices, EUpdateFrequency InUpdateFrequency/*, FRuntimeMeshLockProvider* InSyncRoot*/)
	: UpdateFrequency(InUpdateFrequency)
	, VertexBuffer0(InVertexStructure0)
	, VertexBuffer1(InVertexStructure1)
	, VertexBuffer2(InVertexStructure2)
	, IndexBuffer(b32BitIndices)
	, AdjacencyIndexBuffer(b32BitIndices)
	, LocalBoundingBox(EForceInit::ForceInitToZero)
	, bCollisionEnabled(false)
	, bIsVisible(true)
	, bCastsShadow(true)
//	, SyncRoot(InSyncRoot)
{
}

FRuntimeMeshSection::FRuntimeMeshSection(FArchive& Ar)
	: UpdateFrequency(EUpdateFrequency::Average)
	, VertexBuffer0(GetStreamStructure<FRuntimeMeshNullVertex>())
	, VertexBuffer1(GetStreamStructure<FRuntimeMeshNullVertex>())
	, VertexBuffer2(GetStreamStructure<FRuntimeMeshNullVertex>())
	, IndexBuffer(false)
	, AdjacencyIndexBuffer(false)
	, LocalBoundingBox(EForceInit::ForceInitToZero)
	, bCollisionEnabled(false)
	, bIsVisible(true)
	, bCastsShadow(true)
{
	Ar << *this;
}

int32 FRuntimeMeshSection::NumVertexStreams() const
{
	return
		(VertexBuffer0.IsEnabled() ? 1 : 0) +
		(VertexBuffer1.IsEnabled() ? 1 : 0) +
		(VertexBuffer2.IsEnabled() ? 1 : 0);
}

FRuntimeMeshSectionCreationParamsPtr FRuntimeMeshSection::GetSectionCreationParams()
{
	FRuntimeMeshSectionCreationParamsPtr CreationParams = MakeShared<FRuntimeMeshSectionCreationParams, ESPMode::NotThreadSafe>();

	CreationParams->UpdateFrequency = UpdateFrequency;

	VertexBuffer0.FillUpdateParams(CreationParams->VertexBuffer0);
	VertexBuffer1.FillUpdateParams(CreationParams->VertexBuffer1);
	VertexBuffer2.FillUpdateParams(CreationParams->VertexBuffer2);

	IndexBuffer.FillUpdateParams(CreationParams->IndexBuffer);
	AdjacencyIndexBuffer.FillUpdateParams(CreationParams->AdjacencyIndexBuffer);

	CreationParams->bIsVisible = bIsVisible;
	CreationParams->bCastsShadow = bCastsShadow;

	return CreationParams;
}

FRuntimeMeshSectionUpdateParamsPtr FRuntimeMeshSection::GetSectionUpdateData(ERuntimeMeshBuffersToUpdate BuffersToUpdate)
{
	FRuntimeMeshSectionUpdateParamsPtr UpdateParams = MakeShared<FRuntimeMeshSectionUpdateParams, ESPMode::NotThreadSafe>();

	UpdateParams->BuffersToUpdate = BuffersToUpdate;

	if (!!(BuffersToUpdate & ERuntimeMeshBuffersToUpdate::VertexBuffer0))
	{
		VertexBuffer0.FillUpdateParams(UpdateParams->VertexBuffer0);
	}

	if (!!(BuffersToUpdate & ERuntimeMeshBuffersToUpdate::VertexBuffer1))
	{
		VertexBuffer1.FillUpdateParams(UpdateParams->VertexBuffer1);
	}

	if (!!(BuffersToUpdate & ERuntimeMeshBuffersToUpdate::VertexBuffer2))
	{
		VertexBuffer2.FillUpdateParams(UpdateParams->VertexBuffer2);
	}

	if (!!(BuffersToUpdate & ERuntimeMeshBuffersToUpdate::IndexBuffer))
	{
		IndexBuffer.FillUpdateParams(UpdateParams->IndexBuffer);
	}

	if (!!(BuffersToUpdate & ERuntimeMeshBuffersToUpdate::AdjacencyIndexBuffer))
	{
		AdjacencyIndexBuffer.FillUpdateParams(UpdateParams->AdjacencyIndexBuffer);
	}

	return UpdateParams;
}

TSharedPtr<struct FRuntimeMeshSectionPropertyUpdateParams, ESPMode::NotThreadSafe> FRuntimeMeshSection::GetSectionPropertyUpdateData()
{
	FRuntimeMeshSectionPropertyUpdateParamsPtr UpdateParams = MakeShared<FRuntimeMeshSectionPropertyUpdateParams, ESPMode::NotThreadSafe>();

	UpdateParams->bCastsShadow = bCastsShadow;
	UpdateParams->bIsVisible = bIsVisible;

	return UpdateParams;
}

void FRuntimeMeshSection::UpdateBoundingBox()
{
	FRuntimeMeshVertexStreamPositionAccessor PositionAccessor(&VertexBuffer0.GetData(), VertexBuffer0.GetStructure());
	check(PositionAccessor.Num() == VertexBuffer0.GetNumVertices());

	FBox NewBoundingBox = FBox(EForceInit::ForceInitToZero);

	for (int32 Index = 0; Index < PositionAccessor.Num(); Index++)
	{
		NewBoundingBox += PositionAccessor.Get(Index);
	}

	LocalBoundingBox = NewBoundingBox;
}

int32 FRuntimeMeshSection::GetCollisionData(TArray<FVector>& OutPositions, TArray<FTriIndices>& OutIndices, TArray<FVector2D>& OutUVs)
{
	FRuntimeMeshVertexStreamPositionAccessor PositionAccessor(&VertexBuffer0.GetData(), VertexBuffer0.GetStructure());
	check(PositionAccessor.Num() == VertexBuffer0.GetNumVertices());

	int32 StartVertexPosition = OutPositions.Num();

	for (int32 Index = 0; Index < PositionAccessor.Num(); Index++)
	{
		OutPositions.Add(PositionAccessor.Get(Index));
	}

	bool bCopyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults;

	if (bCopyUVs)
	{
		TUniquePtr<FRuntimeMeshVertexStreamUVAccessor> StreamAccessor = nullptr;

		const auto SetupStreamAccessor = [](TArray<uint8>* Data, const FRuntimeMeshVertexStreamStructureElement& Element) -> TUniquePtr<FRuntimeMeshVertexStreamUVAccessor>
		{
			if (Element.Type == VET_Float2 || Element.Type == VET_Float4)
			{
				return MakeUnique<FRuntimeMeshVertexStreamUVFullPrecisionAccessor>(Data, Element);
			}
			else
			{
				check(Element.Type == VET_Half2 || Element.Type == VET_Half4);
				return MakeUnique<FRuntimeMeshVertexStreamUVHalfPrecisionAccessor>(Data, Element);
			}
		};

		if (VertexBuffer0.GetStructure().HasUVs())
		{
			StreamAccessor = SetupStreamAccessor(&VertexBuffer0.GetData(), VertexBuffer0.GetStructure().UVs[0]);
		}
		else if (VertexBuffer1.GetStructure().HasUVs())
		{
			StreamAccessor = SetupStreamAccessor(&VertexBuffer1.GetData(), VertexBuffer1.GetStructure().UVs[0]);
		}
		else if (VertexBuffer2.GetStructure().HasUVs())
		{
			StreamAccessor = SetupStreamAccessor(&VertexBuffer2.GetData(), VertexBuffer2.GetStructure().UVs[0]);
		}
		else
		{
			// Add blank entries since we can't get UV's for this section
			OutUVs.AddZeroed(PositionAccessor.Num());
		}

		if (StreamAccessor.IsValid())
		{
			OutUVs.Reserve(OutUVs.Num() + StreamAccessor->Num());
			for (int32 Index = 0; Index < StreamAccessor->Num(); Index++)
			{
				OutUVs.Add(StreamAccessor->GetUV(Index));
			}
		}
	}

	TArray<uint8>& IndexData = IndexBuffer.GetData();

	if (IndexBuffer.Is32BitIndices())
	{
		int32 NumIndices = IndexBuffer.GetNumIndices();
		for (int32 Index = 0; Index < NumIndices; Index += 3)
		{
			// Add the triangle
			FTriIndices& Triangle = *new (OutIndices) FTriIndices;
			Triangle.v0 = (*((int32*)&IndexData[(Index + 0) * 4])) + StartVertexPosition;
			Triangle.v1 = (*((int32*)&IndexData[(Index + 1) * 4])) + StartVertexPosition;
			Triangle.v2 = (*((int32*)&IndexData[(Index + 2) * 4])) + StartVertexPosition;
		}
	}
	else
	{
		int32 NumIndices = IndexBuffer.GetNumIndices();
		for (int32 Index = 0; Index < NumIndices; Index += 3)
		{
			// Add the triangle
			FTriIndices& Triangle = *new (OutIndices) FTriIndices;
			Triangle.v0 = (*((uint16*)&IndexData[(Index + 0) * 2])) + StartVertexPosition;
			Triangle.v1 = (*((uint16*)&IndexData[(Index + 1) * 2])) + StartVertexPosition;
			Triangle.v2 = (*((uint16*)&IndexData[(Index + 2) * 2])) + StartVertexPosition;
		}
	}


	return IndexBuffer.GetNumIndices() / 3;
}


