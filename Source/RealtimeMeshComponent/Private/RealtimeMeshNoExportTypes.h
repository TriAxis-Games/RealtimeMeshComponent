// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once
// Help intellisense to avoid interpreting this file's declaration of FVector etc as it assumes !CPP by default
#ifndef CPP
#define CPP 1
#endif

#if CPP

#include "CoreFwd.h"
#include "Materials/MaterialInterface.h"
#include "Core/RealtimeMeshMaterial.h"
#include "Core/RealtimeMeshStreamRange.h"
#include "Core/RealtimeMeshSectionConfig.h"
#include "Core/RealtimeMeshSectionGroupConfig.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "Core/RealtimeMeshConfig.h"
#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshCollision.h"
#include "RealtimeMeshNoExportTypes.generated.h"

#endif

#if !CPP      //noexport struct

UENUM(BlueprintType)
enum class ERealtimeMeshProxyUpdateStatus : uint8
{
	NoProxy,
	NoUpdate,
	Updated,
};

UENUM(BlueprintType)
enum class ERealtimeMeshOutcomePins : uint8
{
	Failure,
	Success
};


USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshPolygonGroupRange
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	int32 StartIndex;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	int32 Count;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	int32 PolygonGroupIndex;
};


USTRUCT(NoExport, BlueprintType, meta=(HasNativeMake="RealtimeMeshComponent.RealtimeMeshBlueprintFunctionLibrary.MakeStreamRange"))
struct FRealtimeMeshStreamRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Streams")
	FInt32Range Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Streams")
	FInt32Range Indices;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshMaterialSlot
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Material")
	FName SlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Material")
	TObjectPtr<UMaterialInterface> Material;
};

UENUM(BlueprintType)
enum class ERealtimeMeshSectionDrawType : uint8
{
	Static,
	Dynamic,
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshSectionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	int32 MaterialSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	bool bIsVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	bool bCastsShadow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config", AdvancedDisplay)
	bool bIsMainPassRenderable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config", AdvancedDisplay)
	bool bForceOpaque;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshSectionGroupConfig
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|SectionGroup|Config", AdvancedDisplay)
	ERealtimeMeshSectionDrawType DrawType;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshLODConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	bool bIsVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	float ScreenSize;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	int32 ForcedLOD;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshLODKey
{
	GENERATED_BODY()
	
	int8 LODIndex;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshSectionGroupKey
{
	GENERATED_BODY()

private:
	FName GroupName;
	int8 LODIndex;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshSectionKey
{
	GENERATED_BODY()

private:
	FName GroupName;
	FName SectionName;
	int8 LODIndex;
};

UENUM(BlueprintType)
enum class ERealtimeMeshStreamType : uint8
{
	Unknown,
	Vertex,
	Index,
};


USTRUCT(NoExport, BlueprintType,  meta=(HasNativeMake="RealtimeMeshComponent.RealtimeMeshBlueprintFunctionLibrary.MakeStreamKey"))
struct FRealtimeMeshStreamKey
{
	GENERATED_BODY()
private:
	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	ERealtimeMeshStreamType StreamType;
	
	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	FName StreamName;
};


UENUM(BlueprintType)
enum class ERealtimeMeshCollisionUpdateResult : uint8
{
	Unknown,
	Updated,
	Ignored,
	Error,
};


USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionConfiguration
{
	GENERATED_BODY()

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bUseComplexAsSimpleCollision;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bUseAsyncCook;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bShouldFastCookMeshes;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bFlipNormals;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bDeformableMesh;
	
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bMergeAllMeshes;
};


USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionShape
{
	GENERATED_BODY()
	
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FVector Center;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FRotator Rotation;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bContributesToMass;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionSphere : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()
	
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Radius;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionBox : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()
	
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FVector Extents;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionCapsule : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()
	
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Radius;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Length;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionTaperedCapsule : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()
	
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float RadiusA;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float RadiusB;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Length;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionConvex : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()
private:
	TArray<FVector> Vertices;
	FBox BoundingBox;
	mutable TSharedPtr<FRealtimeMeshCookedConvexMeshData> Cooked;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshSimpleGeometry
{
	GENERATED_BODY()

private:
	FSimpleShapeSet<FRealtimeMeshCollisionSphere> Spheres;
	FSimpleShapeSet<FRealtimeMeshCollisionBox> Boxes;
	FSimpleShapeSet<FRealtimeMeshCollisionCapsule> Capsules;
	FSimpleShapeSet<FRealtimeMeshCollisionTaperedCapsule> TaperedCapsules;
	FSimpleShapeSet<FRealtimeMeshCollisionConvex> ConvexHulls;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionMesh
{
	GENERATED_BODY()
private:	
	TArray<FVector3f> Vertices;
	TArray<RealtimeMesh::TIndex3<int32>> Triangles;
	TArray<uint16> Materials;
	TArray<TArray<FVector2f>> TexCoords;
	bool bFlipNormals;
	
	mutable TSharedPtr<FRealtimeMeshCookedTriMeshData> Cooked;
	
public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FName Name;
};


USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshComplexGeometry
{
	GENERATED_BODY()

private:
	TSparseArray<FRealtimeMeshCollisionMesh> Meshes;
	TMap<FName, int32> MeshesNameMap;
};

USTRUCT(NoExport, BlueprintType)
struct FRealtimeMeshCollisionInfo
{
	GENERATED_BODY()

public:
	FRealtimeMeshSimpleGeometry SimpleGeometry;
	FRealtimeMeshComplexGeometry ComplexGeometry;
	FRealtimeMeshCollisionConfiguration Configuration;
};



#endif