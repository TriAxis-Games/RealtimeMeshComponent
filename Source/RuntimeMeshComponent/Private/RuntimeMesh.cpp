// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

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




//////////////////////////////////////////////////////////////////////////
//	URuntimeMesh

URuntimeMesh::URuntimeMesh(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer)
	, bQueuedForMeshUpdate(false)
	, bNeedsInitialization(false)
	, bCollisionIsDirty(false)
	, MeshProvider(nullptr)
	, BodySetup(nullptr)
	, ReferenceAnchor(this)
{
}

void URuntimeMesh::Initialize(URuntimeMeshProvider* Provider)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RM(%d): Initialize called"), FPlatformTLS::GetCurrentThreadId());
	check(Provider);
	if (!Provider->HasBeenBound())
	{
		if (MeshProvider)
		{
			Reset();
		}

		// Flag initialized
		bNeedsInitialization = false;

		MeshProvider = Provider;

#if WITH_EDITOR
		Modify(true);
		PostEditChange();
#endif

		InitializeInternal();
	}
	else if (MeshProvider != Provider)
	{
		UE_LOG(RuntimeMeshLog, Error, TEXT("Cannot bind a provider to a RuntimeMesh after it has been bound to another provider."));
	}
}

void URuntimeMesh::Reset()
{
	FScopeLock Lock(&SyncRoot);

	ReferenceAnchor.BeginNewState();

	if (MeshProvider)
	{
		if (!MeshProvider->IsUnreachable())
		{
			MeshProvider->Unlink();
		}
		MeshProvider = nullptr;
	}


	BodySetup = nullptr;
	CollisionSource.Empty();
	AsyncBodySetupQueue.Empty();
	PendingSourceInfo.Reset();
	bCollisionIsDirty = false;

	LODs.Empty();
	RenderProxy.Reset();

	bNeedsInitialization = false;

	MaterialSlots.Empty();
	SlotNameLookup.Empty();

	SectionsToUpdate.Empty();

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
	HitInfo.SourceProvider = MeshProvider;
	HitInfo.SourceType = ERuntimeMeshCollisionFaceSourceType::Collision;
	HitInfo.SectionId = 0;
	HitInfo.FaceIndex = FaceIndex;
	return HitInfo;
}



void URuntimeMesh::ShutdownInternal()
{
	Reset();
}

void URuntimeMesh::ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& InLODs)
{
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

void URuntimeMesh::SetLODScreenSize_Implementation(int32 LODIndex, float ScreenSize)
{
	// TODO: Implement
}

void URuntimeMesh::MarkLODDirty_Implementation(int32 LODIndex)
{
	FScopeLock Lock(&SyncRoot);

	check(LODs.IsValidIndex(LODIndex));

	// Flag for update
	SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(INDEX_NONE) = ESectionUpdateType::Mesh;

	QueueForMeshUpdate();
}

void URuntimeMesh::MarkAllLODsDirty_Implementation()
{
	FScopeLock Lock(&SyncRoot);

	// Flag for update
	for (int32 LODIdx = 0; LODIdx < LODs.Num(); LODIdx++)
	{
		SectionsToUpdate.FindOrAdd(LODIdx).FindOrAdd(INDEX_NONE) = ESectionUpdateType::Mesh;
	}

	QueueForMeshUpdate();
}


void URuntimeMesh::CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	check(IsInGameThread());
	check(LODs.IsValidIndex(LODIndex));
	check(SectionId >= 0);

	FScopeLock Lock(&SyncRoot);

	LODs[LODIndex].Sections.FindOrAdd(SectionId) = SectionProperties;

	// Flag for update
	SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId) = ESectionUpdateType::AllData;
	QueueForMeshUpdate();
}

void URuntimeMesh::SetSectionVisibility_Implementation(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
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

void URuntimeMesh::SetSectionCastsShadow_Implementation(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
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

void URuntimeMesh::MarkSectionDirty_Implementation(int32 LODIndex, int32 SectionId)
{
	check(LODs.IsValidIndex(LODIndex));
	check(LODs[LODIndex].Sections.Contains(SectionId));

	FScopeLock Lock(&SyncRoot);

	// Flag for update
	ESectionUpdateType& UpdateType = SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId);
	UpdateType |= ESectionUpdateType::Mesh;
	UpdateType = (UpdateType & ~ESectionUpdateType::ClearOrRemove);
	QueueForMeshUpdate();
}

void URuntimeMesh::ClearSection_Implementation(int32 LODIndex, int32 SectionId)
{

}

void URuntimeMesh::RemoveSection_Implementation(int32 LODIndex, int32 SectionId)
{
	check(LODs.IsValidIndex(LODIndex));
	check(LODs[LODIndex].Sections.Contains(SectionId));

	FScopeLock Lock(&SyncRoot);

	LODs[LODIndex].Sections.Remove(SectionId);

	if (RenderProxy.IsValid())
	{
		SectionsToUpdate.FindOrAdd(LODIndex).FindOrAdd(SectionId) = ESectionUpdateType::Remove;
		QueueForMeshUpdate();
		RecreateAllComponentSceneProxies();
	}
}

void URuntimeMesh::MarkCollisionDirty_Implementation()
{
	QueueForCollisionUpdate();
}


void URuntimeMesh::SetupMaterialSlot_Implementation(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
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

int32 URuntimeMesh::GetMaterialIndex_Implementation(FName MaterialSlotName)
{
	FScopeLock Lock(&SyncRoot);
	const int32* SlotIndex = SlotNameLookup.Find(MaterialSlotName);
	return SlotIndex ? *SlotIndex : INDEX_NONE;
}

bool URuntimeMesh::IsMaterialSlotNameValid_Implementation(FName MaterialSlotName) const
{
	FScopeLock Lock(&SyncRoot);
	return SlotNameLookup.Contains(MaterialSlotName);
}

FRuntimeMeshMaterialSlot URuntimeMesh::GetMaterialSlot_Implementation(int32 SlotIndex)
{
	FScopeLock Lock(&SyncRoot);
	return MaterialSlots[SlotIndex];
}

int32 URuntimeMesh::GetNumMaterials_Implementation()
{
	FScopeLock Lock(&SyncRoot);
	return MaterialSlots.Num();
}

TArray<FName> URuntimeMesh::GetMaterialSlotNames_Implementation()
{
	FScopeLock Lock(&SyncRoot);
	TArray<FName> OutNames;
	SlotNameLookup.GetKeys(OutNames);
	return OutNames;
}

TArray<FRuntimeMeshMaterialSlot> URuntimeMesh::GetMaterialSlots_Implementation()
{
	FScopeLock Lock(&SyncRoot);
	return MaterialSlots;
}

UMaterialInterface* URuntimeMesh::GetMaterial_Implementation(int32 SlotIndex)
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
	Super::BeginDestroy();

	ReferenceAnchor.BeginDestroy();
}

bool URuntimeMesh::IsReadyForFinishDestroy()
{
	return Super::IsReadyForFinishDestroy() && ReferenceAnchor.IsFree();
}

void URuntimeMesh::PostLoad()
{
	Super::PostLoad();

	QueueForDelayedInitialize();
	QueueForCollisionUpdate();
}



bool URuntimeMesh::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetPhysicsTriMesh);

	if (MeshProvider && !FUObjectThreadContext::Get().IsRoutingPostLoad)
	{
		FRuntimeMeshCollisionData CollisionMesh;

		if (MeshProvider->GetCollisionMesh(CollisionMesh))
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
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_HasPhysicsTriMesh);

	if (MeshProvider && !FUObjectThreadContext::Get().IsRoutingPostLoad)
	{
		return MeshProvider->HasCollisionMesh();
	}

	return false;
}

bool URuntimeMesh::WantsNegXTriMesh()
{
	return false;
}

void URuntimeMesh::GetMeshId(FString& OutMeshId)
{

}


void URuntimeMesh::InitializeInternal()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_Initialize);

	// Create the render proxy
	ERHIFeatureLevel::Type FeatureLevel;
	if (GetSceneFeatureLevel(FeatureLevel))
	{
		GetRenderProxy(FeatureLevel);
	}

	MeshProvider->BindTargetProvider(this);
	MeshProvider->Initialize();

}





void URuntimeMesh::QueueForDelayedInitialize()
{
	FRuntimeMeshMisc::DoOnGameThread([this]()
		{
			if (this->IsValidLowLevel())
			{
				bNeedsInitialization = true;

				GetEngineSubsystem()->QueueMeshForUpdate(GetMeshReference());
			}
		});
}

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
				PinnedRef->HandleUpdate();
			}
		}
	}

	FORCEINLINE TStatId GetStatId() const
	{
		return TStatId();
		// RETURN_QUICK_DECLARE_CYCLE_STAT(ExampleAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};

void URuntimeMesh::QueueForUpdate()
{
	FRuntimeMeshMisc::DoOnGameThread([this]()
		{
			if (this->IsValidLowLevel())
			{
				bCollisionIsDirty = true;
				GetEngineSubsystem()->QueueMeshForUpdate(GetMeshReference());
			}
		});
}

void URuntimeMesh::QueueForMeshUpdate()
{
	FRuntimeMeshMisc::DoOnGameThread([this]()
		{
			if (this->IsValidLowLevel())
			{
				if (bQueuedForMeshUpdate.AtomicSet(true) == false)
				{
					if (MeshProvider->IsThreadSafe())
					{
						(new FAutoDeleteAsyncTask<FRuntimeMeshUpdateTask>(GetMeshReference()))->StartBackgroundTask(GetEngineSubsystem()->GetThreadPool());
					}
					else
					{
						GetEngineSubsystem()->QueueMeshForUpdate(GetMeshReference());
					}
				}
			}
		});
}

void URuntimeMesh::QueueForCollisionUpdate()
{
	FRuntimeMeshMisc::DoOnGameThread([this]()
		{
			if (this->IsValidLowLevel())
			{
				bCollisionIsDirty = true;
				GetEngineSubsystem()->QueueMeshForUpdate(GetMeshReference());
			}
		});
}


void URuntimeMesh::UpdateAllComponentBounds()
{
	FRuntimeMeshMisc::DoOnGameThread([this]()
		{
			DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
				{
					Mesh->NewBoundsReceived();
				});
		});
}

void URuntimeMesh::RecreateAllComponentSceneProxies()
{
	FRuntimeMeshMisc::DoOnGameThread([this]()
		{
			DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
				{
					Mesh->ForceProxyRecreate();
				});
		});
}


void URuntimeMesh::HandleUpdate()
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

	UpdateAllComponentBounds();
	if (bRequiresProxyRecreate)
	{
		RecreateAllComponentSceneProxies();
	}
}


void URuntimeMesh::HandleFullLODUpdate(int32 LODId, bool& bRequiresProxyRecreate)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMD(%d): HandleFullLODUpdate Called"), FPlatformTLS::GetCurrentThreadId());

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
	bool bResult = MeshProvider->GetAllSectionsMeshForLOD(LODId, MeshDatas);

	// Update all the sections or create new ones
	for (auto& Entry : MeshDatas)
	{
		FRuntimeMeshSectionData& Section = Entry.Value;

		if (bResult && Section.MeshData.HasValidMeshData())
		{
			LOD.Sections.FindOrAdd(Entry.Key) = Section.Properties;

			RenderProxy->CreateOrUpdateSection_GameThread(LODId, Entry.Key, Section.Properties, true);
			RenderProxy->UpdateSectionMesh_GameThread(LODId, Entry.Key, MakeShared<FRuntimeMeshRenderableMeshData>(MoveTemp(Section.MeshData)));
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

void URuntimeMesh::HandleSingleSectionUpdate(int32 LODId, int32 SectionId, bool& bRequiresProxyRecreate)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMD(%d): HandleSingleSectionUpdate Called"), FPlatformTLS::GetCurrentThreadId());

	FRuntimeMeshSectionProperties Properties = LODs[LODId].Sections.FindChecked(SectionId);
	TSharedPtr<FRuntimeMeshRenderableMeshData> MeshData = MakeShared<FRuntimeMeshRenderableMeshData>(
		Properties.bUseHighPrecisionTangents,
		Properties.bUseHighPrecisionTexCoords,
		Properties.NumTexCoords,
		Properties.bWants32BitIndices);
	bool bResult = MeshProvider->GetSectionMeshForLOD(LODId, SectionId, *MeshData);

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

	if (MeshProvider)
	{
		FRuntimeMeshCollisionSettings CollisionSettings = MeshProvider->GetCollisionSettings();

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
	LinkedComponents.AddUnique(NewComponent);

	if (BodySetup)
	{
		// Alert collision if we already have it
		NewComponent->NewCollisionMeshReceived();
	}
}

void URuntimeMesh::UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove)
{
	LinkedComponents.RemoveSingleSwap(ComponentToRemove, true);
}


FRuntimeMeshProxyPtr URuntimeMesh::GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel)
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





