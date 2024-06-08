// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCollision.h"
#include "Data/RealtimeMeshShared.h"
#include "Async/Async.h"
#include "Mesh/RealtimeMeshCardRepresentation.h"
#include "Mesh/RealtimeMeshDistanceField.h"

struct FTriMeshCollisionData;
class URealtimeMesh;

namespace RealtimeMesh
{
	struct IRealtimeMeshNaniteResources;
	struct FRealtimeMeshProxyCommandBatch;
	struct FRealtimeMeshUpdateContext;


	class REALTIMEMESHCOMPONENT_API FRealtimeMesh : public TSharedFromThis<FRealtimeMesh, ESPMode::ThreadSafe>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		mutable FRealtimeMeshProxyPtr RenderProxy;
		TFixedLODArray<FRealtimeMeshLODDataRef> LODs;
		FRealtimeMeshConfig Config;
		FRealtimeMeshBounds Bounds;

		TSharedPtr<IRealtimeMeshNaniteResources> NaniteResources;
	public:
		FRealtimeMesh(const FRealtimeMeshSharedResourcesRef& InSharedResources);
		virtual ~FRealtimeMesh() = default;

		const FRealtimeMeshSharedResourcesRef& GetSharedResources() const { return SharedResources; }

		int32 GetNumLODs() const;

		virtual FBoxSphereBounds3f GetLocalBounds() const;

		FRealtimeMeshLODDataPtr GetLOD(FRealtimeMeshLODKey LODKey) const;

		template <typename LODType>
		TSharedPtr<LODType, ESPMode::ThreadSafe> GetLODAs(FRealtimeMeshLODKey LODKey) const
		{
			return StaticCastSharedPtr<LODType>(GetLOD(LODKey));
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
		void InitializeLODs(FRealtimeMeshProxyCommandBatch& Commands, const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs);
		TFuture<ERealtimeMeshProxyUpdateStatus> AddLOD(const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey = nullptr);
		virtual void AddLOD(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey = nullptr);
		TFuture<ERealtimeMeshProxyUpdateStatus> RemoveTrailingLOD(FRealtimeMeshLODKey* OutNewLastLODKey = nullptr);
		virtual void RemoveTrailingLOD(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshLODKey* OutNewLastLODKey = nullptr);

		
		virtual void SetNaniteResources(FRealtimeMeshProxyCommandBatch& Commands, const TSharedRef<IRealtimeMeshNaniteResources>& InNaniteResources);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetNaniteResources(const TSharedRef<IRealtimeMeshNaniteResources>& InNaniteResources);
		virtual void ClearNaniteResources(FRealtimeMeshProxyCommandBatch& Commands);
		TFuture<ERealtimeMeshProxyUpdateStatus> ClearNaniteResources();
		
		virtual void SetDistanceField(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshDistanceField&& InDistanceField);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField);
		virtual void ClearDistanceField(FRealtimeMeshProxyCommandBatch& Commands);
		TFuture<ERealtimeMeshProxyUpdateStatus> ClearDistanceField();

		virtual void SetCardRepresentation(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshCardRepresentation&& InCardRepresentation);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation);
		
		virtual void ClearCardRepresentation(FRealtimeMeshProxyCommandBatch& Commands);
		TFuture<ERealtimeMeshProxyUpdateStatus> ClearCardRepresentation();
		
		TFuture<ERealtimeMeshProxyUpdateStatus> Reset(bool bRemoveRenderProxy = false);
		virtual void Reset(FRealtimeMeshProxyCommandBatch& Commands, bool bRemoveRenderProxy = false);

		virtual bool Serialize(FArchive& Ar, URealtimeMesh* Owner);

		virtual void MarkRenderStateDirty(bool bShouldRecreateProxies)
		{
			SharedResources->BroadcastMeshRenderDataChanged(bShouldRecreateProxies);
		}

		bool HasRenderProxy() const;
		FRealtimeMeshProxyPtr GetRenderProxy(bool bCreateIfNotExists = false) const;

		virtual void InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands) const;

		virtual void ProcessEndOfFrameUpdates() { }
	protected:

		void HandleLODBoundsChanged(const FRealtimeMeshLODKey& LODKey);

		FRealtimeMeshProxyRef CreateRenderProxy(bool bForceRecreate = false) const;
		virtual FBoxSphereBounds3f CalculateBounds() const;

		TFuture<ERealtimeMeshCollisionUpdateResult> UpdateCollision(FRealtimeMeshCollisionData&& InCollisionData);

		friend class URealtimeMesh;
	};
}
