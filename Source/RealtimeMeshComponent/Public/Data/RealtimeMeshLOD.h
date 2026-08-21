// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshBufferSet.h"
#include "Core/RealtimeMeshLODConfig.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLOD : public TSharedFromThis<FRealtimeMeshLOD>
	{
	protected:
		const FRealtimeMeshContextRef Context;
		const FRealtimeMeshLODKey Key;
		TSet<FRealtimeMeshSectionGroupRef, FRealtimeMeshBufferSetRefKeyFuncs> SectionGroups;
		FRealtimeMeshLODConfig Config;
		FRealtimeMeshBounds Bounds;

	public:
		FRealtimeMeshLOD(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshLODKey& InKey);
		virtual ~FRealtimeMeshLOD() = default;

		const FRealtimeMeshLODKey& GetKey(const FRealtimeMeshLockContext& LockContext) const { return Key; }
		bool HasSectionGroups(const FRealtimeMeshLockContext& LockContext) const;
		
		FRealtimeMeshLODConfig GetConfig(const FRealtimeMeshLockContext& LockContext) const { return Config; }

		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetKey& SectionGroupKey) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(LockContext, SectionGroupKey));
		}

		FRealtimeMeshSectionGroupPtr GetSectionGroup(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetKey& SectionGroupKey) const;

		
		template<typename SectionGroupType, typename FuncType>
		void ProcessSectionGroupsAs(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			// The LockContext parameter's contract is that the guard is already held, so this
			// only asserts it (debug builds) rather than re-acquiring the recursive lock.
			FRealtimeMeshScopeGuardReadCheck LockCheck(Context->GetGuard());
			for (TSharedPtr<const FRealtimeMeshBufferSet> SectionGroup : SectionGroups)
			{
				::Invoke(ProcessFunc, *StaticCastSharedPtr<const SectionGroupType>(SectionGroup));
			}
		}

		template<typename FuncType>
		void ProcessSectionGroups(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardReadCheck LockCheck(Context->GetGuard());
			for (TSharedPtr<const FRealtimeMeshBufferSet> SectionGroup : SectionGroups)
			{
				::Invoke(ProcessFunc, *SectionGroup);
			}
		}


		
		TOptional<FBoxSphereBounds3f> GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const;

		void Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& InConfig);
		void Reset(FRealtimeMeshUpdateContext& UpdateContext);

		void UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& InConfig);

		void CreateOrUpdateSectionGroup(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshBufferSetConfig& InConfig);
		void RemoveSectionGroup(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshBufferSetKey& SectionGroupKey);

		bool Serialize(FArchive& Ar);

		void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext);

		TSet<FRealtimeMeshBufferSetKey> GetSectionGroupKeys(const FRealtimeMeshLockContext& LockContext) const;

		void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext);

	protected:

		void MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext);
	};
}
