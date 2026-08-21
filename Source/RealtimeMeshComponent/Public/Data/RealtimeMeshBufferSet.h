// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshDataStream.h"
#include "RealtimeMeshSection.h"
#include "Data/RealtimeMeshShared.h"
#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshBufferSetConfig.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferSet : public TSharedFromThis<FRealtimeMeshBufferSet>
	{
	protected:
		const FRealtimeMeshContextRef Context;
		const FRealtimeMeshBufferSetKey Key;

		TSet<FRealtimeMeshStreamKey> Streams;
		TSet<FRealtimeMeshSectionRef, FRealtimeMeshSectionRefKeyFuncs> Sections;
		FRealtimeMeshBufferSetConfig Config;
		FRealtimeMeshBounds Bounds;

	public:
		FRealtimeMeshBufferSet(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshBufferSetKey& InKey);
		virtual ~FRealtimeMeshBufferSet() = default;

		const FRealtimeMeshBufferSetKey& GetKey(const FRealtimeMeshLockContext& LockContext) const { return Key; }
		FRealtimeMeshBufferSetConfig GetConfig(const FRealtimeMeshLockContext& LockContext) const { return Config; }
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

		template<typename SectionType, typename FuncType>
		void ProcessSectionsAs(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			// The LockContext parameter's contract is that the guard is already held, so this
			// only asserts it (debug builds) rather than re-acquiring the recursive lock.
			FRealtimeMeshScopeGuardReadCheck LockCheck(Context->GetGuard());
			for (TSharedPtr<const FRealtimeMeshSection> Section : Sections)
			{
				::Invoke(ProcessFunc, *StaticCastSharedPtr<const SectionType>(Section));
			}
		}

		template<typename FuncType>
		void ProcessSections(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardReadCheck LockCheck(Context->GetGuard());
			for (FRealtimeMeshSectionRef Section : Sections)
			{
				::Invoke(ProcessFunc, *Section);
			}
		}

		

		void Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshBufferSetConfig& InConfig);
		virtual void Reset(FRealtimeMeshUpdateContext& UpdateContext);

		void SetOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext, const FBoxSphereBounds3f& InBounds);
		void ClearOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext);


		/**
		 * @brief Update the config for this section group
		 * @param UpdateContext Update context used for this operation
		 * @param InConfig New section group config
		 */
		void UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshBufferSetConfig& InConfig);

		/**
		 * @brief Edits the config for this section group using the specified function
		 * @param UpdateContext Update context used for this operation
		 * @param EditFunc Function to call to edit the config
		 */
		void UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, TFunction<void(FRealtimeMeshBufferSetConfig&)> EditFunc);

		virtual void CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream);
		virtual void RemoveStream(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamKey& StreamKey);

		/**
		 * @brief Fast attribute-update entry. For an already-existing vertex stream on a
		 * Dynamic (CPU-lockable) buffer set, queues an in-place GPU update over the given
		 * element range (FInt32Range::Empty() = whole stream) instead of reallocating —
		 * avoiding the clone / publish / vertex-factory-reinit cost. The caller must have
		 * already written the new bytes into this buffer set's CPU stream storage (as
		 * EditMeshData does); this method only schedules the GPU-side update. Anything not
		 * eligible (a new stream, an index/topology stream, a Static buffer set) falls back
		 * to CreateOrUpdateStream. The render thread re-validates and itself falls back to a
		 * reallocating publish if the structure changed or a snapshot is shared.
		 */
		void FastUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream, const FInt32Range& ElementRange);

		virtual void SetAllStreams(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStreamSet&& InStreams);

		void CreateOrUpdateSection(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& InConfig,
										   const FRealtimeMeshStreamRange& InStreamRange);
		void RemoveSection(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionKey& SectionKey);

		virtual bool Serialize(FArchive& Ar);

		virtual void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext);

		void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext);

		// Returning false marks this group's changes as eligible for the in-place fast path
		// (see FRealtimeMeshProxyUpdateBuilder::Commit): Dynamic and compute-writable groups
		// are mutated in place, so a change need not force the reallocating clone+publish path.
		// This flag ONLY gates that fast-path eligibility — it does NOT suppress scene-proxy
		// recreation on the publishing path. Whenever a publish (or a fast-path fallback)
		// actually runs, recreation is broadcast regardless of this flag, because a newly
		// published version is never observed by proxies captured at the prior version.
		bool ShouldRecreateProxyOnChange(const FRealtimeMeshLockContext& LockContext) const { return Config.DrawType == ERealtimeMeshSectionDrawType::Static && !Config.bComputeWritable; }

		// GPU buffer usage inferred from the section group's draw type. Dynamic sections
		// add BUF_KeepCPUAccessible to an otherwise-static buffer. That flag makes the
		// buffer a valid copy target (it grants TRANSFER_DST on Vulkan), which is what the
		// in-place fast-update path needs: a RLM_WriteOnly lock of a *static* buffer over a
		// sub-range goes through the engine's staging-buffer + GPU-copy path, preserving the
		// untouched bytes and keeping the same FRHIBuffer + SRV (no realloc, no VF reinit).
		//
		// We deliberately do NOT use BUF_Dynamic here: a RLM_WriteOnly lock of a dynamic
		// buffer *orphans* it (allocates a fresh, uninitialized allocation every lock), which
		// reallocates each frame and — fatally for us — does not preserve bytes outside the
		// locked range, corrupting ranged/partial updates. Static sections stay plain
		// BUF_Static (not fast-update eligible; they take the realloc path).
		EBufferUsageFlags GetGPUBufferUsageFlags() const
		{
			EBufferUsageFlags Flags = Config.DrawType == ERealtimeMeshSectionDrawType::Dynamic
				? (EBufferUsageFlags::Static | EBufferUsageFlags::KeepCPUAccessible)
				: EBufferUsageFlags::Static;

			// Compute-writable groups need a UAV so a compute pass can write the geometry directly.
			// This is orthogonal to the dynamic/static CPU-update story above.
			if (Config.bComputeWritable)
			{
				Flags |= EBufferUsageFlags::UnorderedAccess;
			}
			return Flags;
		}
	protected:
		const FRealtimeMeshBufferSetKey& GetKey_AssumesLocked() const { return Key; }
		// DUP-027: befriend the shared KeyFuncs specialization so it can read GetKey_AssumesLocked().
		friend struct TRealtimeMeshRefKeyFuncs<FRealtimeMeshBufferSet, FRealtimeMeshBufferSetKey>;
		friend class FRealtimeMeshLOD;
		
		void MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext);

	};

	// DUP-027: converged onto the shared TRealtimeMeshRefKeyFuncs template (Data/RealtimeMeshShared.h).
	using FRealtimeMeshBufferSetRefKeyFuncs = TRealtimeMeshRefKeyFuncs<FRealtimeMeshBufferSet, FRealtimeMeshBufferSetKey>;
}
