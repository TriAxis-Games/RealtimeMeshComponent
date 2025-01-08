// // Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved. your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreFwd.h"
#include "RealtimeMeshCore.h"
#include "SceneViewExtension.h"

class UWorld;

/*
 * We use this extension to manage exactly when updates are processed to the RMC.
 */
class FRealtimeMeshSceneViewExtension final : public FWorldSceneViewExtension
{
private:
	static TSet<RealtimeMesh::FRealtimeMeshProxyWeakPtr> ActiveProxies;
public:
	static void RegisterProxy(const RealtimeMesh::FRealtimeMeshProxyPtr& Proxy);
	static void UnregisterProxy(const RealtimeMesh::FRealtimeMeshProxyPtr& Proxy);

	FRealtimeMeshSceneViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld)
		: FWorldSceneViewExtension(AutoReg, InWorld)
	{
	}
protected:
	
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;
	virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;
};