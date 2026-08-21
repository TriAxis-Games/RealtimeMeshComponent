// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshCore.h"

#include "MeshCardRepresentation.h"
#include "MeshCardBuild.h"

#include "RealtimeMeshCardRepresentation.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCardRepresentation
{
	GENERATED_BODY()
private:
	FBox Bounds;
	bool bMostlyTwoSided;
	TArray<FLumenCardBuildData> CardBuildData;

	static std::atomic<uint32> NextCardRepresentationId;
public:
	FRealtimeMeshCardRepresentation();
	FRealtimeMeshCardRepresentation(const FCardRepresentationData& Src);
	~FRealtimeMeshCardRepresentation();

	bool IsValid() const;

	const FBox& GetBounds() const { return Bounds; }
		
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const;
	SIZE_T GetResourceSizeBytes() const;

	void Serialize(FArchive& Ar, UObject* Owner);

	FCardRepresentationData CreateRenderingData() const;
	FCardRepresentationData MoveToRenderingData();
};
