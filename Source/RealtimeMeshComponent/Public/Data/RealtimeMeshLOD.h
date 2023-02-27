// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODData : public TSharedFromThis<FRealtimeMeshLODData>
	{
	public:
		DECLARE_EVENT_OneParam(FRealtimeMeshLODData, FConfigUpdatedEvent, const FRealtimeMeshLODDataRef&);
		DECLARE_EVENT_OneParam(FRealtimeMeshLODData, FBoundsUpdatedEvent, const FRealtimeMeshLODDataRef&);
		DECLARE_EVENT_TwoParams(FRealtimeMeshLODData, FSectionGroupAdded, const FRealtimeMeshLODDataRef&, FRealtimeMeshSectionGroupKey);
		DECLARE_EVENT_TwoParams(FRealtimeMeshLODData, FSectionGroupRemoved, const FRealtimeMeshLODDataRef&, FRealtimeMeshSectionGroupKey);
	private:
		FConfigUpdatedEvent ConfigUpdatedEvent;
		FBoundsUpdatedEvent BoundsUpdatedEvent;
		FSectionGroupAdded SectionGroupAddedEvent;
		FSectionGroupRemoved SectionGroupRemovedEvent;
	public:
		FConfigUpdatedEvent& OnConfigUpdated() { return ConfigUpdatedEvent; }
		FBoundsUpdatedEvent& OnBoundsUpdated() { return BoundsUpdatedEvent; }
		FSectionGroupAdded& OnSectionGroupAdded() { return SectionGroupAddedEvent; }
		FSectionGroupRemoved& OnSectionGroupRemoved() { return SectionGroupRemovedEvent; }

	protected:
		const FRealtimeMeshClassFactoryRef ClassFactory;
		const FRealtimeMeshWeakPtr MeshWeak;
		const FRealtimeMeshLODKey Key;
		TSparseArray<FRealtimeMeshSectionGroupRef> SectionGroups;
		FRealtimeMeshLODConfig Config;
		FBoxSphereBounds3f LocalBounds;
		mutable FRWLock Lock;

		FName TypeName;
	public:
		FRealtimeMeshLODData(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh,
			FRealtimeMeshLODKey InID, const FRealtimeMeshLODConfig& InConfig);
		virtual ~FRealtimeMeshLODData() = default;

		FName GetMeshName() const;

		FRealtimeMeshLODKey GetID() const { return Key; }
		bool HasSectionGroups() const;

		FRealtimeMeshSectionGroupPtr GetSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey) const;
		template<typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(FRealtimeMeshSectionGroupKey SectionGroupKey) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(SectionGroupKey));
		}

		FBoxSphereBounds3f GetLocalBounds() const;

		void UpdateConfig(const FRealtimeMeshLODConfig& InConfig);

		FRealtimeMeshSectionGroupKey CreateSectionGroup();
		void RemoveSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey);
		void RemoveAllSectionGroups();
		
		virtual void MarkRenderStateDirty(bool bShouldRecreateProxies);
		
		FRealtimeMeshLODProxyInitializationParametersRef GetInitializationParams() const;
		
		virtual bool Serialize(FArchive& Ar);
	protected:
		FName GetParentName() const;
		void DoOnValidProxy(TUniqueFunction<void(const FRealtimeMeshLODProxyRef&)>&& Function) const;

		virtual void UpdateBounds();

		void HandleSectionGroupBoundsChanged(const FRealtimeMeshSectionGroupRef& InSectionGroup);
	};
}
