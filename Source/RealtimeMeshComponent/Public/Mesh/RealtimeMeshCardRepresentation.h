// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshCore.h"

#if RMC_ENGINE_ABOVE_5_2
// This works around a compile issue in 5.2+ where there's invalid code at the bottom of MeshCardRepresentation.h
// TODO: I think this is still a bug to this day, so could submit a bug fix for it.
UE_PUSH_MACRO("UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2")
#ifdef UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#undef UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#endif
#define UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2 0
#include "MeshCardBuild.h"
UE_POP_MACRO("UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2")
#else
#include "MeshCardRepresentation.h"
#endif

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
