// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

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



DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponent - Collision Data Received"), STAT_RuntimeMeshComponent_NewCollisionMeshReceived, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshComponent - Create Scene Proxy"), STAT_RuntimeMeshComponent_CreateSceneProxy, STATGROUP_RuntimeMesh);


URuntimeMeshComponent::URuntimeMeshComponent()
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


FRuntimeMeshCollisionHitInfo URuntimeMeshComponent::GetHitSource(int32 FaceIndex) const
{
	URuntimeMesh* Mesh = GetRuntimeMesh();
	if (Mesh)
	{
		FRuntimeMeshCollisionHitInfo HitInfo = Mesh->GetHitSource(FaceIndex);
		if (HitInfo.SectionId >= 0)
		{
			HitInfo.Material = GetMaterial(HitInfo.SectionId);
		}
		return HitInfo;
	}
	return FRuntimeMeshCollisionHitInfo();
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
	check(IsInGameThread());
	if (bRenderStateCreated)
	{
		MarkRenderStateDirty();
	}
}


FBoxSphereBounds URuntimeMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (GetRuntimeMesh())
	{
		FBoxSphereBounds TempBounds = GetRuntimeMesh()->GetLocalBounds();
		return TempBounds.TransformBy(LocalToWorld);
	}

	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1));
}


FPrimitiveSceneProxy* URuntimeMeshComponent::CreateSceneProxy()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponent_CreateSceneProxy);

	if (RuntimeMeshReference == nullptr || !RuntimeMeshReference->GetRenderProxy(GetScene()->GetFeatureLevel()).IsValid())
	{
		return nullptr;
	}

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



int32 URuntimeMeshComponent::GetMaterialIndex(FName MaterialSlotName) const
{
	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		return Mesh->GetMaterialIndex(MaterialSlotName);
	}
	return INDEX_NONE;
}

TArray<FName> URuntimeMeshComponent::GetMaterialSlotNames() const
{
	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		return Mesh->GetMaterialSlotNames();
	}
	return TArray<FName>();
}

bool URuntimeMeshComponent::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		return Mesh->IsMaterialSlotNameValid(MaterialSlotName);
	}
	return false;
}

void URuntimeMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	TArray<FRuntimeMeshMaterialSlot> Slots = GetMaterialSlots();

	for (int32 Index = 0; Index < Slots.Num(); Index++)
	{
		UMaterialInterface* Mat = Super::GetMaterial(Index);

		if (Mat == nullptr)
		{
			Mat = Slots[Index].Material;
		}

		if (Mat)
		{
			OutMaterials.Add(Mat);
		}
	}
}

int32 URuntimeMeshComponent::GetNumMaterials() const
{
	int32 RuntimeMeshSections = GetRuntimeMesh() != nullptr ? GetRuntimeMesh()->GetNumMaterials() : 0;

	return FMath::Max(Super::GetNumMaterials(), RuntimeMeshSections);
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
		return Mesh->GetMaterial(ElementIndex);
	}

	// Had no RM/Section return null
	return nullptr;
}






void URuntimeMeshComponent::PostLoad()
{
	Super::PostLoad();
}

void URuntimeMeshComponent::BeginDestroy()
{
	Super::BeginDestroy();
}

void URuntimeMeshComponent::OnRegister()
{
	Super::OnRegister();

	if (RuntimeMeshReference)
	{
		RuntimeMeshReference->RegisterLinkedComponent(this);
	}
}

void URuntimeMeshComponent::OnUnregister()
{
	if (RuntimeMeshReference)
	{
		RuntimeMeshReference->UnRegisterLinkedComponent(this);
		RuntimeMeshReference = nullptr;
	}

	Super::OnUnregister();
}