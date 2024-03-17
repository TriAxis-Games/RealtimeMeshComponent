// Copyright TriAxis Games, L.L.C. & xixgames All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "Mesh/RealtimeMeshBuilder.h"
#include "RealtimeMeshBranchingLinesActor.generated.h"

// A simple struct to keep realtime mesh branch segment data together
USTRUCT(BlueprintType)
struct REALTIMEMESHEXAMPLES_API FRealtimeMeshBranchSegment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	FVector End = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float Width = 0.f;

	UPROPERTY()
	int8 ForkGeneration = 0;

	FRealtimeMeshBranchSegment()
	{
		Start = FVector::ZeroVector;
		End = FVector::ZeroVector;
		Width = 1.f;
		ForkGeneration = 0;
	}

	FRealtimeMeshBranchSegment(const FVector& InStart, const FVector& InEnd)
	{
		Start = InStart;
		End = InEnd;
		Width = 1.f;
		ForkGeneration = 0;
	}

	FRealtimeMeshBranchSegment(const FVector& InStart, const FVector& InEnd, float InWidth)
	{
		Start = InStart;
		End = InEnd;
		Width = InWidth;
		ForkGeneration = 0;
	}

	FRealtimeMeshBranchSegment(const FVector& InStart, const FVector& InEnd, float InWidth, int8 InForkGeneration)
	{
		Start = InStart;
		End = InEnd;
		Width = InWidth;
		ForkGeneration = InForkGeneration;
	}
};

UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshBranchingLinesActor : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshBranchingLinesActor();
	
	// Called when the mesh generation should happen. This could be called in the
	// editor for placed actors, or at runtime for spawned actors.
	virtual void OnGenerateMesh_Implementation() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters", meta = (MakeEditWidget))
	FVector Start = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters", meta = (MakeEditWidget))
	FVector End = FVector(0, 0, 300);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	uint8 Iterations = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	int32 RadialSegmentCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	bool bSmoothNormals = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	int32 RandomSeed = 1238;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters", meta = (UIMin = "0.1", ClampMin = "0.1"))
	float MaxBranchOffset = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	bool bMaxBranchOffsetAsPercentageOfLength = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float BranchOffsetReductionEachGenerationPercentage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float TrunkWidth = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters", meta = (UIMin = "0", UIMax = "100", ClampMin = "0", ClampMax = "100"))
	float ChanceOfForkPercentage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float WidthReductionOnFork = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float ForkLengthMin = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float ForkLengthMax = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float ForkRotationMin = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	float ForkRotationMax = 40.0f;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Parameters")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	// Setup random offset directions
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, EditFixedSize, Category = "Procedural Parameters", meta = (ToolTip = "Minimum : 2 dimensions"))
	TArray<FVector> OffsetDirections = {FVector(1., 0., 0.), FVector(0., 0., 1.)};
private:
	void GenerateMesh(RealtimeMesh::TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1>& Builder);
	void CreateSegments();
	void GenerateCylinder(const FVector& StartPoint, const FVector& EndPoint, const float InWidth,
		const int32 InCrossSectionCount,
		RealtimeMesh::TRealtimeMeshBuilderLocal<unsigned short>& Builder,
		const bool bInSmoothNormals = true);

	static FVector RotatePointAroundPivot(const FVector& InPoint, const FVector& InPivot, const FVector& InAngles);
	void PreCacheCrossSection();
	
	TArray<FRealtimeMeshBranchSegment> Segments;
	int32 LastCachedCrossSectionCount = 0;
	TArray<FVector> CachedCrossSectionPoints;
	FRandomStream RngStream;
};
