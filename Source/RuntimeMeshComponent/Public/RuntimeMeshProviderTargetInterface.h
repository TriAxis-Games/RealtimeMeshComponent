// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

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



	UFUNCTION(BlueprintCallable)
	virtual void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs) { }

	UFUNCTION(BlueprintCallable)
	virtual void SetLODScreenSize(int32 LODIndex, float ScreenSize) { }

	UFUNCTION(BlueprintCallable)
	virtual void MarkLODDirty(int32 LODIndex) { }

	UFUNCTION(BlueprintCallable)
	virtual void MarkAllLODsDirty() { }


	UFUNCTION(BlueprintCallable)
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) { }

	UFUNCTION(BlueprintCallable)
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) { }

	UFUNCTION(BlueprintCallable)
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) { }

	UFUNCTION(BlueprintCallable)
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) { }

	UFUNCTION(BlueprintCallable)
	virtual void ClearSection(int32 LODIndex, int32 SectionId) { }

	UFUNCTION(BlueprintCallable)
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) { }

	UFUNCTION(BlueprintCallable)
	virtual void MarkCollisionDirty() { }


	UFUNCTION(BlueprintCallable)
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) { }

	UFUNCTION(BlueprintCallable)
	virtual int32 GetMaterialIndex(FName MaterialSlotName) { return INDEX_NONE; }

	UFUNCTION(BlueprintCallable)
	virtual bool IsMaterialSlotNameValid(FName MaterialSlotName) const { return false; }

	UFUNCTION(BlueprintCallable)
	virtual FRuntimeMeshMaterialSlot GetMaterialSlot(int32 SlotIndex) { return FRuntimeMeshMaterialSlot(); }

	UFUNCTION(BlueprintCallable)
	virtual int32 GetNumMaterials() { return 0; }

	UFUNCTION(BlueprintCallable)
	virtual TArray<FName> GetMaterialSlotNames() { return TArray<FName>(); }

	UFUNCTION(BlueprintCallable)
	virtual TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots() { return TArray<FRuntimeMeshMaterialSlot>(); }

	UFUNCTION(BlueprintCallable)
	virtual UMaterialInterface* GetMaterial(int32 SlotIndex) { return nullptr; }
};
