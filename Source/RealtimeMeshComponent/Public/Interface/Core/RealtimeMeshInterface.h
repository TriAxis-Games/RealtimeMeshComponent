// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshInterfaceFwd.h"
#include "RealtimeMeshKeys.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshStreamSet;

	class IRealtimeMesh_v0
	{
	public:
		static FName GetModularFeatureName()
		{
			static FName FeatureName = TEXT("IRealtimeMesh_v0");
			return FeatureName;
		}
			
		virtual ~IRealtimeMesh_v0() = default;
	};

}


