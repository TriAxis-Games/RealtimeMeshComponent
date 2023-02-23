// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCollision.h"
#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"
#include "RenderProxy/RealtimeMeshProxy.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMesh : public TSharedFromThis<FRealtimeMesh, ESPMode::ThreadSafe>
	{
	public:
		DECLARE_EVENT_OneParam(FRealtimeMesh, FBoundsChangedEvent, const FRealtimeMeshRef&);
		DECLARE_EVENT_TwoParams(FRealtimeMesh, FRenderDataChangedEvent, const FRealtimeMeshRef&, bool /*bShouldProxyRecreate*/);
		DECLARE_EVENT_OneParam(FRealtimeMesh, FCollisionDataUpdated, const FRealtimeMeshRef&);
	private:
		FBoundsChangedEvent BoundsChangedEvent;		
		FRenderDataChangedEvent RenderDataChangedEvent;
		FCollisionDataUpdated CollisionDataUpdatedEvent;
	public:
		FBoundsChangedEvent& OnBoundsChanged() { return BoundsChangedEvent; }
		FRenderDataChangedEvent& OnRenderDataChanged() { return RenderDataChangedEvent; }
		FCollisionDataUpdated OnCollisionDataUpdated() { return CollisionDataUpdatedEvent; }

	protected:
		const FRealtimeMeshClassFactoryRef ClassFactory;
		mutable FRealtimeMeshProxyPtr RenderProxy;	
		TFixedLODArray<FRealtimeMeshLODDataRef> LODs;
		FRealtimeMeshConfig Config;
		FBoxSphereBounds3f LocalBounds;
		FRealtimeMeshCollisionConfiguration CollisionConfig;
		FRealtimeMeshSimpleGeometry SimpleGeometry;
		
		// Name of the mesh, given by the parent URealtimeMesh for debug messaging.
		FName MeshName;

		bool bIsCollisionDirty;
		
		mutable FRWLock RenderDataLock;
		mutable FRWLock CollisionLock;
		mutable FRWLock BoundsLock;

		FName TypeName;
	public:
		
		FRealtimeMesh(const FRealtimeMeshClassFactoryRef& InClassFactory);
		virtual ~FRealtimeMesh() = default;
		
		FName GetMeshName() const { return MeshName; }
		void SetMeshName(FName InMeshName) { MeshName = InMeshName; }

		int32 GetNumLODs() const;

		virtual FBoxSphereBounds3f GetLocalBounds() const;
		bool IsCollisionDirty() const { return bIsCollisionDirty; }
		void ClearCollisionDirtyFlag() { bIsCollisionDirty = false; }

		FRealtimeMeshCollisionConfiguration GetCollisionConfig() const;
		void SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig);
		FRealtimeMeshSimpleGeometry GetSimpleGeometry() const;
		void SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry);

		FRealtimeMeshLODDataPtr GetLOD(FRealtimeMeshLODKey LODKey) const;
		template<typename LODType>
		TSharedPtr<LODType, ESPMode::ThreadSafe> GetLODAs(FRealtimeMeshLODKey LODKey) const
		{
			return StaticCastSharedPtr<LODType>(GetLOD(LODKey));
		}
		FRealtimeMeshSectionGroupPtr GetSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey) const;
		FRealtimeMeshSectionDataPtr GetSection(FRealtimeMeshSectionKey SectionKey) const;
		
		virtual void InitializeLODs(const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs);		
		virtual FRealtimeMeshLODKey AddLOD(const FRealtimeMeshLODConfig& LODConfig);		
		virtual void RemoveTrailingLOD();

		virtual void Reset();
		
		virtual bool Serialize(FArchive& Ar);
		
		virtual FRealtimeMeshProxyInitializationParametersRef GetInitializationParams() const;

		virtual void MarkRenderStateDirty(bool bShouldRecreateProxies) { BroadcastRenderDataChangedEvent(bShouldRecreateProxies); }
		virtual void MarkCollisionDirty();

		void DoOnRenderProxy(TUniqueFunction<void(const FRealtimeMeshProxyRef&)>&& Function) const;
		
		bool HasRenderProxy() const;
		FRealtimeMeshProxyPtr GetRenderProxy(bool bCreateIfNotExists = false) const;
		
		virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) { return false; }
		virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const { return false; }
	protected:
		void CreateRenderProxy(bool bForceRecreate = false) const;
		void UpdateBounds();

		void BroadcastBoundsChangedEvent() { BoundsChangedEvent.Broadcast(this->AsShared()); }
		void BroadcastRenderDataChangedEvent(bool bShouldRecreateProxies) { RenderDataChangedEvent.Broadcast(this->AsShared(), bShouldRecreateProxies); }


		void HandleLODBoundsChanged(const FRealtimeMeshLODDataRef& LOD);
	};
}

