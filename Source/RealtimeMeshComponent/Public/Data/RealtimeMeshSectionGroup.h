// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshConfig.h"
#include "RealtimeMeshDataBuilder.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroup : public TSharedFromThis<FRealtimeMeshSectionGroup>
	{
	public:
		DECLARE_EVENT_OneParam(FRealtimeMeshSectionGroup, FInUseSegmentUpdatedEvent, const FRealtimeMeshSectionGroupRef&);
		DECLARE_EVENT_OneParam(FRealtimeMeshSectionGroup, FBoundsUpdatedEvent, const FRealtimeMeshSectionGroupRef&);
		DECLARE_EVENT_ThreeParams(FRealtimeMeshSectionGroup, FStreamsUpdatedEvent, const FRealtimeMeshSectionGroupRef&, const TArray<FRealtimeMeshStreamKey>&, const TArray<FRealtimeMeshStreamKey>&);
		DECLARE_EVENT_ThreeParams(FRealtimeMeshSectionGroup, FSectionsUpdatedEvent, const FRealtimeMeshSectionGroupRef&, const TArray<FRealtimeMeshSectionKey>&, const TArray<FRealtimeMeshSectionKey>&);
	private:
		FInUseSegmentUpdatedEvent InUseStreamRangeUpdatedEvent;
		FBoundsUpdatedEvent BoundsUpdatedEvent;
		FStreamsUpdatedEvent StreamsUpdatedEvent;
		FSectionsUpdatedEvent SectionsUpdatedEvent;
	public:
		FInUseSegmentUpdatedEvent& OnInUseSegmentUpdated() { return InUseStreamRangeUpdatedEvent; }
		FBoundsUpdatedEvent& OnBoundsUpdated() { return BoundsUpdatedEvent; }
		FStreamsUpdatedEvent& OnStreamsUpdated() { return StreamsUpdatedEvent; }
		FSectionsUpdatedEvent& OnSectionsUpdated() { return SectionsUpdatedEvent; }

	protected:
		const FRealtimeMeshClassFactoryRef ClassFactory;
		const FRealtimeMeshWeakPtr MeshWeak;
		const FRealtimeMeshSectionGroupKey Key;
		TSparseArray<FRealtimeMeshSectionDataRef> Sections;
		FBoxSphereBounds3f LocalBounds;
		FRealtimeMeshStreamRange InUseRange;
		mutable FRWLock Lock;
	public:
		FRealtimeMeshSectionGroup(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionGroupKey InID);
		virtual ~FRealtimeMeshSectionGroup() = default;

		FName GetMeshName() const;

		FRealtimeMeshSectionGroupKey GetID() const { return Key; }
		FRealtimeMeshStreamRange GetInUseRange() const;
		FBoxSphereBounds3f GetLocalBounds() const;
		bool HasSections() const;
		int32 NumSections() const;

		FRealtimeMeshSectionDataPtr GetSection(FRealtimeMeshSectionKey SectionKey) const;
		template<typename SectionType>
		TSharedPtr<SectionType> GetSectionAs(FRealtimeMeshSectionKey SectionKey) const
		{
			return StaticCastSharedPtr<SectionType>(GetSection(SectionKey));
		}

	protected:
		void SetAllStreams(const TArray<FRealtimeMeshStreamKey>& UpdatedStreamKeys, const TArray<FRealtimeMeshStreamKey>& RemovedStreamKeys,
			TArray<FRealtimeMeshSectionGroupStreamUpdateDataRef>&& StreamUpdateData);
		void CreateOrUpdateStream(FRealtimeMeshStreamKey StreamKey, const FRealtimeMeshSectionGroupStreamUpdateDataRef& InStream);
	public:
		virtual void ClearStream(FRealtimeMeshStreamKey StreamKey);
		virtual void RemoveStream(FRealtimeMeshStreamKey StreamKey);

		virtual FRealtimeMeshSectionKey CreateSection(const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange);
		virtual void RemoveSection(FRealtimeMeshSectionKey SectionKey);
		virtual void RemoveAllSections();

		void MarkRenderStateDirty(bool bShouldRecreateProxies);

		virtual FRealtimeMeshSectionGroupProxyInitializationParametersRef GetInitializationParams() const;

		virtual bool Serialize(FArchive& Ar);
	protected:
		FName GetParentName() const;
		void DoOnValidProxy(TUniqueFunction<void(const FRealtimeMeshSectionGroupProxyRef&)>&& Function) const;

		virtual void UpdateBounds();
		virtual void UpdateInUseStreamRange();

		void HandleStreamRangeChanged(const FRealtimeMeshSectionDataRef& InSection);
		void HandleSectionBoundsChanged(const FRealtimeMeshSectionDataRef& InSection);
		
		void BroadcastSectionsChanged(const TArray<FRealtimeMeshSectionKey>& AddedOrUpdatedSections, const TArray<FRealtimeMeshSectionKey>& RemovedSections);
		void BroadcastStreamsChanged(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams, const TArray<FRealtimeMeshStreamKey>& RemovedStreams);
	};
}
