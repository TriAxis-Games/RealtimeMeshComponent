// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "UObject/ObjectMacros.h"
#include "RealtimeMeshConfig.generated.h"

USTRUCT(BlueprintType)
struct FRealtimeMeshConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	int32 ForcedLOD;

	FRealtimeMeshConfig() : ForcedLOD(INDEX_NONE) { }

	bool operator==(const FRealtimeMeshConfig& Other) const
	{
		return ForcedLOD == Other.ForcedLOD;
	}

	bool operator!=(const FRealtimeMeshConfig& Other) const
	{
		return !(*this == Other);
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshConfig& Config);
};