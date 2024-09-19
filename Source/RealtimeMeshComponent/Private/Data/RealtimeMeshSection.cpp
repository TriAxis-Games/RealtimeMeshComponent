// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshSection.h"
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

	FRealtimeMeshSectionGroupPtr FRealtimeMeshSection::GetSectionGroup(const FRealtimeMeshLockContext& LockContext) const
	{
		if (const auto Mesh = SharedResources->GetOwner())
		{
			return Mesh->GetSectionGroup(LockContext, Key);
		}
		return nullptr;
	}

	FRealtimeMeshSectionConfig FRealtimeMeshSection::GetConfig(const FRealtimeMeshLockContext& LockContext) const
	{
		return Config;
	}

	FRealtimeMeshStreamRange FRealtimeMeshSection::GetStreamRange(const FRealtimeMeshLockContext& LockContext) const
	{
		return StreamRange;
	}

	TOptional<FBoxSphereBounds3f> FRealtimeMeshSection::GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const
	{
		return Bounds.Get();
	}

	void FRealtimeMeshSection::Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InRange)
	{
		Config = InConfig;
		StreamRange = InRange;
		Bounds.Reset();

		InitializeProxy(UpdateContext);

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
		UpdateContext.GetState().StreamRangeDirtyTree.Flag(Key);
	}

	void FRealtimeMeshSection::Reset(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Initialize(UpdateContext, FRealtimeMeshSectionConfig(), FRealtimeMeshStreamRange());
	}

	void FRealtimeMeshSection::SetOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext, const FBoxSphereBounds3f& InBounds)
	{
		Bounds.SetUserSetBounds(InBounds);
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshSection::ClearOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Bounds.ClearUserSetBounds();
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshSection::UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionConfig& InConfig)
	{
		UpdateConfig(UpdateContext, [InConfig](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig = InConfig; });
	}

	void FRealtimeMeshSection::UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc)
	{
		bool bShouldRecreateProxy = ShouldRecreateProxyOnChange(UpdateContext);
		EditFunc(Config);
		bShouldRecreateProxy |= ShouldRecreateProxyOnChange(UpdateContext);

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddSectionTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, bShouldRecreateProxy);
		}

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
	}

	void FRealtimeMeshSection::UpdateStreamRange(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamRange& InRange)
	{
		StreamRange = InRange;

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddSectionTask(Key, [StreamRange = StreamRange](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionProxy& Proxy)
			{
				Proxy.UpdateStreamRange(StreamRange);
			}, ShouldRecreateProxyOnChange(UpdateContext));
		}

		UpdateContext.GetState().StreamRangeDirtyTree.Flag(Key);
		MarkBoundsDirtyIfNotOverridden(UpdateContext);
	}

	void FRealtimeMeshSection::SetVisibility(FRealtimeMeshUpdateContext& UpdateContext, bool bIsVisible)
	{
		UpdateConfig(UpdateContext, [bIsVisible](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bIsVisible = bIsVisible; });
	}

	void FRealtimeMeshSection::SetCastShadow(FRealtimeMeshUpdateContext& UpdateContext, bool bCastShadow)
	{		
		UpdateConfig(UpdateContext, [bCastShadow](FRealtimeMeshSectionConfig& ExistingConfig) { ExistingConfig.bCastsShadow = bCastShadow; });
	}

	bool FRealtimeMeshSection::Serialize(FArchive& Ar)
	{
		Ar << Config;
		Ar << StreamRange;
		Ar << Bounds;
		return true;
	}

	void FRealtimeMeshSection::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext)
	{		
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddSectionTask(Key, [Config = Config, StreamRange = StreamRange](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionProxy& Proxy)
			{
				Proxy.Reset();
				Proxy.UpdateConfig(Config);
				Proxy.UpdateStreamRange(StreamRange);
			}, ShouldRecreateProxyOnChange(UpdateContext));
		}
	}

	void FRealtimeMeshSection::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		
	}

	void FRealtimeMeshSection::MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext)
	{		
		Bounds.ClearCachedValue();
		if (!Bounds.HasUserSetBounds())
		{
			UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
		}
	}

	void FRealtimeMeshSection::UpdateCalculatedBounds(FRealtimeMeshUpdateContext& UpdateContext, TOptional<FBoxSphereBounds3f>& CalculatedBounds)
	{
		if (CalculatedBounds)
		{
			Bounds.SetComputedBounds(*CalculatedBounds);
		}
		else
		{
			Bounds.ClearCachedValue();
		}
	}


	bool FRealtimeMeshSection::ShouldRecreateProxyOnChange(const FRealtimeMeshLockContext& LockContext) const
	{
		if (const auto ParentGroup = GetSectionGroup(LockContext))
		{
			return ParentGroup->ShouldRecreateProxyOnChange(LockContext);
		}

		return false;
	}
}
