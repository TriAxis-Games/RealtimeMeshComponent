// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshReference.h"
#include "RuntimeMeshProviderTargetInterface.generated.h"

class URuntimeMesh;

UCLASS(BlueprintType, Blueprintable)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderTargetInterface : public UObject
{
	GENERATED_BODY()
public:
	virtual FRuntimeMeshWeakRef GetMeshReference() { return FRuntimeMeshWeakRef(); };

	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	virtual void UnlinkProviders() { }



	UFUNCTION(BlueprintCallable, Category="RuntimeMeshInterface|ConfigureLODs")
	virtual void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void SetLODScreenSize(int32 LODIndex, float ScreenSize) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void MarkLODDirty(int32 LODIndex) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void MarkAllLODsDirty() { }


	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void ClearSection(int32 LODIndex, int32 SectionId) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void MarkCollisionDirty() { }


	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual int32 GetMaterialIndex(FName MaterialSlotName) { return INDEX_NONE; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual bool IsMaterialSlotNameValid(FName MaterialSlotName) const { return false; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual FRuntimeMeshMaterialSlot GetMaterialSlot(int32 SlotIndex) { return FRuntimeMeshMaterialSlot(); }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual int32 GetNumMaterials() { return 0; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual TArray<FName> GetMaterialSlotNames() { return TArray<FName>(); }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots() { return TArray<FRuntimeMeshMaterialSlot>(); }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshInterface|ConfigureLODs")
	virtual UMaterialInterface* GetMaterial(int32 SlotIndex) { return nullptr; }
};
