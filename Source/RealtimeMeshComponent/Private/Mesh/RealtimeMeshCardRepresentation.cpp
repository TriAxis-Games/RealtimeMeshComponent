// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshCardRepresentation.h"
#include "MeshCardBuild.h"


FRealtimeMeshCardRepresentation::FRealtimeMeshCardRepresentation()
	: Bounds(ForceInit)
	, bMostlyTwoSided(false)
{
}

FRealtimeMeshCardRepresentation::FRealtimeMeshCardRepresentation(const FCardRepresentationData& Src)
	: Bounds(Src.MeshCardsBuildData.Bounds)
	, bMostlyTwoSided(Src.MeshCardsBuildData.bMostlyTwoSided)
	, CardBuildData(Src.MeshCardsBuildData.CardBuildData)
{
}

FRealtimeMeshCardRepresentation::~FRealtimeMeshCardRepresentation()
{
}

bool FRealtimeMeshCardRepresentation::IsValid() const
{
	return Bounds.IsValid && CardBuildData.Num() > 0;
}


void FRealtimeMeshCardRepresentation::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const
{	
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(sizeof(*this));
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(CardBuildData.GetAllocatedSize());
}

SIZE_T FRealtimeMeshCardRepresentation::GetResourceSizeBytes() const
{
	FResourceSizeEx ResSize;
	GetResourceSizeEx(ResSize);
	return ResSize.GetTotalMemoryBytes();
}

void FRealtimeMeshCardRepresentation::Serialize(FArchive& Ar, UObject* Owner)
{
	Ar << Bounds;
	Ar << bMostlyTwoSided;
	Ar << CardBuildData;
}


std::atomic<uint32> FRealtimeMeshCardRepresentation::NextCardRepresentationId { TNumericLimits<uint32>().Max() };

FCardRepresentationData FRealtimeMeshCardRepresentation::CreateRenderingData() const
{
	FCardRepresentationData RenderingData;
	RenderingData.CardRepresentationDataId.Value = NextCardRepresentationId--;
	RenderingData.MeshCardsBuildData.Bounds = Bounds;
	RenderingData.MeshCardsBuildData.bMostlyTwoSided = bMostlyTwoSided;
	RenderingData.MeshCardsBuildData.CardBuildData = CardBuildData;
	return RenderingData;
}

FCardRepresentationData FRealtimeMeshCardRepresentation::MoveToRenderingData()
{
	FCardRepresentationData RenderingData;
	RenderingData.CardRepresentationDataId.Value = NextCardRepresentationId--;
	RenderingData.MeshCardsBuildData.Bounds = MoveTemp(Bounds);
	RenderingData.MeshCardsBuildData.bMostlyTwoSided = MoveTemp(bMostlyTwoSided);
	RenderingData.MeshCardsBuildData.CardBuildData = MoveTemp(CardBuildData);
	return RenderingData;
}
