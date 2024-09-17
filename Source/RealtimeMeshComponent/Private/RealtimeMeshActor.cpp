// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshActor.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshSubsystem.h"
#include "Engine/CollisionProfile.h"
#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Engine/Level.h"
#endif

#define LOCTEXT_NAMESPACE "ARealtimeMeshActor"

ARealtimeMeshActor::ARealtimeMeshActor()
{
	RealtimeMeshComponent = CreateDefaultSubobject<URealtimeMeshComponent>(TEXT("RealtimeMeshComponent"));
	RealtimeMeshComponent->SetMobility(EComponentMobility::Movable);
	RealtimeMeshComponent->SetGenerateOverlapEvents(false);
	RealtimeMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	//RealtimeMeshComponent->CollisionType = ECollisionTraceFlag::CTF_UseDefault;

	SetRootComponent(RealtimeMeshComponent);
}

URealtimeMeshStreamPool* ARealtimeMeshActor::GetStreamPool() const
{
	if (IsValid(StreamPool))
	{
		return StreamPool;
	}

	StreamPool = NewObject<URealtimeMeshStreamPool>();
	return StreamPool;
}

URealtimeMeshStream* ARealtimeMeshActor::MakeStream(const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements)
{
	auto Stream = NewObject<URealtimeMeshStream>(this);
	check(IsValid(Stream));
	Stream->Initialize(StreamKey, StreamType, NumElements);
	return Stream;
}

URealtimeMeshStreamSet* ARealtimeMeshActor::MakeStreamSet()
{
	auto StreamSet = NewObject<URealtimeMeshStreamSet>(this);
	check(IsValid(StreamSet));
	return StreamSet;
}

URealtimeMeshLocalBuilder* ARealtimeMeshActor::MakeMeshBuilder(ERealtimeMeshSimpleStreamConfig WantedTangents, ERealtimeMeshSimpleStreamConfig WantedTexCoords,
	bool bWants32BitIndices, ERealtimeMeshSimpleStreamConfig WantedPolyGroupType, bool bWantsColors, int32 WantedTexCoordChannels, bool bKeepExistingData)
{
	auto Builder = NewObject<URealtimeMeshLocalBuilder>(this);
	check(IsValid(Builder));
	Builder->Initialize(WantedTangents, WantedTexCoords, bWants32BitIndices, WantedPolyGroupType, bWantsColors, WantedTexCoordChannels, bKeepExistingData);
	return Builder;
}

void ARealtimeMeshActor::BeginPlay()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		bReplicates = true;
		SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
		SetReplicates(true);
	}

#if RMC_ENGINE_ABOVE_5_3
	if (RealtimeMeshComponent && RealtimeMeshComponent->BodyInstance.bSimulatePhysics)
	{
		SetPhysicsReplicationMode(EPhysicsReplicationMode::Resimulation);
	}
#endif
	
	Super::BeginPlay();
}

#undef LOCTEXT_NAMESPACE
