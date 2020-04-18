//// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "RuntimeMeshProvider.h"
//#include "HAL/ThreadSafeBool.h"
//
//
//class URuntimeMesh;
//class FRuntimeMeshProxy;
//enum class ESectionUpdateType : uint8;
//
//using FRuntimeMeshProxyPtr = TSharedPtr<FRuntimeMeshProxy, ESPMode::ThreadSafe>;
//
//
//
//
//
//
//
///**
// * 
// */
//class FRuntimeMeshData : public FRuntimeMeshProviderProxy
//{
//	/** Parent mesh object that owns this provider. */
//	TWeakObjectPtr<URuntimeMesh> ParentMeshObject;
//
//	/** Reference to the root provider that we use for all mesh data and collision data */
//	FRuntimeMeshProviderProxyRef BaseProvider;
//
//	/** Render proxy for this mesh */
//	FRuntimeMeshProxyPtr RenderProxy;
//
//	/** This really only tracks basic section configuration. it never stores mesh data. */
//	TArray<FRuntimeMeshMaterialSlot> MaterialSlots;
//	TMap<FName, int32> SlotNameLookup;
//	TArray<FRuntimeMeshLOD, TInlineAllocator<RUNTIMEMESH_MAXLODS>> LODs;
//	//TMap<int32, FRuntimeMeshSectionProperties> Sections;
//
//	FCriticalSection SyncRoot;
//
//	// State tracking for async thread synchronization
//	FRuntimeMeshDataAsyncWorkSyncObject AsyncWorkState;
//
//public:
//	FRuntimeMeshData(const FRuntimeMeshProviderProxyRef& InBaseProvider, TWeakObjectPtr<URuntimeMesh> InParentMeshObject);
//	virtual ~FRuntimeMeshData() override;
//
//	void Reset();
//
//	FRuntimeMeshProviderProxyRef GetCurrentProviderProxy() { return BaseProvider; }
//
//	TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots() const override { return MaterialSlots; }
//	int32 GetNumMaterials() override;
//	UMaterialInterface* GetMaterial(int32 SlotIndex) const;
//	TArray<FName> GetMaterialSlotNames() const;
//	bool IsMaterialSlotNameValid(FName MaterialSlotName) const;
//
//	TArray<FRuntimeMeshLOD, TInlineAllocator<RUNTIMEMESH_MAXLODS>> GetCopyOfConfiguration() const { return LODs; }
//
//protected: // IRuntimeMeshProvider signatures
//	virtual void Initialize() override;
//
//	virtual void ConfigureLODs(TArray<FRuntimeMeshLODProperties> LODSettings) override;
//	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
//	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) override;
//	virtual int32 GetMaterialIndex(FName MaterialSlotName) override;
//	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) override;
//	virtual void MarkLODDirty(int32 LODIndex) override;
//	virtual void MarkAllLODsDirty() override;
//	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) override;
//	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) override;
//	virtual void RemoveSection(int32 LODIndex, int32 SectionId) override;
//	virtual void MarkCollisionDirty() override;
//
//	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override
//	{
//		return BaseProvider->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
//	}
//	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionDatas) override
//	{
//		return BaseProvider->GetCollisionMesh(CollisionDatas);
//	}
//
//	virtual FBoxSphereBounds GetBounds() override { return BaseProvider->GetBounds(); }
//	virtual bool IsThreadSafe() const override { return BaseProvider->IsThreadSafe(); }
//
//	virtual void DoOnGameThreadAndBlockThreads(FRuntimeMeshProviderThreadExclusiveFunction Func);
//private:
//
//	TMap<int32, TMap<int32, ESectionUpdateType>> SectionsToUpdate;
//	void MarkForUpdate();
//	void HandleUpdate();
//	void HandleFullLODUpdate(int32 LODId, bool& bRequiresProxyRecreate);
//	void HandleSingleSectionUpdate(int32 LODId, int32 SectionId, bool& bRequiresProxyRecreate);
//
//	void DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate Func)
//	{
//		class FRuntimeMeshGameThreadTask
//		{
//			TWeakObjectPtr<URuntimeMesh> RuntimeMesh;
//			FRuntimeMeshGameThreadTaskDelegate Delegate;
//		public:
//
//			FRuntimeMeshGameThreadTask(TWeakObjectPtr<URuntimeMesh> InRuntimeMesh, FRuntimeMeshGameThreadTaskDelegate InDelegate)
//				: RuntimeMesh(InRuntimeMesh), Delegate(InDelegate)
//			{
//			}
//
//			FORCEINLINE TStatId GetStatId() const
//			{
//				RETURN_QUICK_DECLARE_CYCLE_STAT(FRuntimeMeshGameThreadTask, STATGROUP_TaskGraphTasks);
//			}
//
//			static ENamedThreads::Type GetDesiredThread()
//			{
//				return ENamedThreads::GameThread;
//			}
//
//			static ESubsequentsMode::Type GetSubsequentsMode()
//			{
//				return ESubsequentsMode::FireAndForget;
//			}
//
//			void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
//			{
//				if (URuntimeMesh * Comp = RuntimeMesh.Get())
//				{
//					Delegate.Execute(Comp);
//				}
//			}
//		};
//
//
//		if (IsInGameThread())
//		{
//			if (URuntimeMesh * Comp = ParentMeshObject.Get())
//			{
//				Func.Execute(Comp);
//			}
//		}
//		else
//		{
//			TGraphTask<FRuntimeMeshGameThreadTask>::CreateTask().ConstructAndDispatchWhenReady(ParentMeshObject, Func);
//		}
//	}
//	void RecreateAllComponentProxies();
//
//	FRuntimeMeshProxyPtr GetOrCreateRenderProxy(ERHIFeatureLevel::Type InFeatureLevel);
//
//	friend class URuntimeMesh;
//	friend class URuntimeMeshComponent;
//	friend class FRuntimeMeshComponentSceneProxy;
//	friend class URuntimeMeshComponentEngineSubsystem;
//};
//
