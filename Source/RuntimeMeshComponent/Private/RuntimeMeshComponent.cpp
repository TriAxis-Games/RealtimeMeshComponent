// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "IPhysXCookingModule.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshSectionProxy.h"
#include "RuntimeMesh.h"
#include "RuntimeMeshComponentProxy.h"
#include "NavigationSystem.h"



DECLARE_CYCLE_STAT(TEXT("RMC - New Collision Data Recieved"), STAT_RuntimeMeshComponent_NewCollisionMeshReceived, STATGROUP_RuntimeMesh);





URuntimeMeshComponent::URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetNetAddressable();

	Super::SetTickGroup(TG_DuringPhysics);
	Super::SetTickableWhenPaused(true);
}

void URuntimeMeshComponent::EnsureHasRuntimeMesh()
{
	if (RuntimeMeshReference == nullptr)
	{
		SetRuntimeMesh(NewObject<URuntimeMesh>(this));
	}
}

void URuntimeMeshComponent::SetRuntimeMesh(URuntimeMesh* NewMesh)
{
	// Unlink from any existing runtime mesh
	if (RuntimeMeshReference)
	{
		RuntimeMeshReference->UnRegisterLinkedComponent(this);
		RuntimeMeshReference = nullptr;
	}

	if (NewMesh)
	{
		RuntimeMeshReference = NewMesh;
		RuntimeMeshReference->RegisterLinkedComponent(this);
	}

	MarkRenderStateDirty();
}


void URuntimeMeshComponent::NewCollisionMeshReceived()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponent_NewCollisionMeshReceived);

	// First recreate the physics state
	RecreatePhysicsState();
	
 	// Now update the navigation.
	FNavigationSystem::UpdateComponentData(*this);
}

void URuntimeMeshComponent::NewBoundsReceived()
{
	UpdateBounds();
	if (bRenderStateCreated)
	{
		MarkRenderTransformDirty();
	}
}

void URuntimeMeshComponent::ForceProxyRecreate()
{
	UE_LOG(LogRuntimeMesh, Warning, TEXT("RMC: Force recreate requested. %d"), FPlatformTLS::GetCurrentThreadId());
	check(IsInGameThread());
	if (bRenderStateCreated)
	{
		UE_LOG(LogRuntimeMesh, Warning, TEXT("RMC: marking state dirty proxy.. %d"), FPlatformTLS::GetCurrentThreadId());
		MarkRenderStateDirty();
	}
}




FBoxSphereBounds URuntimeMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (GetRuntimeMesh())
	{
		return GetRuntimeMesh()->GetLocalBounds().TransformBy(LocalToWorld);
	}

	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1));
}


FPrimitiveSceneProxy* URuntimeMeshComponent::CreateSceneProxy()
{

	if (RuntimeMeshReference == nullptr || RuntimeMeshReference->GetRenderProxy(GetScene()->GetFeatureLevel()) == nullptr)
	{
		UE_LOG(LogRuntimeMesh, Warning, TEXT("RMC: Unable to create proxy.. %d"), FPlatformTLS::GetCurrentThreadId());
		return nullptr;
	}

	UE_LOG(LogRuntimeMesh, Warning, TEXT("RMC: Creating proxy.. %d"), FPlatformTLS::GetCurrentThreadId());
	return new FRuntimeMeshComponentSceneProxy(this);
}

UBodySetup* URuntimeMeshComponent::GetBodySetup()
{
	if (GetRuntimeMesh())
	{
		return GetRuntimeMesh()->BodySetup;
	}
	
	return nullptr;
}



int32 URuntimeMeshComponent::GetNumMaterials() const
{
	int32 RuntimeMeshSections = GetRuntimeMesh() != nullptr ? GetRuntimeMesh()->GetNumMaterials() : 0;

	return FMath::Max(Super::GetNumMaterials(), RuntimeMeshSections);
}

void URuntimeMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		Mesh->GetUsedMaterials(OutMaterials);
	}

	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);
}

UMaterialInterface* URuntimeMeshComponent::GetMaterial(int32 ElementIndex) const
{
	UMaterialInterface* Mat = Super::GetMaterial(ElementIndex);

	// Use default override material system
	if (Mat != nullptr)
		return Mat;

	// fallback to RM sections material
	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		return Mesh->GetMaterialForSlot(ElementIndex);
	}

	// Had no RM/Section return null
	return nullptr;
}

UMaterialInterface* URuntimeMeshComponent::GetOverrideMaterial(int32 ElementIndex) const
{
	return Super::GetMaterial(ElementIndex);
}





void URuntimeMeshComponent::PostLoad()
{
	Super::PostLoad();

	if (RuntimeMeshReference)
	{
		RuntimeMeshReference->RegisterLinkedComponent(this);
	}
}
