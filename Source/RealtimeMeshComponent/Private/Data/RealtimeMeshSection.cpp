// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshSection.h"
#include "RealtimeMeshGuard.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshData.h"
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
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		if (const auto Mesh = SharedResources->GetOwner())
		{
			return Mesh->GetSectionGroup(Key);
		}
		return nullptr;
	}

	FRealtimeMeshSectionConfig FRealtimeMeshSection::GetConfig() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Config;
	}

	FRealtimeMeshStreamRange FRealtimeMeshSection::GetStreamRange() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return StreamRange;
	}

	FBoxSphereBounds3f FRealtimeMeshSection::GetLocalBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Bounds.GetBounds([&]() { return CalculateBounds(); });
	}

	void FRealtimeMeshSection::Initialize(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InRange)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Config = InConfig;
		StreamRange = InRange;
		Bounds.Reset();

		if (Commands)
		{
			InitializeProxy(Commands);
		}

		SharedResources->BroadcastSectionBoundsChanged(Key);
		SharedResources->BroadcastSectionConfigChanged(Key);
		SharedResources->BroadcastSectionStreamRangeChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::Reset()
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		Reset(Commands);
		return Commands.Commit();
	}

	void FRealtimeMeshSection::Reset(FRealtimeMeshProxyCommandBatch& Commands)
	{
		Initialize(Commands, FRealtimeMeshSectionConfig(), FRealtimeMeshStreamRange());
	}

	void FRealtimeMeshSection::SetOverrideBounds(const FBoxSphereBounds3f& InBounds)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Bounds.SetUserSetBounds(InBounds);
		SharedResources->BroadcastSectionBoundsChanged(Key);
	}

	void FRealtimeMeshSection::ClearOverrideBounds()
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Bounds.ClearUserSetBounds();
		SharedResources->BroadcastSectionBoundsChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::UpdateConfig(const FRealtimeMeshSectionConfig& InConfig)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		UpdateConfig(Commands, [InConfig](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig = InConfig; });
		return Commands.Commit();
	}

	void FRealtimeMeshSection::UpdateConfig(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionConfig& InConfig)
	{
		UpdateConfig(Commands, [InConfig](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig = InConfig; });
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::UpdateConfig(TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		UpdateConfig(Commands, EditFunc);
		return Commands.Commit();
	}

	void FRealtimeMeshSection::UpdateConfig(FRealtimeMeshProxyCommandBatch& Commands, TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		bool bShouldRecreateProxy = ShouldRecreateProxyOnChange();
		EditFunc(Config);
		bShouldRecreateProxy |= ShouldRecreateProxyOnChange();

		if (Commands)
		{
			Commands.AddSectionTask(Key, [Config = Config](FRealtimeMeshSectionProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, bShouldRecreateProxy);
		}

		SharedResources->BroadcastSectionConfigChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::UpdateStreamRange(const FRealtimeMeshStreamRange& InRange)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		UpdateStreamRange(Commands, InRange);
		return Commands.Commit();
	}

	void FRealtimeMeshSection::UpdateStreamRange(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamRange& InRange)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		StreamRange = InRange;

		if (Commands)
		{
			Commands.AddSectionTask(Key, [StreamRange = StreamRange](FRealtimeMeshSectionProxy& Proxy)
			{
				Proxy.UpdateStreamRange(StreamRange);
			}, ShouldRecreateProxyOnChange());
		}

		SharedResources->BroadcastSectionStreamRangeChanged(Key);
		MarkBoundsDirtyIfNotOverridden();
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::SetVisibility(bool bIsVisible)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		UpdateConfig(Commands, [bIsVisible](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bIsVisible = bIsVisible; });
		return Commands.Commit();
	}

	void FRealtimeMeshSection::SetVisibility(FRealtimeMeshProxyCommandBatch& Commands, bool bIsVisible)
	{
		UpdateConfig(Commands, [bIsVisible](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bIsVisible = bIsVisible; });
	}


	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSection::SetCastShadow(bool bCastShadow)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		UpdateConfig(Commands, [bCastShadow](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bCastsShadow = bCastShadow; });
		return Commands.Commit();
	}

	void FRealtimeMeshSection::SetCastShadow(FRealtimeMeshProxyCommandBatch& Commands, bool bCastShadow)
	{
		UpdateConfig(Commands, [bCastShadow](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bCastsShadow = bCastShadow; });
	}

	bool FRealtimeMeshSection::Serialize(FArchive& Ar)
	{
		Ar << Config;
		Ar << StreamRange;
		Ar << Bounds;
		return true;
	}

	void FRealtimeMeshSection::InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands)
	{
		Commands.AddSectionTask(Key, [Config = Config, StreamRange = StreamRange](FRealtimeMeshSectionProxy& Proxy)
		{
			Proxy.Reset();
			Proxy.UpdateConfig(Config);
			Proxy.UpdateStreamRange(StreamRange);
		}, ShouldRecreateProxyOnChange());
	}

	void FRealtimeMeshSection::MarkBoundsDirtyIfNotOverridden() const
	{
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
}
