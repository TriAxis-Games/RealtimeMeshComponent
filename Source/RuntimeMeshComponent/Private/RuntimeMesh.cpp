// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMesh.h"
#include "RuntimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "IPhysXCookingModule.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshProxy.h"
#include "RuntimeMeshData.h"


//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshCollisionCookTickObject

/*
*	This tick function is used to drive the collision cooker.
*	It is enabled for one frame when we need to update collision.
*	This keeps from cooking on each individual create/update section as the original PMC did
*/
struct FRuntimeMeshDelayedActionTickObject : FTickableGameObject
{
private:
	TQueue<TWeakObjectPtr<URuntimeMesh>, EQueueMode::Mpsc> UpdateList;

public:
	FRuntimeMeshDelayedActionTickObject() {}

	static FRuntimeMeshDelayedActionTickObject& GetInstance()
	{
		static TUniquePtr<FRuntimeMeshDelayedActionTickObject> UpdaterObject;
		if (!UpdaterObject.IsValid())
		{
			UpdaterObject = MakeUnique<FRuntimeMeshDelayedActionTickObject>();
		}
		return *UpdaterObject.Get();
	}
	void RegisterForUpdate(TWeakObjectPtr<URuntimeMesh> InMesh)
	{
		UpdateList.Enqueue(InMesh);
	}

	virtual void Tick(float DeltaTime)
	{
		TWeakObjectPtr<URuntimeMesh> TempMesh;
		while (UpdateList.Dequeue(TempMesh))
		{
			URuntimeMesh* Mesh = TempMesh.Get();
			if (Mesh)
			{
				if (Mesh->bNeedsInitialization)
				{
					if (Mesh->MeshProvider)
					{
						Mesh->InitializeInternal();
						Mesh->bNeedsInitialization = false;
					}
					Mesh->UpdateAllComponentsBounds();
				}

				if (Mesh->bCollisionIsDirty)
				{
					Mesh->UpdateCollision();
					Mesh->bCollisionIsDirty = false;
				}
			}
		}
	}
	virtual bool IsTickable() const { return !UpdateList.IsEmpty(); }
	virtual bool IsTickableInEditor() const { return true; }
	virtual TStatId GetStatId() const { return TStatId(); }
};



//////////////////////////////////////////////////////////////////////////
//	URuntimeMesh

URuntimeMesh::URuntimeMesh(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer)
	, bNeedsInitialization(false)
	, bCollisionIsDirty(false)
	, BodySetup(nullptr)
{
}

void URuntimeMesh::Initialize(URuntimeMeshProvider* Provider)
{
	MeshProvider = Provider;
	MarkChanged();
	InitializeInternal();
}

int32 URuntimeMesh::GetNumMaterials()
{
	return Data.IsValid() ? Data->GetNumMaterials() : 0;
}

void URuntimeMesh::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials)
{
	if (Data.IsValid())
	{
		Data->GetUsedMaterials(OutMaterials);
	}
}

UMaterialInterface* URuntimeMesh::GetMaterialForSlot(int32 SlotIndex)
{
	return Data.IsValid() ? Data->GetMaterial(SlotIndex) : nullptr;
}

FBoxSphereBounds URuntimeMesh::GetLocalBounds() const
{
	return Data.IsValid() ? Data->GetBounds() : FBoxSphereBounds();
}


void URuntimeMesh::MarkCollisionDirty()
{
	// Flag the collision as dirty
	bCollisionIsDirty = true;
	FRuntimeMeshDelayedActionTickObject::GetInstance().RegisterForUpdate(TWeakObjectPtr<URuntimeMesh>(this));
}

UBodySetup* URuntimeMesh::CreateNewBodySetup()
{
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	return NewBodySetup;
}

void URuntimeMesh::UpdateCollision(bool bForceCookNow)
{
	check(IsInGameThread());

	if (Data.IsValid())
	{
		FRuntimeMeshCollisionSettings CollisionSettings = Data->BaseProvider->GetCollisionSettings();

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
			for (const FRuntimeMeshCollisionConvexMesh& Convex : CollisionSettings.ConvexElements)
			{
				FKConvexElem& NewConvexElem = *new(ConvexElems) FKConvexElem();
				NewConvexElem.VertexData = Convex.VertexBuffer;
				// TODO: Store this on the section so we don't have to compute it on each cook
				NewConvexElem.ElemBox = Convex.BoundingBox;
			}

			auto& BoxElems = Setup->AggGeom.BoxElems;
			BoxElems.Empty();
			for (const FRuntimeMeshCollisionBox& Box : CollisionSettings.Boxes)
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
			for (const FRuntimeMeshCollisionSphere& Sphere : CollisionSettings.Spheres)
			{
				FKSphereElem& NewSphere = *new(SphereElems)FKSphereElem();
				NewSphere.Center = Sphere.Center;
				NewSphere.Radius = Sphere.Radius;
			}

			auto& SphylElems = Setup->AggGeom.SphylElems;
			SphylElems.Empty();
			for (const FRuntimeMeshCollisionCapsule& Capsule : CollisionSettings.Capsules)
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
				FOnAsyncPhysicsCookFinished::CreateUObject(this, &URuntimeMesh::FinishPhysicsAsyncCook, NewBodySetup));
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

void URuntimeMesh::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
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

void URuntimeMesh::FinalizeNewCookedData()
{
	check(IsInGameThread());

	// Alert all linked components so they can update their physics state.
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
		{
			Mesh->NewCollisionMeshReceived();
		});

	// Call user event to notify of collision updated.
	if (CollisionUpdated.IsBound())
	{
		CollisionUpdated.Broadcast();
	}
}




bool URuntimeMesh::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	if (Data.IsValid())
	{
		FRuntimeMeshCollisionData CollisionMesh;

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

bool URuntimeMesh::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	if (Data.IsValid())
	{
		return Data->BaseProvider->HasCollisionMesh();
	}

	return false;
}

void URuntimeMesh::InitializeInternal()
{
	auto ProviderProxy = MeshProvider->SetupProxy();

	// This is a very loaded assignment...
	// If the old provider loses all its references here then it will take with it
	// the render side proxy and all its resources, so this is a very 
	// deceptively heavy operation if a provider already existed.
	TWeakObjectPtr<URuntimeMesh> ThisPtr = TWeakObjectPtr<URuntimeMesh>(this);
	Data = MakeShared<FRuntimeMeshData, ESPMode::ThreadSafe>(ProviderProxy, ThisPtr);


	ERHIFeatureLevel::Type FeatureLevel;
	if (GetSceneFeatureLevel(FeatureLevel))
	{
		Data->GetOrCreateRenderProxy(FeatureLevel);
	}

	// Now we can initialize the data provider and actually get the component fully up and running
	Data->Initialize();
}

void URuntimeMesh::RegisterLinkedComponent(URuntimeMeshComponent* NewComponent)
{
	LinkedComponents.AddUnique(NewComponent);
}

void URuntimeMesh::UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove)
{
	check(LinkedComponents.Contains(ComponentToRemove));

	LinkedComponents.RemoveSingleSwap(ComponentToRemove, true);
}


bool URuntimeMesh::GetSceneFeatureLevel(ERHIFeatureLevel::Type& OutFeatureLevel)
{
	for (auto& Comp : LinkedComponents)
	{
		if (Comp.IsValid())
		{
			URuntimeMeshComponent* Component = Comp.Get();
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

void URuntimeMesh::EnsureReadyToRender(ERHIFeatureLevel::Type InFeatureLevel)
{
// 	if (Data.IsValid())
// 	{
// 		Data->EnsureReadyToRender(InFeatureLevel);
// 	}
}

FRuntimeMeshProxyPtr URuntimeMesh::GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel)
{
	return Data.IsValid() ? Data->GetOrCreateRenderProxy(InFeatureLevel) : nullptr;
}

void URuntimeMesh::UpdateAllComponentsBounds()
{
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
	{
		Mesh->NewBoundsReceived();
	});
}

void URuntimeMesh::RecreateAllComponentProxies()
{
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
	{
		Mesh->ForceProxyRecreate();
	});
}

void URuntimeMesh::MarkChanged()
{
#if WITH_EDITOR
	Modify(true);
	PostEditChange();
#endif
}

void URuntimeMesh::PostLoad()
{
	Super::PostLoad();

	bNeedsInitialization = true; 
	FRuntimeMeshDelayedActionTickObject::GetInstance().RegisterForUpdate(TWeakObjectPtr<URuntimeMesh>(this));
}


