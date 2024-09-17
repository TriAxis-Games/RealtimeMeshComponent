// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshSection.h"
#include "RealtimeMeshGuard.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshData.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"


namespace RealtimeMesh
{
	FRealtimeMeshSection::FRealtimeMeshSection(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey& InKey)
		: SharedResources(InSharedResources)
		  , Key(InKey)
	{
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMeshSection::GetSectionGroup() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		if (const auto Mesh = SharedResources->GetOwner())
		{
			return Mesh->GetSectionGroup(Key);
		}
		return nullptr;
	}

	FRealtimeMeshSectionConfig FRealtimeMeshSection::GetConfig() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return Config;
	}

	FRealtimeMeshStreamRange FRealtimeMeshSection::GetStreamRange() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return StreamRange;
	}

	FBoxSphereBounds3f FRealtimeMeshSection::GetLocalBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return Bounds.GetBounds([&]() { return CalculateBounds(); });
	}

	void FRealtimeMeshSection::Initialize(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InRange)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		Config = InConfig;
		StreamRange = InRange;
		Bounds.Reset();

		if (ProxyBuilder)
		{
			InitializeProxy(ProxyBuilder);
		}

		SharedResources->BroadcastSectionBoundsChanged(Key);
		SharedResources->BroadcastSectionConfigChanged(Key);
		SharedResources->BroadcastSectionStreamRangeChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::Reset()
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		Reset(UpdateContext);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSection::Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		Initialize(ProxyBuilder, FRealtimeMeshSectionConfig(), FRealtimeMeshStreamRange());
	}

	void FRealtimeMeshSection::SetOverrideBounds(const FBoxSphereBounds3f& InBounds)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		Bounds.SetUserSetBounds(InBounds);
		SharedResources->BroadcastSectionBoundsChanged(Key);
	}

	void FRealtimeMeshSection::ClearOverrideBounds()
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		Bounds.ClearUserSetBounds();
		SharedResources->BroadcastSectionBoundsChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::UpdateConfig(const FRealtimeMeshSectionConfig& InConfig)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		UpdateConfig(UpdateContext, [InConfig](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig = InConfig; });
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSection::UpdateConfig(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionConfig& InConfig)
	{
		UpdateConfig(ProxyBuilder, [InConfig](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig = InConfig; });
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::UpdateConfig(TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		UpdateConfig(UpdateContext, EditFunc);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSection::UpdateConfig(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		bool bShouldRecreateProxy = ShouldRecreateProxyOnChange();
		EditFunc(Config);
		bShouldRecreateProxy |= ShouldRecreateProxyOnChange();

		if (ProxyBuilder)
		{
			ProxyBuilder.AddSectionTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, bShouldRecreateProxy);
		}

		SharedResources->BroadcastSectionConfigChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::UpdateStreamRange(const FRealtimeMeshStreamRange& InRange)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		UpdateStreamRange(UpdateContext, InRange);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSection::UpdateStreamRange(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshStreamRange& InRange)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		StreamRange = InRange;

		if (ProxyBuilder)
		{
			ProxyBuilder.AddSectionTask(Key, [StreamRange = StreamRange](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionProxy& Proxy)
			{
				Proxy.UpdateStreamRange(StreamRange);
			}, ShouldRecreateProxyOnChange());
		}

		SharedResources->BroadcastSectionStreamRangeChanged(Key);
		MarkBoundsDirtyIfNotOverridden();
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::SetVisibility(bool bIsVisible)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		UpdateConfig(UpdateContext, [bIsVisible](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bIsVisible = bIsVisible; });
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSection::SetVisibility(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, bool bIsVisible)
	{
		UpdateConfig(ProxyBuilder, [bIsVisible](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bIsVisible = bIsVisible; });
	}


	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::SetCastShadow(bool bCastShadow)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		UpdateConfig(UpdateContext, [bCastShadow](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bCastsShadow = bCastShadow; });
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSection::SetCastShadow(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, bool bCastShadow)
	{		
		UpdateConfig(ProxyBuilder, [bCastShadow](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bCastsShadow = bCastShadow; });
	}

	bool FRealtimeMeshSection::Serialize(FArchive& Ar)
	{
		Ar << Config;
		Ar << StreamRange;
		Ar << Bounds;
		return true;
	}

	void FRealtimeMeshSection::InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardReadCheck LockCheck(SharedResources);
		
		if (ProxyBuilder)
		{
			ProxyBuilder.AddSectionTask(Key, [Config = Config, StreamRange = StreamRange](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionProxy& Proxy)
			{
				Proxy.Reset();
				Proxy.UpdateConfig(Config);
				Proxy.UpdateStreamRange(StreamRange);
			}, ShouldRecreateProxyOnChange());
		}
	}

	void FRealtimeMeshSection::MarkBoundsDirtyIfNotOverridden() const
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		
		Bounds.ClearCachedValue();
		if (!Bounds.HasUserSetBounds())
		{
			SharedResources->BroadcastSectionBoundsChanged(Key);
		}
	}


	FBoxSphereBounds3f FRealtimeMeshSection::CalculateBounds() const
	{
		return FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f));
	}

	bool FRealtimeMeshSection::ShouldRecreateProxyOnChange() const
	{
		if (const auto ParentGroup = GetSectionGroup())
		{
			return ParentGroup->ShouldRecreateProxyOnChange();
		}

		return false;
	}
}
