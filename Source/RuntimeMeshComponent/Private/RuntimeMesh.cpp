// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMesh.h"
#include "RuntimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "IPhysXCookingModule.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshProxy.h"
#include "Providers/RuntimeMeshProviderStatic.h"
#include "RuntimeMeshComponentEngineSubsystem.h"
#include "Async/AsyncWork.h"
#include "UObject/UObjectThreadContext.h"


DECLARE_DWORD_COUNTER_STAT(TEXT("RuntimeMeshDelayedActions - Updated Actors"), STAT_RuntimeMeshDelayedActions_UpdatedActors, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Tick"), STAT_RuntimeMeshDelayedActions_Tick, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Initialize"), STAT_RuntimeMesh_Initialize, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Get Physics TriMesh"), STAT_RuntimeMesh_GetPhysicsTriMesh, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Has Physics TriMesh"), STAT_RuntimeMesh_HasPhysicsTriMesh, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Update Collision"), STAT_RuntimeMesh_UpdateCollision, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Finish Collision Async Cook"), STAT_RuntimeMesh_FinishCollisionAsyncCook, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Finalize Collision Cooked Data"), STAT_RuntimeMesh_FinalizeCollisionCookedData, STATGROUP_RuntimeMesh);


#define RMC_LOG_VERBOSE(Format, ...) \
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("[RM:%d Thread:%d]: " Format), GetMeshId(), FPlatformTLS::GetCurrentThreadId(), ##__VA_ARGS__);




class FRuntimeMeshUpdateTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FRuntimeMeshUpdateTask>;

	FRuntimeMeshWeakRef Ref;

	FRuntimeMeshUpdateTask(const FRuntimeMeshWeakRef& InRef)
		: Ref(InRef)
	{
	}

	void DoWork()
	{
		FRuntimeMeshSharedRef PinnedRef = Ref.Pin();
		if (PinnedRef)
		{
			if (PinnedRef->bQueuedForMeshUpdate.AtomicSet(false))
			{
				// TODO: This really shouldn't be required.... it shouldn't be unreachable and still getting a pinned ref
				//if (!PinnedRef->IsUnreachable())
				//{
				PinnedRef->HandleUpdate();
				//}
			}

		}
	}

	FORCEINLINE TStatId GetStatId() const
	{
		return TStatId();
		// RETURN_QUICK_DECLARE_CYCLE_STAT(ExampleAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};





//////////////////////////////////////////////////////////////////////////
//	URuntimeMesh

URuntimeMesh::URuntimeMesh(const FObjectInitializer& ObjectInitializer)
	: URuntimeMeshProviderTargetInterface(ObjectInitializer)
	, bQueuedForMeshUpdate(false)
	, bCollisionIsDirty(false)
	, MeshProviderPtr(nullptr)
	, BodySetup(nullptr)
	, GCAnchor(this)
{
}

void URuntimeMesh::Initialize(URuntimeMeshProvider* Provider)
{
	RMC_LOG_VERBOSE("Initialize called");

	if (Provider == nullptr)
	{
		RMC_LOG_VERBOSE("Initialize called with a null provider... resetting.");
		Reset();		
		return;
	}
	
	// If this provider is still bound, remove it from the current bound mesh
	if (Provider->IsBound())
	{
		RMC_LOG_VERBOSE("Initialize called with provider already bound to a mesh... Taking ownership.");
		Provider->Shutdown();
	}

	check(!Provider->IsBound());


// 	{
// 		FReadScopeLock Lock(MeshProviderLock);
// 
// 		// Are we already bound to this provider? If so ignore it
// 		if (MeshProviderPtr == Provider)
// 		{
// 			RMC_LOG_VERBOSE("Initialize called with the same provider as already bound... ignoring.");
// 			return;
// 		}
// 
// 		// Is this new provider somehow bound to us, but not us to it? 
// 		if (Provider->IsBound())
// 		{
// 			// Are we somehow in some invalid state where the provider is bound to us, but we're not bound to it?
// 			auto OtherMeshRef = Provider->GetMeshReference().Pin();
// 			if (OtherMeshRef && (OtherMeshRef.Get() == this))
// 			{
// 				RMC_LOG_VERBOSE("Initialize called with RuntimeMeshProvider(%d) bound to us, but not to it...", Provider->GetUniqueID());
// 				return;
// 			}
// 
// 			RMC_LOG_VERBOSE("Initialize called with provider already bound to RuntimeMesh(%d)... ignoring.", (OtherMeshRef? OtherMeshRef->GetMeshId() : -1));
// 			return;
// 		}
// 	}
		

#if WITH_EDITOR
	Modify(true);
#endif

	// Clear any existing binding and all data/proxies so we start over with the new provider
	Reset();

	{
		FWriteScopeLock Lock(MeshProviderLock);
		MeshProviderPtr = Provider;
	}

	{
		FReadScopeLock Lock(MeshProviderLock);
		if (MeshProviderPtr)
		{
			MeshProviderPtr->BindTargetProvider(this);
			MeshProviderPtr->Initialize();
		}
	}
}


void URuntimeMesh::Reset()
{
	RMC_LOG_VERBOSE("Reset called.");
	GCAnchor.BeginNewState();

	{
		FWriteScopeLock Lock(MeshProviderLock);
		if (MeshProviderPtr)
		{
			MeshProviderPtr->UnlinkProviders();
			MeshProviderPtr = nullptr;
		}
	}

	FScopeLock Lock(&SyncRoot);

	BodySetup = nullptr;
	CollisionSource.Empty();
	AsyncBodySetupQueue.Empty();
	PendingSourceInfo.Reset();
	bCollisionIsDirty = false;

	LODs.Empty();
	MaterialSlots.Empty();
	SlotNameLookup.Empty();
	SectionsToUpdate.Empty();

	if (RenderProxy)
	{
		RenderProxy->ResetProxy_GameThread();
		RenderProxy.Reset();
	}

	for (const auto& LinkedComponent : LinkedComponents)
	{
		URuntimeMeshComponent* Comp = LinkedComponent.Get();

		if (Comp && Comp->IsValidLowLevel())
		{
			Comp->ForceProxyRecreate();
			Comp->NewBoundsReceived();
			Comp->NewCollisionMeshReceived();
		}
	}
}

FRuntimeMeshCollisionHitInfo URuntimeMesh::GetHitSource(int32 FaceIndex) const
{
	const TArray<FRuntimeMeshCollisionSourceSectionInfo>& TempSections = CollisionSource;

	if (TempSections.Num() > 0)
	{
		for (const auto& Section : TempSections)
		{
			if (Section.StartIndex <= FaceIndex && Section.EndIndex >= FaceIndex)
			{
				FRuntimeMeshCollisionHitInfo HitInfo;
				HitInfo.SourceProvider = Section.SourceProvider;
				HitInfo.SourceType = Section.SourceType;
				HitInfo.SectionId = Section.SectionId;
				HitInfo.FaceIndex = FaceIndex - Section.StartIndex;
				return HitInfo;
			}
		}
	}

	FRuntimeMeshCollisionHitInfo HitInfo;
	//HitInfo.SourceProvider = MeshProvider;
	HitInfo.SourceType = ERuntimeMeshCollisionFaceSourceType::Collision;
	HitInfo.SectionId = 0;
	HitInfo.FaceIndex = FaceIndex;
	return HitInfo;
}


FBoxSphereBounds URuntimeMesh::GetLocalBounds() const
{
	FReadScopeLock Lock(MeshProviderLock);
	if (MeshProviderPtr)
	{
		return MeshProviderPtr->GetBounds();
	}

	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1.0f));
}

// 
// void URuntimeMesh::ShutdownInternal()
// {
// 	Reset();
// }

void URuntimeMesh::ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs)
{
	RMC_LOG_VERBOSE("ConfigureLODs called.");
	{
		FScopeLock Lock(&SyncRoot);
		LODs.Empty();
		LODs.SetNum(InLODs.Num());
		for (int32 Index = 0; Index < InLODs.Num(); Index++)
		{
			LODs[Index].Properties = InLODs[Index];
		}
	}

	if (RenderProxy.IsValid())
	{
		RenderProxy->InitializeLODs_GameThread(InLODs);
		RecreateAllComponentSceneProxies();
	}
}

void URuntimeMesh::SetLODScreenSize(int32 LODIndex, float ScreenSize)
{
	RMC_LOG_VERBOSE("MarkLODDirty called: LOD:%d ScreenSize:%f", LODIndex, ScreenSize);
	// TODO: Implement
}

void URuntimeMesh::MarkLODDirty(int32 LODIndex)
{
	RMC_LOG_VERBOSE("MarkLODDirty called: LOD:%d", LODIndex);

	FScopeLock Lock(&SyncRoot);

	check(LODs.IsValidIndex(LODIndex));

	// Flag for update
	SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(INDEX_NONE) = ESectionUpdateType::Mesh;

	QueueForMeshUpdate();
}

void URuntimeMesh::MarkAllLODsDirty()
{
	RMC_LOG_VERBOSE("MarkAllLODsDirty called.");

	FScopeLock Lock(&SyncRoot);

	// Flag for update
	for (int32 LODIdx = 0; LODIdx < LODs.Num(); LODIdx++)
	{
		SectionsToUpdate.FindOrAdd(LODIdx).FindOrAdd(INDEX_NONE) = ESectionUpdateType::Mesh;
	}

	QueueForMeshUpdate();
}


void URuntimeMesh::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	RMC_LOG_VERBOSE("CreateSection called: LOD:%d Section:%d", LODIndex, SectionId);

	check(LODs.IsValidIndex(LODIndex));
	check(SectionId >= 0);

	FScopeLock Lock(&SyncRoot);

	LODs[LODIndex].Sections.FindOrAdd(SectionId) = SectionProperties;

	// Flag for update
	SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId) = ESectionUpdateType::AllData;
	QueueForMeshUpdate();
}

void URuntimeMesh::SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	RMC_LOG_VERBOSE("SetSectionVisibility called: LOD:%d Section:%d IsVisible:%d", LODIndex, SectionId, bIsVisible);

	check(LODs.IsValidIndex(LODIndex));
	check(LODs[LODIndex].Sections.Contains(SectionId));

	FScopeLock Lock(&SyncRoot);

	FRuntimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section && Section->bIsVisible != bIsVisible)
	{
		Section->bIsVisible = bIsVisible;
		ESectionUpdateType& UpdateType = SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId);
		UpdateType |= ESectionUpdateType::Properties;
		UpdateType = (UpdateType & ~ESectionUpdateType::ClearOrRemove);
		QueueForMeshUpdate();
	}
}

void URuntimeMesh::SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	RMC_LOG_VERBOSE("SetSectionCastsShadow called: LOD:%d Section:%d CastsShadow:%d", LODIndex, SectionId, bCastsShadow);

	check(LODs.IsValidIndex(LODIndex));
	check(LODs[LODIndex].Sections.Contains(SectionId));

	FScopeLock Lock(&SyncRoot);

	FRuntimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section && Section->bCastsShadow != bCastsShadow)
	{
		Section->bCastsShadow = bCastsShadow;
		ESectionUpdateType& UpdateType = SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId);
		UpdateType |= ESectionUpdateType::Properties;
		UpdateType = (UpdateType & ~ESectionUpdateType::ClearOrRemove);
		QueueForMeshUpdate();
	}
}

void URuntimeMesh::MarkSectionDirty(int32 LODIndex, int32 SectionId)
{
	RMC_LOG_VERBOSE("MarkSectionDirty called: LOD:%d Section:%d", LODIndex, SectionId);

	check(LODs.IsValidIndex(LODIndex));
	check(LODs[LODIndex].Sections.Contains(SectionId));

	FScopeLock Lock(&SyncRoot);

	// Flag for update
	ESectionUpdateType& UpdateType = SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId);
	UpdateType |= ESectionUpdateType::Mesh;
	UpdateType = (UpdateType & ~ESectionUpdateType::ClearOrRemove);
	QueueForMeshUpdate();
}

void URuntimeMesh::ClearSection(int32 LODIndex, int32 SectionId)
{
	RMC_LOG_VERBOSE("ClearSection called: LOD:%d Section:%d", LODIndex, SectionId);

	FScopeLock Lock(&SyncRoot);

	check(LODs.IsValidIndex(LODIndex));

	FRuntimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section)
	{
		ESectionUpdateType& UpdateType = SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId);
		UpdateType = ESectionUpdateType::Clear;
		QueueForMeshUpdate();
	}
}

void URuntimeMesh::RemoveSection(int32 LODIndex, int32 SectionId)
{
	RMC_LOG_VERBOSE("RemoveSection called: LOD:%d Section:%d", LODIndex, SectionId);

	FScopeLock Lock(&SyncRoot);

	check(LODs.IsValidIndex(LODIndex));

	if (LODs[LODIndex].Sections.Remove(SectionId))
	{
		if (RenderProxy.IsValid())
		{
			SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId) = ESectionUpdateType::Remove;
			QueueForMeshUpdate();
			RecreateAllComponentSceneProxies();
		}
	}
}

void URuntimeMesh::MarkCollisionDirty()
{
	RMC_LOG_VERBOSE("MarkCollisionDirty called.");

	QueueForCollisionUpdate();
}


void URuntimeMesh::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	RMC_LOG_VERBOSE("SetupMaterialSlot called: Slot:%d Name:%s Mat:%s", MaterialSlot, *SlotName.ToString(), InMaterial? *InMaterial->GetName() : TEXT(""));
	{
		FScopeLock Lock(&SyncRoot);
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
	}

	RecreateAllComponentSceneProxies();
}

int32 URuntimeMesh::GetMaterialIndex(FName MaterialSlotName)
{
	FScopeLock Lock(&SyncRoot);
	const int32* SlotIndex = SlotNameLookup.Find(MaterialSlotName);
	return SlotIndex ? *SlotIndex : INDEX_NONE;
}

bool URuntimeMesh::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	FScopeLock Lock(&SyncRoot);
	return SlotNameLookup.Contains(MaterialSlotName);
}

FRuntimeMeshMaterialSlot URuntimeMesh::GetMaterialSlot(int32 SlotIndex)
{
	FScopeLock Lock(&SyncRoot);
	return MaterialSlots[SlotIndex];
}

int32 URuntimeMesh::GetNumMaterials()
{
	FScopeLock Lock(&SyncRoot);
	return MaterialSlots.Num();
}

TArray<FName> URuntimeMesh::GetMaterialSlotNames()
{
	FScopeLock Lock(&SyncRoot);
	TArray<FName> OutNames;
	SlotNameLookup.GetKeys(OutNames);
	return OutNames;
}

TArray<FRuntimeMeshMaterialSlot> URuntimeMesh::GetMaterialSlots()
{
	FScopeLock Lock(&SyncRoot);
	return MaterialSlots;
}

UMaterialInterface* URuntimeMesh::GetMaterial(int32 SlotIndex)
{
	FScopeLock Lock(&SyncRoot);
	if (MaterialSlots.IsValidIndex(SlotIndex))
	{
		return MaterialSlots[SlotIndex].Material;
	}
	return nullptr;
}





void URuntimeMesh::BeginDestroy()
{
	RMC_LOG_VERBOSE("BeginDestroy called.");
	Reset();
	GCAnchor.BeginDestroy();
	Super::BeginDestroy();
}

bool URuntimeMesh::IsReadyForFinishDestroy()
{
	return Super::IsReadyForFinishDestroy() && GCAnchor.IsFree();
}

void URuntimeMesh::PostLoad()
{
	Super::PostLoad();

	//QueueForDelayedInitialize();
	//QueueForCollisionUpdate();
}

void URuntimeMesh::PostEditImport()
{
	Super::PostEditImport();
}

void URuntimeMesh::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
}



bool URuntimeMesh::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	RMC_LOG_VERBOSE("GetPhysicsTriMeshData called.");
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetPhysicsTriMesh);

	FReadScopeLock Lock(MeshProviderLock);
	if (MeshProviderPtr)
	{
		FRuntimeMeshCollisionData CollisionMesh;

		if (MeshProviderPtr->GetCollisionMesh(CollisionMesh))
		{
			CollisionData->Vertices = CollisionMesh.Vertices.TakeContents();
			CollisionData->Indices = CollisionMesh.Triangles.TakeContents();
			CollisionData->UVs = CollisionMesh.TexCoords.TakeContents();
			CollisionData->MaterialIndices = CollisionMesh.MaterialIndices.TakeContents();

			CollisionData->bDeformableMesh = CollisionMesh.bDeformableMesh;
			CollisionData->bDisableActiveEdgePrecompute = CollisionMesh.bDisableActiveEdgePrecompute;
			CollisionData->bFastCook = CollisionMesh.bFastCook;
			CollisionData->bFlipNormals = CollisionMesh.bFlipNormals;

			*PendingSourceInfo = MoveTemp(CollisionMesh.CollisionSources);

			return true;
		}
	}

	return false;
}

bool URuntimeMesh::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	RMC_LOG_VERBOSE("ContainsPhysicsTriMeshData called.");
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_HasPhysicsTriMesh);

	FReadScopeLock Lock(MeshProviderLock);
	if (MeshProviderPtr)
	{
		return MeshProviderPtr->HasCollisionMesh();
	}

	return false;
}

bool URuntimeMesh::WantsNegXTriMesh()
{
	return false;
}

void URuntimeMesh::GetMeshId(FString& OutMeshId)
{
	OutMeshId = TEXT("RuntimeMesh:") + FString::FromInt(GetMeshId());
}



void URuntimeMesh::QueueForDelayedInitialize()
{
// 	FRuntimeMeshMisc::DoOnGameThread([this]()
// 		{
// 			if (this->IsValidLowLevel())
// 			{
// 				bNeedsInitialization = true;
// 
// 				GetEngineSubsystem()->QueueMeshForUpdate(GetMeshReference());
// 			}
// 		});
}




void URuntimeMesh::QueueForUpdate()
{
	FRuntimeMeshMisc::DoOnGameThread([MeshPtr = GetMeshReference()]()
	{
		FRuntimeMeshSharedRef Mesh = MeshPtr.Pin();
		if (Mesh)
		{
			Mesh->bCollisionIsDirty = true;
			Mesh->GetEngineSubsystem()->QueueMeshForUpdate(Mesh->GetMeshReference());
		}
	});
}

void URuntimeMesh::QueueForMeshUpdate()
{
	// TODO: We shouldn't have to kick this to the game thread first

	FRuntimeMeshMisc::DoOnGameThread([MeshPtr = GetMeshReference()]()
	{
		FRuntimeMeshSharedRef Mesh = MeshPtr.Pin();
		if (Mesh)
		{
			if (Mesh->bQueuedForMeshUpdate.AtomicSet(true) == false)
			{
				FReadScopeLock Lock(Mesh->MeshProviderLock);
				if (Mesh->MeshProviderPtr)
				{
					if (Mesh->MeshProviderPtr->IsThreadSafe())
					{
						(new FAutoDeleteAsyncTask<FRuntimeMeshUpdateTask>(Mesh->GetMeshReference()))->StartBackgroundTask(Mesh->GetEngineSubsystem()->GetThreadPool());
					}
					else
					{
						Mesh->GetEngineSubsystem()->QueueMeshForUpdate(Mesh->GetMeshReference());
					}
				}
			}
		}
	});
}

void URuntimeMesh::QueueForCollisionUpdate()
{
	FRuntimeMeshMisc::DoOnGameThread([MeshPtr = GetMeshReference()]()
	{
		FRuntimeMeshSharedRef Mesh = MeshPtr.Pin();
		if (Mesh)
		{
			Mesh->bCollisionIsDirty = true;
			Mesh->GetEngineSubsystem()->QueueMeshForUpdate(Mesh->GetMeshReference());
		}
	});
}


void URuntimeMesh::UpdateAllComponentBounds()
{
	FRuntimeMeshMisc::DoOnGameThread([MeshPtr = GetMeshReference()]()
	{
		FRuntimeMeshSharedRef Mesh = MeshPtr.Pin();
			if (Mesh)
			{
				Mesh->DoForAllLinkedComponents([](URuntimeMeshComponent* MeshComponent)
					{
						MeshComponent->NewBoundsReceived();
					});
			}
		});
}

void URuntimeMesh::RecreateAllComponentSceneProxies()
{
	RMC_LOG_VERBOSE("RecreateAllComponentSceneProxies called.");
	FRuntimeMeshMisc::DoOnGameThread([MeshPtr = GetMeshReference()]()
	{
		FRuntimeMeshSharedRef Mesh = MeshPtr.Pin();
		if (Mesh)
		{
			Mesh->DoForAllLinkedComponents([](URuntimeMeshComponent* MeshComponent)
				{
					MeshComponent->ForceProxyRecreate();
				});
		}
	});
}


void URuntimeMesh::HandleUpdate()
{
	// TODO: this really shouldn't be necessary
	// but editor can sometimes force a delete
	FRuntimeMeshProxyPtr RenderProxyRef = RenderProxy;

	if (!RenderProxyRef.IsValid())
	{
		return;
	}

	RMC_LOG_VERBOSE("HandleUpdate called.");

	FReadScopeLock ProviderLock(MeshProviderLock);
	if (MeshProviderPtr)
	{
		TMap<int32, TSet<int32>> SectionsToGetMesh;


		bool bRequiresProxyRecreate = false;

		{	// Copy the update list so we can only hold the lock for a moment
			FScopeLock ConfigLock(&SyncRoot);

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
								RenderProxyRef->CreateOrUpdateSection_GameThread(LODId, SectionId, Section, false);
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
								RenderProxyRef->RemoveSection_GameThread(LODId, SectionId);
							}
							else if (EnumHasAllFlags(UpdateType, ESectionUpdateType::Clear))
							{
								RenderProxyRef->ClearSection_GameThread(LODId, SectionId);
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
				HandleFullLODUpdate(RenderProxyRef, LODId, bRequiresProxyRecreate);
			}
			else if (Sections.Contains(INDEX_NONE))
			{
				for (const auto& Section : LOD.Sections)
				{
					HandleSingleSectionUpdate(RenderProxyRef, LODId, Section.Key, bRequiresProxyRecreate);
				}
			}
			else
			{
				for (const auto& Section : Sections)
				{
					HandleSingleSectionUpdate(RenderProxyRef, LODId, Section, bRequiresProxyRecreate);
				}
			}
		}

		UpdateAllComponentBounds();
		if (bRequiresProxyRecreate)
		{
			RecreateAllComponentSceneProxies();
		}
	}
}


void URuntimeMesh::HandleFullLODUpdate(const FRuntimeMeshProxyPtr& RenderProxyRef, int32 LODId, bool& bRequiresProxyRecreate)
{
	RMC_LOG_VERBOSE("HandleFullLODUpdate called: LOD:%d", LODId);

	auto& LOD = LODs[LODId];

	TMap<int32, FRuntimeMeshSectionData> MeshDatas;
	TSet<int32> ExistingSections;

	// Setup mesh datas
	for (auto& Entry : LOD.Sections)
	{
		FRuntimeMeshSectionProperties Properties = Entry.Value;
		MeshDatas.Add(Entry.Key, FRuntimeMeshSectionData(Properties));
		ExistingSections.Add(Entry.Key);
	}

	// Get all meshes
	bool bResult = MeshProviderPtr->GetAllSectionsMeshForLOD(LODId, MeshDatas);

	// Update all the sections or create new ones
	for (auto& Entry : MeshDatas)
	{
		FRuntimeMeshSectionData& Section = Entry.Value;

		if (bResult && Section.MeshData.HasValidMeshData())
		{
			LOD.Sections.FindOrAdd(Entry.Key) = Section.Properties;

			RenderProxyRef->CreateOrUpdateSection_GameThread(LODId, Entry.Key, Section.Properties, true);


			TSharedPtr<FRuntimeMeshSectionUpdateData> UpdateData = MakeShared<FRuntimeMeshSectionUpdateData>(MoveTemp(Section.MeshData));

			// Push the data to the gpu from this thread if we're not on the game thread and the current RHI supports async
			if (GRHISupportsAsyncTextureCreation && GIsThreadedRendering && !IsInGameThread())
			{
				UpdateData->CreateRHIBuffers<false>(Section.Properties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Frequent);
			}


			RenderProxyRef->UpdateSectionMesh_GameThread(LODId, Entry.Key, UpdateData);
			bRequiresProxyRecreate = true;
		}
		else
		{
			// Clear existing section
			RenderProxyRef->ClearSection_GameThread(LODId, Entry.Key);
			bRequiresProxyRecreate = true;
		}

		// Remove the key from existing sections
		ExistingSections.Remove(Entry.Key);
	}

	// Remove all old sections that don't exist now
	for (auto& Entry : ExistingSections)
	{
		LODs[LODId].Sections.Remove(Entry);
		RenderProxyRef->RemoveSection_GameThread(LODId, Entry);
		bRequiresProxyRecreate = true;
	}
}

void URuntimeMesh::HandleSingleSectionUpdate(const FRuntimeMeshProxyPtr& RenderProxyRef, int32 LODId, int32 SectionId, bool& bRequiresProxyRecreate)
{
	RMC_LOG_VERBOSE("HandleFullLODUpdate called: LOD:%d Section:%d", LODId, SectionId);

	FRuntimeMeshSectionProperties Properties = LODs[LODId].Sections.FindChecked(SectionId);
	FRuntimeMeshRenderableMeshData MeshData(
		Properties.bUseHighPrecisionTangents,
		Properties.bUseHighPrecisionTexCoords,
		Properties.NumTexCoords,
		Properties.bWants32BitIndices);
	bool bResult = MeshProviderPtr->GetSectionMeshForLOD(LODId, SectionId, MeshData);
	
	if (bResult && MeshData.HasValidMeshData())
	{
		// Update section
		TSharedPtr<FRuntimeMeshSectionUpdateData> UpdateData = MakeShared<FRuntimeMeshSectionUpdateData>(MoveTemp(MeshData));

		// Push the data to the gpu from this thread if we're not on the game thread and the current RHI supports async
		if (GRHISupportsAsyncTextureCreation && GIsThreadedRendering && !IsInGameThread())
		{
			UpdateData->CreateRHIBuffers<false>(Properties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Frequent);
		}

		RenderProxyRef->UpdateSectionMesh_GameThread(LODId, SectionId, UpdateData);
		bRequiresProxyRecreate |= Properties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent;
		bRequiresProxyRecreate = true;
	}
	else
	{
		// Clear section
		RenderProxyRef->ClearSection_GameThread(LODId, SectionId);
		bRequiresProxyRecreate |= Properties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent;
		bRequiresProxyRecreate = true;
	}
}



URuntimeMeshComponentEngineSubsystem* URuntimeMesh::GetEngineSubsystem()
{
	URuntimeMeshComponentEngineSubsystem* RMCSubsystem = GEngine->GetEngineSubsystem<URuntimeMeshComponentEngineSubsystem>();
	check(RMCSubsystem);
	return RMCSubsystem;
}

UBodySetup* URuntimeMesh::CreateNewBodySetup()
{
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	return NewBodySetup;
}

void URuntimeMesh::UpdateCollision(bool bForceCookNow)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateCollision);

	check(IsInGameThread());

	FReadScopeLock Lock(MeshProviderLock);
	if (MeshProviderPtr)
	{
		FRuntimeMeshCollisionSettings CollisionSettings = MeshProviderPtr->GetCollisionSettings();

		UWorld* World = GetWorld();
		const bool bShouldCookAsync = !bForceCookNow && World && World->IsGameWorld() && CollisionSettings.bUseAsyncCooking;

		const auto& SetupCollisionConfiguration = [&](UBodySetup* Setup)
		{
			Setup->BodySetupGuid = FGuid::NewGuid();

			Setup->bGenerateMirroredCollision = false;
			Setup->bDoubleSidedGeometry = true;
			Setup->CollisionTraceFlag = CollisionSettings.bUseComplexAsSimple ? CTF_UseComplexAsSimple : CTF_UseDefault;

			auto& ConvexElems = Setup->AggGeom.ConvexElems;
			ConvexElems.Empty();
			for (const FRuntimeMeshCollisionConvexMesh& Convex : CollisionSettings.ConvexElements)
			{
				FKConvexElem& NewConvexElem = *new(ConvexElems) FKConvexElem();
				NewConvexElem.VertexData = Convex.VertexBuffer;
				// TODO: Store this on the section so we don't have to compute it on each cook
				NewConvexElem.ElemBox = Convex.BoundingBox;
			}

			auto& BoxElems = Setup->AggGeom.BoxElems;
			BoxElems.Empty();
			for (const FRuntimeMeshCollisionBox& Box : CollisionSettings.Boxes)
			{
				FKBoxElem& NewBox = *new(BoxElems) FKBoxElem();
				NewBox.Center = Box.Center;
				NewBox.Rotation = Box.Rotation;
				NewBox.X = Box.Extents.X;
				NewBox.Y = Box.Extents.Y;
				NewBox.Z = Box.Extents.Z;
			}

			auto& SphereElems = Setup->AggGeom.SphereElems;
			SphereElems.Empty();
			for (const FRuntimeMeshCollisionSphere& Sphere : CollisionSettings.Spheres)
			{
				FKSphereElem& NewSphere = *new(SphereElems)FKSphereElem();
				NewSphere.Center = Sphere.Center;
				NewSphere.Radius = Sphere.Radius;
			}

			auto& SphylElems = Setup->AggGeom.SphylElems;
			SphylElems.Empty();
			for (const FRuntimeMeshCollisionCapsule& Capsule : CollisionSettings.Capsules)
			{
				FKSphylElem& NewSphyl = *new(SphylElems)FKSphylElem();
				NewSphyl.Center = Capsule.Center;
				NewSphyl.Rotation = Capsule.Rotation;
				NewSphyl.Radius = Capsule.Radius;
				NewSphyl.Length = Capsule.Length;
			}
		};


		if (bShouldCookAsync)
		{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 21
			// Abort all previous ones still standing
			for (const auto& OldBody : AsyncBodySetupQueue)
			{
				OldBody.BodySetup->AbortPhysicsMeshAsyncCreation();
			}
#endif

			UBodySetup* NewBodySetup = CreateNewBodySetup();
			SetupCollisionConfiguration(NewBodySetup);

			// Create pending source info while the mesh updates
			PendingSourceInfo = MakeUnique<TArray<FRuntimeMeshCollisionSourceSectionInfo>>();

			NewBodySetup->CreatePhysicsMeshesAsync(
				FOnAsyncPhysicsCookFinished::CreateUObject(this, &URuntimeMesh::FinishPhysicsAsyncCook, NewBodySetup));

			// Copy source info and reset pending
			AsyncBodySetupQueue.Add(FRuntimeMeshAsyncBodySetupData(NewBodySetup, MoveTemp(*PendingSourceInfo.Get())));
			PendingSourceInfo.Reset();
		}
		else
		{
			AsyncBodySetupQueue.Empty();
			UBodySetup* NewBodySetup = CreateNewBodySetup();

			// Change body setup guid 
			NewBodySetup->BodySetupGuid = FGuid::NewGuid();

			SetupCollisionConfiguration(NewBodySetup);

			// Update meshes
			NewBodySetup->bHasCookedCollisionData = true;

			// Create pending source info while the mesh updates
			PendingSourceInfo = MakeUnique<TArray<FRuntimeMeshCollisionSourceSectionInfo>>();

			NewBodySetup->InvalidatePhysicsData();
			NewBodySetup->CreatePhysicsMeshes();

			// Copy source info and reset pending
			CollisionSource = MoveTemp(*PendingSourceInfo.Get());
			PendingSourceInfo.Reset();

			BodySetup = NewBodySetup;
			FinalizeNewCookedData();
		}
	}
}

void URuntimeMesh::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_FinishCollisionAsyncCook);

	check(IsInGameThread());

	const auto& SearchPredicate = [&](const FRuntimeMeshAsyncBodySetupData& Entry)
	{
		return Entry.BodySetup == FinishedBodySetup;
	};
	int32 FoundIdx = AsyncBodySetupQueue.IndexOfByPredicate(SearchPredicate);

	if (FoundIdx != INDEX_NONE)
	{
		if (bSuccess)
		{
			// The new body was found in the array meaning it's newer so use it
			BodySetup = FinishedBodySetup;
			CollisionSource = MoveTemp(AsyncBodySetupQueue[FoundIdx].CollisionSources);

			// Shift down all remaining body setups, removing any old setups
			for (int32 Index = FoundIdx + 1; Index < AsyncBodySetupQueue.Num(); Index++)
			{
				AsyncBodySetupQueue[Index - (FoundIdx + 1)] = MoveTemp(AsyncBodySetupQueue[Index]);
			}
			AsyncBodySetupQueue.SetNum(AsyncBodySetupQueue.Num() - (FoundIdx + 1));

			FinalizeNewCookedData();

		}
		else
		{
			AsyncBodySetupQueue.RemoveAt(FoundIdx);
		}
	}
}

void URuntimeMesh::FinalizeNewCookedData()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_FinalizeCollisionCookedData);

	check(IsInGameThread());

	// Alert all linked components so they can update their physics state.
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
		{
			Mesh->NewCollisionMeshReceived();
		});

	// Call user event to notify of collision updated.
	if (CollisionUpdated.IsBound())
	{
		CollisionUpdated.Broadcast();
	}
}


void URuntimeMesh::RegisterLinkedComponent(URuntimeMeshComponent* NewComponent)
{
	RMC_LOG_VERBOSE("Registering RMC:%d", NewComponent->GetUniqueID());

	LinkedComponents.AddUnique(NewComponent);

	EnsureRenderProxyReady();

	if (BodySetup)
	{
		// Alert collision if we already have it
		NewComponent->NewCollisionMeshReceived();
	}
}

void URuntimeMesh::UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove)
{
	RMC_LOG_VERBOSE("Unregistering RMC:%d", ComponentToRemove->GetUniqueID());
	LinkedComponents.RemoveSingleSwap(ComponentToRemove, true);
}



void URuntimeMesh::EnsureRenderProxyReady()
{
	FReadScopeLock Lock(MeshProviderLock);
	if (MeshProviderPtr && !MeshProviderPtr->IsBound())
	{
		MeshProviderPtr->BindTargetProvider(this);
		MeshProviderPtr->Initialize();
	}
}

FRuntimeMeshProxyPtr URuntimeMesh::GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel)
{
	if (RenderProxy.IsValid())
	{
		return RenderProxy;
	}

	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_Initialize);

	RenderProxy = MakeShareable(new FRuntimeMeshProxy(GetMeshId()), FRuntimeMeshRenderThreadDeleter<FRuntimeMeshProxy>());

	FScopeLock Lock(&SyncRoot);
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
			for (auto pair : LOD.Sections)
			{
				RenderProxy->CreateOrUpdateSection_GameThread(LODIndex, pair.Key, pair.Value, true);
				bHadAnyInitialized = true;

			}
		}

		if (bHadAnyInitialized)
		{
			MarkAllLODsDirty();
		}
	}


	return RenderProxy;
}

bool URuntimeMesh::GetSceneFeatureLevel(ERHIFeatureLevel::Type& OutFeatureLevel)
{
	for (auto& Comp : LinkedComponents)
	{
		if (Comp.IsValid())
		{
			URuntimeMeshComponent* Component = Comp.Get();
			auto* Scene = Component->GetScene();
			if (Scene)
			{
				OutFeatureLevel = Scene->GetFeatureLevel();
				return true;
			}
		}
	}
	return false;
}





#undef RMC_LOG_VERBOSE