// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DistanceFieldAtlas.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshDistanceField.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshDistanceField
{
	GENERATED_BODY()
private:
	TStaticArray<FSparseDistanceFieldMip, DistanceField::NumMips> Mips;
	TArray<uint8> AlwaysLoadedMip;
	FByteBulkData StreamableMips;
	
	FBox3f Bounds;
	bool bMostlyTwoSided;
public:
	FRealtimeMeshDistanceField();
	FRealtimeMeshDistanceField(const FDistanceFieldVolumeData& Src);
	~FRealtimeMeshDistanceField();

	bool IsValid() const;

	const FBox3f& GetBounds() const { return Bounds; }
	
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const;
	SIZE_T GetResourceSizeBytes() const;

	void Serialize(FArchive& Ar, UObject* Owner);

	FDistanceFieldVolumeData CreateRenderingData() const;
	FDistanceFieldVolumeData MoveToRenderingData();
};
