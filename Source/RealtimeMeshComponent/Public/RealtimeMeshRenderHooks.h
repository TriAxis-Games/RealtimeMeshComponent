// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"

class FRDGBuilder;

namespace RealtimeMesh
{
	// Per-frame info handed to pre-render hooks (for animated / time-based GPU work).
	struct FRealtimeMeshRenderFrameInfo
	{
		float TimeSeconds = 0.0f;
		float DeltaSeconds = 0.0f;
		uint32 FrameNumber = 0;
		// View.Family->FrameCounter — must match ResolvedView.FrameCounter for the motion-vector
		// (prev-position) gate to engage. Used to stamp the prev-position loose uniform buffer.
		uint32 FrameCounter = 0;
	};

	DECLARE_MULTICAST_DELEGATE_TwoParams(FRealtimeMeshPreRenderFrame, FRDGBuilder&, const FRealtimeMeshRenderFrameInfo&);

	/**
	 * Render-thread extension point: broadcast by the RMC scene view extension inside each frame's
	 * RDG graph, before the base pass consumes RMC geometry. External modules (e.g.
	 * RealtimeMeshCompute) add hooks here to record RDG passes that generate or mutate mesh streams
	 * on the GPU.
	 *
	 * Add/Remove and Broadcast all happen on the render thread — enqueue a render command to
	 * (un)register. The scene view extension is per-world, so a hook can fire more than once per
	 * frame (multiple view families / worlds); hooks that must run once per frame should guard on
	 * FrameNumber.
	 */
	REALTIMEMESHCOMPONENT_API FRealtimeMeshPreRenderFrame& OnRealtimeMeshPreRenderFrame();
}
