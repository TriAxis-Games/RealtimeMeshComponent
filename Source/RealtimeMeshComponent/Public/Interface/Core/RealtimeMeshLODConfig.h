// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"

struct FRealtimeMeshLODConfig
{
	bool bIsVisible;
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
	
	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODConfig& Config);
};