// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"

struct FRealtimeMeshSectionConfig
{
	int32 MaterialSlot;
	bool bIsVisible;
	bool bCastsShadow;
	bool bIsMainPassRenderable;
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

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionConfig& Config);
};