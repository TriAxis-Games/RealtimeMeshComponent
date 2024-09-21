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
	UPROPERTY(Category = "RealtimeMeshActor", VisibleAnywhere, BlueprintReadOnly,
		meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|StaticMesh", AllowPrivateAccess = "true"))
	TObjectPtr<class URealtimeMeshComponent> RealtimeMeshComponent;

public:

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


	/**
	 * If true, the then OnGenerateMesh will get called, and you should move
	 * your mesh generation logic there, instead of the construction script.
	 * This helps improve editor performace by not running generation in
	 * construction which fires every frame when dragging an actor in the editor.
	 */
	UPROPERTY(Category = "RealtimeMeshActor", EditAnywhere, BlueprintReadWrite)
	bool bDeferGeneration = false;
	
	/**
	 * If true, the RealtimeMeshComponent will be "Frozen" in its current state, and automatic rebuilding
	 * will be disabled. However the RealtimeMesh can still be modified by explicitly-called functions/etc.
	 */
	UPROPERTY(Category = "RealtimeMeshActor", EditAnywhere, BlueprintReadWrite)
	bool bFrozen = false;

	/** If true, the RealtimeMeshComponent will be cleared before the OnRebuildGeneratedMesh event is executed. */
	UPROPERTY(Category = "RealtimeMeshActor|Advanced", EditAnywhere, BlueprintReadWrite)
	bool bResetOnRebuild = true;

public:
	ARealtimeMeshActor();
	virtual ~ARealtimeMeshActor() override;

	UFUNCTION(Category = RealtimeMeshActor, BlueprintCallable)
	URealtimeMeshComponent* GetRealtimeMeshComponent() const { return RealtimeMeshComponent; }

	/**
	 * This event will be fired to notify the BP that the generated Mesh should
	 * be rebuilt. GeneratedRealtimeMeshActor BP subclasses should rebuild their 
	 * meshes on this event, instead of doing so directly from the Construction Script.
	 */
	UFUNCTION(BlueprintNativeEvent, CallInEditor, Category = "Events")
	void OnGenerateMesh();

	virtual void OnGenerateMesh_Implementation()
	{
	}


	/**
	 * This function will fire the OnRebuildGeneratedMesh event if the actor has been
	 * marked for a pending rebuild (eg via OnConstruction)
	 */
	virtual void ExecuteRebuildGeneratedMeshIfPending();

public:
	//~ Begin UObject/AActor Interface
	virtual void BeginPlay() override;
	virtual void PostLoad() override;
	virtual void PostActorCreated() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

	// these are called when Actor exists in a sublevel that is hidden/shown
	virtual void PreRegisterAllComponents() override;
	virtual void PostUnregisterAllComponents() override;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

protected:
	// this internal flag is set in OnConstruction, and will cause ExecuteRebuildGeneratedMesh to
	// fire the OnRebuildGeneratedMesh event, after which the flag will be cleared
	bool bGeneratedMeshRebuildPending = false;

	// indicates that this Actor is registered with the UEditorGeometryGenerationSubsystem, which 
	// is where the mesh rebuilds are executed
	bool bIsRegisteredWithGenerationManager = false;

	void RegisterWithGenerationManager();
	void UnregisterWithGenerationManager();
};