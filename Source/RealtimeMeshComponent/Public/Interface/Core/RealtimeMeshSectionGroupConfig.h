// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"

/* The rendering path to use for this section.
 * Static has lower overhead but requires a proxy recreation on change for all components
 * Dynamic has slightly higher overhead but allows for more efficient section updates
 */
enum class ERealtimeMeshSectionDrawType : uint8
{
	Static,
	Dynamic,
};

struct FRealtimeMeshSectionGroupConfig
{
	ERealtimeMeshSectionDrawType DrawType;
	
	FRealtimeMeshSectionGroupConfig(ERealtimeMeshSectionDrawType InDrawType = ERealtimeMeshSectionDrawType::Static)
		: DrawType(InDrawType)
	{ }

	bool operator==(const FRealtimeMeshSectionGroupConfig& Other) const
	{
		return DrawType == Other.DrawType;
	}

	bool operator!=(const FRealtimeMeshSectionGroupConfig& Other) const
	{
		return !(*this == Other);
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionGroupConfig& Config);
};