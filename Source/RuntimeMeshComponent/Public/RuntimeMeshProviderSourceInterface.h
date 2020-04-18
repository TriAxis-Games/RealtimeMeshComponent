//// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "UObject/Interface.h"
//#include "RuntimeMeshRenderable.h"
//#include "RuntimeMeshCollision.h"
//#include "RuntimeMeshProviderSourceInterface.generated.h"
//
//class URuntimeMeshProviderSourceInterface;
//class IRuntimeMeshProviderSourceInterface;
//class URuntimeMeshProviderTargetInterface;
//class IRuntimeMeshProviderTargetInterface;
//
//
//
//
//
//
//UINTERFACE(BlueprintType, Blueprintable)
//class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderSourceInterface : public UInterface
//{
//	GENERATED_UINTERFACE_BODY()
//};
//
//class RUNTIMEMESHCOMPONENT_API IRuntimeMeshProviderSourceInterface
//{
//	GENERATED_IINTERFACE_BODY()
//protected:
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	void BindTargetProvider(const TScriptInterface<IRuntimeMeshProviderTargetInterface>& InTarget);
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	void Unlink();
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	void Initialize();
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	FBoxSphereBounds GetBounds();
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData);
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas);
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	FRuntimeMeshCollisionSettings GetCollisionSettings();
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	bool HasCollisionMesh();
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData);
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	void CollisionUpdateCompleted();
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
//	bool IsThreadSafe();
//
//	friend class URuntimeMesh;
//};
