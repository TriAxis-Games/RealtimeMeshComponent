// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshComponent.h"

#include "GameDelegates.h"
#include "MaterialDomain.h"
#include "NaniteVertexFactory.h"
#include "RealtimeMeshComponentModule.h"
#include "RenderProxy/RealtimeMeshComponentProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMesh.h"
#include "NavigationSystem.h"
#include "RenderProxy/RealtimeMeshNaniteProxyInterface.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Net/UnrealNetwork.h"


DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponent - Collision Data Received"), STAT_RealtimeMeshComponent_NewCollisionMeshReceived, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponent - Create Scene Proxy"), STAT_RealtimeMeshComponent_CreateSceneProxy, STATGROUP_RealtimeMesh);

URealtimeMeshComponent::URealtimeMeshComponent()
{
	SetNetAddressable();
	SetIsReplicatedByDefault(true);
}

URealtimeMeshComponent::FRealtimeMeshComponentSyncCandidateEvent& URealtimeMeshComponent::OnMeshSyncCandidateChanged()
{
	static FRealtimeMeshComponentSyncCandidateEvent Event;
	return Event;
}

void URealtimeMeshComponent::SetReplicateMeshData(bool bNewReplicateMeshData)
{
	if (bReplicateMeshData != bNewReplicateMeshData)
	{
		bReplicateMeshData = bNewReplicateMeshData;
		if (IsRegistered())
		{
			OnMeshSyncCandidateChanged().Broadcast(this, bReplicateMeshData);
		}
	}
}

void URealtimeMeshComponent::SetRealtimeMesh(URealtimeMesh* NewMesh)
{
	// Bail if we're already assigned to this mesh
	if (IsValid(NewMesh) && IsValid(RealtimeMesh) && NewMesh == RealtimeMesh)
	{
		return;
	}

	bool bUpdatedMesh = false;
	// Unlink from any existing runtime mesh
	if (IsValid(RealtimeMesh))
	{
		UnbindFromEvents(RealtimeMesh);
		RealtimeMesh = nullptr;
		bUpdatedMesh = true;
	}

	if (IsValid(NewMesh))
	{
		RealtimeMesh = NewMesh;
		BindToEvents(RealtimeMesh);
		bUpdatedMesh = true;
	}

	if (bUpdatedMesh)
	{
		// The mesh (and thus its local bounds) changed; force CalcBounds to re-query on next use.
		bCachedLocalBoundsValid = false;
		UpdateBounds();
		UpdateCollision();
		MarkRenderStateDirty();

		// A mesh swap changes what (if anything) a sync layer should replicate for this
		// component; re-offer it so sessions rebind to the new mesh.
		if (IsRegistered())
		{
			OnMeshSyncCandidateChanged().Broadcast(this, true);
		}
	}
}

URealtimeMesh* URealtimeMeshComponent::InitializeRealtimeMesh(TSubclassOf<URealtimeMesh> MeshClass)
{
	URealtimeMesh* NewMesh = nullptr;
	if (MeshClass)
	{
		NewMesh = NewObject<URealtimeMesh>(this, MeshClass);
		if (!ensureMsgf(IsValid(NewMesh), TEXT("RealtimeMeshComponent: Failed to create mesh object from class %s"), 
			MeshClass ? *MeshClass->GetName() : TEXT("NULL")))
		{
			return nullptr;
		}
	}
	SetRealtimeMesh(NewMesh);
	if (!ensureMsgf(IsValid(NewMesh), TEXT("RealtimeMeshComponent: SetRealtimeMesh failed to set valid mesh")))
	{
		return nullptr;
	}
	return NewMesh;
}

void URealtimeMeshComponent::OnRep_RealtimeMesh(class URealtimeMesh *OldRealtimeMesh)
{
	if (RealtimeMesh != OldRealtimeMesh)
	{		
		// Properly handle replicated RealtimeMesh property change by putting the old value back
		// and applying the modification through a proper call to SetStaticMesh.
		URealtimeMesh* NewRealtimeMesh = RealtimeMesh;

		// Put back the old value with minimal logic involved
		RealtimeMesh = OldRealtimeMesh;

		// Go through all the logic required to properly apply a new realtime mesh.
		SetRealtimeMesh(NewRealtimeMesh);
	}
}

void URealtimeMeshComponent::OnRegister()
{
	Super::OnRegister();

	if (RealtimeMesh)
	{
		BindToEvents(RealtimeMesh);
		UpdateCollision();
	}

	OnMeshSyncCandidateChanged().Broadcast(this, true);
}

void URealtimeMeshComponent::OnUnregister()
{
	Super::OnUnregister();

	if (RealtimeMesh)
	{
		UnbindFromEvents(RealtimeMesh);
	}

	OnMeshSyncCandidateChanged().Broadcast(this, false);
}

FBoxSphereBounds URealtimeMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (URealtimeMesh* Mesh = GetRealtimeMesh())
	{
		// Only take the whole-mesh read lock (via GetLocalBounds) when the cache is stale. The cache is
		// invalidated on mesh swap and on OnBoundsChanged, so per-frame CalcBounds calls from moving/
		// physics components just re-transform the cached mesh-space bounds without contending the lock.
		if (!bCachedLocalBoundsValid)
		{
			CachedLocalBounds = FBoxSphereBounds(Mesh->GetLocalBounds());
			bCachedLocalBoundsValid = true;
		}
		return CachedLocalBounds.TransformBy(LocalToWorld);
	}

	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1));
}


FPrimitiveSceneProxy* URealtimeMeshComponent::CreateSceneProxy()
{
	SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponent_CreateSceneProxy);

	if (IsValid(RealtimeMesh))
	{		
		if (const auto MeshRenderProxy = RealtimeMesh->GetMesh()->GetRenderProxy(true))
		{
			// This is using the implementation in the RMC-Pro to support nanite, without that module present, the RMC doesn't support nanite.
			if (RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager::IsNaniteSupportAvailable())
			{
				RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager& NaniteModule = RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager::GetNaniteModule();

				if (MeshRenderProxy->HasNaniteResources_GT() && NaniteModule.ShouldUseNanite(this))
				{					
					return RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager::GetNaniteModule().CreateNewSceneProxy(this, MeshRenderProxy.ToSharedRef());
				}				
			}
			
			return new RealtimeMesh::FRealtimeMeshComponentSceneProxy(this, MeshRenderProxy.ToSharedRef());
		}
	}

	return nullptr;
}

UBodySetup* URealtimeMeshComponent::GetBodySetup()
{
	if (GetRealtimeMesh())
	{
		return GetRealtimeMesh()->GetBodySetup();
	}

	return nullptr;
}

int32 URealtimeMeshComponent::GetMaterialIndex(FName MaterialSlotName) const
{
	if (const URealtimeMesh* Mesh = GetRealtimeMesh())
	{
		return Mesh->GetMaterialIndex(MaterialSlotName);
	}
	return INDEX_NONE;
}

FName URealtimeMeshComponent::GetMaterialSlotName(uint32 Index) const
{
	if (const URealtimeMesh* Mesh = GetRealtimeMesh())
	{
		return Mesh->GetMaterialSlotName(Index);
	}
	return NAME_None;
}

TArray<FName> URealtimeMeshComponent::GetMaterialSlotNames() const
{
	if (const URealtimeMesh* Mesh = GetRealtimeMesh())
	{
		return Mesh->GetMaterialSlotNames();
	}
	return TArray<FName>();
}

bool URealtimeMeshComponent::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	if (const URealtimeMesh* Mesh = GetRealtimeMesh())
	{
		return Mesh->IsMaterialSlotNameValid(MaterialSlotName);
	}
	return false;
}

void URealtimeMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	TSet<UMaterialInterface*> InUseMats;

	const int32 NumMats = GetNumMaterials();
	for (int32 Index = 0; Index < NumMats; Index++)
	{
		if (UMaterialInterface* Mat = GetMaterial(Index))
		{
			InUseMats.Add(Mat);
		}
	}

	OutMaterials.Append(InUseMats.Array());
}

int32 URealtimeMeshComponent::GetNumMaterials() const
{
	const int32 NumOverrideMaterials = GetNumOverrideMaterials();
	const int32 NumMaterialSlots = GetRealtimeMesh() != nullptr ? GetRealtimeMesh()->GetNumMaterials() : 0;
	return FMath::Max(NumOverrideMaterials, NumMaterialSlots);
}

UMaterialInterface* URealtimeMeshComponent::GetMaterial(int32 ElementIndex) const
{
	UMaterialInterface* Mat = Super::GetMaterial(ElementIndex);

	// Return override material if it exists
	if (IsValid(Mat))
	{
		return Mat;
	}

	// fallback to RM sections material
	if (const URealtimeMesh* Mesh = GetRealtimeMesh())
	{
		return Mesh->GetMaterial(ElementIndex);
	}

	// Had no RM/Section return null
	return nullptr;
}

void URealtimeMeshComponent::CollectPSOPrecacheData(const FPSOPrecacheParams& BasePrecachePSOParams, FMaterialInterfacePSOPrecacheParamsList& OutParams)
{
	if (RealtimeMesh)
	{
		if (const auto MeshRenderProxy = RealtimeMesh->GetMesh()->GetRenderProxy(true))
		{
			// Collect the vertex-factory types this mesh can be drawn with so their PSOs precache
			// ahead of first draw instead of hitching when procedural content first appears.
			FPSOPrecacheVertexFactoryDataList VFDataList;

			if (RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager::IsNaniteSupportAvailable() && MeshRenderProxy->HasNaniteResources_GT())
			{
				VFDataList.Add(FPSOPrecacheVertexFactoryData(&FNaniteVertexFactory::StaticType));
			}
			else
			{
				VFDataList.Add(FPSOPrecacheVertexFactoryData(&RealtimeMesh::FRealtimeMeshLocalVertexFactory::StaticType));
			}

			const int32 NumMaterials = GetNumMaterials();
			for (int32 MaterialId = 0; MaterialId < NumMaterials; MaterialId++)
			{
				if (UMaterialInterface* MaterialInterface = GetMaterial(MaterialId))
				{
					FMaterialInterfacePSOPrecacheParams& ComponentParams = OutParams.AddDefaulted_GetRef();
					ComponentParams.Priority = EPSOPrecachePriority::Medium;
					ComponentParams.MaterialInterface = MaterialInterface;
					ComponentParams.VertexFactoryDataList = VFDataList;
					ComponentParams.PSOPrecacheParams = BasePrecachePSOParams;
				}
			}
		}
	}

	Super::CollectPSOPrecacheData(BasePrecachePSOParams, OutParams);
}

void URealtimeMeshComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(URealtimeMeshComponent, RealtimeMesh);
}

void URealtimeMeshComponent::BindToEvents(URealtimeMesh* InRealtimeMesh)
{
	// Guard against double-binding. If we're already bound (e.g. SetRealtimeMesh was called
	// while unregistered, then OnRegister runs BindToEvents again), unbind the existing
	// handles first so we don't leak a duplicate delegate set and fire each handler N times.
	if (BoundsChangedHandle.IsValid() || RenderDataChangedHandle.IsValid() || CollisionBodyUpdatedHandle.IsValid())
	{
		UnbindFromEvents(InRealtimeMesh);
	}

	BoundsChangedHandle = InRealtimeMesh->OnBoundsChanged().AddUObject(this, &URealtimeMeshComponent::HandleBoundsUpdated);
	RenderDataChangedHandle = InRealtimeMesh->OnRenderDataChanged().AddUObject(this, &URealtimeMeshComponent::HandleMeshRenderingDataChanged);
	CollisionBodyUpdatedHandle = InRealtimeMesh->OnCollisionBodyUpdated().AddUObject(this, &URealtimeMeshComponent::HandleCollisionBodyUpdated);
}

void URealtimeMeshComponent::UnbindFromEvents(URealtimeMesh* InRealtimeMesh)
{
	if (BoundsChangedHandle.IsValid())
	{
		InRealtimeMesh->OnBoundsChanged().Remove(BoundsChangedHandle);
		BoundsChangedHandle.Reset();
	}
	if (RenderDataChangedHandle.IsValid())
	{
		InRealtimeMesh->OnRenderDataChanged().Remove(RenderDataChangedHandle);
		RenderDataChangedHandle.Reset();
	}
	if (CollisionBodyUpdatedHandle.IsValid())
	{
		InRealtimeMesh->OnCollisionBodyUpdated().Remove(CollisionBodyUpdatedHandle);
		CollisionBodyUpdatedHandle.Reset();
	}
}


void URealtimeMeshComponent::HandleBoundsUpdated(URealtimeMesh* InRealtimeMesh)
{
	// The mesh's local bounds changed; drop the cache so the next CalcBounds re-queries them.
	bCachedLocalBoundsValid = false;
	UpdateBounds();
}

void URealtimeMeshComponent::HandleMeshRenderingDataChanged(URealtimeMesh* InRealtimeMesh, bool bShouldProxyRecreate)
{
	// A render-data change means the geometry — and therefore the computed local bounds — may have
	// changed. CalcBounds caches the mesh-space bounds and is otherwise only invalidated on mesh swap
	// and OnBoundsChanged, but the core update path never broadcasts OnBoundsChanged (only the Ext
	// Constructed provider does). Without refreshing here the scene proxy keeps the first-computed
	// bounds, so the mesh flickers as it is frustum-culled against a box that no longer matches the
	// geometry. Movement-only updates don't raise this event, so the cache still spares them the lock.
	bCachedLocalBoundsValid = false;
	UpdateBounds();

	if (bShouldProxyRecreate)
	{
		PrecachePSOs();
		MarkRenderStateDirty();
	}
}

void URealtimeMeshComponent::HandleCollisionBodyUpdated(URealtimeMesh* InRealtimeMesh, UBodySetup* BodySetup)
{
	UpdateCollision();
}

void URealtimeMeshComponent::UpdateCollision()
{
	if (KeepMomentumOnCollisionUpdate)
	{
		// RecreatePhysicsState resets velocity, so save and restore it to keep momentum.
		const FVector PrevLinearVelocity = GetPhysicsLinearVelocity();
		const FVector PrevAngularVelocity = GetPhysicsAngularVelocityInDegrees();

		RecreatePhysicsState();

		SetPhysicsLinearVelocity(PrevLinearVelocity, false);
		SetPhysicsAngularVelocityInDegrees(PrevAngularVelocity, false);
	}
	else
	{
		RecreatePhysicsState();
	}

	// Now update the navigation, unless it's been opted out of or this component can never affect
	// navigation anyway. This avoids continuous navmesh tile rebuilds for per-frame deformable
	// collision that has no navigation relevance.
	if (bUpdateNavigationOnCollisionUpdate && CanEverAffectNavigation())
	{
		FNavigationSystem::UpdateComponentData(*this);
	}
}
