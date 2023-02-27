// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionData : public TSharedFromThis<FRealtimeMeshSectionData>
	{
	public:
		DECLARE_EVENT_OneParam(FRealtimeMeshSectionData, FConfigUpdatedEvent, const FRealtimeMeshSectionDataRef&);
		DECLARE_EVENT_OneParam(FRealtimeMeshSectionData, FSegmentUpdatedEvent, const FRealtimeMeshSectionDataRef&);
		DECLARE_EVENT_OneParam(FRealtimeMeshSectionData, FBoundsUpdatedEvent, const FRealtimeMeshSectionDataRef&);
	private:
		FConfigUpdatedEvent ConfigUpdatedEvent;
		FSegmentUpdatedEvent StreamRangeUpdatedEvent;
		FBoundsUpdatedEvent BoundsUpdatedEvent;
	public:
		FConfigUpdatedEvent& OnConfigUpdated() { return ConfigUpdatedEvent; }
		FSegmentUpdatedEvent& OnStreamRangeUpdated() { return StreamRangeUpdatedEvent; }
		FBoundsUpdatedEvent& OnBoundsUpdated() { return BoundsUpdatedEvent; }

	protected:
		const FRealtimeMeshClassFactoryRef ClassFactory;
		const FRealtimeMeshWeakPtr MeshWeak;
		const FRealtimeMeshSectionKey Key;
		FRealtimeMeshSectionConfig Config;
		FRealtimeMeshStreamRange StreamRange;
		FBoxSphereBounds3f LocalBounds;
		mutable FRWLock Lock;
	public:
		FRealtimeMeshSectionData(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionKey InKey,
		                         const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange);
		virtual ~FRealtimeMeshSectionData() = default;

		FName GetMeshName() const;

		FRealtimeMeshSectionGroupPtr GetSectionGroup() const;
		template<typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs() const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup());
		}
		
		FRealtimeMeshSectionKey GetKey() const { return Key; }
		FRealtimeMeshSectionConfig GetConfig() const;
		FRealtimeMeshStreamRange GetStreamRange() const;
		FBoxSphereBounds3f GetLocalBounds() const;

		virtual void Initialize(const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange);
		virtual FRealtimeMeshSectionProxyInitializationParametersRef GetInitializationParams() const;

		virtual void UpdateBounds(const FBoxSphereBounds3f& InBounds);
		virtual void UpdateConfig(const FRealtimeMeshSectionConfig& InConfig);
		virtual void UpdateStreamRange(const FRealtimeMeshStreamRange& InRange);

		virtual bool IsVisible() const;		
		virtual void SetVisibility(bool bIsVisible);		
		virtual bool IsCastingShadow() const;		
		virtual void SetCastShadow(bool bCastShadow);
		
		void MarkRenderStateDirty(bool bShouldRecreateProxies);

		virtual void OnStreamsChanged(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams, const TArray<FRealtimeMeshStreamKey>& RemovedStreams);

		virtual bool Serialize(FArchive& Ar);
	protected:
		void DoOnValidProxy(TUniqueFunction<void(const FRealtimeMeshSectionProxyRef&)>&& Function) const;
	};
}
