// Fill out your copyright notice in the Description page of Project Settings.


#include "RealtimeMeshSceneViewExtension.h"

#include "RealtimeMeshComponentModule.h"
#include "RenderGraphBuilder.h"
#include "RenderProxy/RealtimeMeshProxy.h"

TSet<RealtimeMesh::FRealtimeMeshProxyWeakPtr> FRealtimeMeshSceneViewExtension::ActiveProxies;

void FRealtimeMeshSceneViewExtension::RegisterProxy(const RealtimeMesh::FRealtimeMeshProxyPtr& Proxy)
{
	ActiveProxies.Add(Proxy);
}

void FRealtimeMeshSceneViewExtension::UnregisterProxy(const RealtimeMesh::FRealtimeMeshProxyPtr& Proxy)
{
	ActiveProxies.Remove(Proxy);
}

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
	/*for (auto It = ActiveProxies.CreateIterator(); It; ++It)
	{
		if (auto Pinned = It->Pin())
		{
			Pinned->ProcessCommands(GraphBuilder.RHICmdList);
		}
		else
		{
			It.RemoveCurrent();
		}
	}*/
}

void FRealtimeMeshSceneViewExtension::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
}
