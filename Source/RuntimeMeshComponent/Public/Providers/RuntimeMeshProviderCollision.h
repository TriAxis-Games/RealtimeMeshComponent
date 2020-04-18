// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
//#include "RuntimeMeshProviderCollision.generated.h"



//UCLASS(HideCategories = Object, BlueprintType)
//class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderCollisionFromRenderable : public URuntimeMeshProvider
//{
//	GENERATED_BODY()
//
//private:
//	int32 LODForMeshCollision;
//	TMap<int32, TArray<FVector>> RenderableCollisionData;
//
//	FRuntimeMeshCollisionSettings CollisionSettings;
//	FRuntimeMeshCollisionData CollisionMesh;
//
//	FCriticalSection SyncRoot;
//
//	friend class FRuntimeMeshProviderCollisionFromRenderableProxy;
//public:
//
//	URuntimeMeshProviderCollisionFromRenderable();
//
//	UPROPERTY(EditAnywhere)
//	URuntimeMeshProvider* SourceProvider;
//
//	UFUNCTION(BlueprintCallable)
//	void SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings);
//
//	UFUNCTION(BlueprintCallable)
//		void SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh);
//
//	UFUNCTION(BlueprintCallable)
//		void SetRenderableLODForCollision(int32 LODIndex);
//
//	UFUNCTION(BlueprintCallable)
//	void SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled);
//
//
//
//protected:
//	virtual bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
//
//	virtual bool GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) override;
//
//	virtual void ClearSection_Implementation(int32 LODIndex, int32 SectionId) override;
//
//	virtual void RemoveSection_Implementation(int32 LODIndex, int32 SectionId) override;
//
//	virtual bool IsThreadSafe_Implementation() const override { return true; }
//};