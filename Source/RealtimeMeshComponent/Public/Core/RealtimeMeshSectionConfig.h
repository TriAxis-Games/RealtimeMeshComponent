// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "UObject/ObjectMacros.h"
#include "RealtimeMeshSectionConfig.generated.h"

USTRUCT(BlueprintType)
struct FRealtimeMeshSectionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	int32 MaterialSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	bool bIsVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	bool bCastsShadow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config", AdvancedDisplay)
	bool bIsMainPassRenderable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config", AdvancedDisplay)
	bool bForceOpaque;

	FRealtimeMeshSectionConfig(int32 InMaterialSlot = 0)
		: MaterialSlot(InMaterialSlot)
		, bIsVisible(true)
		, bCastsShadow(true)
		, bIsMainPassRenderable(true)
		, bForceOpaque(false)
	{ }

	bool operator==(const FRealtimeMeshSectionConfig& Other) const
	{
		return MaterialSlot == Other.MaterialSlot
			&& bIsVisible == Other.bIsVisible
			&& bCastsShadow == Other.bCastsShadow
			&& bIsMainPassRenderable == Other.bIsMainPassRenderable
			&& bForceOpaque == Other.bForceOpaque;
	}

	bool operator!=(const FRealtimeMeshSectionConfig& Other) const
	{
		return !(*this == Other);
	}

	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionConfig& Config);
};