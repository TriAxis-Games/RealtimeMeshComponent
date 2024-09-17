// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMesh.h"
#include "RealtimeMeshComponent.h"
#include "Data/RealtimeMeshData.h"
#include "Data/RealtimeMeshLOD.h"
#include "Interface_CollisionDataProviderCore.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Core/RealtimeMeshFuture.h"
#include "Misc/LazySingleton.h"
#include "PhysicsEngine/BodySetup.h"
#include "Logging/MessageLog.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Engine/World.h"
#endif

#define LOCTEXT_NAMESPACE "RealtimeMesh"

DECLARE_DWORD_COUNTER_STAT(TEXT("RealtimeMeshDelayedActions - Updated Actors"), STAT_RealtimeMeshDelayedActions_UpdatedActors, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshDelayedActions - Tick"), STAT_RealtimeMeshDelayedActions_Tick, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshDelayedActions - Initialize"), STAT_RealtimeMesh_Initialize, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshDelayedActions - Get Physics TriMesh"), STAT_RealtimeMesh_GetPhysicsTriMesh, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshDelayedActions - Has Physics TriMesh"), STAT_RealtimeMesh_HasPhysicsTriMesh, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshDelayedActions - Update Collision"), STAT_RealtimeMesh_UpdateCollision, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshDelayedActions - Finish Collision Async Cook"), STAT_RealtimeMesh_FinishCollisionAsyncCook, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshDelayedActions - Finalize Collision Cooked Data"), STAT_RealtimeMesh_FinalizeCollisionCookedData, STATGROUP_RealtimeMesh);


//////////////////////////////////////////////////////////////////////////
//	URealtimeMesh

URealtimeMesh::URealtimeMesh(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer)
	, BodySetup(nullptr)
	, CollisionUpdateVersionCounter(0)
	, CurrentCollisionVersion(0)
	, bShouldSerializeMeshData(true)
{

}

void URealtimeMesh::BroadcastBoundsChangedEvent()
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<URealtimeMesh> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis]()
		{
			if (const auto Mesh = WeakThis.Get())
			{
				Mesh->BoundsChangedEvent.Broadcast(Mesh);
			}
		});
	}
	else
	{
		BoundsChangedEvent.Broadcast(this);
	}
}

void URealtimeMesh::BroadcastRenderDataChangedEvent(bool bShouldRecreateProxies, int32 CommandsVersion)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<URealtimeMesh> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, bShouldRecreateProxies, CommandsVersion]()
		{
			if (const auto Mesh = WeakThis.Get())
			{
				Mesh->RenderDataChangedEvent.Broadcast(Mesh, bShouldRecreateProxies);
			}
		});
	}
	else
	{
		RenderDataChangedEvent.Broadcast(this, bShouldRecreateProxies);
	}
}

void URealtimeMesh::BroadcastCollisionBodyUpdatedEvent(UBodySetup* NewBodySetup)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<URealtimeMesh> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, NewBodySetup]()
		{
			if (const auto Mesh = WeakThis.Get())
			{
				Mesh->CollisionBodyUpdatedEvent.Broadcast(Mesh, NewBodySetup);
			}
		});
	}
	else
	{
		CollisionBodyUpdatedEvent.Broadcast(this, NewBodySetup);
	}
}

void URealtimeMesh::Initialize(const TSharedRef<RealtimeMesh::FRealtimeMeshSharedResources>& InSharedResources)
{
	if (SharedResources)
	{
		SharedResources->OnMeshBoundsChanged().RemoveAll(this);
		SharedResources->OnMeshRenderDataChanged().RemoveAll(this);
	}

	SharedResources = InSharedResources;

	SharedResources->OnMeshBoundsChanged().AddUObject(this, &URealtimeMesh::HandleBoundsUpdated);
	SharedResources->OnMeshRenderDataChanged().AddUObject(this, &URealtimeMesh::HandleMeshRenderingDataChanged);

	SharedResources->GetEndOfFrameRequestHandler() = RealtimeMesh::FRealtimeMeshRequestEndOfFrameUpdateDelegate::CreateUObject(this, &URealtimeMesh::MarkForEndOfFrameUpdate);
	SharedResources->GetCollisionUpdateHandler() = RealtimeMesh::FRealtimeMeshCollisionUpdateDelegate::CreateUObject(this, &URealtimeMesh::InitiateCollisionUpdate);

	MeshRef = SharedResources->CreateRealtimeMesh();
	SharedResources->SetOwnerMesh(this, MeshRef.ToSharedRef());
}

bool URealtimeMesh::CalcTexCoordAtLocation(const FVector& BodySpaceLocation, int32 ElementIndex, int32 FaceIndex, int32 UVChannel, FVector2D& UV) const
{
	bool bSuccess = false;

	if (UVData.IsValidIndex(ElementIndex))
	{
		const auto& UVInfo = UVData[ElementIndex];
		
		if (UVInfo.TexCoords.IsValidIndex(UVChannel) && UVInfo.Triangles.IsValidIndex(FaceIndex))
		{
			const int32 Index0 = UVInfo.Triangles[FaceIndex].V0;
			const int32 Index1 = UVInfo.Triangles[FaceIndex].V1;
			const int32 Index2 = UVInfo.Triangles[FaceIndex].V2;

			const FVector Pos0 = FVector(UVInfo.Positions[Index0]);
			const FVector Pos1 = FVector(UVInfo.Positions[Index1]);
			const FVector Pos2 = FVector(UVInfo.Positions[Index2]);

			FVector2D UV0 = FVector2D(UVInfo.TexCoords[UVChannel][Index0]);
			FVector2D UV1 = FVector2D(UVInfo.TexCoords[UVChannel][Index1]);
			FVector2D UV2 = FVector2D(UVInfo.TexCoords[UVChannel][Index2]);

			// Transform hit location from world to local space.
			// Find barycentric coords
			const FVector BaryCoords = FMath::ComputeBaryCentric2D(BodySpaceLocation, Pos0, Pos1, Pos2);
			// Use to blend UVs
			UV = (BaryCoords.X * UV0) + (BaryCoords.Y * UV1) + (BaryCoords.Z * UV2);

			bSuccess = true;
		}
	}

	return bSuccess;
}

void URealtimeMesh::Reset(bool bCreateNewMeshData)
{
	RealtimeMesh::FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
	
	UE_LOG(LogTemp, Warning, TEXT("RM Resetting... %s"), *GetName());
	if (!bCreateNewMeshData)
	{
		GetMesh()->Reset();
	}
	else
	{
		Initialize(SharedResources->CreateSharedResources());
	}

	MaterialSlots.Empty();
	SlotNameLookup.Empty();
	
	BodySetup = nullptr;
	UVData.Empty();

	BroadcastBoundsChangedEvent();
	//BroadcastRenderDataChangedEvent(true);
	BroadcastCollisionBodyUpdatedEvent(nullptr);
}

FBoxSphereBounds URealtimeMesh::GetLocalBounds() const
{
	return FBoxSphereBounds(GetMesh()->GetLocalBounds());
}


FRealtimeMeshLODKey URealtimeMesh::AddLOD(const FRealtimeMeshLODConfig& Config)
{
	FRealtimeMeshLODKey LODKey;
	GetMesh()->AddLOD(Config, &LODKey);
	return LODKey;
}

void URealtimeMesh::UpdateLODConfig(FRealtimeMeshLODKey LODKey, const FRealtimeMeshLODConfig& Config)
{
	if (const auto LOD = GetMesh()->GetLOD(LODKey))
	{
		LOD->UpdateConfig(Config);
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("UpdateLODConfig_InvalidLODKey", "UpdateLODConfig: Invalid LOD key {0}"), FText::FromString(LODKey.ToString())));
	}
}

void URealtimeMesh::RemoveTrailingLOD()
{
	GetMesh()->RemoveTrailingLOD();
}


void URealtimeMesh::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	RealtimeMesh::FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
	
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
	MaterialSlots[MaterialSlot] = FRealtimeMeshMaterialSlot(SlotName, InMaterial);
	SlotNameLookup.Add(SlotName, MaterialSlots.Num() - 1);

	BroadcastRenderDataChangedEvent(true);
}

int32 URealtimeMesh::GetMaterialIndex(FName MaterialSlotName) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
	
	const int32* SlotIndex = SlotNameLookup.Find(MaterialSlotName);
	return SlotIndex ? *SlotIndex : INDEX_NONE;
}

FName URealtimeMesh::GetMaterialSlotName(int32 Index) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

	if (MaterialSlots.IsValidIndex(Index))
	{
		return MaterialSlots[Index].SlotName;
	}
	return NAME_None;	
}

bool URealtimeMesh::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
	return SlotNameLookup.Contains(MaterialSlotName);
}

FRealtimeMeshMaterialSlot URealtimeMesh::GetMaterialSlot(int32 SlotIndex) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
	return MaterialSlots[SlotIndex];
}

int32 URealtimeMesh::GetNumMaterials() const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
	return MaterialSlots.Num();
}

TArray<FName> URealtimeMesh::GetMaterialSlotNames() const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
	TArray<FName> OutNames;
	SlotNameLookup.GetKeys(OutNames);
	return OutNames;
}

TArray<FRealtimeMeshMaterialSlot> URealtimeMesh::GetMaterialSlots() const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
	return MaterialSlots;
}

UMaterialInterface* URealtimeMesh::GetMaterial(int32 SlotIndex) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
	if (MaterialSlots.IsValidIndex(SlotIndex))
	{
		return MaterialSlots[SlotIndex].Material;
	}
	return nullptr;
}

UWorld* URealtimeMesh::GetWorld() const
{
	return Super::GetWorld();
}

void URealtimeMesh::PostInitProperties()
{
	UObject::PostInitProperties();

	if (!IsTemplate() && SharedResources)
	{
		SharedResources->SetMeshName(this->GetFName());
	}
}

void URealtimeMesh::BeginDestroy()
{
	Super::BeginDestroy();
}

void URealtimeMesh::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (!IsTemplate())
	{
		Ar.UsingCustomVersion(RealtimeMesh::FRealtimeMeshVersion::GUID);

		bool bShouldSerializeData = bShouldSerializeMeshData;
		// Supporting optional serialization from editor.
		if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::SupportOptionalDataSerialization)
		{
			Ar << bShouldSerializeData;
		}
		else
		{
			bShouldSerializeData = true;
		}

		if (bShouldSerializeData)
		{
			// Serialize the mesh data
			GetMesh()->Serialize(Ar, this);
		}
	}
}

void URealtimeMesh::PostDuplicate(bool bDuplicateForPIE)
{
	UObject::PostDuplicate(bDuplicateForPIE);
}

void URealtimeMesh::InitiateCollisionUpdate(const TSharedRef<TPromise<ERealtimeMeshCollisionUpdateResult>>& Promise, const TSharedRef<FRealtimeMeshCollisionInfo>& CollisionUpdate,
                                            bool bForceSyncUpdate)
{
	check(IsInGameThread());
	check(SharedResources && MeshRef);

	RealtimeMesh::FRealtimeMeshScopeGuardWrite Guard(SharedResources->GetGuard());

	const int32 UpdateKey = CollisionUpdateVersionCounter++;

	// TODO: We can skip cook based on simpleascomplex or complexassimple
	TArray<int32> MeshesNeedingCook = CollisionUpdate->ComplexGeometry.GetMeshIDsNeedingCook();
	TArray<int32> ConvexObjectsNeedingCook = CollisionUpdate->SimpleGeometry.GetMeshIDsNeedingCook();
	const bool bNeedsCookAnything = MeshesNeedingCook.Num() > 0 || ConvexObjectsNeedingCook.Num() > 0;
	const bool bShouldAsyncCook = !bForceSyncUpdate && GetWorld() && GetWorld()->IsGameWorld() && CollisionUpdate->Configuration.bUseAsyncCook;

	struct FCollisionUpdateState
	{
		TSharedRef<TPromise<ERealtimeMeshCollisionUpdateResult>> Promise;
		TSharedRef<FRealtimeMeshCollisionInfo> CollisionInfo;
		int32 UpdateKey;
		bool bForceSyncUpdate;

		FCollisionUpdateState(const TSharedRef<TPromise<ERealtimeMeshCollisionUpdateResult>>& InPromise, const TSharedRef<FRealtimeMeshCollisionInfo>& InCollisionInfo, int32 InUpdateKey, bool bInForceSyncUpdate)
			: Promise(InPromise)
			, CollisionInfo(InCollisionInfo)
			, UpdateKey(InUpdateKey)
			, bForceSyncUpdate(bInForceSyncUpdate)
		{
		}
	};

	TSharedRef<FCollisionUpdateState> UpdateState = MakeShared<FCollisionUpdateState>(Promise, CollisionUpdate, UpdateKey, bForceSyncUpdate);

	TUniqueFunction<TSharedRef<FCollisionUpdateState>()> CookFunction =
		[UpdateState, MeshesNeedingCook = MoveTemp(MeshesNeedingCook), ConvexObjectsNeedingCook = MoveTemp(ConvexObjectsNeedingCook)]() 
		{			
			ParallelForTemplate(MeshesNeedingCook.Num() + ConvexObjectsNeedingCook.Num(), [UpdateState, &MeshesNeedingCook, &ConvexObjectsNeedingCook](int32 Index)
			{
				if (Index < MeshesNeedingCook.Num())
				{
					URealtimeMeshCollisionTools::CookComplexMesh(UpdateState->CollisionInfo->ComplexGeometry.GetByIndex(Index));
				}
				else
				{
					URealtimeMeshCollisionTools::CookConvexHull(UpdateState->CollisionInfo->SimpleGeometry.ConvexHulls.GetByIndex(Index - MeshesNeedingCook.Num()));
				}
			});			
					
			return UpdateState;
		};

	TUniqueFunction<ERealtimeMeshCollisionUpdateResult(const TSharedRef<FCollisionUpdateState>&)> ApplyCollisionFunction =
		[ThisWeak = TWeakObjectPtr<URealtimeMesh>(this)](const TSharedRef<FCollisionUpdateState>& CollisionState) -> ERealtimeMeshCollisionUpdateResult
		{
			check(IsInGameThread());

			if (!ThisWeak.IsValid() || ThisWeak->CurrentCollisionVersion > CollisionState->UpdateKey)
			{
				return ERealtimeMeshCollisionUpdateResult::Ignored;
			}

			URealtimeMesh* ThisPtr = ThisWeak.Get();
			check(ThisPtr);
				

			UBodySetup* NewBodySetup = NewObject<UBodySetup>(ThisPtr, NAME_None, (ThisPtr->IsTemplate() ? RF_Public : RF_NoFlags));
			NewBodySetup->BodySetupGuid = FGuid::NewGuid();
			NewBodySetup->bGenerateMirroredCollision = false;
			NewBodySetup->bDoubleSidedGeometry = true;
			NewBodySetup->bCreatedPhysicsMeshes = true;
			NewBodySetup->bSupportUVsAndFaceRemap = true;
			NewBodySetup->CollisionTraceFlag = CollisionState->CollisionInfo->Configuration.bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;

			if (NewBodySetup->CollisionTraceFlag != CTF_UseComplexAsSimple)
			{
				URealtimeMeshCollisionTools::CopySimpleGeometryToBodySetup(CollisionState->CollisionInfo->SimpleGeometry, NewBodySetup);
				for (auto& Convex : NewBodySetup->AggGeom.ConvexElems)
				{
					Convex.GetChaosConvexMesh()->SetDoCollide(false);				
#if TRACK_CHAOS_GEOMETRY
					Convex.GetChaosConvexMesh()->Track(Chaos::MakeSerializable(Convex.GetChaosConvexMesh()), "Realtime Mesh");
#endif
				}
			}

			TArray<FRealtimeMeshCollisionMeshCookedUVData> NewUVData;
			if (NewBodySetup->CollisionTraceFlag != CTF_UseSimpleAsComplex)
			{
				URealtimeMeshCollisionTools::CopyComplexGeometryToBodySetup(CollisionState->CollisionInfo->ComplexGeometry, NewBodySetup, NewUVData);
#if RMC_ENGINE_ABOVE_5_4
				for (auto& Mesh : NewBodySetup->TriMeshGeometries)
#else
				for (auto& Mesh : NewBodySetup->ChaosTriMeshes)
#endif
				{
					Mesh->SetDoCollide(false);
#if TRACK_CHAOS_GEOMETRY
					Mesh->Track(Chaos::MakeSerializable(Mesh), "Realtime Mesh");
#endif
				}
			}

			ThisPtr->BodySetup = NewBodySetup;
			ThisPtr->UVData = MoveTemp(NewUVData);
			ThisPtr->CurrentCollisionVersion = CollisionState->UpdateKey;
			
			ThisPtr->BroadcastCollisionBodyUpdatedEvent(NewBodySetup);

			return ERealtimeMeshCollisionUpdateResult::Updated;
		};
	

	// Cook if we need to cook
	TFuture<TSharedRef<FCollisionUpdateState>> CookFuture = bNeedsCookAnything
		? bShouldAsyncCook
			? Async(EAsyncExecution::TaskGraph, MoveTemp(CookFunction))
			: MakeFulfilledPromise<TSharedRef<FCollisionUpdateState>>(CookFunction()).GetFuture()
		: MakeFulfilledPromise<TSharedRef<FCollisionUpdateState>>(UpdateState).GetFuture();

	// Finalize collision update
	CookFuture.Then([Promise, ApplyCollisionFunction = MoveTemp(ApplyCollisionFunction)](TFuture<TSharedRef<FCollisionUpdateState>>&& Future) mutable
	{
		AsyncTask(ENamedThreads::GameThread, [Promise, ApplyCollisionFunction = MoveTemp(ApplyCollisionFunction), CollisionState = Future.Get()]() mutable
		{
			Promise->EmplaceValue(ApplyCollisionFunction(CollisionState));
		});
	});
}

void URealtimeMesh::HandleBoundsUpdated()
{
	BroadcastBoundsChangedEvent();
}

void URealtimeMesh::HandleMeshRenderingDataChanged(bool bShouldRecreateProxies, int32 CommandsVersion)
{
	Modify(true);
	BroadcastRenderDataChangedEvent(bShouldRecreateProxies, CommandsVersion);
}


struct FRealtimeMeshEndOfFrameUpdateManager
{
private:
	FCriticalSection SyncRoot;
	TSet<TWeakObjectPtr<URealtimeMesh>> MeshesToUpdate;
	FDelegateHandle EndOfFrameUpdateHandle;

	void OnPreSendAllEndOfFrameUpdates(UWorld* World)
	{
		SyncRoot.Lock();
		auto MeshesCopy = MoveTemp(MeshesToUpdate);
		SyncRoot.Unlock();

		for (const auto& Mesh : MeshesCopy)
		{
			if (Mesh.IsValid())
			{
				Mesh->ProcessEndOfFrameUpdates();
			}
		}
	}

public:
	~FRealtimeMeshEndOfFrameUpdateManager()
	{
		if (EndOfFrameUpdateHandle.IsValid())
		{
			FWorldDelegates::OnWorldPostActorTick.Remove(EndOfFrameUpdateHandle);
			EndOfFrameUpdateHandle.Reset();
		}
	}

	void MarkComponentForUpdate(URealtimeMesh* RealtimeMesh)
	{
		FScopeLock Lock(&SyncRoot);
		if (!EndOfFrameUpdateHandle.IsValid())
		{
			
			// TODO: Moved this to post actor tick from OnWorldPreSendAlLEndOfFrameUpdates... Is this the best option?
			// Servers were not getting events but ever ~60 seconds
			EndOfFrameUpdateHandle = FWorldDelegates::OnWorldPostActorTick.AddLambda([this](UWorld* World, ELevelTick TickType, float DeltaSeconds) { OnPreSendAllEndOfFrameUpdates(World); });
		}
		MeshesToUpdate.Add(RealtimeMesh);
	}

	void ClearComponentForUpdate(URealtimeMesh* RealtimeMesh)
	{
		FScopeLock Lock(&SyncRoot);
		MeshesToUpdate.Remove(RealtimeMesh);
	}

	static FRealtimeMeshEndOfFrameUpdateManager& Get()
	{
		return TLazySingleton<FRealtimeMeshEndOfFrameUpdateManager>::Get();
	}
};


void URealtimeMesh::ProcessEndOfFrameUpdates()
{
	if (MeshRef)
	{
		(*MeshRef).ProcessEndOfFrameUpdates();
	}
}

void URealtimeMesh::MarkForEndOfFrameUpdate()
{
	FRealtimeMeshEndOfFrameUpdateManager::Get().MarkComponentForUpdate(this);
}


#undef LOCTEXT_NAMESPACE
