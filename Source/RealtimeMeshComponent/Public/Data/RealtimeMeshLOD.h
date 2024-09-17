// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshSectionGroup.h"
#include "Core/RealtimeMeshLODConfig.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLOD : public TSharedFromThis<FRealtimeMeshLOD>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshLODKey Key;
		TSet<FRealtimeMeshSectionGroupRef, FRealtimeMeshSectionGroupRefKeyFuncs> SectionGroups;
		FRealtimeMeshLODConfig Config;
		FRealtimeMeshBounds Bounds;

	public:
		FRealtimeMeshLOD(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey);
		virtual ~FRealtimeMeshLOD();

		const FRealtimeMeshLODKey& GetKey() const { return Key; }
		bool HasSectionGroups() const;

		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(SectionGroupKey));
		}

		FRealtimeMeshSectionGroupPtr GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const;

		template<typename SectionGroupType, typename FuncType>
		void ProcessSectionGroupsAs(FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
			for (TSharedPtr<const FRealtimeMeshSectionGroup> SectionGroup : SectionGroups)
			{
				::Invoke(ProcessFunc, *StaticCastSharedPtr<const SectionGroupType>(SectionGroup));
			}
		}
		
		template<typename FuncType>
		void ProcessSectionGroups(FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
			for (TSharedPtr<const FRealtimeMeshSectionGroup> SectionGroup : SectionGroups)
			{
				::Invoke(ProcessFunc, *SectionGroup);
			}
		}


		
		FBoxSphereBounds3f GetLocalBounds() const;

		virtual void Initialize(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshLODConfig& InConfig);
		TFuture<ERealtimeMeshProxyUpdateStatus> Reset();
		virtual void Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder);

		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateConfig(const FRealtimeMeshLODConfig& InConfig);
		virtual void UpdateConfig(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshLODConfig& InConfig);

		TFuture<ERealtimeMeshProxyUpdateStatus> CreateOrUpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionGroupConfig& InConfig);
		virtual void CreateOrUpdateSectionGroup(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionGroupConfig& InConfig);
		TFuture<ERealtimeMeshProxyUpdateStatus> RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);
		virtual void RemoveSectionGroup(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroupKey& SectionGroupKey);

		virtual bool Serialize(FArchive& Ar);

		virtual void InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder);

		TSet<FRealtimeMeshSectionGroupKey> GetSectionGroupKeys() const;

	protected:
		virtual FBoxSphereBounds3f CalculateBounds() const;
		void HandleSectionGroupBoundsChanged(const FRealtimeMeshSectionGroupKey& RealtimeMeshSectionGroupKey);

		virtual bool ShouldRecreateProxyOnChange() { return true; }
	};
}
