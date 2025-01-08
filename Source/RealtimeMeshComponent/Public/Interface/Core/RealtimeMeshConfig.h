// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"

struct FRealtimeMeshConfig
{
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