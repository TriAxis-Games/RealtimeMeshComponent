// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RealtimeMeshActor.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshComponentPlugin.h"
#include "Engine/CollisionProfile.h"



ARealtimeMeshActor::ARealtimeMeshActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanBeDamaged = false;

	RealtimeMeshComponent = CreateDefaultSubobject<URealtimeMeshComponent>(TEXT("RealtimeMeshComponent0"));
	RealtimeMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RealtimeMeshComponent->Mobility = EComponentMobility::Static;

	RealtimeMeshComponent->SetGenerateOverlapEvents(false);
	RootComponent = RealtimeMeshComponent;
}

ERealtimeMeshMobility ARealtimeMeshActor::GetRealtimeMeshMobility()
{
	if (RealtimeMeshComponent)
	{
		return RealtimeMeshComponent->GetRealtimeMeshMobility();
	}
	return ERealtimeMeshMobility::Static;
}

void ARealtimeMeshActor::SetRealtimeMeshMobility(ERealtimeMeshMobility NewMobility)
{
	if (RealtimeMeshComponent)
	{
		RealtimeMeshComponent->SetRealtimeMeshMobility(NewMobility);
	}
}

void ARealtimeMeshActor::SetMobility(EComponentMobility::Type InMobility)
{
	if (RealtimeMeshComponent)
	{
		RealtimeMeshComponent->SetMobility(InMobility);
	}
}

EComponentMobility::Type ARealtimeMeshActor::GetMobility()
{
	if (RealtimeMeshComponent)
	{
		return RealtimeMeshComponent->Mobility;
	}
	return EComponentMobility::Static;
}

void ARealtimeMeshActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateMeshes();
}

void ARealtimeMeshActor::BeginPlay()
{
	Super::BeginPlay();
}

void ARealtimeMeshActor::GenerateMeshes_Implementation()
{

}

