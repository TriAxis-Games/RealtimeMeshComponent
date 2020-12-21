// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshActor.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentPlugin.h"
#include "Engine/CollisionProfile.h"



ARuntimeMeshActor::ARuntimeMeshActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 24
	SetCanBeDamaged(false);
#else
	bCanBeDamaged = false;
#endif

	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent0"));
	RuntimeMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RuntimeMeshComponent->Mobility = EComponentMobility::Static;

	RuntimeMeshComponent->SetGenerateOverlapEvents(false);
	RootComponent = RuntimeMeshComponent;
}

ERuntimeMeshMobility ARuntimeMeshActor::GetRuntimeMeshMobility()
{
	if (RuntimeMeshComponent)
	{
		return RuntimeMeshComponent->GetRuntimeMeshMobility();
	}
	return ERuntimeMeshMobility::Static;
}

void ARuntimeMeshActor::SetRuntimeMeshMobility(ERuntimeMeshMobility NewMobility)
{
	if (RuntimeMeshComponent)
	{
		RuntimeMeshComponent->SetRuntimeMeshMobility(NewMobility);
	}
}

void ARuntimeMeshActor::SetMobility(EComponentMobility::Type InMobility)
{
	if (RuntimeMeshComponent)
	{
		RuntimeMeshComponent->SetMobility(InMobility);
	}
}

EComponentMobility::Type ARuntimeMeshActor::GetMobility()
{
	if (RuntimeMeshComponent)
	{
		return RuntimeMeshComponent->Mobility;
	}
	return EComponentMobility::Static;
}

