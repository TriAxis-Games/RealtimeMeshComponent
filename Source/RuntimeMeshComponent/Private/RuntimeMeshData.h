// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"


class URuntimeMesh;
class FRuntimeMeshProxy;

using FRuntimeMeshProxyPtr = TSharedPtr<FRuntimeMeshProxy, ESPMode::ThreadSafe>;

DECLARE_DELEGATE_OneParam(FRuntimeMeshGameThreadTaskDelegate, URuntimeMesh*);


/**
 * 
 */
class FRuntimeMeshData : public FRuntimeMeshProviderProxy
{
	/** Parent mesh object that owns this provider. */
	TWeakObjectPtr<URuntimeMesh> ParentMeshObject;

	/** Reference to the root provider that we use for all mesh data and collision data */
	FRuntimeMeshProviderProxyRef BaseProvider;

	/** Render proxy for this mesh */
	FRuntimeMeshProxyPtr RenderProxy;

	/** This really only tracks basic section configuration. it never stores mesh data. */
	TArray<FRuntimeMeshMaterialSlot> MaterialSlots;
	TMap<FName, int32> SlotNameLookup;
	TArray<FRuntimeMeshLOD, TInlineAllocator<RUNTIMEMESH_MAXLODS>> LODs;
	//TMap<int32, FRuntimeMeshSectionProperties> Sections;

	FCriticalSection SyncRoot;

public:
	FRuntimeMeshData(const FRuntimeMeshProviderProxyRef& InBaseProvider, TWeakObjectPtr<URuntimeMesh> InParentMeshObject);
	virtual ~FRuntimeMeshData() override;

	FRuntimeMeshProviderProxyRef GetCurrentProviderProxy() { return BaseProvider; }

	TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots() const { return MaterialSlots; }
	int32 GetNumMaterials() const;
	UMaterialInterface* GetMaterial(int32 SlotIndex) const;
	int32 GetMaterialIndex(FName MaterialSlotName) const;
	TArray<FName> GetMaterialSlotNames() const;
	bool IsMaterialSlotNameValid(FName MaterialSlotName) const;

	TArray<FRuntimeMeshLOD, TInlineAllocator<RUNTIMEMESH_MAXLODS>> GetCopyOfConfiguration() const { return LODs; }
protected: // IRuntimeMeshProvider signatures
	virtual void Initialize() override;

	virtual void ConfigureLOD(int32 LODIndex, const FRuntimeMeshLODProperties& LODProperties) override;
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual bool SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) override;
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) override;
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) override;
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) override;
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) override;
	virtual void MarkCollisionDirty() override;

	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override
	{
		return BaseProvider->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
	}
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionDatas) override
	{
		return BaseProvider->GetCollisionMesh(CollisionDatas);
	}

	virtual FBoxSphereBounds GetBounds() override { return BaseProvider->GetBounds(); }

private:
	void HandleProxySectionPropertiesUpdate(int32 LODIndex, int32 SectionId);
	void HandleProxySectionUpdate(int32 LODIndex, int32 SectionId, bool bForceRecreateProxies = false, bool bSkipRecreateProxies = false);

	void DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate Func)
	{
		class FRuntimeMeshGameThreadTask
		{
			TWeakObjectPtr<URuntimeMesh> RuntimeMesh;
			FRuntimeMeshGameThreadTaskDelegate Delegate;
		public:

			FRuntimeMeshGameThreadTask(TWeakObjectPtr<URuntimeMesh> InRuntimeMesh, FRuntimeMeshGameThreadTaskDelegate InDelegate)
				: RuntimeMesh(InRuntimeMesh), Delegate(InDelegate)
			{
			}

			FORCEINLINE TStatId GetStatId() const
			{
				RETURN_QUICK_DECLARE_CYCLE_STAT(FRuntimeMeshGameThreadTask, STATGROUP_TaskGraphTasks);
			}

			static ENamedThreads::Type GetDesiredThread()
			{
				return ENamedThreads::GameThread;
			}

			static ESubsequentsMode::Type GetSubsequentsMode()
			{
				return ESubsequentsMode::FireAndForget;
			}

			void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
			{
				if (URuntimeMesh * Comp = RuntimeMesh.Get())
				{
					Delegate.Execute(Comp);
				}
			}
		};


		if (IsInGameThread())
		{
			if (URuntimeMesh * Comp = ParentMeshObject.Get())
			{
				Func.Execute(Comp);
			}
			else
			{
				check(false);
			}
		}
		else
		{
			TGraphTask<FRuntimeMeshGameThreadTask>::CreateTask().ConstructAndDispatchWhenReady(ParentMeshObject, Func);
		}
	}
	void RecreateAllComponentProxies();

// 	void EnsureReadyToRender(ERHIFeatureLevel::Type InFeatureLevel);
// 	FRuntimeMeshProxyPtr GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel) const;

	FRuntimeMeshProxyPtr GetOrCreateRenderProxy(ERHIFeatureLevel::Type InFeatureLevel);

	friend class URuntimeMesh;
	friend class URuntimeMeshComponent;
	friend class FRuntimeMeshComponentSceneProxy;
};

using FRuntimeMeshDataRef = TSharedRef<FRuntimeMeshData, ESPMode::ThreadSafe>;
using FRuntimeMeshDataPtr = TSharedPtr<FRuntimeMeshData, ESPMode::ThreadSafe>;
