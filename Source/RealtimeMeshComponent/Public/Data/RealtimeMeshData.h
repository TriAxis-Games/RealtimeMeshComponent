// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshLOD.h"
#include "RealtimeMeshCollisionLibrary.h"
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


	class REALTIMEMESHCOMPONENT_API FRealtimeMesh : public TSharedFromThis<FRealtimeMesh, ESPMode::ThreadSafe>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		mutable FRealtimeMeshProxyPtr RenderProxy;
		TFixedLODArray<FRealtimeMeshLODRef> LODs;
		FRealtimeMeshConfig Config;
		FRealtimeMeshBounds Bounds;
		FThreadSafeCounter VersionCounter;
		int64 LastFrameProxyUpdated;

		TSharedPtr<IRealtimeMeshNaniteResources> NaniteResources;
	public:
		FRealtimeMesh(const FRealtimeMeshSharedResourcesRef& InSharedResources);
		virtual ~FRealtimeMesh() = default;

		const FRealtimeMeshSharedResourcesRef& GetSharedResources() const { return SharedResources; }

		int32 GetProxyVersion() const;
		int32 IncrementProxyVersionIfNotSameFrame();

		int32 GetNumLODs() const;

		virtual FBoxSphereBounds3f GetLocalBounds() const;

		FRealtimeMeshLODPtr GetLOD(FRealtimeMeshLODKey LODKey) const;

		template <typename LODType>
		TSharedPtr<LODType> GetLODAs(FRealtimeMeshLODKey LODKey) const
		{
			return StaticCastSharedPtr<LODType>(GetLOD(LODKey));
		}

		template<typename LODType, typename FuncType>
		void ProcessLODsAs(FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
			for (TSharedPtr<const FRealtimeMeshLOD> LOD : LODs)
			{
				::Invoke(ProcessFunc, *StaticCastSharedPtr<const LODType>(LOD));
			}
		}

		template<typename FuncType>
		void ProcessLODs(FuncType ProcessFunc) const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
			for (TSharedPtr<const FRealtimeMeshLOD> LOD : LODs)
			{
				::Invoke(ProcessFunc, *LOD);
			}
		}



		
		FRealtimeMeshSectionGroupPtr GetSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey) const;
		
		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(SectionGroupKey));
		}
		FRealtimeMeshSectionPtr GetSection(FRealtimeMeshSectionKey SectionKey) const;
		template <typename SectionType>
		TSharedPtr<SectionType> GetSectionAs(const FRealtimeMeshSectionKey& SectionKey) const
		{
			return StaticCastSharedPtr<SectionType>(GetSection(SectionKey));
		}

		TFuture<ERealtimeMeshProxyUpdateStatus> InitializeLODs(const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs);
		auto InitializeLODs(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs) -> void;
		TFuture<ERealtimeMeshProxyUpdateStatus> AddLOD(const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey = nullptr);
		virtual void AddLOD(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey = nullptr);
		TFuture<ERealtimeMeshProxyUpdateStatus> RemoveTrailingLOD(FRealtimeMeshLODKey* OutNewLastLODKey = nullptr);
		virtual void RemoveTrailingLOD(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshLODKey* OutNewLastLODKey = nullptr);

		
		virtual void SetNaniteResources(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const TSharedRef<IRealtimeMeshNaniteResources>& InNaniteResources);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetNaniteResources(const TSharedRef<IRealtimeMeshNaniteResources>& InNaniteResources);
		virtual void ClearNaniteResources(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder);
		TFuture<ERealtimeMeshProxyUpdateStatus> ClearNaniteResources();
		
		virtual void SetDistanceField(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshDistanceField&& InDistanceField);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField);
		virtual void ClearDistanceField(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder);
		TFuture<ERealtimeMeshProxyUpdateStatus> ClearDistanceField();

		virtual void SetCardRepresentation(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshCardRepresentation&& InCardRepresentation);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation);
		
		virtual void ClearCardRepresentation(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder);
		TFuture<ERealtimeMeshProxyUpdateStatus> ClearCardRepresentation();

		/**
		 * Set up a material slot for the Realtime Mesh.
		 *
		 * @param MaterialSlot The slot index for the material.
		 * @param SlotName The name of the material slot.
		 * @param InMaterial The material to be assigned to the slot.
		 */
		void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial = nullptr);

		/**
		 * Get the index of a material slot by its name.
		 *
		 * @param MaterialSlotName The name of the material slot.
		 * @return The index of the material slot. Returns INDEX_NONE if the material slot does not exist.
		 */
		int32 GetMaterialIndex(FName MaterialSlotName) const;

		/**
		 * Get the name of the material slot at the specified index
		 * @param Index Index of the material to get the name for
		 * @return 
		 */
		FName GetMaterialSlotName(int32 Index) const;
		
		/**
		 * Check if the given material slot name is valid.
		 *
		 * @param MaterialSlotName The name of the material slot to check.
		 * @return true if the material slot name is valid, false otherwise.
		 */
		bool IsMaterialSlotNameValid(FName MaterialSlotName) const;

		/**
		 * Gets the material slot at the specified index.
		 *
		 * @param SlotIndex The index of the material slot.
		 * @return The material slot at the specified index.
		 */
		FRealtimeMeshMaterialSlot GetMaterialSlot(int32 SlotIndex) const;

		/**
		 * Get the number of material slots in the RealtimeMesh.
		 *
		 * @return The number of material slots.
		 */
		int32 GetNumMaterials() const;

		/**
		 * Get the names of all material slots in the Realtime Mesh.
		 *
		 * @return An array of FName representing the names of all material slots.
		 */
		TArray<FName> GetMaterialSlotNames() const;

		/**
		 * Get the material slots of the Realtime Mesh.
		 *
		 * @return An array of FRealtimeMeshMaterialSlot representing the material slots of the Realtime Mesh.
		 */
		TArray<FRealtimeMeshMaterialSlot> GetMaterialSlots() const;

		/**
		 * Get the material at the specified slot index.
		 *
		 * @param SlotIndex The index of the material slot.
		 * @return The material at the specified slot index. Returns nullptr if the slot index is invalid.
		 */
		UMaterialInterface* GetMaterial(int32 SlotIndex) const;

	
		
		TFuture<ERealtimeMeshProxyUpdateStatus> Reset(bool bRemoveRenderProxy = false);
		virtual void Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, bool bRemoveRenderProxy = false);

		virtual bool Serialize(FArchive& Ar, URealtimeMesh* Owner);

		virtual void MarkRenderStateDirty(bool bShouldRecreateProxies, int32 CommandsVersion)
		{			
			SharedResources->BroadcastMeshRenderDataChanged(bShouldRecreateProxies, CommandsVersion);
		}

		bool HasRenderProxy() const;
		FRealtimeMeshProxyPtr GetRenderProxy(bool bCreateIfNotExists = false) const;

		virtual void InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder) const;

		virtual void ProcessEndOfFrameUpdates() { }
	protected:

		void HandleLODBoundsChanged(const FRealtimeMeshLODKey& LODKey);

		FRealtimeMeshProxyRef CreateRenderProxy(bool bForceRecreate = false) const;
		virtual FBoxSphereBounds3f CalculateBounds() const;

		TFuture<ERealtimeMeshCollisionUpdateResult> UpdateCollision(FRealtimeMeshCollisionInfo&& InCollisionData);

		friend class URealtimeMesh;
	};
}
