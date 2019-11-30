// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RealtimeMesh.h"
#include "RealtimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "IPhysXCookingModule.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshProxy.h"
#include "RealtimeMeshData.h"

//////////////////////////////////////////////////////////////////////////
//	URealtimeMesh


//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshCollisionCookTickObject
void FRealtimeMeshCollisionCookTickObject::Tick(float DeltaTime)
{
	URealtimeMesh* Mesh = Owner.Get();
	if (Mesh && Mesh->bCollisionIsDirty)
	{
		Mesh->UpdateCollision();
		Mesh->bCollisionIsDirty = false;
	}
}

bool FRealtimeMeshCollisionCookTickObject::IsTickable() const
{
	URealtimeMesh* Mesh = Owner.Get();
	if (Mesh)
	{
		return Mesh->bCollisionIsDirty;
	}
	return false;
}

TStatId FRealtimeMeshCollisionCookTickObject::GetStatId() const
{
	return TStatId();
}


UWorld* FRealtimeMeshCollisionCookTickObject::GetTickableGameObjectWorld() const
{
	URealtimeMesh* Mesh = Owner.Get();
	if (Mesh)
	{
		return Mesh->GetWorld();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
//	URealtimeMesh

URealtimeMesh::URealtimeMesh(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer)
	, bCollisionIsDirty(false)
	, BodySetup(nullptr)
{
}

void URealtimeMesh::Initialize(URealtimeMeshProvider* Provider)
{
	MeshProvider = Provider;
	MarkChanged();
	InitializeInternal();
}

int32 URealtimeMesh::GetNumMaterials()
{
	return Data.IsValid() ? Data->GetNumMaterials() : 0;
}

void URealtimeMesh::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials)
{
	if (Data.IsValid())
	{
		Data->GetUsedMaterials(OutMaterials);
	}
}

UMaterialInterface* URealtimeMesh::GetMaterialForSlot(int32 SlotIndex)
{
	return Data.IsValid() ? Data->GetMaterial(SlotIndex) : nullptr;
}

FBoxSphereBounds URealtimeMesh::GetLocalBounds() const
{
	return Data.IsValid() ? Data->GetBounds() : FBoxSphereBounds();
}


void URealtimeMesh::MarkCollisionDirty()
{
	// Flag the collision as dirty
	bCollisionIsDirty = true;

	if (!CookTickObject.IsValid())
	{
		CookTickObject = MakeUnique<FRealtimeMeshCollisionCookTickObject>(TWeakObjectPtr<URealtimeMesh>(this));
	}
}

UBodySetup* URealtimeMesh::CreateNewBodySetup()
{
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	return NewBodySetup;
}

void URealtimeMesh::UpdateCollision(bool bForceCookNow)
{
	check(IsInGameThread());

	if (Data.IsValid())
	{
		FRealtimeMeshCollisionSettings CollisionSettings = Data->BaseProvider->GetCollisionSettings();

		UWorld* World = GetWorld();
		const bool bShouldCookAsync = !bForceCookNow && World && World->IsGameWorld() && CollisionSettings.bUseAsyncCooking;

		const auto& SetupCollisionConfiguration = [&](UBodySetup* Setup)
		{
			Setup->BodySetupGuid = FGuid::NewGuid();

			Setup->bGenerateMirroredCollision = false;
			Setup->bDoubleSidedGeometry = true;
			Setup->CollisionTraceFlag = CollisionSettings.bUseComplexAsSimple ? CTF_UseComplexAsSimple : CTF_UseDefault;

			auto& ConvexElems = Setup->AggGeom.ConvexElems;
			ConvexElems.Empty();
			for (const FRealtimeMeshCollisionConvexMesh& Convex : CollisionSettings.ConvexElements)
			{
				FKConvexElem& NewConvexElem = *new(ConvexElems) FKConvexElem();
				NewConvexElem.VertexData = Convex.VertexBuffer;
				// TODO: Store this on the section so we don't have to compute it on each cook
				NewConvexElem.ElemBox = Convex.BoundingBox;
			}

			auto& BoxElems = Setup->AggGeom.BoxElems;
			BoxElems.Empty();
			for (const FRealtimeMeshCollisionBox& Box : CollisionSettings.Boxes)
			{
				FKBoxElem& NewBox = *new(BoxElems) FKBoxElem();
				NewBox.Center = Box.Center;
				NewBox.Rotation = Box.Rotation;
				NewBox.X = Box.Extents.X;
				NewBox.Y = Box.Extents.Y;
				NewBox.Z = Box.Extents.Z;
			}

			auto& SphereElems = Setup->AggGeom.SphereElems;
			SphereElems.Empty();
			for (const FRealtimeMeshCollisionSphere& Sphere : CollisionSettings.Spheres)
			{
				FKSphereElem& NewSphere = *new(SphereElems)FKSphereElem();
				NewSphere.Center = Sphere.Center;
				NewSphere.Radius = Sphere.Radius;
			}

			auto& SphylElems = Setup->AggGeom.SphylElems;
			SphylElems.Empty();
			for (const FRealtimeMeshCollisionCapsule& Capsule : CollisionSettings.Capsules)
			{
				FKSphylElem& NewSphyl = *new(SphylElems)FKSphylElem();
				NewSphyl.Center = Capsule.Center;
				NewSphyl.Rotation = Capsule.Rotation;
				NewSphyl.Radius = Capsule.Radius;
				NewSphyl.Length = Capsule.Length;
			}
		};


		if (bShouldCookAsync)
		{
			// Abort all previous ones still standing
			for (UBodySetup* OldBody : AsyncBodySetupQueue)
			{
				OldBody->AbortPhysicsMeshAsyncCreation();
			}

			UBodySetup* NewBodySetup = CreateNewBodySetup();
			AsyncBodySetupQueue.Add(NewBodySetup);

			SetupCollisionConfiguration(NewBodySetup);

			NewBodySetup->CreatePhysicsMeshesAsync(
				FOnAsyncPhysicsCookFinished::CreateUObject(this, &URealtimeMesh::FinishPhysicsAsyncCook, NewBodySetup));
		}
		else
		{
			AsyncBodySetupQueue.Empty();
			UBodySetup* NewBodySetup = CreateNewBodySetup();

			// Change body setup guid 
			NewBodySetup->BodySetupGuid = FGuid::NewGuid();

			SetupCollisionConfiguration(NewBodySetup);

			// Update meshes
			NewBodySetup->bHasCookedCollisionData = true;
			NewBodySetup->InvalidatePhysicsData();
			NewBodySetup->CreatePhysicsMeshes();

			BodySetup = NewBodySetup;
			FinalizeNewCookedData();
		}
	}
}

void URealtimeMesh::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
	check(IsInGameThread());

	int32 FoundIdx;
	if (AsyncBodySetupQueue.Find(FinishedBodySetup, FoundIdx))
	{
		if (bSuccess)
		{
			// The new body was found in the array meaning it's newer so use it
			BodySetup = FinishedBodySetup;

			// Shift down all remaining body setups, removing any old setups
			for (int32 Index = FoundIdx + 1; Index < AsyncBodySetupQueue.Num(); Index++)
			{
				AsyncBodySetupQueue[Index - (FoundIdx + 1)] = AsyncBodySetupQueue[Index];
				AsyncBodySetupQueue[Index] = nullptr;
			}
			AsyncBodySetupQueue.SetNum(AsyncBodySetupQueue.Num() - (FoundIdx + 1));

			FinalizeNewCookedData();

		}
		else
		{
			AsyncBodySetupQueue.RemoveAt(FoundIdx);
		}
	}
}

void URealtimeMesh::FinalizeNewCookedData()
{
	check(IsInGameThread());

	// Alert all linked components so they can update their physics state.
	DoForAllLinkedComponents([](URealtimeMeshComponent* Mesh)
		{
			Mesh->NewCollisionMeshReceived();
		});

	// Call user event to notify of collision updated.
	if (CollisionUpdated.IsBound())
	{
		CollisionUpdated.Broadcast();
	}
}




bool URealtimeMesh::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	if (Data.IsValid())
	{
		FRealtimeMeshCollisionData CollisionMesh;

		if (Data->BaseProvider->GetCollisionMesh(CollisionMesh))
		{
			CollisionData->Vertices = CollisionMesh.Vertices.TakeContents();
			CollisionData->Indices = CollisionMesh.Triangles.TakeContents();
			CollisionData->UVs = CollisionMesh.TexCoords.TakeContents();
			CollisionData->MaterialIndices = CollisionMesh.MaterialIndices.TakeContents();

			CollisionData->bDeformableMesh = CollisionMesh.bDeformableMesh;
			CollisionData->bDisableActiveEdgePrecompute = CollisionMesh.bDisableActiveEdgePrecompute;
			CollisionData->bFastCook = CollisionMesh.bFastCook;
			CollisionData->bFlipNormals = CollisionMesh.bFlipNormals;

			return true;
		}
	}

	return false;
}

bool URealtimeMesh::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	if (Data.IsValid())
	{
		return Data->BaseProvider->HasCollisionMesh();
	}

	return false;
}

void URealtimeMesh::InitializeInternal()
{
	auto ProviderProxy = MeshProvider->GetProxy();

	// This is a very loaded assignment...
	// If the old provider loses all its references here then it will take with it
	// the render side proxy and all its resources, so this is a very 
	// deceptively heavy operation if a provider already existed.
	TWeakObjectPtr<URealtimeMesh> ThisPtr = TWeakObjectPtr<URealtimeMesh>(this);
	Data = MakeShared<FRealtimeMeshData, ESPMode::ThreadSafe>(ProviderProxy, ThisPtr);


	ERHIFeatureLevel::Type FeatureLevel;
	if (GetSceneFeatureLevel(FeatureLevel))
	{
		Data->GetOrCreateRenderProxy(FeatureLevel);
	}

	// Now we can initialize the data provider and actually get the component fully up and running
	Data->Initialize();
}

void URealtimeMesh::RegisterLinkedComponent(URealtimeMeshComponent* NewComponent)
{
	LinkedComponents.AddUnique(NewComponent);
}

void URealtimeMesh::UnRegisterLinkedComponent(URealtimeMeshComponent* ComponentToRemove)
{
	check(LinkedComponents.Contains(ComponentToRemove));

	LinkedComponents.RemoveSingleSwap(ComponentToRemove, true);
}


bool URealtimeMesh::GetSceneFeatureLevel(ERHIFeatureLevel::Type& OutFeatureLevel)
{
	for (auto& Comp : LinkedComponents)
	{
		if (Comp.IsValid())
		{
			URealtimeMeshComponent* Component = Comp.Get();
			auto* Scene = Component->GetScene();
			if (Scene)
			{
				OutFeatureLevel = Scene->GetFeatureLevel();
				return true;
			}
		}
	}
	return false;
}

void URealtimeMesh::EnsureReadyToRender(ERHIFeatureLevel::Type InFeatureLevel)
{
// 	if (Data.IsValid())
// 	{
// 		Data->EnsureReadyToRender(InFeatureLevel);
// 	}
}

FRealtimeMeshProxyPtr URealtimeMesh::GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel)
{
	return Data.IsValid() ? Data->GetOrCreateRenderProxy(InFeatureLevel) : nullptr;
}

void URealtimeMesh::UpdateAllComponentsBounds()
{
	DoForAllLinkedComponents([](URealtimeMeshComponent* Mesh)
	{
		Mesh->NewBoundsReceived();
	});
}

void URealtimeMesh::RecreateAllComponentProxies()
{
	DoForAllLinkedComponents([](URealtimeMeshComponent* Mesh)
	{
		Mesh->ForceProxyRecreate();
	});
}

void URealtimeMesh::MarkChanged()
{
#if WITH_EDITOR
	Modify(true);
	PostEditChange();
#endif
}

void URealtimeMesh::PostLoad()
{
	Super::PostLoad();

	if (MeshProvider)
	{

		UE_LOG(LogRealtimeMesh, Warning, TEXT("RM: Initializing mesh from load. %d"), FPlatformTLS::GetCurrentThreadId());
		InitializeInternal();
	}

	UpdateAllComponentsBounds();
	//MarkCollisionDirty();
}
