// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.


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


struct FSparseDistanceFieldMip_OLD
{
	FIntVector IndirectionDimensions;
	int32 NumDistanceFieldBricks;
	FVector VolumeToVirtualUVScale;
	FVector VolumeToVirtualUVAdd;
	FVector2D DistanceFieldToVolumeScaleBias;
	uint32 BulkOffset;
	uint32 BulkSize;

	friend FArchive& operator<<(FArchive& Ar, FSparseDistanceFieldMip_OLD& Mip)
	{
		Ar << Mip.IndirectionDimensions << Mip.NumDistanceFieldBricks << Mip.VolumeToVirtualUVScale << Mip.VolumeToVirtualUVAdd << Mip.DistanceFieldToVolumeScaleBias << Mip.BulkOffset << Mip.BulkSize;
		return Ar;
	}
};


void FRealtimeMeshDistanceField::Serialize(FArchive& Ar, UObject* Owner)
{
	Ar << Bounds;
	Ar << bMostlyTwoSided;

#if RMC_ENGINE_ABOVE_5_4
	if (Ar.EngineVer().GetMajor() == 5 && Ar.EngineVer().GetMinor() < 4)
	{
		TStaticArray<FSparseDistanceFieldMip_OLD, DistanceField::NumMips> OldMips;
		Ar << OldMips;

		for (int32 Index = 0; Index < Mips.Num(); Index++)
		{
			Mips[Index].IndirectionDimensions = OldMips[Index].IndirectionDimensions;
			Mips[Index].NumDistanceFieldBricks = OldMips[Index].NumDistanceFieldBricks;
			Mips[Index].VolumeToVirtualUVScale = FVector3f(OldMips[Index].VolumeToVirtualUVScale);
			Mips[Index].VolumeToVirtualUVAdd = FVector3f(OldMips[Index].VolumeToVirtualUVAdd);
			Mips[Index].DistanceFieldToVolumeScaleBias = FVector2f(OldMips[Index].DistanceFieldToVolumeScaleBias);
			Mips[Index].BulkOffset = OldMips[Index].BulkOffset;
			Mips[Index].BulkSize = OldMips[Index].BulkSize;
		}		
	}
	else
#endif
	{
		Ar << Mips;
	}

	

	Ar << AlwaysLoadedMip;
	StreamableMips.Serialize(Ar, Owner, 0);
}

bool FRealtimeMeshDistanceField::IsValid() const
{
	return Mips[0].IndirectionDimensions.GetMax() > 0;
}

FDistanceFieldVolumeData FRealtimeMeshDistanceField::CreateRenderingData() const
{
	FDistanceFieldVolumeData RenderingData;
#if RMC_ENGINE_ABOVE_5_4
	RenderingData.LocalSpaceMeshBounds = Bounds;
#else
	RenderingData.LocalSpaceMeshBounds = FBox(Bounds);
#endif
	RenderingData.bMostlyTwoSided = bMostlyTwoSided;
	RenderingData.Mips = Mips;
	RenderingData.AlwaysLoadedMip = AlwaysLoadedMip;
	RenderingData.StreamableMips = StreamableMips;	
	return RenderingData;
}

FDistanceFieldVolumeData FRealtimeMeshDistanceField::MoveToRenderingData()
{
	FDistanceFieldVolumeData RenderingData;
#if RMC_ENGINE_ABOVE_5_4
	RenderingData.LocalSpaceMeshBounds = MoveTemp(Bounds);
#else
	RenderingData.LocalSpaceMeshBounds = FBox(Bounds);
	Bounds.Init();
#endif
	RenderingData.bMostlyTwoSided = MoveTemp(bMostlyTwoSided);
	RenderingData.Mips = MoveTemp(Mips);
	RenderingData.AlwaysLoadedMip = MoveTemp(AlwaysLoadedMip);
	RenderingData.StreamableMips = MoveTemp(StreamableMips);
	return RenderingData;	
}
