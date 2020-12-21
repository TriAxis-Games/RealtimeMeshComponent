// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshActor.generated.h"

UCLASS(HideCategories = (Input), ShowCategories = ("Input|MouseInput", "Input|TouchInput"), ComponentWrapperClass, Meta = (ChildCanTick))
class RUNTIMEMESHCOMPONENT_API ARuntimeMeshActor : public AActor
{
	GENERATED_UCLASS_BODY()
		
private:
	UPROPERTY(Category = "RuntimeMeshActor", VisibleAnywhere, BlueprintReadOnly, Meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|RuntimeMesh", AllowPrivateAccess = "true"))
	class URuntimeMeshComponent* RuntimeMeshComponent;
	
public:

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshActor", Meta = (DisplayName = "Get Mobility"))
	ERuntimeMeshMobility GetRuntimeMeshMobility();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshActor", Meta = (DisplayName = "Set Mobility"))
	void SetRuntimeMeshMobility(ERuntimeMeshMobility NewMobility);

public:

	/** Function to change mobility type */
	void SetMobility(EComponentMobility::Type InMobility);
	EComponentMobility::Type GetMobility();

	/** Returns RuntimeMeshComponent subobject **/
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshActor", Meta = (DisplayName = "Get Runtime Mesh Component"))
	class URuntimeMeshComponent* GetRuntimeMeshComponent() const { return RuntimeMeshComponent; }


};
