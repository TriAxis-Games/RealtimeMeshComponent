// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"

struct IRealtimeMeshNaniteResources
{
	virtual ~IRealtimeMeshNaniteResources() = default;
	
	virtual void InitResources(URealtimeMesh* Owner) = 0;
};