// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshProvider.h"


class URealtimeMesh;
class FRealtimeMeshProxy;

using FRealtimeMeshProxyPtr = TSharedPtr<FRealtimeMeshProxy, ESPMode::ThreadSafe>;

DECLARE_DELEGATE_OneParam(FRealtimeMeshGameThreadTaskDelegate, URealtimeMesh*);


/**
 * 
 */
class FRealtimeMeshData : public IRealtimeMeshProviderProxy
{
	/** Parent mesh object that owns this provider. */
	TWeakObjectPtr<URealtimeMesh> ParentMeshObject;

	/** Reference to the root provider that we use for all mesh data and collision data */
	IRealtimeMeshProviderProxyRef BaseProvider;

	/** Render proxy for this mesh */
	FRealtimeMeshProxyPtr RenderProxy;

	/** This really only tracks basic section configuration. it never stores mesh data. */
	TArray<FRealtimeMeshMaterialSlot> MaterialSlots;
	TArray<FRealtimeMeshLOD, TInlineAllocator<REALTIMEMESH_MAXLODS>> LODs;
	//TMap<int32, FRealtimeMeshSectionProperties> Sections;

	FCriticalSection SyncRoot;

public:
	FRealtimeMeshData(const IRealtimeMeshProviderProxyRef& InBaseProvider, TWeakObjectPtr<URealtimeMesh> InParentMeshObject);
	virtual ~FRealtimeMeshData() override;


	int32 GetNumMaterials();
	void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials);
	UMaterialInterface* GetMaterial(int32 SlotIndex);


protected: // IRealtimeMeshProvider signatures
	virtual void Initialize() override;

	virtual void ConfigureLOD(uint8 LODIndex, const FRealtimeMeshLODProperties& LODProperties) override;
	virtual void CreateSection(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties) override;
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) override;
	virtual void MarkSectionDirty(uint8 LODIndex, int32 SectionId) override;
	virtual void SetSectionVisibility(uint8 LODIndex, int32 SectionId, bool bIsVisible) override;
	virtual void SetSectionCastsShadow(uint8 LODIndex, int32 SectionId, bool bCastsShadow) override;
	virtual void RemoveSection(uint8 LODIndex, int32 SectionId) override;
	virtual void MarkCollisionDirty() override;

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRealtimeMeshRenderableMeshData& MeshData) override
	{
		return BaseProvider->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
	}
	virtual bool GetCollisionMesh(FRealtimeMeshCollisionData& CollisionDatas) override
	{
		return BaseProvider->GetCollisionMesh(CollisionDatas);
	}

	virtual FBoxSphereBounds GetBounds() override { return BaseProvider->GetBounds(); }

private:
	void HandleProxySectionPropertiesUpdate(int32 LODIndex, int32 SectionId);
	void HandleProxySectionUpdate(int32 LODIndex, int32 SectionId, bool bForceRecreateProxies = false, bool bSkipRecreateProxies = false);

	void DoOnGameThread(FRealtimeMeshGameThreadTaskDelegate Func)
	{
		class FRealtimeMeshGameThreadTask
		{
			TWeakObjectPtr<URealtimeMesh> RealtimeMesh;
			FRealtimeMeshGameThreadTaskDelegate Delegate;
		public:

			FRealtimeMeshGameThreadTask(TWeakObjectPtr<URealtimeMesh> InRealtimeMesh, FRealtimeMeshGameThreadTaskDelegate InDelegate)
				: RealtimeMesh(InRealtimeMesh), Delegate(InDelegate)
			{
			}

			FORCEINLINE TStatId GetStatId() const
			{
				RETURN_QUICK_DECLARE_CYCLE_STAT(FRealtimeMeshGameThreadTask, STATGROUP_TaskGraphTasks);
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
				if (URealtimeMesh * Comp = RealtimeMesh.Get())
				{
					Delegate.Execute(Comp);
				}
			}
		};


		if (IsInGameThread())
		{
			if (URealtimeMesh * Comp = ParentMeshObject.Get())
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
			TGraphTask<FRealtimeMeshGameThreadTask>::CreateTask().ConstructAndDispatchWhenReady(ParentMeshObject, Func);
		}
	}
	void RecreateAllComponentProxies();

// 	void EnsureReadyToRender(ERHIFeatureLevel::Type InFeatureLevel);
// 	FRealtimeMeshProxyPtr GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel) const;

	FRealtimeMeshProxyPtr GetOrCreateRenderProxy(ERHIFeatureLevel::Type InFeatureLevel);

	friend class URealtimeMesh;
	friend class URealtimeMeshComponent;
	friend class FRealtimeMeshComponentSceneProxy;
};

using FRealtimeMeshDataRef = TSharedRef<FRealtimeMeshData, ESPMode::ThreadSafe>;
using FRealtimeMeshDataPtr = TSharedPtr<FRealtimeMeshData, ESPMode::ThreadSafe>;
