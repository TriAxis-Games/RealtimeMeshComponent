// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshLOD.h"
#include "RealtimeMeshCollisionLibrary.h"
#include "RealtimeMeshUpdateBuilder.h"
#include "Data/RealtimeMeshShared.h"
#include "Async/Async.h"
#include "Core/RealtimeMeshConfig.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "Core/RealtimeMeshMaterial.h"
#include "Mesh/RealtimeMeshCardRepresentation.h"
#include "Mesh/RealtimeMeshDistanceField.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"

struct FTriMeshCollisionData;
class URealtimeMesh;

namespace RealtimeMesh
{
	struct FRealtimeMeshUpdateContext;
	struct IRealtimeMeshNaniteResources;


	class REALTIMEMESHCOMPONENT_API FRealtimeMesh : public TSharedFromThis<FRealtimeMesh>//, public FGCObject
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		mutable FRealtimeMeshProxyPtr RenderProxy;
		TFixedLODArray<FRealtimeMeshLODRef> LODs;
		FRealtimeMeshConfig Config;
		FRealtimeMeshBounds Bounds;

		TSharedPtr<IRealtimeMeshNaniteResources> NaniteResources;

		
		/* Counter for generating version identifier for collision updates */
		FThreadSafeCounter CollisionUpdateVersionCounter;
	public:
		FRealtimeMesh(const FRealtimeMeshSharedResourcesRef& InSharedResources);
		virtual ~FRealtimeMesh() = default;

		const FRealtimeMeshSharedResourcesRef& GetSharedResources() const { return SharedResources; }

		int32 GetNumLODs(const FRealtimeMeshLockContext& LockContext) const;

		virtual TOptional<FBoxSphereBounds3f> GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const;

		FRealtimeMeshLODPtr GetLOD(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshLODKey LODKey) const;

		template <typename LODType>
		TSharedPtr<LODType> GetLODAs(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshLODKey LODKey) const
		{
			return StaticCastSharedPtr<LODType>(GetLOD(LockContext, LODKey));
		}

		template<typename LODType, typename FuncType>
		void ProcessLODsAs(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
			for (TSharedPtr<const FRealtimeMeshLOD> LOD : LODs)
			{
				::Invoke(ProcessFunc, *StaticCastSharedPtr<const LODType>(LOD));
			}
		}

		template<typename FuncType>
		void ProcessLODs(const FRealtimeMeshLockContext& LockContext, FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
			for (TSharedPtr<const FRealtimeMeshLOD> LOD : LODs)
			{
				::Invoke(ProcessFunc, *LOD);
			}
		}



		
		FRealtimeMeshSectionGroupPtr GetSectionGroup(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshSectionGroupKey SectionGroupKey) const;
		
		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(LockContext, SectionGroupKey));
		}
		FRealtimeMeshSectionPtr GetSection(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshSectionKey SectionKey) const;
		template <typename SectionType>
		TSharedPtr<SectionType> GetSectionAs(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionKey& SectionKey) const
		{
			return StaticCastSharedPtr<SectionType>(GetSection(LockContext, SectionKey));
		}

		auto InitializeLODs(FRealtimeMeshUpdateContext& UpdateContext, const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs) -> void;
		virtual void AddLOD(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey = nullptr);
		virtual void RemoveTrailingLOD(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODKey* OutNewLastLODKey = nullptr);

		
		virtual void SetNaniteResources(FRealtimeMeshUpdateContext& UpdateContext, const TSharedRef<IRealtimeMeshNaniteResources>& InNaniteResources);
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

	
		
		virtual void Reset(FRealtimeMeshUpdateContext& UpdateContext, bool bRemoveRenderProxy = false);

		virtual bool Serialize(FArchive& Ar, URealtimeMesh* Owner);

		bool HasRenderProxy(const FRealtimeMeshLockContext& LockContext) const;
		FRealtimeMeshProxyPtr GetRenderProxy(bool bCreateIfNotExists = false) const;

		virtual void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext) const;

		virtual void ProcessEndOfFrameUpdates() { }
		
		virtual void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext);
	protected:

		int32 GetNextCollisionUpdateVersion() { return CollisionUpdateVersionCounter.Increment(); }
		FRealtimeMeshProxyRef CreateRenderProxy(bool bForceRecreate = false) const;

		TFuture<ERealtimeMeshCollisionUpdateResult> UpdateCollision(FRealtimeMeshCollisionInfo&& InCollisionData, int32 NewCollisionKey);

		void MarkForEndOfFrameUpdate() const;
		void MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext);
		virtual bool ShouldRecreateProxyOnChange(const FRealtimeMeshLockContext& LockContext) { return true; }

		friend class URealtimeMesh;
	};
}
