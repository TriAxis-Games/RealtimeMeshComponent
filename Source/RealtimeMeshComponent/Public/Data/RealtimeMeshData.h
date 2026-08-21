// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshLOD.h"
#include "RealtimeMeshCollisionLibrary.h"
#include "RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "Data/RealtimeMeshShared.h"
#include "Async/Async.h"
#include "Core/RealtimeMeshConfig.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "Core/RealtimeMeshMaterial.h"
#include "Mesh/RealtimeMeshCardRepresentation.h"
#include "Mesh/RealtimeMeshDistanceField.h"
#include "Mesh/RealtimeMeshNaniteResourcesInterface.h"

struct FTriMeshCollisionData;
class URealtimeMesh;

namespace RealtimeMesh
{
	struct FRealtimeMeshUpdateContext;
	struct IRealtimeMeshNaniteMeshResourcesImplementation;


	class REALTIMEMESHCOMPONENT_API FRealtimeMesh : public TSharedFromThis<FRealtimeMesh>//, public FGCObject
	{
	protected:
		const FRealtimeMeshContextRef Context;

		// Latest-published render-thread proxy version. Replaced wholesale on
		// every Commit; older versions stay alive while scene proxies pin them.
		// Guarded by RenderProxyLock for cross-thread (GT/RT) access.
		mutable FRealtimeMeshProxyPtr RenderProxy;
		mutable FCriticalSection RenderProxyLock;

		TFixedLODArray<FRealtimeMeshLODRef> LODs;
		FRealtimeMeshConfig Config;
		FRealtimeMeshBounds Bounds;



		/* Counter for generating version identifier for collision updates */
		FThreadSafeCounter CollisionUpdateVersionCounter;
	public:
		FRealtimeMesh(const FRealtimeMeshContextRef& InContext);
		virtual ~FRealtimeMesh();

		const FRealtimeMeshContextRef& GetContext() const { return Context; }

		int32 GetNumLODs(const FRealtimeMeshLockContext& LockContext) const;

		TOptional<FBoxSphereBounds3f> GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const;

		FRealtimeMeshLODPtr GetLOD(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshLODKey LODKey) const;

		template <typename LODType>
		TSharedPtr<LODType> GetLODAs(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshLODKey LODKey) const
		{
			return StaticCastSharedPtr<LODType>(GetLOD(LockContext, LODKey));
		}

		template<typename LODType, typename FuncType>
		void ProcessLODsAs(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			// The LockContext parameter's contract is that the guard is already held, so this
			// only asserts it (debug builds) rather than re-acquiring the recursive lock.
			FRealtimeMeshScopeGuardReadCheck LockCheck(Context);
			for (TSharedPtr<const FRealtimeMeshLOD> LOD : LODs)
			{
				::Invoke(ProcessFunc, *StaticCastSharedPtr<const LODType>(LOD));
			}
		}

		template<typename FuncType>
		void ProcessLODs(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardReadCheck LockCheck(Context);
			for (TSharedPtr<const FRealtimeMeshLOD> LOD : LODs)
			{
				::Invoke(ProcessFunc, *LOD);
			}
		}



		
		FRealtimeMeshSectionGroupPtr GetSectionGroup(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshBufferSetKey SectionGroupKey) const;
		
		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetKey& SectionGroupKey) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(LockContext, SectionGroupKey));
		}
		FRealtimeMeshSectionPtr GetSection(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshSectionKey SectionKey) const;
		template <typename SectionType>
		TSharedPtr<SectionType> GetSectionAs(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionKey& SectionKey) const
		{
			return StaticCastSharedPtr<SectionType>(GetSection(LockContext, SectionKey));
		}

		void InitializeLODs(FRealtimeMeshUpdateContext& UpdateContext, const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs);
		void AddLOD(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey = nullptr);
		void RemoveTrailingLOD(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODKey* OutNewLastLODKey = nullptr);


		// Type-tier factories. Leaves override to return their concrete types.
		// Callers reach these via the mesh weak-pin in Context (cold path —
		// only invoked when a new Section/SectionGroup/LOD is added).
		virtual FRealtimeMeshSectionRef     CreateSection(const FRealtimeMeshSectionKey& InKey) const;
		virtual FRealtimeMeshSectionGroupRef CreateSectionGroup(const FRealtimeMeshBufferSetKey& InKey) const;
		virtual FRealtimeMeshLODRef         CreateLOD(const FRealtimeMeshLODKey& InKey) const;
		virtual FRealtimeMeshUpdateStateRef CreateUpdateState() const;

		
		virtual void SetNaniteResources(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshNaniteResourcesPtr&& InNaniteResources);
		virtual void ClearNaniteResources(FRealtimeMeshUpdateContext& UpdateContext);
		
		virtual void SetDistanceField(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshDistanceField&& InDistanceField);
		virtual void ClearDistanceField(FRealtimeMeshUpdateContext& UpdateContext);

		virtual void SetCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshCardRepresentation&& InCardRepresentation);		
		virtual void ClearCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext);

		/**
		 * Set up a material slot for the Realtime Mesh.
		 *
		 * @param MaterialSlot The slot index for the material.
		 * @param SlotName The name of the material slot.
		 * @param InMaterial The material to be assigned to the slot.
		 */
		void SetupMaterialSlot(FRealtimeMeshUpdateContext& UpdateContext, int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial = nullptr);

		/**
		 * Get the index of a material slot by its name.
		 *
		 * @param MaterialSlotName The name of the material slot.
		 * @return The index of the material slot. Returns INDEX_NONE if the material slot does not exist.
		 */
		int32 GetMaterialIndex(const FRealtimeMeshLockContext& LockContext, FName MaterialSlotName) const;

		/**
		 * Get the name of the material slot at the specified index
		 * @param Index Index of the material to get the name for
		 * @return 
		 */
		FName GetMaterialSlotName(const FRealtimeMeshLockContext& LockContext, int32 Index) const;
		
		/**
		 * Check if the given material slot name is valid.
		 *
		 * @param MaterialSlotName The name of the material slot to check.
		 * @return true if the material slot name is valid, false otherwise.
		 */
		bool IsMaterialSlotNameValid(const FRealtimeMeshLockContext& LockContext, FName MaterialSlotName) const;

		/**
		 * Gets the material slot at the specified index.
		 *
		 * @param SlotIndex The index of the material slot.
		 * @return The material slot at the specified index.
		 */
		FRealtimeMeshMaterialSlot GetMaterialSlot(const FRealtimeMeshLockContext& LockContext, int32 SlotIndex) const;

		/**
		 * Get the number of material slots in the RealtimeMesh.
		 *
		 * @return The number of material slots.
		 */
		int32 GetNumMaterials(const FRealtimeMeshLockContext& LockContext) const;

		/**
		 * Get the names of all material slots in the Realtime Mesh.
		 *
		 * @return An array of FName representing the names of all material slots.
		 */
		TArray<FName> GetMaterialSlotNames(const FRealtimeMeshLockContext& LockContext) const;

		/**
		 * Get the material slots of the Realtime Mesh.
		 *
		 * @return An array of FRealtimeMeshMaterialSlot representing the material slots of the Realtime Mesh.
		 */
		TArray<FRealtimeMeshMaterialSlot> GetMaterialSlots(const FRealtimeMeshLockContext& LockContext) const;

		/**
		 * Get the material at the specified slot index.
		 *
		 * @param SlotIndex The index of the material slot.
		 * @return The material at the specified slot index. Returns nullptr if the slot index is invalid.
		 */
		UMaterialInterface* GetMaterial(const FRealtimeMeshLockContext& LockContext, int32 SlotIndex) const;

	
		
		// Named ResetInternal, not Reset, to avoid name-hiding against
		// URealtimeMesh::Reset() (the UObject-layer no-arg UFUNCTION). Subclasses
		// override this directly; the UObject-layer Reset is wired up separately.
		virtual void ResetInternal(FRealtimeMeshUpdateContext& UpdateContext, bool bRemoveRenderProxy);

		virtual bool Serialize(FArchive& Ar, URealtimeMesh* Owner);

		bool HasRenderProxy(const FRealtimeMeshLockContext& LockContext) const;
		FRealtimeMeshProxyPtr GetRenderProxy(bool bCreateIfNotExists = false) const;

		/**
		 * Render-thread: apply a batch of tasks to a draft cloned from the latest
		 * published proxy, finalize it (UpdatedCachedState), and atomically swap
		 * it in as the new latest. Old versions stay alive while scene proxies
		 * pin them.
		 *
		 * Called by FRealtimeMeshProxyUpdateBuilder::Commit via
		 * ENQUEUE_RENDER_COMMAND. Returns false if there is no published proxy to
		 * clone from (i.e. proxy never bootstrapped) — caller should treat as
		 * NoProxy.
		 */
		bool ApplyAndPublish_RenderThread(
			FRHICommandListBase& RHICmdList,
			TArray<FRealtimeMeshProxyUpdateBuilder::TaskFunctionType>& InTasks);

		/**
		 * Render-thread fast path for batches consisting solely of in-place stream updates
		 * (see FRealtimeMeshInPlaceStreamUpdate). If every target buffer set's proxy node is
		 * uniquely owned, the GPU buffer contents are overwritten in place on the CURRENT
		 * published version — no clone, no new version, no vertex-factory reinit. If any
		 * target is shared with a live snapshot (or is no longer eligible, e.g. its element
		 * count changed), it falls back to a normal clone + reallocating update + publish so
		 * the result is still correct. Returns which of those happened.
		 */
		ERealtimeMeshInPlaceApplyResult ApplyInPlace_RenderThread(
			FRHICommandListBase& RHICmdList,
			TArray<FRealtimeMeshInPlaceStreamUpdate>& InUpdates);

		virtual void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext) const;

		virtual void ProcessEndOfFrameUpdates() { }
		
		virtual void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext);

	protected:

		int32 GetNextCollisionUpdateVersion() { return CollisionUpdateVersionCounter.Increment(); }
		FRealtimeMeshProxyRef CreateRenderProxy(bool bForceRecreate = false) const;

		TFuture<ERealtimeMeshCollisionUpdateResult> UpdateCollision(FRealtimeMeshCollisionInfo&& InCollisionData, int32 NewCollisionKey);

		void MarkForEndOfFrameUpdate() const;
		void MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext);

		friend class URealtimeMesh;
	};
}
