// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interface_CollisionDataProviderCore.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "PhysicsEngine/BodySetup.h"
#include "RealtimeMeshCollision.generated.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshStreamSet;
}

class UBodySetup;



USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionConfiguration
{
	GENERATED_BODY()

public:
	FRealtimeMeshCollisionConfiguration()
		: bUseComplexAsSimpleCollision(true)
		  , bUseAsyncCook(true)
		  , bShouldFastCookMeshes(false)
		  , bFlipNormals(false)
		  , bDeformableMesh(false)
	, bDirectCook(false)
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
	
	bool bDirectCook;


	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConfiguration& Config);
};


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FVector Center;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FRotator Rotation;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	bool bContributesToMass;

	FRealtimeMeshCollisionShape()
		: Name(NAME_None)
		, Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, bContributesToMass(true)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionShape& Shape);
};


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionSphere : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Radius;
	
	FRealtimeMeshCollisionSphere()
		: Radius(0.5f)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionSphere& Shape);
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionBox : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FVector Extents;
	
	FRealtimeMeshCollisionBox(const FVector& InExtents = FVector(0.5f, 0.5f, 0.5f))
		: Extents(InExtents)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionBox& Shape);
};


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionCapsule : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Radius;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Length;
	
	FRealtimeMeshCollisionCapsule()
		: Radius(0.5f)
		, Length(0.5f)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionCapsule& Shape);
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionTaperedCapsule : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float RadiusA;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float RadiusB;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Length;
	
	FRealtimeMeshCollisionTaperedCapsule()
		: RadiusA(0.5f)
		, RadiusB(0.5f)
		, Length(0.5f)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionTaperedCapsule& Shape);
};


struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCookedConvexMeshData
{
private:
	Chaos::FConvexPtr NonMirrored;
public:
	FRealtimeMeshCookedConvexMeshData(const Chaos::FConvexPtr& InNonMirrored)
		: NonMirrored(InNonMirrored) { }
	
	bool HasNonMirrored() const { return NonMirrored.IsValid(); }

	Chaos::FConvexPtr GetNonMirrored() const { return NonMirrored; }
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionConvex : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()
private:
	TArray<FVector> Vertices;
	
	mutable TSharedPtr<FRealtimeMeshCookedConvexMeshData> Cooked;
	
public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FBox BoundingBox;
	
	FRealtimeMeshCollisionConvex()
	{
	}

	void SetVertices(const TArray<FVector>& InVertices) { Vertices = InVertices; ReleaseCooked(); }
	void SetVertices(TArray<FVector>&& InVertices) { Vertices = MoveTemp(InVertices); ReleaseCooked(); }
	void ClearVertices() { Vertices.Empty(); ReleaseCooked(); }
	const TArray<FVector>& GetVertices() const { return Vertices; }

	void EditVertices(const TFunctionRef<void(TArray<FVector>&)>& ProcessFunc)
	{
		ProcessFunc(Vertices);
		ReleaseCooked();
	}
	
	TSharedPtr<FRealtimeMeshCookedConvexMeshData> Cook() const;
	bool NeedsCook() const { return !Cooked.IsValid(); }
	bool HasCookedMesh() const { return Cooked.IsValid() && Cooked->HasNonMirrored(); }
	TSharedPtr<FRealtimeMeshCookedConvexMeshData> GetCooked() const { return Cooked; }
	void ReleaseCooked() const { Cooked.Reset(); }

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

	
	TArray<int32> GetMeshIDsNeedingCook() const;
	void CookHull(int32 Index) const;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshSimpleGeometry& SimpleGeometry);

	void CopyToBodySetup(UBodySetup* BodySetup) const;
};



struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionMeshCookedUVData
{
	/** Index buffer, required to go from face index to UVs */
	TArray<RealtimeMesh::TIndex3<int32>> Triangles;
	/** Vertex positions, used to determine barycentric co-ords */
	TArray<FVector3f> Positions;
	/** UV channels for each vertex */
	TArray<TArray<FVector2f>> TexCoords;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionMeshCookedUVData& UVInfo)
	{
		Ar << UVInfo.Triangles;
		Ar << UVInfo.Positions;
		Ar << UVInfo.TexCoords;

		return Ar;
	}

	/** Get resource size of UV info */
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const;

	void FillFromTriMesh(const struct FRealtimeMeshCollisionMesh& TriMeshCollisionData);
};

struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCookedTriMeshData
{
private:
	Chaos::FTriangleMeshImplicitObjectPtr TriMesh;
	TArray<int32> VertexRemap;
	TArray<int32> FaceRemap;
	FRealtimeMeshCollisionMeshCookedUVData UVInfo;
public:
	
	FRealtimeMeshCookedTriMeshData()
		: TriMesh(nullptr)
	{ }
	FRealtimeMeshCookedTriMeshData(const Chaos::FTriangleMeshImplicitObjectPtr& InTriMesh,
		TArray<int32>&& InVertexRemap, TArray<int32>&& InFaceRemap, FRealtimeMeshCollisionMeshCookedUVData&& InUVInfo)
		: TriMesh(InTriMesh)
		, VertexRemap(MoveTemp(InVertexRemap))
		, FaceRemap(MoveTemp(InFaceRemap))
		, UVInfo(MoveTemp(InUVInfo))
	{ }
	
	bool HasMesh() const { return TriMesh.IsValid(); }
	bool HasVertexRemap() const { return VertexRemap.Num() > 0; }
	bool HasFaceRemap() const { return FaceRemap.Num() > 0; }
	bool HasUVInfo() const { return UVInfo.Positions.Num() > 0 && UVInfo.Triangles.Num() > 0 && UVInfo.TexCoords.Num() > 0; }

	Chaos::FTriangleMeshImplicitObjectPtr GetMesh() const { return TriMesh; }
	const TArray<int32>& GetVertexRemap() const { return VertexRemap; }
	const TArray<int32>& GetFaceRemap() const { return FaceRemap; }
	const FRealtimeMeshCollisionMeshCookedUVData& GetUVInfo() const { return UVInfo; }

	TArray<int32> ConsumeVertexRemap() { return MoveTemp(VertexRemap); }
	TArray<int32> ConsumeFaceRemap() { return MoveTemp(FaceRemap); }
	FRealtimeMeshCollisionMeshCookedUVData ConsumeUVInfo() { return MoveTemp(UVInfo); }	
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionMesh
{
	GENERATED_BODY()
private:	
	TArray<FVector3f> Vertices;
	TArray<RealtimeMesh::TIndex3<int32>> Triangles;
	TArray<uint16> Materials;
	TArray<TArray<FVector2f>> TexCoords;
	
	mutable TSharedPtr<FRealtimeMeshCookedTriMeshData> Cooked;
	
public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FName Name;
	
	FRealtimeMeshCollisionMesh()
	{
	}

	void SetVertices(const TArray<FVector3f>& InVertices) { Vertices = InVertices; ReleaseCooked(); }
	void SetVertices(TArray<FVector3f>&& InVertices) { Vertices = MoveTemp(InVertices); ReleaseCooked(); }
	void ClearVertices() { Vertices.Empty(); ReleaseCooked(); }
	const TArray<FVector3f>& GetVertices() const { return Vertices; }
	
	void SetTriangles(const TArray<RealtimeMesh::TIndex3<int32>>& InTriangles) { Triangles = InTriangles; ReleaseCooked(); }
	void SetTriangles(TArray<RealtimeMesh::TIndex3<int32>>&& InTriangles) { Triangles = MoveTemp(InTriangles); ReleaseCooked(); }
	void ClearTriangles() { Triangles.Empty(); ReleaseCooked(); }
	const TArray<RealtimeMesh::TIndex3<int32>>& GetTriangles() const { return Triangles; }
	
	void SetMaterials(const TArray<uint16>& InMaterials) { Materials = InMaterials; ReleaseCooked(); }
	void SetMaterials(TArray<uint16>&& InMaterials) { Materials = MoveTemp(InMaterials); ReleaseCooked(); }
	void ClearMaterials() { Materials.Empty(); ReleaseCooked(); }
	const TArray<uint16>& GetMaterials() const { return Materials; }
	
	void SetTexCoords(const TArray<TArray<FVector2f>>& InTexCoords) { TexCoords = InTexCoords; ReleaseCooked(); }
	void SetTexCoords(TArray<TArray<FVector2f>>&& InTexCoords) { TexCoords = MoveTemp(InTexCoords); ReleaseCooked(); }
	void ClearTexCoords() { TexCoords.Empty(); ReleaseCooked(); }
	const TArray<TArray<FVector2f>>& GetTexCoords() const { return TexCoords; }

	bool Append(const RealtimeMesh::FRealtimeMeshStreamSet& Streams, int32 MaterialIndex);
	bool Append(const RealtimeMesh::FRealtimeMeshStreamSet& Streams, int32 MaterialIndex, int32 FirstTriangle, int32 TriangleCount);
	
	TSharedPtr<FRealtimeMeshCookedTriMeshData> Cook(bool bFlipNormals = true) const;
	bool NeedsCook() const { return !Cooked.IsValid(); }
	bool HasCookedMesh() const { return Cooked.IsValid() && Cooked->HasMesh(); }
	TSharedPtr<FRealtimeMeshCookedTriMeshData> GetCooked() const { return Cooked; }
	void ReleaseCooked() const { Cooked.Reset(); }

	
	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionMesh& Shape);
};


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshComplexGeometry
{
	GENERATED_BODY()

private:
	TSparseArray<FRealtimeMeshCollisionMesh> Meshes;
	TMap<FName, int32> MeshesNameMap;

public:

	int32 NumMeshes() const { return Meshes.Num(); }
	void Reset() { Meshes.Empty(); MeshesNameMap.Empty(); }
	
	int32 AddMesh(const FRealtimeMeshCollisionMesh& InMesh);
	int32 AddMesh(FRealtimeMeshCollisionMesh&& InMesh);
	bool InsertMesh(int32 Index, const FRealtimeMeshCollisionMesh& InMesh);
	bool InsertMesh(int32 Index, FRealtimeMeshCollisionMesh&& InMesh);
	int32 GetMeshIndexByName(FName MeshName) const;
	bool GetMeshByName(FName MeshName, FRealtimeMeshCollisionMesh& OutMesh);
	bool UpdateMesh(int32 Index, const FRealtimeMeshCollisionMesh& InMesh);
	bool UpdateMesh(int32 Index, FRealtimeMeshCollisionMesh&& InMesh);
	bool RemoveMesh(int32 Index);
	bool RemoveMesh(FName MeshName);

	TArray<int32> GetMeshIDsNeedingCook() const;
	void CookMesh(int32 Index) const;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshComplexGeometry& SimpleGeometry);

	void CopyToBodySetup(UBodySetup* BodySetup, TArray<FRealtimeMeshCollisionMeshCookedUVData>& OutUVData) const;
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionInfo
{
	GENERATED_BODY()

public:
	FRealtimeMeshSimpleGeometry SimpleGeometry;
	FRealtimeMeshComplexGeometry ComplexGeometry;
	FRealtimeMeshCollisionConfiguration Configuration;

public:

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshComplexGeometry& SimpleGeometry);

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

UCLASS()
class REALTIMEMESHCOMPONENT_API URealtimeMeshCollisionTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Collision")
	static bool FindCollisionUVRealtimeMesh(const struct FHitResult& Hit, int32 UVChannel, FVector2D& UV);
};


// ReSharper restore CppUEBlueprintCallableFunctionUnused


UENUM()
enum class ERealtimeMeshCollisionUpdateResult : uint8
{
	Unknown,
	Updated,
	Ignored,
	Error,
};



