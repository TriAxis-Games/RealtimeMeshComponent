// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

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
		virtual ~FRealtimeMeshLOD() = default;

		const FRealtimeMeshLODKey& GetKey(const FRealtimeMeshLockContext& LockContext) const { return Key; }
		bool HasSectionGroups(const FRealtimeMeshLockContext& LockContext) const;
		
		FRealtimeMeshLODConfig GetConfig(const FRealtimeMeshLockContext& LockContext) const { return Config; }

		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(LockContext, SectionGroupKey));
		}

		FRealtimeMeshSectionGroupPtr GetSectionGroup(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionGroupKey& SectionGroupKey) const;

		
		template<typename SectionGroupType, typename FuncType>
		void ProcessSectionGroupsAs(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
			for (TSharedPtr<const FRealtimeMeshSectionGroup> SectionGroup : SectionGroups)
			{
				::Invoke(ProcessFunc, *StaticCastSharedPtr<const SectionGroupType>(SectionGroup));
			}
		}
		
		template<typename FuncType>
		void ProcessSectionGroups(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
			for (TSharedPtr<const FRealtimeMeshSectionGroup> SectionGroup : SectionGroups)
			{
				::Invoke(ProcessFunc, *SectionGroup);
			}
		}


		
		TOptional<FBoxSphereBounds3f> GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const;

		virtual void Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& InConfig);
		virtual void Reset(FRealtimeMeshUpdateContext& UpdateContext);

		virtual void UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& InConfig);

		virtual void CreateOrUpdateSectionGroup(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionGroupConfig& InConfig);
		virtual void RemoveSectionGroup(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionGroupKey& SectionGroupKey);

		virtual bool Serialize(FArchive& Ar);

		virtual void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext);

		TSet<FRealtimeMeshSectionGroupKey> GetSectionGroupKeys(const FRealtimeMeshLockContext& LockContext) const;

		virtual void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext);
		
		virtual bool ShouldRecreateProxyOnChange(const FRealtimeMeshLockContext& LockContext) { return true; }

	protected:

		void MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext);
	};
}
