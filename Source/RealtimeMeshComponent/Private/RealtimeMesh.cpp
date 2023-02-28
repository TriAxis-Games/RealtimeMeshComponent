// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMesh.h"
#include "RealtimeMeshComponent.h"
#include "Data/RealtimeMeshData.h"
#include "Data/RealtimeMeshLOD.h"
#include "Interface_CollisionDataProviderCore.h"
#include "PhysicsEngine/BodySetup.h"

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
{
}

void URealtimeMesh::BroadcastCollisionBodyUpdatedEvent(UBodySetup* NewBodySetup)
{
	CollisionBodyUpdatedEvent.Broadcast(this, NewBodySetup);
}

void URealtimeMesh::UnbindEvents()
{
	GetMesh()->OnBoundsChanged().RemoveAll(this);
	GetMesh()->OnRenderDataChanged().RemoveAll(this);
}

void URealtimeMesh::Reset(bool bCreateNewMeshData)
{
	UE_LOG(LogTemp, Warning, TEXT("RM Resetting... %s"), *GetName());
	if (!bCreateNewMeshData)
	{
		GetMesh()->Reset();
	}
	else
	{
		// TODO: pull the mesh data creation into the factory so we can recreate it here
	}

	BodySetup = nullptr;
	

	BroadcastBoundsChangedEvent();
	BroadcastRenderDataChangedEvent(true);
	BroadcastCollisionBodyUpdatedEvent(nullptr);
}

FBoxSphereBounds URealtimeMesh::GetLocalBounds() const
{
	return FBoxSphereBounds(GetMesh()->GetLocalBounds());
}



FRealtimeMeshLODKey URealtimeMesh::AddLOD(const FRealtimeMeshLODConfig& Config)
{
	return GetMesh()->AddLOD(Config);
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

FRealtimeMeshCollisionConfiguration URealtimeMesh::GetCollisionConfig() const
{
	return GetMesh()->GetCollisionConfig();
}

void URealtimeMesh::SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig)
{
	GetMesh()->SetCollisionConfig(InCollisionConfig);
}

FRealtimeMeshSimpleGeometry URealtimeMesh::GetSimpleGeometry() const
{
	return GetMesh()->GetSimpleGeometry();
}

void URealtimeMesh::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry)
{
	GetMesh()->SetSimpleGeometry(InSimpleGeometry);
}


void URealtimeMesh::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
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
	const int32* SlotIndex = SlotNameLookup.Find(MaterialSlotName);
	return SlotIndex ? *SlotIndex : INDEX_NONE;
}

bool URealtimeMesh::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	return SlotNameLookup.Contains(MaterialSlotName);
}

FRealtimeMeshMaterialSlot URealtimeMesh::GetMaterialSlot(int32 SlotIndex) const
{
	return MaterialSlots[SlotIndex];
}

int32 URealtimeMesh::GetNumMaterials() const
{
	return MaterialSlots.Num();
}

TArray<FName> URealtimeMesh::GetMaterialSlotNames() const
{
	TArray<FName> OutNames;
	SlotNameLookup.GetKeys(OutNames);
	return OutNames;
}

TArray<FRealtimeMeshMaterialSlot> URealtimeMesh::GetMaterialSlots() const
{
	return MaterialSlots;
}

UMaterialInterface* URealtimeMesh::GetMaterial(int32 SlotIndex) const
{
	if (MaterialSlots.IsValidIndex(SlotIndex))
	{
		return MaterialSlots[SlotIndex].Material;
	}
	return nullptr;
}

void URealtimeMesh::PostInitProperties()
{
	UObject::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		GetMesh()->OnBoundsChanged().AddUObject(this, &URealtimeMesh::HandleBoundsUpdated);
		GetMesh()->OnRenderDataChanged().AddUObject(this, &URealtimeMesh::HandleMeshRenderingDataChanged);

		GetMesh()->SetMeshName(this->GetFName());
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
	
		// Serialize the mesh data
		GetMesh()->Serialize(Ar);
	}
}

void URealtimeMesh::PostDuplicate(bool bDuplicateForPIE)
{
	UObject::PostDuplicate(bDuplicateForPIE);
}

bool URealtimeMesh::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	SCOPE_CYCLE_COUNTER(STAT_RealtimeMesh_GetPhysicsTriMesh);
	
	return GetMesh()->GetPhysicsTriMeshData(CollisionData, InUseAllTriData);
}

bool URealtimeMesh::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	SCOPE_CYCLE_COUNTER(STAT_RealtimeMesh_HasPhysicsTriMesh);

	return GetMesh()->ContainsPhysicsTriMeshData(InUseAllTriData);
}

void URealtimeMesh::Tick(float DeltaTime)
{
	if (GetMesh()->IsCollisionDirty())
	{
		UpdateCollision();
	}
}

ETickableTickType URealtimeMesh::GetTickableTickType() const
{
	return ETickableTickType::Conditional;
}

bool URealtimeMesh::IsTickable() const
{
	return IsAllowedToTick() && GetMesh()->IsCollisionDirty();
}

bool URealtimeMesh::IsAllowedToTick() const
{
	return !HasAllFlags(RF_ClassDefaultObject | RF_ArchetypeObject);
}

TStatId URealtimeMesh::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URealtimeMesh, STATGROUP_Tickables);
}

UBodySetup* URealtimeMesh::CreateNewBodySetup()
{
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();


	const FRealtimeMeshCollisionConfiguration CollisionConfig = GetMesh()->GetCollisionConfig();
	
	NewBodySetup->bGenerateMirroredCollision = false;
	NewBodySetup->bDoubleSidedGeometry = true;
	NewBodySetup->CollisionTraceFlag = CollisionConfig.bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;

	const FRealtimeMeshSimpleGeometry SimpleGeometry = GetMesh()->GetSimpleGeometry();
	SimpleGeometry.CopyToBodySetup(NewBodySetup);
	
	return NewBodySetup;
}

void URealtimeMesh::UpdateCollision(bool bForceCookNow)
{
	check(IsInGameThread());
	FRealtimeMeshCollisionConfiguration CollisionConfig = GetMesh()->GetCollisionConfig();
	
	const bool bShouldCookAsync = !bForceCookNow && (!GIsEditor || GIsPlayInEditorWorld) && CollisionConfig.bUseAsyncCook;

	if (bShouldCookAsync)
	{
		// Abort all previous ones still standing
		for (const auto& OldBody : AsyncBodySetupQueue)
		{
			OldBody->AbortPhysicsMeshAsyncCreation();
		}

		UBodySetup* NewBodySetup = CreateNewBodySetup();

		// Kick the cook off asynchronously
		NewBodySetup->CreatePhysicsMeshesAsync(
			FOnAsyncPhysicsCookFinished::CreateUObject(this, &URealtimeMesh::FinishPhysicsAsyncCook, NewBodySetup));

		// Copy source info and reset pending
		AsyncBodySetupQueue.Add(NewBodySetup);
	}
	else
	{
		AsyncBodySetupQueue.Empty();
		UBodySetup* NewBodySetup = CreateNewBodySetup();

		// Update meshes
		NewBodySetup->bHasCookedCollisionData = true;
		NewBodySetup->InvalidatePhysicsData();
		NewBodySetup->CreatePhysicsMeshes();

		// Copy source info and reset pending
		AsyncBodySetupQueue.Empty();

		BodySetup = NewBodySetup;
		BroadcastCollisionBodyUpdatedEvent(BodySetup);
	}

	GetMesh()->ClearCollisionDirtyFlag();
}

void URealtimeMesh::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
	check(IsInGameThread());
	
	const int32 FoundIdx = AsyncBodySetupQueue.IndexOfByKey(FinishedBodySetup);

	if (FoundIdx != INDEX_NONE)
	{
		if (bSuccess)
		{
			// The new body was found in the array meaning it's newer so use it
			BodySetup = FinishedBodySetup;

			// Remove all older bodies
			AsyncBodySetupQueue.RemoveAt(0, FoundIdx + 1);

			BroadcastCollisionBodyUpdatedEvent(BodySetup);
		}
		else
		{
			AsyncBodySetupQueue.RemoveAt(FoundIdx);
		}
	}
}


void URealtimeMesh::HandleBoundsUpdated(const RealtimeMesh::FRealtimeMeshRef& IncomingMesh)
{
	if (ensure(IncomingMesh == GetMesh()))
	{
		BroadcastBoundsChangedEvent();
	}
}

void URealtimeMesh::HandleMeshRenderingDataChanged(const RealtimeMesh::FRealtimeMeshRef& IncomingMesh, bool bShouldProxyRecreate)
{
	Modify(true);
	if (ensure(IncomingMesh == GetMesh()))
	{
		BroadcastRenderDataChangedEvent(bShouldProxyRecreate);
	}
}

#undef LOCTEXT_NAMESPACE