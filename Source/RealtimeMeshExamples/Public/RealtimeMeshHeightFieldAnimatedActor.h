// Copyright TriAxis Games, L.L.C. & xixgames All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "Mesh/RealtimeMeshBuilder.h"
#include "RealtimeMeshHeightFieldAnimatedActor.generated.h"

UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshHeightFieldAnimatedActor : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshHeightFieldAnimatedActor();
	
	// Called when the mesh generation should happen. This could be called in the
	// editor for placed actors, or at runtime for spawned actors.
	virtual void OnGenerateMesh_Implementation() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	FVector Size = FVector(1000.0f, 1000.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float ScaleFactor = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	int32 LengthSections = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	int32 WidthSections = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	bool bAnimateMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float AnimationSpeedX = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float AnimationSpeedY = 4.5f;
	
	virtual void Tick(float DeltaSeconds) override;

protected:
	float CurrentAnimationFrameX = 0.0f;
	float CurrentAnimationFrameY = 0.0f;

private:
	void GenerateMesh(RealtimeMesh::TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1>& Builder);
	void SetupMesh();
	void GeneratePoints();
	void GenerateGrid(const FVector2D InSize, const int32 InLengthSections,
		const int32 InWidthSections, const TArray<float>& InHeightValues,
		RealtimeMesh::TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1>& Builder);
	
	TArray<float> HeightValues;
	float MaxHeightValue = 0.0f;
};
