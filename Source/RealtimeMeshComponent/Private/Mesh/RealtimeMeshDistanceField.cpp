// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshDistanceField.h"


FRealtimeMeshDistanceField::FRealtimeMeshDistanceField()
	: Bounds(ForceInit)
	, bMostlyTwoSided(false)
{
}

FRealtimeMeshDistanceField::FRealtimeMeshDistanceField(const FDistanceFieldVolumeData& Src)
	: Mips(Src.Mips)
	, AlwaysLoadedMip(Src.AlwaysLoadedMip)
	, StreamableMips(Src.StreamableMips)
	, Bounds(Src.LocalSpaceMeshBounds)
	, bMostlyTwoSided(Src.bMostlyTwoSided)
{
}

FRealtimeMeshDistanceField::~FRealtimeMeshDistanceField()
{
}

void FRealtimeMeshDistanceField::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const
{
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(sizeof(*this));
		
	for (const FSparseDistanceFieldMip& Mip : Mips)
	{
		Mip.GetResourceSizeEx(CumulativeResourceSize);
	}

	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(AlwaysLoadedMip.GetAllocatedSize());
}

SIZE_T FRealtimeMeshDistanceField::GetResourceSizeBytes() const
{
	FResourceSizeEx ResSize;
	GetResourceSizeEx(ResSize);
	return ResSize.GetTotalMemoryBytes();
}

void FRealtimeMeshDistanceField::Serialize(FArchive& Ar, UObject* Owner)
{	
	Ar << Bounds << bMostlyTwoSided << Mips << AlwaysLoadedMip;
	StreamableMips.Serialize(Ar, Owner, 0);
}

bool FRealtimeMeshDistanceField::IsValid() const
{
	return Mips[0].IndirectionDimensions.GetMax() > 0;
}

FDistanceFieldVolumeData FRealtimeMeshDistanceField::CreateRenderingData() const
{
	FDistanceFieldVolumeData RenderingData;
	RenderingData.LocalSpaceMeshBounds = Bounds;
	RenderingData.bMostlyTwoSided = bMostlyTwoSided;
	RenderingData.Mips = Mips;
	RenderingData.AlwaysLoadedMip = AlwaysLoadedMip;
	RenderingData.StreamableMips = StreamableMips;	
	return RenderingData;
}

FDistanceFieldVolumeData FRealtimeMeshDistanceField::MoveToRenderingData()
{
	FDistanceFieldVolumeData RenderingData;
	RenderingData.LocalSpaceMeshBounds = MoveTemp(Bounds);
	RenderingData.bMostlyTwoSided = MoveTemp(bMostlyTwoSided);
	RenderingData.Mips = MoveTemp(Mips);
	RenderingData.AlwaysLoadedMip = MoveTemp(AlwaysLoadedMip);
	RenderingData.StreamableMips = MoveTemp(StreamableMips);
	return RenderingData;	
}
