// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

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
#include "Net/UnrealNetwork.h"


DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponent - Collision Data Received"), STAT_RealtimeMeshComponent_NewCollisionMeshReceived, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponent - Create Scene Proxy"), STAT_RealtimeMeshComponent_CreateSceneProxy, STATGROUP_RealtimeMesh);

URealtimeMeshComponent::URealtimeMeshComponent()
{
	SetNetAddressable();
	SetIsReplicatedByDefault(true);
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
		///RemoveReplicatedSubObject(RealtimeMesh);
		
		UnbindFromEvents(RealtimeMesh);
		RealtimeMesh = nullptr;
		bUpdatedMesh = true;
	}

	if (IsValid(NewMesh))
	{
		RealtimeMesh = NewMesh;
		BindToEvents(RealtimeMesh);
		bUpdatedMesh = true;

		//AddReplicatedSubObject(RealtimeMesh);
	}

	if (bUpdatedMesh)
	{
		UpdateBounds();
		UpdateCollision();
		MarkRenderStateDirty();
	}
}

URealtimeMesh* URealtimeMeshComponent::InitializeRealtimeMesh(TSubclassOf<URealtimeMesh> MeshClass)
{
	URealtimeMesh* NewMesh = nullptr;
	if (MeshClass)
	{
		NewMesh = NewObject<URealtimeMesh>(this, MeshClass);
		check(IsValid(NewMesh));
	}
	SetRealtimeMesh(NewMesh);
	check(IsValid(NewMesh));
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
}

void URealtimeMeshComponent::OnUnregister()
{
	Super::OnUnregister();

	if (RealtimeMesh)
	{
		UnbindFromEvents(RealtimeMesh);
	}
}

FBoxSphereBounds URealtimeMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (GetRealtimeMesh())
	{
		const FBoxSphereBounds TempBounds = FBoxSphereBounds(GetRealtimeMesh()->GetLocalBounds());
		return TempBounds.TransformBy(LocalToWorld);
	}

	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1));
}


FPrimitiveSceneProxy* URealtimeMeshComponent::CreateSceneProxy()
{
	SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponent_CreateSceneProxy);

	if (RealtimeMesh != nullptr)
	{
		if (const auto MeshRenderProxy = RealtimeMesh->GetMesh()->GetRenderProxy(true))
		{
			// This is using the implementation in the RMC-Pro to support nanite, without that module present, the RMC doesn't support nanite.
			if (RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager::IsNaniteSupportAvailable() && MeshRenderProxy->HasNaniteResources())
			{
				RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager& NaniteModule = RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager::GetNaniteModule();

				if (NaniteModule.ShouldUseNanite(this))
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

bool URealtimeMeshComponent::UseNaniteOverrideMaterials() const
{
	return Super::UseNaniteOverrideMaterials();
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
	if (Mat != nullptr)
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

#if RMC_ENGINE_ABOVE_5_4
void URealtimeMeshComponent::CollectPSOPrecacheData(const FPSOPrecacheParams& BasePrecachePSOParams, FMaterialInterfacePSOPrecacheParamsList& OutParams)
{
	FPSOPrecacheVertexFactoryDataList VFDataList;
	const FVertexFactoryType* VFType = nullptr;

	if (RealtimeMesh)
	{
		if (const auto MeshRenderProxy = RealtimeMesh->GetMesh()->GetRenderProxy(true))
		{
			if (RealtimeMesh::IRealtimeMeshNaniteSceneProxyManager::IsNaniteSupportAvailable() && MeshRenderProxy->HasNaniteResources())
			{
#if RMC_ENGINE_BELOW_5_5
				if (NaniteLegacyMaterialsSupported())
				{
					VFDataList.Add(FPSOPrecacheVertexFactoryData(&Nanite::FVertexFactory::StaticType));
				}

				if (NaniteComputeMaterialsSupported())
				{
					VFDataList.Add(FPSOPrecacheVertexFactoryData(&FNaniteVertexFactory::StaticType));
				}
#endif

				for (int32 MaterialId = 0; MaterialId < GetNumMaterials(); MaterialId++)
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
	}
	
	Super::CollectPSOPrecacheData(BasePrecachePSOParams, OutParams);
}
#endif

void URealtimeMeshComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(URealtimeMeshComponent, RealtimeMesh);
}

void URealtimeMeshComponent::BindToEvents(URealtimeMesh* InRealtimeMesh)
{
	InRealtimeMesh->OnBoundsChanged().AddUObject(this, &URealtimeMeshComponent::HandleBoundsUpdated);
	InRealtimeMesh->OnRenderDataChanged().AddUObject(this, &URealtimeMeshComponent::HandleMeshRenderingDataChanged);
	InRealtimeMesh->OnCollisionBodyUpdated().AddUObject(this, &URealtimeMeshComponent::HandleCollisionBodyUpdated);
}

void URealtimeMeshComponent::UnbindFromEvents(URealtimeMesh* InRealtimeMesh)
{
	InRealtimeMesh->OnBoundsChanged().RemoveAll(this);
	InRealtimeMesh->OnRenderDataChanged().RemoveAll(this);
	InRealtimeMesh->OnCollisionBodyUpdated().RemoveAll(this);
}


void URealtimeMeshComponent::HandleBoundsUpdated(URealtimeMesh* InRealtimeMesh)
{
	UpdateBounds();
}

void URealtimeMeshComponent::HandleMeshRenderingDataChanged(URealtimeMesh* InRealtimeMesh, bool bShouldProxyRecreate)
{
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
		// First Store Velocities
		const FVector PrevLinearVelocity = GetPhysicsLinearVelocity();
		const FVector PrevAngularVelocity = GetPhysicsAngularVelocityInDegrees();

		// Recreate the physics state
		RecreatePhysicsState();

		// Apply Velocities
		SetPhysicsLinearVelocity(PrevLinearVelocity, false);
		SetPhysicsAngularVelocityInDegrees(PrevAngularVelocity, false);
	}
	else
	{
		//First recreate the physics state
		RecreatePhysicsState();
	}

	// Now update the navigation.
	FNavigationSystem::UpdateComponentData(*this);
}
