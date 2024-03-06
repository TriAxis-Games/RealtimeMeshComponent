// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#pragma once

#include "RealtimeMeshBuilder.h"
#include "RealtimeMeshCore.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshSimpleData.generated.h"

using namespace RealtimeMesh;

struct UE_DEPRECATED(All, "FRealtimeMeshSimpleMeshData has been deprecated in favor of using the FRealtimeMeshStreamSet which is far more configurable and efficient") FRealtimeMeshSimpleMeshData;

USTRUCT(BlueprintType, meta=(HasNativeMake="RealtimeMeshComponent.RealtimeMeshSimpleBlueprintFunctionLibrary.MakeRealtimeMeshSimpleStream"))
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSimpleMeshData
{
	GENERATED_BODY()
public:
	FRealtimeMeshSimpleMeshData()
		: bUseHighPrecisionTangents(false)
		, bUseHighPrecisionTexCoords(false)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<int32> Triangles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Positions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Normals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Tangents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector> Binormals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FLinearColor> LinearColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector2D> UV0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FColor> Colors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<int32> MaterialIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	bool bUseHighPrecisionTangents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	bool bUseHighPrecisionTexCoords;


	bool CopyToStreamSet(FRealtimeMeshStreamSet& Streams, bool bCreateMissingStreams) const;
};

UCLASS(meta=(ScriptName="RealtimeMeshSimpleLibrary"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshSimpleBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UE_DEPRECATED(all, "Use FRealtimeMeshStreamSet instead")
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|Simple",
		meta=(AdvancedDisplay="Triangles,Positions,Normals,Tangents,Binormals,LinearColors,UV0,UV1,UV2,UV3,Colors,bUseHighPrecisionTangents,bUseHighPrecisionTexCoords",
			AutoCreateRefTerm="Triangles,Positions,Normals,Tangents,Binormals,LinearColors,UV0,UV1,UV2,UV3,Colors"))
	static FRealtimeMeshSimpleMeshData MakeRealtimeMeshSimpleStream(
		const TArray<int32>& Triangles,
		const TArray<FVector>& Positions,
		const TArray<FVector>& Normals,
		const TArray<FVector>& Tangents,
		const TArray<FVector>& Binormals,
		const TArray<FLinearColor>& LinearColors,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FVector2D>& UV2,
		const TArray<FVector2D>& UV3,
		const TArray<FColor>& Colors,
		bool bUseHighPrecisionTangents,
		bool bUseHighPrecisionTexCoords);
};