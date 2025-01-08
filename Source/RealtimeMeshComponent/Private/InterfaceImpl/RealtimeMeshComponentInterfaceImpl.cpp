// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshComponent.h"
#include "Core/RealtimeMeshComponentInterface.h"
#include "Core/RealtimeMeshModularFeatures.h"


namespace RealtimeMesh
{
	struct FRealtimeMeshInterfaceImpl_v0 : public IRealtimeMeshInterface_v0
	{

		virtual bool IsRealtimeMesh(UMeshComponent* MeshComponent) const override
		{
			return MeshComponent->IsA<URealtimeMeshComponent>();
		}
		virtual UMeshComponent* CreateRealtimeMeshComponent(AActor* Owner, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags) const override
		{
			return NewObject<URealtimeMeshComponent>(Owner, Name, Flags);
		}
		/*virtual TSharedRef<IRealtimeMesh_v0> GetRealtimeMesh(UMeshComponent* MeshComponent) const override
		{
			URealtimeMeshComponent* RealtimeMeshComp = CastChecked<URealtimeMeshComponent>(MeshComponent);
			return nullptr;
		}*/
	};
	
	// Register the interface
	TRealtimeMeshModularFeatureRegistration<FRealtimeMeshInterfaceImpl_v0> GRealtimeMeshInterfaceImpl_v0;
}