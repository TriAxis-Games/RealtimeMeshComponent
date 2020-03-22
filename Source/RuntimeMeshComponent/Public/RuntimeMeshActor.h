// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

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
	// This is purposefully not a UPROPERTY as we want it to not store across sessions
	bool bHasGeneratedThisRun;

private:
	UPROPERTY(Category = "RuntimeMeshActor", VisibleAnywhere, BlueprintReadOnly, Meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|RuntimeMesh", AllowPrivateAccess = "true"))
	class URuntimeMeshComponent* RuntimeMeshComponent;

protected:
	UPROPERTY(Category = "RuntimeMeshActor", EditAnywhere, BlueprintReadOnly, Meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|RuntimeMesh", AllowPrivateAccess = "true"))
	bool GenerateOnBeginPlayInGame;

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


	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMeshActor")
	void GenerateMeshes();

};
