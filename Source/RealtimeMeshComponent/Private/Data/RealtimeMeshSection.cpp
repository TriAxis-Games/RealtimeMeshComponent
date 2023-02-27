// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshData.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"


namespace RealtimeMesh
{
	FRealtimeMeshSectionData::FRealtimeMeshSectionData(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh,
		FRealtimeMeshSectionKey InKey, const FRealtimeMeshSectionConfig& InConfig,
		const FRealtimeMeshStreamRange& InStreamRange)
		: ClassFactory(InClassFactory)
		, MeshWeak(InMesh)
		, Key(InKey)
		, Config(InConfig)
		, StreamRange(InStreamRange)
		, LocalBounds(FSphere3f(FVector3f::ZeroVector, 1.0f))
	{
	}

	FName FRealtimeMeshSectionData::GetMeshName() const
	{
		if (const auto Mesh = MeshWeak.Pin())
		{
			return Mesh->GetMeshName();
		}
		return NAME_None;
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMeshSectionData::GetSectionGroup() const
	{
		if (const auto Mesh = MeshWeak.Pin())
		{
			return Mesh->GetSectionGroup(Key.GetSectionGroupKey());
		}
		return nullptr;
	}

	FRealtimeMeshSectionConfig FRealtimeMeshSectionData::GetConfig() const
	{
		FReadScopeLock ScopeLock(Lock);
		return Config;
	}

	FRealtimeMeshStreamRange FRealtimeMeshSectionData::GetStreamRange() const
	{
		FReadScopeLock ScopeLock(Lock);
		return StreamRange;
	}

	FBoxSphereBounds3f FRealtimeMeshSectionData::GetLocalBounds() const
	{
		FReadScopeLock ScopeLock(Lock);
		return LocalBounds;
	}

	void FRealtimeMeshSectionData::Initialize(const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange)
	{
		FWriteScopeLock ScopeLock(Lock);
		Config = InConfig;
		StreamRange = InStreamRange;

		MarkRenderStateDirty(true);
	}

	void FRealtimeMeshSectionData::UpdateBounds(const FBoxSphereBounds3f& InBounds)
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);
		LocalBounds = InBounds;
		ScopeLock.Release();

		BoundsUpdatedEvent.Broadcast(this->AsShared());
	}

	void FRealtimeMeshSectionData::UpdateConfig(const FRealtimeMeshSectionConfig& InConfig)
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);
		Config = InConfig;
		ScopeLock.Release();

		DoOnValidProxy([InConfig](const FRealtimeMeshSectionProxyRef& Proxy)
		{
			Proxy->UpdateConfig(InConfig);
		});

		ConfigUpdatedEvent.Broadcast(this->AsShared());
		
		MarkRenderStateDirty(true);
	}

	void FRealtimeMeshSectionData::UpdateStreamRange(const FRealtimeMeshStreamRange& InRange)
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);
		StreamRange = InRange;
		ScopeLock.Release();

		DoOnValidProxy([InRange](const FRealtimeMeshSectionProxyRef& Proxy)
		{
			Proxy->UpdateStreamRange(InRange);
		});

		StreamRangeUpdatedEvent.Broadcast(this->AsShared());
		
		MarkRenderStateDirty(true);
	}

	bool FRealtimeMeshSectionData::IsVisible() const
	{
		FReadScopeLock ScopeLock(Lock);
		return Config.bIsVisible;
	}

	void FRealtimeMeshSectionData::SetVisibility(bool bIsVisible)
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);
		Config.bIsVisible = bIsVisible;

		auto ConfigCopy = Config;
		ScopeLock.Release();

		DoOnValidProxy([ConfigCopy](const FRealtimeMeshSectionProxyRef& Proxy)
		{
			Proxy->UpdateConfig(ConfigCopy);
		});

		ConfigUpdatedEvent.Broadcast(this->AsShared());
		
		MarkRenderStateDirty(true);
	}

	bool FRealtimeMeshSectionData::IsCastingShadow() const
	{
		FReadScopeLock ScopeLock(Lock);
		return Config.bCastsShadow;
	}

	void FRealtimeMeshSectionData::SetCastShadow(bool bCastShadow)
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);
		Config.bCastsShadow = bCastShadow;

		auto ConfigCopy = Config;
		ScopeLock.Release();

		DoOnValidProxy([ConfigCopy](const FRealtimeMeshSectionProxyRef& Proxy)
		{
			Proxy->UpdateConfig(ConfigCopy);
		});

		ConfigUpdatedEvent.Broadcast(this->AsShared());
		
		MarkRenderStateDirty(true);
	}

	void FRealtimeMeshSectionData::MarkRenderStateDirty(bool bShouldRecreateProxies)
	{
		if (const auto Mesh = MeshWeak.Pin())
		{
			Mesh->MarkRenderStateDirty(bShouldRecreateProxies);
		}
	}

	void FRealtimeMeshSectionData::OnStreamsChanged(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams,
	                                                const TArray<FRealtimeMeshStreamKey>& RemovedStreams)
	{
	}

	bool FRealtimeMeshSectionData::Serialize(FArchive& Ar)
	{
		Ar << Config;
		Ar << StreamRange;
		Ar << LocalBounds;
		return true;
	}

	FRealtimeMeshSectionProxyInitializationParametersRef FRealtimeMeshSectionData::GetInitializationParams() const
	{
		FReadScopeLock ScopeLock(Lock);

		auto InitParams = MakeShared<FRealtimeMeshSectionProxyInitializationParameters>();
		InitParams->Config = Config;
		InitParams->StreamRange = StreamRange;
		return InitParams;
	}

	void FRealtimeMeshSectionData::DoOnValidProxy(TUniqueFunction<void(const FRealtimeMeshSectionProxyRef&)>&& Function) const
	{
		if (const FRealtimeMeshPtr Parent = MeshWeak.Pin())
		{
			Parent->DoOnRenderProxy([Key = Key, Function = MoveTemp(Function)](const FRealtimeMeshProxyRef& Proxy)
			{
				if (const FRealtimeMeshLODProxyPtr LODProxy = Proxy->GetLOD(Key.GetLODKey()))
				{
					if (const FRealtimeMeshSectionGroupProxyPtr SectionGroupProxy = LODProxy->GetSectionGroup(Key.GetSectionGroupKey()))
					{
						if (const FRealtimeMeshSectionProxyPtr SectionProxy = SectionGroupProxy->GetSection(Key))
						{
							Function(SectionProxy.ToSharedRef());
						}
					}
				}
			});
		}
	}
}
