// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshDataStream.h"
#include "RealtimeMeshSection.h"
#include "Data/RealtimeMeshShared.h"
#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshSectionGroupConfig.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroup : public TSharedFromThis<FRealtimeMeshSectionGroup>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshSectionGroupKey Key;

		TSet<FRealtimeMeshStreamKey> Streams;
		TSet<FRealtimeMeshSectionRef, FRealtimeMeshSectionRefKeyFuncs> Sections;
		FRealtimeMeshSectionGroupConfig Config;
		FRealtimeMeshBounds Bounds;

	public:
		FRealtimeMeshSectionGroup(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey);
		virtual ~FRealtimeMeshSectionGroup() = default;

		const FRealtimeMeshSectionGroupKey& GetKey(const FRealtimeMeshLockContext& LockContext) const { return Key; }
		FRealtimeMeshStreamRange GetInUseRange(const FRealtimeMeshLockContext& LockContext) const;
		TOptional<FBoxSphereBounds3f> GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const;
		bool HasSections(const FRealtimeMeshLockContext& LockContext) const;
		int32 NumSections(const FRealtimeMeshLockContext& LockContext) const;
		bool HasStreams(const FRealtimeMeshLockContext& LockContext) const;

		TSet<FRealtimeMeshStreamKey> GetStreamKeys(const FRealtimeMeshLockContext& LockContext) const;
		TSet<FRealtimeMeshSectionKey> GetSectionKeys(const FRealtimeMeshLockContext& LockContext) const;

		template <typename SectionType>
		TSharedPtr<SectionType> GetSectionAs(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionKey& SectionKey) const
		{
			return StaticCastSharedPtr<SectionType>(GetSection(LockContext, SectionKey));
		}

		FRealtimeMeshSectionPtr GetSection(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionKey& SectionKey) const;


		virtual void Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionGroupConfig& InConfig);
		virtual void Reset(FRealtimeMeshUpdateContext& UpdateContext);

		virtual void SetOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext, const FBoxSphereBounds3f& InBounds);
		virtual void ClearOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext);


		/**
		 * @brief Update the config for this section group
		 * @param UpdateContext Update context used for this operation
		 * @param InConfig New section group config
		 */
		virtual void UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionGroupConfig& InConfig);

		/**
		 * @brief Edits the config for this section group using the specified function
		 * @param UpdateContext Update context used for this operation
		 * @param EditFunc Function to call to edit the config
		 */
		virtual void UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, TFunction<void(FRealtimeMeshSectionGroupConfig&)> EditFunc);

		virtual void CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream);
		virtual void RemoveStream(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamKey& StreamKey);

		virtual void SetAllStreams(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStreamSet&& InStreams);

		virtual void CreateOrUpdateSection(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& InConfig,
										   const FRealtimeMeshStreamRange& InStreamRange);
		virtual void RemoveSection(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionKey& SectionKey);

		virtual bool Serialize(FArchive& Ar);

		virtual void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext);

		virtual void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext);
		
		virtual bool ShouldRecreateProxyOnChange(const FRealtimeMeshLockContext& LockContext) const { return Config.DrawType == ERealtimeMeshSectionDrawType::Static; }
	protected:
		const FRealtimeMeshSectionGroupKey& GetKey_AssumesLocked() const { return Key; }
		friend struct FRealtimeMeshSectionGroupRefKeyFuncs;
		friend class FRealtimeMeshLOD;
		
		void MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext);

	};

	struct FRealtimeMeshSectionGroupRefKeyFuncs : BaseKeyFuncs<TSharedRef<FRealtimeMeshSectionGroup>, FRealtimeMeshSectionGroupKey, false>
	{
		/**
		 * @return The key used to index the given element.
		 */
		static KeyInitType GetSetKey(const TSharedRef<FRealtimeMeshSectionGroup>& Element)
		{
			return Element->GetKey_AssumesLocked();
		}

		/**
		 * @return True if the keys match.
		 */
		static bool Matches(KeyInitType A, KeyInitType B)
		{
			return A == B;
		}

		/** Calculates a hash index for a key. */
		static uint32 GetKeyHash(KeyInitType Key)
		{
			return GetTypeHash(Key);
		}
	};
}
