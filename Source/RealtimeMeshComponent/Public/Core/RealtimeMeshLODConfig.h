// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "UObject/ObjectMacros.h"
#include "RealtimeMeshLODConfig.generated.h"

USTRUCT(BlueprintType)
struct FRealtimeMeshLODConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	bool bIsVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	float ScreenSize;

	FRealtimeMeshLODConfig(float InScreenSize = 0.0f)
		: bIsVisible(true)
		, ScreenSize(InScreenSize)
	{
	}

	bool operator==(const FRealtimeMeshLODConfig& Other) const
	{
		return bIsVisible == Other.bIsVisible
			&& ScreenSize == Other.ScreenSize;
	}

	bool operator!=(const FRealtimeMeshLODConfig& Other) const
	{
		return !(*this == Other);
	}
	
	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODConfig& Config);
};