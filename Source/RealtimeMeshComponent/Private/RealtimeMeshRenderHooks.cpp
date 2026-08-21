// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshRenderHooks.h"

namespace RealtimeMesh
{
	FRealtimeMeshPreRenderFrame& OnRealtimeMeshPreRenderFrame()
	{
		static FRealtimeMeshPreRenderFrame Delegate;
		return Delegate;
	}
}
