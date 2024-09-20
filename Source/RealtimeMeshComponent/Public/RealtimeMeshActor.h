// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RealtimeMeshComponent.h"
#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "RealtimeMeshActor.generated.h"

UCLASS(ConversionRoot, ComponentWrapperClass, ClassGroup=RealtimeMesh, meta = (ChildCanTick))
class REALTIMEMESHCOMPONENT_API ARealtimeMeshActor : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(Category = "RealtimeMesh", VisibleAnywhere, BlueprintReadOnly,
		meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|StaticMesh", AllowPrivateAccess = "true"))
	TObjectPtr<class URealtimeMeshComponent> RealtimeMeshComponent;
	
public:
	ARealtimeMeshActor();

	UFUNCTION(Category = RealtimeMeshActor, BlueprintCallable)
	URealtimeMeshComponent* GetRealtimeMeshComponent() const { return RealtimeMeshComponent; }
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh")
	URealtimeMeshStream* MakeStream(const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh")
	URealtimeMeshStreamSet* MakeStreamSet();
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh")
	URealtimeMeshLocalBuilder* MakeMeshBuilder(ERealtimeMeshSimpleStreamConfig WantedTangents = ERealtimeMeshSimpleStreamConfig::Normal,
		ERealtimeMeshSimpleStreamConfig WantedTexCoords = ERealtimeMeshSimpleStreamConfig::Normal,
		bool bWants32BitIndices = false,
		ERealtimeMeshSimpleStreamConfig WantedPolyGroupType = ERealtimeMeshSimpleStreamConfig::None,
		bool bWantsColors = true, int32 WantedTexCoordChannels = 1, bool bKeepExistingData = true);


	//~ Begin UObject/AActor Interface
	virtual void BeginPlay() override;
};