// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"

namespace RealtimeMesh
{
	struct IRealtimeMeshNaniteResources
	{
		virtual ~IRealtimeMeshNaniteResources() = default;
	
		virtual void InitResources(URealtimeMesh* Owner) = 0;
	};
}