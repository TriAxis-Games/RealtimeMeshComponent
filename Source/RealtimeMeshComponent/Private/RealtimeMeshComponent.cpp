// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RealtimeMeshComponent.h"
#include "RealtimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "IPhysXCookingModule.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshRenderable.h"
#include "RealtimeMeshSectionProxy.h"
#include "RealtimeMesh.h"
#include "RealtimeMeshComponentProxy.h"
#include "NavigationSystem.h"



DECLARE_CYCLE_STAT(TEXT("RMC - New Collision Data Recieved"), STAT_RealtimeMeshComponent_NewCollisionMeshReceived, STATGROUP_RealtimeMesh);





URealtimeMeshComponent::URealtimeMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetNetAddressable();

	Super::SetTickGroup(TG_DuringPhysics);
	Super::SetTickableWhenPaused(true);
}

void URealtimeMeshComponent::EnsureHasRealtimeMesh()
{
	if (RealtimeMeshReference == nullptr)
	{
		SetRealtimeMesh(NewObject<URealtimeMesh>(this));
	}
}

void URealtimeMeshComponent::SetRealtimeMesh(URealtimeMesh* NewMesh)
{
	// Unlink from any existing runtime mesh
	if (RealtimeMeshReference)
	{
		RealtimeMeshReference->UnRegisterLinkedComponent(this);
		RealtimeMeshReference = nullptr;
	}

	if (NewMesh)
	{
		RealtimeMeshReference = NewMesh;
		RealtimeMeshReference->RegisterLinkedComponent(this);
	}

	MarkRenderStateDirty();
}


void URealtimeMeshComponent::NewCollisionMeshReceived()
{
	SCOPE_CYCLE_COUNTER(STAT_RealtimeMeshComponent_NewCollisionMeshReceived);

	// First recreate the physics state
	RecreatePhysicsState();
	
 	// Now update the navigation.
	FNavigationSystem::UpdateComponentData(*this);
}

void URealtimeMeshComponent::NewBoundsReceived()
{
	UpdateBounds();
	if (bRenderStateCreated)
	{
		MarkRenderTransformDirty();
	}
}

void URealtimeMeshComponent::ForceProxyRecreate()
{
	UE_LOG(LogRealtimeMesh, Warning, TEXT("RMC: Force recreate requested. %d"), FPlatformTLS::GetCurrentThreadId());
	check(IsInGameThread());
	if (bRenderStateCreated)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RMC: marking state dirty proxy.. %d"), FPlatformTLS::GetCurrentThreadId());
		MarkRenderStateDirty();
	}
}




FBoxSphereBounds URealtimeMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (GetRealtimeMesh())
	{
		return GetRealtimeMesh()->GetLocalBounds().TransformBy(LocalToWorld);
	}

	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1));
}


FPrimitiveSceneProxy* URealtimeMeshComponent::CreateSceneProxy()
{

	if (RealtimeMeshReference == nullptr || RealtimeMeshReference->GetRenderProxy(GetScene()->GetFeatureLevel()) == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RMC: Unable to create proxy.. %d"), FPlatformTLS::GetCurrentThreadId());
		return nullptr;
	}

	UE_LOG(LogRealtimeMesh, Warning, TEXT("RMC: Creating proxy.. %d"), FPlatformTLS::GetCurrentThreadId());
	return new FRealtimeMeshComponentSceneProxy(this);
}

UBodySetup* URealtimeMeshComponent::GetBodySetup()
{
	if (GetRealtimeMesh())
	{
		return GetRealtimeMesh()->BodySetup;
	}
	
	return nullptr;
}



int32 URealtimeMeshComponent::GetNumMaterials() const
{
	int32 RuntimeMeshSections = GetRealtimeMesh() != nullptr ? GetRealtimeMesh()->GetNumMaterials() : 0;

	return FMath::Max(Super::GetNumMaterials(), RuntimeMeshSections);
}

void URealtimeMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (URealtimeMesh* Mesh = GetRealtimeMesh())
	{
		Mesh->GetUsedMaterials(OutMaterials);
	}

	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);
}

UMaterialInterface* URealtimeMeshComponent::GetMaterial(int32 ElementIndex) const
{
	UMaterialInterface* Mat = Super::GetMaterial(ElementIndex);

	// Use default override material system
	if (Mat != nullptr)
		return Mat;

	// fallback to RM sections material
	if (URealtimeMesh* Mesh = GetRealtimeMesh())
	{
		return Mesh->GetMaterialForSlot(ElementIndex);
	}

	// Had no RM/Section return null
	return nullptr;
}

UMaterialInterface* URealtimeMeshComponent::GetOverrideMaterial(int32 ElementIndex) const
{
	return Super::GetMaterial(ElementIndex);
}





void URealtimeMeshComponent::PostLoad()
{
	Super::PostLoad();

	if (RealtimeMeshReference)
	{
		RealtimeMeshReference->RegisterLinkedComponent(this);
	}
}
