// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshActor.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshSubsystem.h"
#include "Engine/CollisionProfile.h"
#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "Engine/Level.h"

URealtimeMeshStream* ARealtimeMeshActor::MakeStream(const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements)
{
	auto Stream = NewObject<URealtimeMeshStream>(this);
	Stream->Initialize(StreamKey, StreamType, NumElements);
	return Stream;
}

URealtimeMeshStreamSet* ARealtimeMeshActor::MakeStreamSet()
{
	auto StreamSet = NewObject<URealtimeMeshStreamSet>(this);
	return StreamSet;
}

URealtimeMeshLocalBuilder* ARealtimeMeshActor::MakeMeshBuilder(ERealtimeMeshSimpleStreamConfig WantedTangents, ERealtimeMeshSimpleStreamConfig WantedTexCoords,
	bool bWants32BitIndices, ERealtimeMeshSimpleStreamConfig WantedPolyGroupType, bool bWantsColors, int32 WantedTexCoordChannels, bool bKeepExistingData)
{
	auto Builder = NewObject<URealtimeMeshLocalBuilder>(this);
	Builder->Initialize(WantedTangents, WantedTexCoords, bWants32BitIndices, WantedPolyGroupType, bWantsColors, WantedTexCoordChannels, bKeepExistingData);
	return Builder;
}

void ARealtimeMeshActor::BeginPlay()
{
	// Replication is opt-in: respect the configured/archetype bReplicates value rather than
	// forcing every RealtimeMesh actor into the replication graph. Only wire up the remote
	// role and physics replication mode when the user actually enabled replication.
	if (bReplicates && GetLocalRole() == ROLE_Authority)
	{
		SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
		SetReplicates(true);

		if (RealtimeMeshComponent && RealtimeMeshComponent->BodyInstance.bSimulatePhysics)
		{
			SetPhysicsReplicationMode(EPhysicsReplicationMode::Resimulation);
		}
	}

	Super::BeginPlay();
}

ARealtimeMeshActor::ARealtimeMeshActor()
{
	RealtimeMeshComponent = CreateDefaultSubobject<URealtimeMeshComponent>(TEXT("RealtimeMeshComponent"));
	RealtimeMeshComponent->SetMobility(EComponentMobility::Movable);
	RealtimeMeshComponent->SetGenerateOverlapEvents(false);
	RealtimeMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	SetRootComponent(RealtimeMeshComponent);

}


ARealtimeMeshActor::~ARealtimeMeshActor()
{
	// Deliberately do NOT unregister here. Walking the outer chain (GetWorld()->GetSubsystem)
	// during GC teardown is UB because the outers may already be destructed. Unregistration is
	// handled by Destroyed() (and PostUnregisterAllComponents on level disassociation, plus
	// PostEditUndo in-editor) while the actor is still live, and the subsystem prunes stale
	// weak pointers on tick (API-H3), so a destructor-time unregister is unnecessary.
}


void ARealtimeMeshActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MarkGeneratedMeshRebuildPending();
}

void ARealtimeMeshActor::PostLoad()
{
	Super::PostLoad();
	RegisterWithGenerationManager();
}

void ARealtimeMeshActor::PostActorCreated()
{
	Super::PostActorCreated();
	RegisterWithGenerationManager();
}

void ARealtimeMeshActor::Destroyed()
{
	UnregisterWithGenerationManager();
	Super::Destroyed();
}


void ARealtimeMeshActor::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();

	// Handle UWorld::AddToWorld() to catch ULevel visibility toggles
	if (GetLevel() && GetLevel()->bIsAssociatingLevel)
	{
		RegisterWithGenerationManager();
	}
}

void ARealtimeMeshActor::PostUnregisterAllComponents()
{
	// Handle UWorld::RemoveFromWorld() to catch ULevel visibility toggles
	if (GetLevel() && GetLevel()->bIsDisassociatingLevel)
	{
		UnregisterWithGenerationManager();
	}

	Super::PostUnregisterAllComponents();
}


#if WITH_EDITOR

void ARealtimeMeshActor::PostEditUndo()
{
	Super::PostEditUndo();

	// There is no direct signal that an Actor is being created or destroyed due to undo/redo.
	// Currently (5.1) the checks below will tell us if the undo/redo has destroyed the
	// Actor, and we assume otherwise it was created

	if (IsActorBeingDestroyed() || !IsValid(this)) // equivalent to AActor::IsPendingKillPending()
	{
		UnregisterWithGenerationManager();
	}
	else
	{
		RegisterWithGenerationManager();
	}
}

#endif


void ARealtimeMeshActor::RegisterWithGenerationManager()
{
	// Ignore generation on CDO, or if we duplicated to PIE from editor
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	if (bIsRegisteredWithGenerationManager == false)
	{
		// this could fail if the subsystem is not initialized yet, or if it is shutting down
		if (URealtimeMeshSubsystem* Subsystem = URealtimeMeshSubsystem::GetInstance(GetWorld()))
		{
			bIsRegisteredWithGenerationManager = Subsystem->RegisterGeneratedMeshActor(this);
		}
	}
}


void ARealtimeMeshActor::UnregisterWithGenerationManager()
{
	if (bIsRegisteredWithGenerationManager)
	{
		if (UWorld* World = GetWorld())
		{
			if (URealtimeMeshSubsystem* Subsystem = URealtimeMeshSubsystem::GetInstance(World))
			{
				Subsystem->UnregisterGeneratedMeshActor(this);
			}
		}
		bIsRegisteredWithGenerationManager = false;
		bGeneratedMeshRebuildPending = false;
	}
}


void ARealtimeMeshActor::MarkGeneratedMeshRebuildPending()
{
	bGeneratedMeshRebuildPending = true;

	// Place the actor on the subsystem's per-frame pending list so it (and only actors with pending
	// work) get visited by Tick. Only queue actors that are actually registered with the generation
	// manager, keeping registration the single gate for participation. Non-deferred actors are
	// dropped again by the subsystem after the first visit since WantsGeneratedMeshRebuild() is false.
	if (bIsRegisteredWithGenerationManager)
	{
		if (URealtimeMeshSubsystem* Subsystem = URealtimeMeshSubsystem::GetInstance(GetWorld()))
		{
			Subsystem->MarkActorRebuildPending(this);
		}
	}
}


void ARealtimeMeshActor::ExecuteRebuildGeneratedMeshIfPending()
{
	if (!bDeferGeneration || bFrozen ||
		!bGeneratedMeshRebuildPending ||
		!IsValid(RealtimeMeshComponent))
	{
		return;
	}

	if (bResetOnRebuild)
	{
		RealtimeMeshComponent->SetRealtimeMesh(nullptr);
	}

	FEditorScriptExecutionGuard Guard;

	OnGenerateMesh();

	bGeneratedMeshRebuildPending = false;
}






