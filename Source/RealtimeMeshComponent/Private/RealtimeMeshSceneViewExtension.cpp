// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshSceneViewExtension.h"

#include "RealtimeMeshComponentModule.h"
#include "RenderGraphBuilder.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RealtimeMeshRenderHooks.h"
#include "SceneView.h"

void FRealtimeMeshSceneViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FRealtimeMeshSceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
}

void FRealtimeMeshSceneViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FRealtimeMeshSceneViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	// Broadcast the pre-render extension point inside this frame's RDG graph, before the base pass
	// that consumes RMC geometry. External modules (e.g. RealtimeMeshCompute's provider driver)
	// register hooks here to record RDG passes that write compute-writable mesh streams.
	if (RealtimeMesh::OnRealtimeMeshPreRenderFrame().IsBound())
	{
		RealtimeMesh::FRealtimeMeshRenderFrameInfo Frame;
		Frame.TimeSeconds = static_cast<float>(InViewFamily.Time.GetWorldTimeSeconds());
		Frame.DeltaSeconds = InViewFamily.Time.GetDeltaWorldTimeSeconds();
		Frame.FrameNumber = InViewFamily.FrameNumber;
		Frame.FrameCounter = static_cast<uint32>(InViewFamily.FrameCounter);
		RealtimeMesh::OnRealtimeMeshPreRenderFrame().Broadcast(GraphBuilder, Frame);
	}
}

void FRealtimeMeshSceneViewExtension::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
}
