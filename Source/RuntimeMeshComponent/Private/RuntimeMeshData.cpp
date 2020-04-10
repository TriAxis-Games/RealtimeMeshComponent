// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshData.h"
#include "RuntimeMeshProxy.h"
#include "RuntimeMesh.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "RuntimeMeshCore.h"

DECLARE_CYCLE_STAT(TEXT("RuntimeMeshData - Initialize"), STAT_RuntimeMeshData_Initialize, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshData - Update Section Properties"), STAT_RuntimeMeshData_UpdateSectionProperties, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshData - Update Section"), STAT_RuntimeMeshData_UpdateSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshData - Recreate All Proxies"), STAT_RuntimeMeshData_RecreateProxies, STATGROUP_RuntimeMesh);

enum class ESectionUpdateType : uint8
{
	None = 0x0,
	Properties = 0x1,
	Mesh = 0x2,
	Clear = 0x4,
	Remove = 0x8,


	AllData = Properties | Mesh,
	ClearOrRemove = Clear | Remove,
};

ENUM_CLASS_FLAGS(ESectionUpdateType);


FRuntimeMeshData::FRuntimeMeshData(const FRuntimeMeshProviderProxyRef& InBaseProvider, TWeakObjectPtr<URuntimeMesh> InParentMeshObject)
	: FRuntimeMeshProviderProxy(nullptr)
	, ParentMeshObject(InParentMeshObject)
	, BaseProvider(InBaseProvider)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMD(%d): Created"), FPlatformTLS::GetCurrentThreadId());
}

FRuntimeMeshData::~FRuntimeMeshData()
{

}

int32 FRuntimeMeshData::GetNumMaterials()
{
	return MaterialSlots.Num();
}

UMaterialInterface* FRuntimeMeshData::GetMaterial(int32 SlotIndex) const
{
	if (!MaterialSlots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	TWeakObjectPtr<UMaterialInterface> Material = MaterialSlots[SlotIndex].Material;
	return Material.Get();
}

int32 FRuntimeMeshData::GetMaterialIndex(FName MaterialSlotName)
{
	const int32* SlotIndex = SlotNameLookup.Find(MaterialSlotName);
	return SlotIndex ? *SlotIndex : INDEX_NONE;
}

TArray<FName> FRuntimeMeshData::GetMaterialSlotNames() const
{
	TArray<FName> OutNames;
	SlotNameLookup.GetKeys(OutNames);
	return OutNames;
}

bool FRuntimeMeshData::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	return SlotNameLookup.Contains(MaterialSlotName);
}

void FRuntimeMeshData::Initialize()
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMD(%d): Initialized"), FPlatformTLS::GetCurrentThreadId());

	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshData_Initialize);

	// Make sure the provider chain is bound
	BaseProvider->BindPreviousProvider(this->AsShared());

	BaseProvider->Initialize();
}

void FRuntimeMeshData::ConfigureLODs(TArray<FRuntimeMeshLODProperties> LODSettings)
{
	check(IsInGameThread());
	{
		FScopeLock Lock(&SyncRoot);
		LODs.Empty();
		LODs.SetNum(LODSettings.Num());
		for (int32 Index = 0; Index < LODSettings.Num(); Index++)
		{
			LODs[Index].Properties = LODSettings[Index];
		}
	}

	if (RenderProxy.IsValid())
	{
		RenderProxy->InitializeLODs_GameThread(LODSettings);
		RecreateAllComponentProxies();
	}
}

void FRuntimeMeshData::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	check(IsInGameThread());
	check(LODs.IsValidIndex(LODIndex));
	check(SectionId >= 0);
	
	FScopeLock Lock(&SyncRoot);

	LODs[LODIndex].Sections.FindOrAdd(SectionId) = SectionProperties;

	// Flag for update
	SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId) = ESectionUpdateType::AllData;
	MarkForUpdate();
}

void FRuntimeMeshData::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda([this, MaterialSlot, SlotName, InMaterial](URuntimeMesh*) {
		check(IsInGameThread());

		// Does this slot already exist?
		if (SlotNameLookup.Contains(SlotName))
		{
			// If the indices match then just go with it
			if (SlotNameLookup[SlotName] == MaterialSlot)
			{
				MaterialSlots[SlotNameLookup[SlotName]].Material = InMaterial;
			}
			else
			{
				MaterialSlots[SlotNameLookup[SlotName]].SlotName = NAME_None;
			}
		}

		if (!MaterialSlots.IsValidIndex(MaterialSlot))
		{
			MaterialSlots.SetNum(MaterialSlot + 1);
		}
		MaterialSlots[MaterialSlot] = FRuntimeMeshMaterialSlot(SlotName, InMaterial);
		SlotNameLookup.Add(SlotName, MaterialSlots.Num() - 1);

		RecreateAllComponentProxies();
	}));
}

void FRuntimeMeshData::MarkSectionDirty(int32 LODIndex, int32 SectionId)
{
	FScopeLock Lock(&SyncRoot);

	check(LODs.IsValidIndex(LODIndex));
	check(LODs[LODIndex].Sections.Contains(SectionId));

	// Flag for update
	ESectionUpdateType& UpdateType = SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId);
	UpdateType |= ESectionUpdateType::Mesh;
	UpdateType = (UpdateType & ~ESectionUpdateType::ClearOrRemove);
	MarkForUpdate();
}

void FRuntimeMeshData::MarkLODDirty(int32 LODIndex)
{
	FScopeLock Lock(&SyncRoot);

	check(LODs.IsValidIndex(LODIndex));

	// Flag for update
	SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(INDEX_NONE) = ESectionUpdateType::Mesh;

	MarkForUpdate();
}

void FRuntimeMeshData::MarkAllLODsDirty()
{
	FScopeLock Lock(&SyncRoot);

	// Flag for update
	for (int32 LODIdx = 0; LODIdx < LODs.Num(); LODIdx++)
	{
		SectionsToUpdate.FindOrAdd(LODIdx).FindOrAdd(INDEX_NONE) = ESectionUpdateType::Mesh;
	}

	MarkForUpdate();
}

void FRuntimeMeshData::SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	FScopeLock Lock(&SyncRoot);

	check(LODs.IsValidIndex(LODIndex));
	check(LODs[LODIndex].Sections.Contains(SectionId));

	FRuntimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section)
	{
		Section->bIsVisible = bIsVisible;
		ESectionUpdateType& UpdateType = SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId);
		UpdateType |= ESectionUpdateType::Properties;
		UpdateType = (UpdateType & ~ESectionUpdateType::ClearOrRemove);
		MarkForUpdate();
	}
}

void FRuntimeMeshData::SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	FScopeLock Lock(&SyncRoot);

	check(LODs.IsValidIndex(LODIndex));
	check(LODs[LODIndex].Sections.Contains(SectionId));

	FRuntimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section)
	{
		Section->bCastsShadow = bCastsShadow;
		ESectionUpdateType& UpdateType = SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId);
		UpdateType |= ESectionUpdateType::Properties;
		UpdateType = (UpdateType & ~ESectionUpdateType::ClearOrRemove);
		MarkForUpdate();
	}
}

void FRuntimeMeshData::RemoveSection(int32 LODIndex, int32 SectionId)
{
	FScopeLock Lock(&SyncRoot);

	LODs[LODIndex].Sections.Remove(SectionId);

	if (RenderProxy.IsValid())
	{
		SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId) = ESectionUpdateType::Remove;
		MarkForUpdate();
		RecreateAllComponentProxies();
	}
}

void FRuntimeMeshData::MarkCollisionDirty()
{
	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
		[](URuntimeMesh* Mesh)
		{
			Mesh->MarkCollisionDirty();
		}
	));
}






/*
*	This tick function is used to offload the mesh updates to the end of frame.
*	This is meant to only update the mesh a single time per frame as any more would be a waste
*/
struct FRuntimeMeshDataDelayedActionTickObject : FTickableGameObject
{
private:
	TQueue<TTuple<FRuntimeMeshDataWeakPtr, FRuntimeMeshProviderThreadExclusiveFunction>> ProxyParamsUpdateList;
	mutable FCriticalSection ProxyParamsUpdateLock;
	TQueue<FRuntimeMeshDataWeakPtr> UpdateList;
	mutable FCriticalSection UpdateListLock;

	bool DequeueProxyParamsUpdate(TTuple<FRuntimeMeshDataWeakPtr, FRuntimeMeshProviderThreadExclusiveFunction>& Entry)
	{
		FScopeLock Lock(&ProxyParamsUpdateLock);
		return ProxyParamsUpdateList.Dequeue(Entry);
	}

	void EnqueueProxyParamsUpdate(const TTuple<FRuntimeMeshDataWeakPtr, FRuntimeMeshProviderThreadExclusiveFunction>& Entry)
	{
		FScopeLock Lock(&ProxyParamsUpdateLock);
		ProxyParamsUpdateList.Enqueue(Entry);
	}

	bool DequeueAsyncTask(FRuntimeMeshDataWeakPtr& Entry)
	{
		FScopeLock Lock(&UpdateListLock);
		return UpdateList.Dequeue(Entry);
	}

	void EnqueueAsyncTask(const FRuntimeMeshDataWeakPtr& Entry)
	{
		FScopeLock Lock(&UpdateListLock);
		UpdateList.Enqueue(Entry);
	}




	class FRuntimeMeshDataBackgroundWorker : public FRunnable
	{
		FRuntimeMeshDataDelayedActionTickObject* Parent;
		FThreadSafeBool ShouldRun;
	public:

		FRuntimeMeshDataBackgroundWorker(FRuntimeMeshDataDelayedActionTickObject* InParent)
			: Parent(InParent)
			, ShouldRun(true)
		{

		}

		uint32 Run() override
		{
			static const double TimeSlice = 1.0f;
			while (ShouldRun)
			{
				bool bHasMoreWork = Parent->DoThreadedWork(TimeSlice);

				if (!bHasMoreWork)
				{
					FPlatformProcess::Sleep(1.0 / 60);
				}
			}
			return 0;
		}


		void Stop() override
		{
			ShouldRun = false;
		}

	};

	enum class ECurrentThreadingType
	{
		None,
		Internal,
		UserSupplied,
	};

	ECurrentThreadingType CurrentThreadingType;
	TArray<TUniquePtr<FRuntimeMeshDataBackgroundWorker>> Workers;
	TArray<TUniquePtr<FRunnableThread>> WorkerThreads;

public:
	FRuntimeMeshDataDelayedActionTickObject()
		: CurrentThreadingType(ECurrentThreadingType::None)
	{}

	~FRuntimeMeshDataDelayedActionTickObject()
	{
		RemoveThreadsToMaxCount(0);

		check(Workers.Num() == 0 && WorkerThreads.Num() == 0);
	}

	static FRuntimeMeshDataDelayedActionTickObject& GetInstance()
	{
		static TUniquePtr<FRuntimeMeshDataDelayedActionTickObject> UpdaterObject;
		if (!UpdaterObject.IsValid())
		{
			UpdaterObject = MakeUnique<FRuntimeMeshDataDelayedActionTickObject>();
		}
		return *UpdaterObject.Get();
	}

	void RegisterForProxyParamsUpdate(FRuntimeMeshDataWeakPtr InMesh, FRuntimeMeshProviderThreadExclusiveFunction Func)
	{
		FRuntimeMeshDataPtr Mesh = InMesh.Pin();

		if (Mesh)
		{
			bool bIsRegistered = Mesh->AsyncWorkState.SetHasGameThreadWork();

			// Are we already queued?
			if (!bIsRegistered)
			{
				EnqueueProxyParamsUpdate(MakeTuple(InMesh, Func));
			}
		}
	}

	void RegisterForUpdate(FRuntimeMeshDataWeakPtr InMesh)
	{
		FRuntimeMeshDataPtr Mesh = InMesh.Pin();

		if (Mesh)
		{
			bool bIsRegistered = Mesh->AsyncWorkState.SetHasAsyncWork();

			// Are we already queued?
			if (!bIsRegistered)
			{
				EnqueueAsyncTask(InMesh);
			}
		}
	}

	virtual void Tick(float DeltaTime)
	{
		//SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshDelayedActions_Tick);

		UpdateGameThreadTasks();
		
		if (CurrentThreadingType == ECurrentThreadingType::None)
		{
			DoThreadedWork(1000 / 60);
		}
	}
	virtual bool IsTickable() const 
	{
		{
			FScopeLock Lock(&ProxyParamsUpdateLock);
			if (!ProxyParamsUpdateList.IsEmpty())
			{
				return true;
			}
		}
		if (CurrentThreadingType == ECurrentThreadingType::None)
		{
			FScopeLock Lock(&UpdateListLock);
			return !UpdateList.IsEmpty();
		}

		return false;
	}
	virtual bool IsTickableInEditor() const { return true; }
	virtual TStatId GetStatId() const { return TStatId(); }

	void RemoveThreadsToMaxCount(int32 NewNumThreads)
	{
		// Remove excessive threads
		for (int32 Index = Workers.Num() - 1; Index > (NewNumThreads - 1); Index--)
		{
			Workers[Index - 1]->Stop();
		}

		for (int32 Index = Workers.Num() - 1; Index > (NewNumThreads - 1); Index--)
		{
			WorkerThreads[Index]->WaitForCompletion();
			WorkerThreads[Index]->Kill();

			WorkerThreads.RemoveAt(Index);
			Workers.RemoveAt(Index);
		}
	}


	void InitializeMultiThreading(int32 NumThreads, int32 StackSize = 0, EThreadPriority ThreadPriority = TPri_BelowNormal)
	{
		CurrentThreadingType = ECurrentThreadingType::Internal;

		// Add new threads			
		for (int32 Index = Workers.Num(); Index < NumThreads; Index++)
		{
			Workers.Add(MakeUnique<FRuntimeMeshDataBackgroundWorker>(this));

			WorkerThreads.Add(TUniquePtr<FRunnableThread>(FRunnableThread::Create(Workers[Index].Get(),
				*FString::Printf(TEXT("RuntimeMeshBackgroundThread: %d"), Index), StackSize, ThreadPriority)));
		}

		RemoveThreadsToMaxCount(NumThreads);
	}

	FRuntimeMeshBackgroundWorkDelegate InitializeUserSuppliedThreading()
	{
		CurrentThreadingType = ECurrentThreadingType::UserSupplied;

		RemoveThreadsToMaxCount(0);

		return FRuntimeMeshBackgroundWorkDelegate::CreateLambda([](double MaxAllowedTime)
			{
				FRuntimeMeshDataDelayedActionTickObject::GetInstance().DoThreadedWork(MaxAllowedTime);
			});
	}

	void UpdateGameThreadTasks()
	{
		TQueue<TTuple<FRuntimeMeshDataWeakPtr, FRuntimeMeshProviderThreadExclusiveFunction>> RemainingList;


		// Here we handle the game thread only tasks, Each tick we try to handle every tasks
		// If a task is currently doing background work we add it back to the queue for next tick
		TTuple<FRuntimeMeshDataWeakPtr, FRuntimeMeshProviderThreadExclusiveFunction> TempData;
		while (DequeueProxyParamsUpdate(TempData))
		{
			FRuntimeMeshDataPtr Mesh = TempData.Key.Pin();
			if (!Mesh.IsValid())
			{
				continue;
			}

			if (Mesh->AsyncWorkState.TryLockForGameThread())
			{
				TempData.Value.Execute();
			}
			else
			{
				RemainingList.Enqueue(TempData);
			}
		}

		// Add the remaining list back to the queue for next tick
		while (RemainingList.Dequeue(TempData))
		{
			EnqueueProxyParamsUpdate(TempData);
		}
	}

	bool DoThreadedWork(double MaxAllowedTime)
	{
		// Convert time to sections
		MaxAllowedTime /= 60.0;

		double StartTime = FPlatformTime::Seconds();

		FRuntimeMeshDataWeakPtr FirstSkippedMesh;
		FRuntimeMeshDataWeakPtr TempMesh;
		while ((FPlatformTime::Seconds() - StartTime) < MaxAllowedTime && DequeueAsyncTask(TempMesh))
		{
			bool bHandled = false;
			
			FRuntimeMeshDataPtr Mesh = TempMesh.IsValid() ? TempMesh.Pin() : nullptr;
			if (!Mesh.IsValid())
			{
				continue;
			}

			// This stops the loop when we come full circle. So if we skip any due to inability to lock
			// we don't just keep tight looping to retry 
			if (FirstSkippedMesh.IsValid() && FirstSkippedMesh == TempMesh)
			{
				break;
			}

			if (Mesh->AsyncWorkState.TryLockForAsyncThread())
			{
				Mesh->HandleUpdate();
			}
			else
			{
				EnqueueAsyncTask(TempMesh);
			}

		}

		// Is there still more work to do?
		return !UpdateList.IsEmpty();
	}
};



void FRuntimeMeshData::InitializeMultiThreading(int32 NumThreads, int32 StackSize /*= 0*/, EThreadPriority ThreadPriority /*= TPri_BelowNormal*/)
{
	FRuntimeMeshDataDelayedActionTickObject::GetInstance().InitializeMultiThreading(NumThreads, StackSize, ThreadPriority);
}

FRuntimeMeshBackgroundWorkDelegate FRuntimeMeshData::InitializeUserSuppliedThreading()
{
	return FRuntimeMeshDataDelayedActionTickObject::GetInstance().InitializeUserSuppliedThreading();
}


void FRuntimeMeshData::DoOnGameThreadAndBlockThreads(FRuntimeMeshProviderThreadExclusiveFunction Func)
{
	FRuntimeMeshDataDelayedActionTickObject::GetInstance().RegisterForProxyParamsUpdate(this->AsSharedType<FRuntimeMeshData>(), Func);
}

void FRuntimeMeshData::MarkForUpdate()
{
	FRuntimeMeshDataDelayedActionTickObject::GetInstance().RegisterForUpdate(this->AsSharedType<FRuntimeMeshData>());
}

void FRuntimeMeshData::HandleUpdate()
{
	if (!RenderProxy.IsValid())
	{
		return;
	}


	TMap<int32, TSet<int32>> SectionsToGetMesh;


	bool bRequiresProxyRecreate = false;

	{	// Copy the update list so we can only hold the lock for a moment
		FScopeLock Lock(&SyncRoot);

		for (const auto& LODToUpdate : SectionsToUpdate)
		{
			int32 LODId = LODToUpdate.Key;

			for (const auto& SectionToUpdate : LODToUpdate.Value)
			{
				int32 SectionId = SectionToUpdate.Key;
				ESectionUpdateType UpdateType = SectionToUpdate.Value;

				if (SectionId == INDEX_NONE)
				{
					SectionsToGetMesh.FindOrAdd(LODId).Add(INDEX_NONE);
					continue;
				}

				if (LODs[LODId].Sections.Contains(SectionId))
				{
					if (EnumHasAnyFlags(UpdateType, ESectionUpdateType::AllData))
					{
						if (EnumHasAllFlags(UpdateType, ESectionUpdateType::Properties))
						{
							const auto& Section = LODs[LODId].Sections[SectionId];
							RenderProxy->CreateOrUpdateSection_GameThread(LODId, SectionId, Section, false);
						}

						if (EnumHasAllFlags(UpdateType, ESectionUpdateType::Mesh))
						{
							SectionsToGetMesh.FindOrAdd(LODId).Add(SectionId);
						}
					}
					else
					{
						if (EnumHasAllFlags(UpdateType, ESectionUpdateType::Remove))
						{
							RenderProxy->RemoveSection_GameThread(LODId, SectionId);
						}
						else if (EnumHasAllFlags(UpdateType, ESectionUpdateType::Clear))
						{
							RenderProxy->ClearSection_GameThread(LODId, SectionId);
						}
					}
				}
			}
		}

		SectionsToUpdate.Reset();
	}

	for (const auto& LODEntry : SectionsToGetMesh)
	{
		const auto& LODId = LODEntry.Key;
		auto& LOD = LODs[LODId];
		const auto& Sections = LODEntry.Value;


		// Update the meshes, use the bulk update path if available and requested
		if ((LOD.Properties.bCanGetAllSectionsAtOnce || !LOD.Properties.bCanGetSectionsIndependently) && (Sections.Contains(INDEX_NONE) || Sections.Num() == LOD.Sections.Num()))
		{
			HandleFullLODUpdate(LODId, bRequiresProxyRecreate);
		}
		else if (Sections.Contains(INDEX_NONE))
		{
			for (const auto& Section : LOD.Sections)
			{
				HandleSingleSectionUpdate(LODId, Section.Key, bRequiresProxyRecreate);
			}
		}
		else
		{
			for (const auto& Section : Sections)
			{
				HandleSingleSectionUpdate(LODId, Section, bRequiresProxyRecreate);
			}
		}
	}

	if (bRequiresProxyRecreate)
	{
		RecreateAllComponentProxies();
	}
}

void FRuntimeMeshData::HandleFullLODUpdate(int32 LODId, bool& bRequiresProxyRecreate)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMD(%d): HandleFullLODUpdate Called"), FPlatformTLS::GetCurrentThreadId());

	auto& LOD = LODs[LODId];

	TMap<int32, TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData>> MeshDatas;
	TSet<int32> ExistingSections;

	// Setup mesh datas
	for (auto& Entry : LOD.Sections)
	{
		FRuntimeMeshSectionProperties Properties = Entry.Value;
		MeshDatas.Add(Entry.Key, MakeTuple(Properties, FRuntimeMeshRenderableMeshData(
			Properties.bUseHighPrecisionTangents,
			Properties.bUseHighPrecisionTexCoords,
			Properties.NumTexCoords,
			Properties.bWants32BitIndices)));
		ExistingSections.Add(Entry.Key);
	}

	// Get all meshes
	bool bResult = BaseProvider->GetAllSectionsMeshForLOD(LODId, MeshDatas);

	// Update all the sections or create new ones
	for (auto& Entry : MeshDatas)
	{
		TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData>& Section = Entry.Value;

		if (bResult && Section.Value.HasValidMeshData())
		{
			LOD.Sections.FindOrAdd(Entry.Key) = Section.Key;

			RenderProxy->CreateOrUpdateSection_GameThread(LODId, Entry.Key, Section.Key, true);
			RenderProxy->UpdateSectionMesh_GameThread(LODId, Entry.Key, MakeShared<FRuntimeMeshRenderableMeshData>(MoveTemp(Section.Value)));
			bRequiresProxyRecreate = true;
		}
		else
		{
			// Clear existing section
			RenderProxy->ClearSection_GameThread(LODId, Entry.Key);
			bRequiresProxyRecreate = true;
		}

		// Remove the key from existing sections
		ExistingSections.Remove(Entry.Key);
	}

	// Remove all old sections that don't exist now
	for (auto& Entry : ExistingSections)
	{
		LODs[LODId].Sections.Remove(Entry);
		RenderProxy->RemoveSection_GameThread(LODId, Entry);
		bRequiresProxyRecreate = true;
	}
}

void FRuntimeMeshData::HandleSingleSectionUpdate(int32 LODId, int32 SectionId, bool& bRequiresProxyRecreate)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMD(%d): HandleSingleSectionUpdate Called"), FPlatformTLS::GetCurrentThreadId());

	FRuntimeMeshSectionProperties Properties = LODs[LODId].Sections.FindChecked(SectionId);
	TSharedPtr<FRuntimeMeshRenderableMeshData> MeshData = MakeShared<FRuntimeMeshRenderableMeshData>(
		Properties.bUseHighPrecisionTangents,
		Properties.bUseHighPrecisionTexCoords,
		Properties.NumTexCoords,
		Properties.bWants32BitIndices);
	bool bResult = BaseProvider->GetSectionMeshForLOD(LODId, SectionId, *MeshData);

	if (bResult)
	{
		// Update section
		RenderProxy->UpdateSectionMesh_GameThread(LODId, SectionId, MeshData);
		bRequiresProxyRecreate |= Properties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent;
		bRequiresProxyRecreate = true;
	}
	else
	{
		// Clear section
		RenderProxy->ClearSection_GameThread(LODId, SectionId);
		bRequiresProxyRecreate |= Properties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent;
		bRequiresProxyRecreate = true;
	}
}

void FRuntimeMeshData::RecreateAllComponentProxies()
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMD(%d): RecreateAllComponentProxies Called"), FPlatformTLS::GetCurrentThreadId());

	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshData_RecreateProxies);

	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
		[](URuntimeMesh* Mesh)
		{
			Mesh->RecreateAllComponentProxies();
		}
	));
}

FRuntimeMeshProxyPtr FRuntimeMeshData::GetOrCreateRenderProxy(ERHIFeatureLevel::Type InFeatureLevel)
{
	if (RenderProxy.IsValid())
	{
		check(InFeatureLevel == RenderProxy->GetFeatureLevel());
		return RenderProxy;
	}

	{
		FScopeLock Lock(&SyncRoot);

		RenderProxy = MakeShareable(new FRuntimeMeshProxy(InFeatureLevel), FRuntimeMeshRenderThreadDeleter<FRuntimeMeshProxy>());

		if (LODs.Num() > 0)
		{
			TArray<FRuntimeMeshLODProperties> LODProperties;
			LODProperties.SetNum(LODs.Num());
			for (int32 Index = 0; Index < LODs.Num(); Index++)
			{
				LODProperties[Index] = LODs[Index].Properties;
			}

			RenderProxy->InitializeLODs_GameThread(LODProperties);

			bool bHadAnyInitialized = false;
			for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
			{
				FRuntimeMeshLOD& LOD = LODs[LODIndex];
				for (int32 SectionId = 0; SectionId < LOD.Sections.Num(); SectionId++)
				{
					RenderProxy->CreateOrUpdateSection_GameThread(LODIndex, SectionId, LOD.Sections[SectionId], true);
					bHadAnyInitialized = true;

				}
			}

			if (bHadAnyInitialized)
			{
				MarkAllLODsDirty();
			}
		}
	}
	return RenderProxy;
}
