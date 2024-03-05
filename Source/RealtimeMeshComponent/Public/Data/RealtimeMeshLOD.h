// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshSectionGroup.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODData : public TSharedFromThis<FRealtimeMeshLODData>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshLODKey Key;
		TSet<FRealtimeMeshSectionGroupRef, FRealtimeMeshSectionGroupRefKeyFuncs> SectionGroups;
		FRealtimeMeshLODConfig Config;
		FRealtimeMeshBounds Bounds;

	public:
		FRealtimeMeshLODData(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey);
		virtual ~FRealtimeMeshLODData();

		const FRealtimeMeshLODKey& GetKey() const { return Key; }
		bool HasSectionGroups() const;

		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(SectionGroupKey));
		}

		FRealtimeMeshSectionGroupPtr GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const;

		FBoxSphereBounds3f GetLocalBounds() const;

		virtual void Initialize(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshLODConfig& InConfig);
		TFuture<ERealtimeMeshProxyUpdateStatus> Reset();
		virtual void Reset(FRealtimeMeshProxyCommandBatch& Commands);

		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateConfig(const FRealtimeMeshLODConfig& InConfig);
		virtual void UpdateConfig(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshLODConfig& InConfig);

		TFuture<ERealtimeMeshProxyUpdateStatus> CreateOrUpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);
		virtual void CreateOrUpdateSectionGroup(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionGroupKey& SectionGroupKey);
		TFuture<ERealtimeMeshProxyUpdateStatus> RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);
		virtual void RemoveSectionGroup(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionGroupKey& SectionGroupKey);

		virtual bool Serialize(FArchive& Ar);

		virtual void InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands);

		TSet<FRealtimeMeshSectionGroupKey> GetSectionGroupKeys() const;

	protected:
		virtual FBoxSphereBounds3f CalculateBounds() const;
		void HandleSectionGroupBoundsChanged(const FRealtimeMeshSectionGroupKey& RealtimeMeshSectionGroupKey);

		virtual bool ShouldRecreateProxyOnChange() { return true; }
	};
}
