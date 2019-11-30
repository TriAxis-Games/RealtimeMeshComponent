// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshActor.generated.h"

UCLASS(HideCategories = (Input), ShowCategories = ("Input|MouseInput", "Input|TouchInput"), ComponentWrapperClass, Meta = (ChildCanTick))
class REALTIMEMESHCOMPONENT_API ARealtimeMeshActor : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(Category = "RealtimeMeshActor", VisibleAnywhere, BlueprintReadOnly, Meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|RealtimeMesh", AllowPrivateAccess = "true"))
	class URealtimeMeshComponent* RealtimeMeshComponent;

public:

	UFUNCTION(BlueprintCallable, Category = "RealtimeMeshActor", Meta = (AllowPrivateAccess = "true", DisplayName = "Get Mobility"))
	ERealtimeMeshMobility GetRealtimeMeshMobility();

	UFUNCTION(BlueprintCallable, Category = "RealtimeMeshActor", Meta = (AllowPrivateAccess = "true", DisplayName = "Set Mobility"))
	void SetRealtimeMeshMobility(ERealtimeMeshMobility NewMobility);

public:

	/** Function to change mobility type */
	void SetMobility(EComponentMobility::Type InMobility);

	EComponentMobility::Type GetMobility();

	/** Returns RealtimeMeshComponent subobject **/
	class URealtimeMeshComponent* GetRealtimeMeshComponent() const { return RealtimeMeshComponent; }


	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RealtimeMeshActor")
	void GenerateMeshes();

};
