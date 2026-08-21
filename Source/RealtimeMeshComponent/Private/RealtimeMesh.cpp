// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMesh.h"
#include "RealtimeMeshComponent.h"
#include "Data/RealtimeMeshData.h"
#include "Data/RealtimeMeshLOD.h"
#include "Interface_CollisionDataProviderCore.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "Async/Async.h"
#include "Templates/Function.h"
#include "Misc/LazySingleton.h"
#include "PhysicsEngine/BodySetup.h"
#include "Logging/MessageLog.h"
#include "Engine/World.h"

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
	, CurrentCollisionVersion(0)
{

}

void URealtimeMesh::DispatchToGameThread(TUniqueFunction<void(URealtimeMesh*)>&& Func)
{
	if (IsInGameThread())
	{
		Func(this);
	}
	else
	{
		TWeakObjectPtr<URealtimeMesh> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, Func = MoveTemp(Func)]() mutable
		{
		if (const auto Mesh = WeakThis.Get())
		{
		Func(Mesh);
		}
		});
	}
}

void URealtimeMesh::BroadcastBoundsChangedEvent()
{
	DispatchToGameThread([](URealtimeMesh* Mesh)
	{
		Mesh->BoundsChangedEvent.Broadcast(Mesh);
	});
}

void URealtimeMesh::BroadcastRenderDataChangedEvent(bool bShouldRecreateProxies)
{
	DispatchToGameThread([bShouldRecreateProxies](URealtimeMesh* Mesh)
	{
		Mesh->RenderDataChangedEvent.Broadcast(Mesh, bShouldRecreateProxies);
	});
}

void URealtimeMesh::BroadcastCollisionBodyUpdatedEvent(UBodySetup* NewBodySetup)
{
	TWeakObjectPtr<UBodySetup> WeakBodySetup(NewBodySetup);
	DispatchToGameThread([WeakBodySetup](URealtimeMesh* Mesh)
	{
		Mesh->CollisionBodyUpdatedEvent.Broadcast(Mesh, WeakBodySetup.Get());
	});
}

void URealtimeMesh::Initialize(const TSharedRef<RealtimeMesh::FRealtimeMeshContext>& InContext,
                               const RealtimeMesh::FRealtimeMeshRef& InMesh)
{
	if (Context)
	{
		Context->OnRenderProxyRequiresUpdate().RemoveAll(this);
		Context->OnBoundsChanged().RemoveAll(this);
	}

	Context = InContext;

	Context->OnRenderProxyRequiresUpdate().AddUObject(this, &URealtimeMesh::HandleRenderProxyRequiresUpdate);
	Context->OnBoundsChanged().AddUObject(this, &URealtimeMesh::HandleBoundsUpdated);

	MeshRef = InMesh;
	Context->SetOwnerMesh(this, MeshRef.ToSharedRef());
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

			// Blend the triangle's UVs by the hit's barycentric coordinates.
			const FVector BaryCoords = FMath::ComputeBaryCentric2D(BodySpaceLocation, Pos0, Pos1, Pos2);
			UV = (BaryCoords.X * UV0) + (BaryCoords.Y * UV1) + (BaryCoords.Z * UV2);

			bSuccess = true;
		}
	}

	return bSuccess;
}

void URealtimeMesh::Reset()
{
	if (MeshRef.IsValid())
	{
		RealtimeMesh::FRealtimeMeshUpdateContext UpdateContext(GetMesh());
		GetMesh()->ResetInternal(UpdateContext, false);
	
		BroadcastBoundsChangedEvent();
		//BroadcastRenderDataChangedEvent(true);
		BroadcastCollisionBodyUpdatedEvent(nullptr);
	}
	
	MaterialSlots.Empty();
	SlotNameLookup.Empty();

	if (BodySetup)
	{
		BodySetup->InvalidatePhysicsData();
	}
	BodySetup = nullptr;
	UVData.Empty();
}

FBoxSphereBounds URealtimeMesh::GetLocalBounds() const
{
	RealtimeMesh::FRealtimeMeshAccessContext AccessContext(GetMesh());
	auto LocalBounds = GetMesh()->GetLocalBounds(AccessContext);
	return LocalBounds.IsSet()? FBoxSphereBounds(LocalBounds.GetValue()) : FBoxSphereBounds(FSphere(FVector::ZeroVector, 1));
}


FRealtimeMeshLODKey URealtimeMesh::AddLOD(const FRealtimeMeshLODConfig& Config)
{
	RealtimeMesh::FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	FRealtimeMeshLODKey LODKey;
	GetMesh()->AddLOD(UpdateContext, Config, &LODKey);
	return LODKey;
}

void URealtimeMesh::UpdateLODConfig(FRealtimeMeshLODKey LODKey, const FRealtimeMeshLODConfig& Config)
{
	RealtimeMesh::FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	if (const auto LOD = GetMesh()->GetLOD(UpdateContext, LODKey))
	{
		LOD->UpdateConfig(UpdateContext, Config);
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("UpdateLODConfig_InvalidLODKey", "UpdateLODConfig: Invalid LOD key {0}"), FText::FromString(LODKey.ToString())));
	}
}

void URealtimeMesh::RemoveTrailingLOD()
{
	RealtimeMesh::FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetMesh()->RemoveTrailingLOD(UpdateContext);
}


void URealtimeMesh::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	{
		RealtimeMesh::FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());

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
		else
		{
			// We're overwriting an existing slot; drop its old name from the lookup so it
			// doesn't keep resolving to this index after we rename the slot.
			SlotNameLookup.Remove(MaterialSlots[MaterialSlot].SlotName);
		}
		MaterialSlots[MaterialSlot] = FRealtimeMeshMaterialSlot(SlotName, InMaterial);
		SlotNameLookup.Add(SlotName, MaterialSlot);
	}

	// Broadcast outside the write guard so user handlers don't run under the mesh lock.
	BroadcastRenderDataChangedEvent(true);
}

void URealtimeMesh::SetNumMaterialSlots(int32 NewNumSlots)
{
	NewNumSlots = FMath::Max(0, NewNumSlots);

	{
		RealtimeMesh::FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());

		if (NewNumSlots == MaterialSlots.Num())
		{
			return;
		}

		// When shrinking, drop any name lookup entries that resolve to a trimmed index so
		// GetMaterialIndex/IsMaterialSlotNameValid can't keep pointing past the end of the array.
		if (NewNumSlots < MaterialSlots.Num())
		{
			TArray<FName> NamesToRemove;
			for (const TPair<FName, int32>& Pair : SlotNameLookup)
			{
				if (Pair.Value >= NewNumSlots)
				{
					NamesToRemove.Add(Pair.Key);
				}
			}
			for (const FName& Name : NamesToRemove)
			{
				SlotNameLookup.Remove(Name);
			}
		}

		// Trims trailing slots when shrinking; grows with empty/None slots when growing.
		MaterialSlots.SetNum(NewNumSlots);
	}

	// Broadcast outside the write guard so user handlers don't run under the mesh lock.
	BroadcastRenderDataChangedEvent(true);
}

int32 URealtimeMesh::GetMaterialIndex(FName MaterialSlotName) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
	
	const int32* SlotIndex = SlotNameLookup.Find(MaterialSlotName);
	return SlotIndex ? *SlotIndex : INDEX_NONE;
}

FName URealtimeMesh::GetMaterialSlotName(int32 Index) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());

	if (MaterialSlots.IsValidIndex(Index))
	{
		return MaterialSlots[Index].SlotName;
	}
	return NAME_None;	
}

bool URealtimeMesh::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
	return SlotNameLookup.Contains(MaterialSlotName);
}

FRealtimeMeshMaterialSlot URealtimeMesh::GetMaterialSlot(int32 SlotIndex) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
	if (!MaterialSlots.IsValidIndex(SlotIndex))
	{
		return FRealtimeMeshMaterialSlot();
	}
	return MaterialSlots[SlotIndex];
}

int32 URealtimeMesh::GetNumMaterials() const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
	return MaterialSlots.Num();
}

TArray<FName> URealtimeMesh::GetMaterialSlotNames() const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
	TArray<FName> OutNames;
	SlotNameLookup.GetKeys(OutNames);
	return OutNames;
}

TArray<FRealtimeMeshMaterialSlot> URealtimeMesh::GetMaterialSlots() const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
	return MaterialSlots;
}

UMaterialInterface* URealtimeMesh::GetMaterial(int32 SlotIndex) const
{
	RealtimeMesh::FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
	if (MaterialSlots.IsValidIndex(SlotIndex))
	{
		return MaterialSlots[SlotIndex].Material;
	}
	return nullptr;
}

void URealtimeMesh::PostInitProperties()
{
	UObject::PostInitProperties();

	if (!IsTemplate() && Context)
	{
		Context->SetMeshName(this->GetFName());
	}
}

void URealtimeMesh::BeginDestroy()
{

	if (Context)
	{
		Context->OnRenderProxyRequiresUpdate().RemoveAll(this);
		Context->OnBoundsChanged().RemoveAll(this);
	}

	Reset();

	Super::BeginDestroy();
}

void URealtimeMesh::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (!IsTemplate())
	{
		Ar.UsingCustomVersion(RealtimeMesh::FRealtimeMeshVersion::GUID);

		// Subclasses (URealtimeMeshManaged in particular) may want to gate this
		// on a "serialize my data?" flag; the base path always serializes.
		GetMesh()->Serialize(Ar, this);
	}
}

ERealtimeMeshCollisionUpdateResult URealtimeMesh::ApplyCollisionUpdate(FRealtimeMeshCollisionInfo&& InCollisionData, int32 NewCollisionKey)
{
	if (NewCollisionKey > CurrentCollisionVersion)
	{
		UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
		NewBodySetup->BodySetupGuid = FGuid::NewGuid();
		NewBodySetup->bGenerateMirroredCollision = false;
		NewBodySetup->bDoubleSidedGeometry = true;
		NewBodySetup->bCreatedPhysicsMeshes = true;
		NewBodySetup->bSupportUVsAndFaceRemap = true;
		NewBodySetup->CollisionTraceFlag = InCollisionData.Configuration.bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;

		if (NewBodySetup->CollisionTraceFlag != CTF_UseComplexAsSimple)
		{
			URealtimeMeshCollisionTools::CopySimpleGeometryToBodySetup(InCollisionData.SimpleGeometry, NewBodySetup);
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
			URealtimeMeshCollisionTools::CopyComplexGeometryToBodySetup(InCollisionData.ComplexGeometry, NewBodySetup, NewUVData);
			for (auto& Mesh : NewBodySetup->TriMeshGeometries)
			{
				Mesh->SetDoCollide(false);
#if TRACK_CHAOS_GEOMETRY
				Mesh->Track(Chaos::MakeSerializable(Mesh), "Realtime Mesh");
#endif
			}
		}

		BodySetup = NewBodySetup;
		UVData = MoveTemp(NewUVData);
		CurrentCollisionVersion = NewCollisionKey;
			
		BroadcastCollisionBodyUpdatedEvent(NewBodySetup);

		return ERealtimeMeshCollisionUpdateResult::Updated;
	}
	return ERealtimeMeshCollisionUpdateResult::Ignored;
}

void URealtimeMesh::HandleBoundsUpdated()
{
	BroadcastBoundsChangedEvent();
}

void URealtimeMesh::HandleRenderProxyRequiresUpdate()
{
	// Only dirty the package / snapshot into the transaction buffer for editor meshes whose
	// data is actually serialized. Procedural meshes updated every editor tick (or any mesh
	// running in a PIE/game world) would otherwise pay a per-frame Modify() -> package dirty
	// plus a full transaction snapshot on every render-data change. HasAnyFlags(RF_Transactional)
	// gates out transient/non-serialized meshes; the world-type check gates out PIE/game.
	bool bShouldModify = HasAnyFlags(RF_Transactional) && GetPackage() != GetTransientPackage();
	if (bShouldModify)
	{
		if (const UWorld* World = GetWorld())
		{
			bShouldModify = World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview;
		}
	}

	if (bShouldModify)
	{
		Modify();
	}

	BroadcastRenderDataChangedEvent(true);
}


#undef LOCTEXT_NAMESPACE
