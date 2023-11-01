// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interface_CollisionDataProviderCore.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshCollision.generated.h"

class UBodySetup;


USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionConfiguration
{
	GENERATED_BODY()

public:
	FRealtimeMeshCollisionConfiguration()
		: bUseComplexAsSimpleCollision(true)
		  , bUseAsyncCook(true)
		  , bShouldFastCookMeshes(false)
		  , bFlipNormals(false)
		  , bDeformableMesh(false)
	{
	}

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


	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConfiguration& Config);
};


USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	FRealtimeMeshCollisionShape()
		: Name(NAME_None)
		  , Center(FVector::ZeroVector)
		  , Rotation(FRotator::ZeroRotator)
		  , bContributesToMass(true)
	{
	}

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FVector Center;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FRotator Rotation;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bContributesToMass;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionShape& Shape);
};


USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionSphere : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	FRealtimeMeshCollisionSphere()
		: Radius(0.5f)
	{
	}

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Radius;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionSphere& Shape);
};

USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionBox : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	FRealtimeMeshCollisionBox(const FVector& InExtents = FVector(0.5f, 0.5f, 0.5f))
		: Extents(InExtents)
	{
	}

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FVector Extents;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionBox& Shape);
};


USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionCapsule : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	FRealtimeMeshCollisionCapsule()
		: Radius(0.5f)
		  , Length(0.5f)
	{
	}

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Radius;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Length;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionCapsule& Shape);
};

USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionTaperedCapsule : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	FRealtimeMeshCollisionTaperedCapsule()
		: RadiusA(0.5f)
		  , RadiusB(0.5f)
		  , Length(0.5f)
	{
	}

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float RadiusA;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float RadiusB;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Length;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionTaperedCapsule& Shape);
};

USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionConvex : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	FRealtimeMeshCollisionConvex()
	{
	}

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	TArray<FVector> Vertices;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FBox BoundingBox;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConvex& Shape);
};


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSimpleGeometry
{
	GENERATED_BODY()

private:
	TSparseArray<FRealtimeMeshCollisionSphere> Spheres;
	TMap<FName, int32> SpheresNameMap;
	TSparseArray<FRealtimeMeshCollisionBox> Boxes;
	TMap<FName, int32> BoxesNameMap;
	TSparseArray<FRealtimeMeshCollisionCapsule> Capsules;
	TMap<FName, int32> CapsulesNameMap;
	TSparseArray<FRealtimeMeshCollisionTaperedCapsule> TaperedCapsules;
	TMap<FName, int32> TaperedCapsulesNameMap;
	TSparseArray<FRealtimeMeshCollisionConvex> ConvexHulls;
	TMap<FName, int32> ConvexHullsNameMap;

public:
	// Sphere Functions
	int32 AddSphere(const FRealtimeMeshCollisionSphere& InSphere);
	bool InsertSphere(int32 Index, const FRealtimeMeshCollisionSphere& InSphere);
	int32 GetSphereIndexByName(FName SphereName) const;
	bool GetSphereByName(FName SphereName, FRealtimeMeshCollisionSphere& OutSphere);
	bool UpdateSphere(int32 Index, const FRealtimeMeshCollisionSphere& InSphere);
	bool RemoveSphere(int32 Index);
	bool RemoveSphere(FName SphereName);

	// Box Functions
	int32 AddBox(const FRealtimeMeshCollisionBox& InBox);
	bool InsertBox(int32 Index, const FRealtimeMeshCollisionBox& InBox);
	int32 GetBoxIndexByName(FName BoxName) const;
	bool GetBoxByName(FName BoxName, FRealtimeMeshCollisionBox& OutBox);
	bool UpdateBox(int32 Index, const FRealtimeMeshCollisionBox& InBox);
	bool RemoveBox(int32 Index);
	bool RemoveBox(FName BoxName);

	// Capsule Functions
	int32 AddCapsule(const FRealtimeMeshCollisionCapsule& InCapsule);
	bool InsertCapsule(int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule);
	int32 GetCapsuleIndexByName(FName CapsuleName) const;
	bool GetCapsuleByName(FName CapsuleName, FRealtimeMeshCollisionCapsule& OutCapsule);
	bool UpdateCapsule(int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule);
	bool RemoveCapsule(int32 Index);
	bool RemoveCapsule(FName CapsuleName);

	// Tapered Capsule Functions
	int32 AddTaperedCapsule(const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule);
	bool InsertTaperedCapsule(int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule);
	int32 GetTaperedCapsuleIndexByName(FName TaperedCapsuleName) const;
	bool GetTaperedCapsuleByName(FName TaperedCapsuleName, FRealtimeMeshCollisionTaperedCapsule& OutTaperedCapsule);
	bool UpdateTaperedCapsule(int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule);
	bool RemoveTaperedCapsule(int32 Index);
	bool RemoveTaperedCapsule(FName TaperedCapsuleName);

	// Convex Hull Functions
	int32 AddConvexHull(const FRealtimeMeshCollisionConvex& InConvexHull);
	bool InsertConvexHull(int32 Index, const FRealtimeMeshCollisionConvex& InConvexHull);
	int32 GetConvexHullIndexByName(FName ConvexHullName) const;
	bool GetConvexHullByName(FName ConvexHullName, FRealtimeMeshCollisionConvex& OutConvexHull);
	bool UpdateConvexHull(int32 Index, const FRealtimeMeshCollisionConvex& InConvexHull);
	bool RemoveConvexHull(int32 Index);
	bool RemoveConvexHull(FName ConvexHullName);

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshSimpleGeometry& SimpleGeometry);

	void CopyToBodySetup(UBodySetup* BodySetup) const;
};


// ReSharper disable CppUEBlueprintCallableFunctionUnused
UCLASS()
class REALTIMEMESHCOMPONENT_API URealtimeMeshSimpleGeometryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Sphere Functions
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Spheres")
	static FRealtimeMeshSimpleGeometry& AddSphere(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, const FRealtimeMeshCollisionSphere& InSphere, int32& OutIndex);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Spheres")
	static FRealtimeMeshSimpleGeometry& InsertSphere(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionSphere& InSphere, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Spheres")
	static FRealtimeMeshSimpleGeometry& GetSphereByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName SphereName, bool& Success, FRealtimeMeshCollisionSphere& OutSphere);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Spheres")
	static FRealtimeMeshSimpleGeometry& UpdateSphere(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionSphere& InSphere, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Spheres")
	static FRealtimeMeshSimpleGeometry& RemoveSphere(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Spheres")
	static FRealtimeMeshSimpleGeometry& RemoveSphereByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName SphereName, bool& Success);

	// Box Functions
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Boxes")
	static FRealtimeMeshSimpleGeometry& AddBox(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, const FRealtimeMeshCollisionBox& InBox, int32& OutIndex);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Boxes")
	static FRealtimeMeshSimpleGeometry& InsertBox(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionBox& InBox, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Boxes")
	static FRealtimeMeshSimpleGeometry& GetBoxByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName BoxName, bool& Success, FRealtimeMeshCollisionBox& OutBox);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Boxes")
	static FRealtimeMeshSimpleGeometry& UpdateBox(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionBox& InBox, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Boxes")
	static FRealtimeMeshSimpleGeometry& RemoveBox(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Boxes")
	static FRealtimeMeshSimpleGeometry& RemoveBoxByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName BoxName, bool& Success);

	// Capsule Functions
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Capsules")
	static FRealtimeMeshSimpleGeometry& AddCapsule(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, const FRealtimeMeshCollisionCapsule& InCapsule, int32& OutIndex);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Capsules")
	static FRealtimeMeshSimpleGeometry& InsertCapsule(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Capsules")
	static FRealtimeMeshSimpleGeometry& GetCapsuleByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName CapsuleName, bool& Success, FRealtimeMeshCollisionCapsule& OutCapsule);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Capsules")
	static FRealtimeMeshSimpleGeometry& UpdateCapsule(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Capsules")
	static FRealtimeMeshSimpleGeometry& RemoveCapsule(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Capsules")
	static FRealtimeMeshSimpleGeometry& RemoveCapsuleByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName CapsuleName, bool& Success);

	// Tapered Capsule Functions
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Tapered Capsules")
	static FRealtimeMeshSimpleGeometry& AddTaperedCapsule(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule, int32& OutIndex);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Tapered Capsules")
	static FRealtimeMeshSimpleGeometry& InsertTaperedCapsule(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Tapered Capsules")
	static FRealtimeMeshSimpleGeometry& GetTaperedCapsuleByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName TaperedCapsuleName, bool& Success, FRealtimeMeshCollisionTaperedCapsule& OutTaperedCapsule);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Tapered Capsules")
	static FRealtimeMeshSimpleGeometry& UpdateTaperedCapsule(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Tapered Capsules")
	static FRealtimeMeshSimpleGeometry& RemoveTaperedCapsule(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Tapered Capsules")
	static FRealtimeMeshSimpleGeometry& RemoveTaperedCapsuleByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName TaperedCapsuleName, bool& Success);

	// Convex Hull Functions
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Convex Hulls")
	static FRealtimeMeshSimpleGeometry& AddConvex(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, const FRealtimeMeshCollisionConvex& InConvexHull, int32& OutIndex);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Convex Hulls")
	static FRealtimeMeshSimpleGeometry& InsertConvex(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionConvex& InConvexHull, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Convex Hulls")
	static FRealtimeMeshSimpleGeometry& GetConvexByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName ConvexHullName, bool& Success, FRealtimeMeshCollisionConvex& OutConvexHull);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Convex Hulls")
	static FRealtimeMeshSimpleGeometry& UpdateConvex(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, const FRealtimeMeshCollisionConvex& InConvexHull, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Convex Hulls")
	static FRealtimeMeshSimpleGeometry& RemoveConvex(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, int32 Index, bool& Success);
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Convex Hulls")
	static FRealtimeMeshSimpleGeometry& RemoveConvexByName(UPARAM(ref) FRealtimeMeshSimpleGeometry& SimpleGeometry, FName ConvexHullName, bool& Success);
};

// ReSharper restore CppUEBlueprintCallableFunctionUnused

struct REALTIMEMESHCOMPONENT_API FRealtimeMeshTriMeshData
{
private:
	TArray<FVector3f> Vertices;
	TArray<FTriIndices> Triangles;
	TArray<uint16> Materials;
	TArray<TArray<FVector2D>> UVs;

public:
	const TArray<FVector3f>& GetVertices() const { return Vertices; }
	TArray<FVector3f>& GetVertices() { return Vertices; }


	const TArray<FTriIndices>& GetTriangles() const { return Triangles; }
	TArray<FTriIndices>& GetTriangles() { return Triangles; }
	const TArray<uint16>& GetMaterials() const { return Materials; }
	TArray<uint16>& GetMaterials() { return Materials; }
	const TArray<TArray<FVector2D>>& GetUVs() const { return UVs; }
	TArray<TArray<FVector2D>>& GetUVs() { return UVs; }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshTriMeshData& MeshData);
};


struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionData
{
	FRealtimeMeshCollisionConfiguration Config;
	FRealtimeMeshSimpleGeometry SimpleGeometry;
	FRealtimeMeshTriMeshData ComplexGeometry;
};

UENUM()
enum class ERealtimeMeshCollisionUpdateResult : uint8
{
	Unknown,
	Updated,
	Ignored,
	Error,
};

