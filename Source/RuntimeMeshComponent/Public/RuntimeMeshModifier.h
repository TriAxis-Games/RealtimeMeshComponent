// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RuntimeMeshReference.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshModifier.generated.h"


UCLASS(HideCategories = Object, BlueprintType, Blueprintable, Meta = (ShortTooltip = "A RuntimeMeshModifier is a class containing logic to modify a mesh data passed to it in some way. This can range from optimizers, to additional data generators."))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshModifier : public UObject
{
	GENERATED_BODY()
public:
	URuntimeMeshModifier();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="RuntimeMeshModifier")
	void ApplyToMesh(FRuntimeMeshRenderableMeshData& MeshData);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RuntimeMeshModifier")
	void ApplyToCollisionMesh(FRuntimeMeshCollisionData& MeshData);

};
