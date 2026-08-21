// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogRealtimeMeshExamples, Log, All);

ARealtimeMeshExampleActor::ARealtimeMeshExampleActor()
{
	// Most examples build once in OnConstruction and never tick. The few animated
	// examples opt back into ticking in their own constructors.
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshExampleActor::VerifyMeshBuilt() const
{
	const URealtimeMeshComponent* Component = GetRealtimeMeshComponent();
	const URealtimeMesh* Mesh = Component ? Component->GetRealtimeMesh() : nullptr;

	if (!ensureMsgf(Mesh != nullptr, TEXT("%s: no RealtimeMesh was initialized"), *GetName()))
	{
		UE_LOG(LogRealtimeMeshExamples, Warning, TEXT("%s did not initialize a RealtimeMesh"), *GetName());
		return;
	}

	const FBoxSphereBounds Bounds = Mesh->GetLocalBounds();
	if (!ensureMsgf(Bounds.SphereRadius > 0.0f, TEXT("%s: mesh built with degenerate bounds"), *GetName()))
	{
		UE_LOG(LogRealtimeMeshExamples, Warning, TEXT("%s built a mesh with degenerate bounds"), *GetName());
	}
}
