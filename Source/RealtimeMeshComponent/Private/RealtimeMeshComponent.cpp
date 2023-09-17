// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshComponent.h"
#include "RealtimeMeshComponentModule.h"
#include "RenderProxy/RealtimeMeshComponentProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMesh.h"
#include "NavigationSystem.h"


DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponent - Collision Data Received"), STAT_RealtimeMeshComponent_NewCollisionMeshReceived, STATGROUP_RealtimeMesh);
DECLARE_CYCLE_STAT(TEXT("RealtimeMeshComponent - Create Scene Proxy"), STAT_RealtimeMeshComponent_CreateSceneProxy, STATGROUP_RealtimeMesh);

URealtimeMeshComponent::URealtimeMeshComponent()
{
	SetNetAddressable();
}

void URealtimeMeshComponent::SetRealtimeMesh(URealtimeMesh* NewMesh)
{
	// Bail if we're already assigned to this mesh
	if (IsValid(NewMesh) && IsValid(RealtimeMeshReference) && NewMesh == RealtimeMeshReference)
	{
		return;
	}

	bool bUpdatedMesh = false;
	// Unlink from any existing runtime mesh
	if (IsValid(RealtimeMeshReference))
	{
		UnbindFromEvents(RealtimeMeshReference);
		RealtimeMeshReference = nullptr;
		bUpdatedMesh = true;
	}

	if (IsValid(NewMesh))
	{
		RealtimeMeshReference = NewMesh;
		BindToEvents(RealtimeMeshReference);
		bUpdatedMesh = true;
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
		NewMesh = NewObject<URealtimeMesh>(IsValid(GetOuter()) ? GetOuter() : this, MeshClass);
	}
	SetRealtimeMesh(NewMesh);
	return NewMesh;
}


void URealtimeMeshComponent::OnRegister()
{
	Super::OnRegister();

	if (RealtimeMeshReference)
	{
		BindToEvents(RealtimeMeshReference);
		UpdateCollision();
	}
}

void URealtimeMeshComponent::OnUnregister()
{
	Super::OnUnregister();

	if (RealtimeMeshReference)
	{
		UnbindFromEvents(RealtimeMeshReference);
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

	if (RealtimeMeshReference != nullptr)
	{
		if (const auto MeshRenderProxy = RealtimeMeshReference->GetMesh()->GetRenderProxy(true))
		{
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

void URealtimeMeshComponent::BindToEvents(URealtimeMesh* RealtimeMesh)
{
	RealtimeMesh->OnBoundsChanged().AddUObject(this, &URealtimeMeshComponent::HandleBoundsUpdated);
	RealtimeMesh->OnRenderDataChanged().AddUObject(this, &URealtimeMeshComponent::HandleMeshRenderingDataChanged);
	RealtimeMesh->OnCollisionBodyUpdated().AddUObject(this, &URealtimeMeshComponent::HandleCollisionBodyUpdated);
}

void URealtimeMeshComponent::UnbindFromEvents(URealtimeMesh* RealtimeMesh)
{
	RealtimeMesh->OnBoundsChanged().RemoveAll(this);
	RealtimeMesh->OnRenderDataChanged().RemoveAll(this);
	RealtimeMesh->OnCollisionBodyUpdated().RemoveAll(this);
}


void URealtimeMeshComponent::HandleBoundsUpdated(URealtimeMesh* IncomingMesh)
{
	UpdateBounds();
}

void URealtimeMeshComponent::HandleMeshRenderingDataChanged(URealtimeMesh* IncomingMesh, bool bShouldProxyRecreate)
{
	if (bShouldProxyRecreate)
	{
		MarkRenderStateDirty();
	}
}

void URealtimeMeshComponent::HandleCollisionBodyUpdated(URealtimeMesh* RealtimeMesh, UBodySetup* BodySetup)
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
