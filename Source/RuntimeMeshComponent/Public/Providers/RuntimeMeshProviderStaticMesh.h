//// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "RuntimeMeshProvider.h"
//#include "RuntimeMeshStaticMeshConverter.h"
//#include "RuntimeMeshProviderStaticMesh.generated.h"
//
///**
// * 
// */
//UCLASS()
//class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderStaticMesh : public URuntimeMeshProvider
//{
//	GENERATED_BODY()
//
//public:
//	UPROPERTY(Category = "RuntimeMesh|Providers|StaticMesh", EditAnywhere, BlueprintReadWrite)
//	UStaticMesh* StaticMesh;
//
//	UPROPERTY(Category = "RuntimeMesh|Providers|StaticMesh", EditAnywhere, BlueprintReadWrite)
//	int32 MaxLOD;
//
//	UPROPERTY(Category = "RuntimeMesh|Providers|StaticMesh", EditAnywhere, BlueprintReadWrite)
//	int32 ComplexCollisionLOD;
//	
//protected:
//	URuntimeMeshProviderStaticMesh();
//
//	virtual void Initialize_Implementation() override;
//
//	virtual FBoxSphereBounds GetBounds_Implementation() override;
//
//	virtual bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
//
//	virtual FRuntimeMeshCollisionSettings GetCollisionSettings_Implementation() override;
//	virtual bool HasCollisionMesh_Implementation() override;
//	virtual bool GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData) override;
//};
