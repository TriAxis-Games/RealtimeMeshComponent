// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"

namespace RealtimeMesh
{
	class IRealtimeMesh_v0;

	class IRealtimeMeshInterface_v0 : public IModularFeature
	{
	public:
		static FName GetModularFeatureName()
		{
			static FName FeatureName = TEXT("IRealtimeMeshInterface_v0");
			return FeatureName;
		}
	
		virtual ~IRealtimeMeshInterface_v0() = default;

	

		virtual bool IsRealtimeMesh(UMeshComponent* MeshComponent) const = 0;
		virtual UMeshComponent* CreateRealtimeMeshComponent(AActor* Owner, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags) const = 0;
		virtual void SetRealtimeMesh(UMeshComponent* MeshComponent, UObject* Mesh) const = 0;
		/*virtual TSharedRef<IRealtimeMesh_v0> GetRealtimeMesh(UMeshComponent* MeshComponent) const = 0;*/
	
	};
}