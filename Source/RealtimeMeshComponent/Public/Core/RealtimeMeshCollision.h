// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshDataTypes.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "UObject/ObjectMacros.h"
#include "RealtimeMeshCollision.generated.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshStreamSet;
}

class UBodySetup;

UENUM(BlueprintType)
enum class ERealtimeMeshCollisionUpdateResult : uint8
{
	Unknown,
	Updated,
	Ignored,
	Error,
};

USTRUCT(BlueprintType)
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
	
	FRealtimeMeshCollisionConfiguration()
		: bUseComplexAsSimpleCollision(true)
		, bUseAsyncCook(true)
		, bShouldFastCookMeshes(false)
		, bFlipNormals(false)
		, bDeformableMesh(false)
		, bMergeAllMeshes(false)
	{ }

	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConfiguration& Config);
};


USTRUCT(BlueprintType)
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

	FRealtimeMeshCollisionShape()
		: Name(NAME_None)
		, Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, bContributesToMass(true)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionShape& Shape);
};


USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionSphere : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Radius;
	
	FRealtimeMeshCollisionSphere()
		: Radius(0.5f)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionSphere& Shape);
};

USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionBox : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FVector Extents;
	
	FRealtimeMeshCollisionBox(const FVector& InExtents = FVector(0.5f, 0.5f, 0.5f))
		: Extents(InExtents)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionBox& Shape);
};


USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionCapsule : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Radius;

	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	float Length;
	
	FRealtimeMeshCollisionCapsule()
		: Radius(0.5f)
		, Length(0.5f)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionCapsule& Shape);
};

USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionTaperedCapsule : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

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
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionTaperedCapsule& Shape);
};


struct FRealtimeMeshCookedConvexMeshData
{
private:
	Chaos::FConvexPtr CookedMesh;
public:
	FRealtimeMeshCookedConvexMeshData(const Chaos::FConvexPtr& InNonMirrored)
		: CookedMesh(InNonMirrored) { }	
	bool HasNonMirrored() const { return CookedMesh.IsValid(); }
	auto GetNonMirrored() const { return CookedMesh; }
};

USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionConvex : public FRealtimeMeshCollisionShape
{
	GENERATED_BODY()

private:
	TArray<FVector> Vertices;	
	FBox BoundingBox;
	
	mutable TSharedPtr<FRealtimeMeshCookedConvexMeshData> Cooked;
	
public:	
	FRealtimeMeshCollisionConvex() = default;

	void SetVertices(const TArray<FVector>& InVertices) { Vertices = InVertices; ReleaseCooked(); }
	void SetVertices(TArray<FVector>&& InVertices) { Vertices = MoveTemp(InVertices); ReleaseCooked(); }
	void ClearVertices() { Vertices.Empty(); ReleaseCooked(); }
	const TArray<FVector>& GetVertices() const { return Vertices; }
	void EditVertices(const TFunctionRef<void(TArray<FVector>&)>& ProcessFunc) { ProcessFunc(Vertices); ReleaseCooked(); }

	bool NeedsCook() const { return !Cooked.IsValid(); }
	bool HasCookedMesh() const { return Cooked.IsValid() && Cooked->HasNonMirrored(); }
	TSharedPtr<FRealtimeMeshCookedConvexMeshData> GetCooked() const { return Cooked; }
	void ReleaseCooked() const { Cooked.Reset(); }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConvex& Shape);
	friend class URealtimeMeshCollisionTools;
};


// Namespace-scope declarations so the export attribute is on the FIRST declaration: the
// friend declarations inside FSimpleShapeSet (a template) are otherwise the first ones the
// compiler sees, and a visibility attribute on a later redeclaration is ignored.
struct FRealtimeMeshSimpleGeometry;
struct FRealtimeMeshComplexGeometry;
REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshSimpleGeometry& SimpleGeometry);
REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshComplexGeometry& ComplexGeometry);

template<typename ShapeType>
struct FSimpleShapeSet
{
private:
	TSparseArray<ShapeType> Shapes;
	TMap<FName, int32> NameMap;

	void AddToNameMap(const FName& Name, int32 Index)
	{
		if (Name != NAME_None)
		{
			if (!NameMap.Contains(Name))
			{
				NameMap.Add(Name, Index);
			}
			else
			{
				UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Name %s already exists in the map"), *Name.ToString());
			}
		}
	}

	void RemoveIndexFromNameMap(const FName& Name, int32 Index)
	{
		if (Name != NAME_None)
		{
			if (const int32* FoundEntry = NameMap.Find(Name))
			{
				if (*FoundEntry == Index)
				{
					NameMap.Remove(Name);
				}
				else
				{
					UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Name %s does not match the index %d"), *Name.ToString(), Index);
				}
			}
			else
			{
				UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Name %s does not exist in the map"), *Name.ToString());
			}
		}
	}

	void RebuildNameMap()
	{
		NameMap.Empty();
		for (typename TSparseArray<ShapeType>::TConstIterator It(Shapes); It; ++It)
		{
			AddToNameMap(It->Name, It.GetIndex());
		}
	}

public:	
	int32 Add(const ShapeType& NewShape)
	{
		const int32 NewIndex = Shapes.Add(NewShape);
		AddToNameMap(NewShape.Name, NewIndex);
		return NewIndex;
	}

	int32 Add(ShapeType&& NewShape)
	{
		const int32 NewIndex = Shapes.Add(MoveTemp(NewShape));
		// NewShape.Name is read after MoveTemp above. This is intentional and safe: the
		// only field read here is the FName Name, and FName is trivially copyable, so its move
		// is a copy that leaves the source value intact. Reading it post-move yields the same
		// name the moved-to element carries.
		AddToNameMap(NewShape.Name, NewIndex);
		return NewIndex;
	}

	bool Insert(int32 Index, const ShapeType& NewShape)
	{
		if (!Shapes.IsValidIndex(Index) && Index >= 0)
		{
			Shapes.Insert(Index, NewShape);
			AddToNameMap(NewShape.Name, Index);
			return true;
		}
		return false;
	}

	bool Insert(int32 Index, ShapeType&& NewShape)
	{
		if (!Shapes.IsValidIndex(Index) && Index >= 0)
		{
			Shapes.Insert(Index, MoveTemp(NewShape));
			AddToNameMap(Shapes[Index].Name, Index);
			return true;
		}
		return false;
	}

	ShapeType& GetByIndex(int32 Index)
	{
		return Shapes[Index];
	}

	const ShapeType& GetByIndex(int32 Index) const
	{
		return Shapes[Index];
	}
	
	int32 GetIndexFromName(FName ShapeName) const
	{
		if (const int32* FoundEntry = NameMap.Find(ShapeName))
		{
			return *FoundEntry;
		}
		UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Name %s does not exist in the map"), *ShapeName.ToString());
		return INDEX_NONE;
	}

	bool GetByName(FName ShapeName, ShapeType& OutShape) const
	{
		const int32 Index = GetIndexFromName(ShapeName);
		if (Index != INDEX_NONE)
		{
			OutShape = Shapes[Index];
			return true;
		}
		return false;
	}

	bool Update(int32 Index, const ShapeType& InShape)
	{
		if (Shapes.IsValidIndex(Index))
		{
			RemoveIndexFromNameMap(Shapes[Index].Name, Index);
			Shapes[Index] = InShape;
			AddToNameMap(InShape.Name, Index);
			return true;
		}
		return false;
	}

	bool Update(int32 Index, ShapeType&& InShape)
	{
		if (Shapes.IsValidIndex(Index))
		{
			RemoveIndexFromNameMap(Shapes[Index].Name, Index);
			Shapes[Index] = MoveTemp(InShape);
			AddToNameMap(Shapes[Index].Name, Index);
			return true;
		}
		return false;
	}

	bool Remove(int32 Index)
	{
		if (Shapes.IsValidIndex(Index))
		{
			RemoveIndexFromNameMap(Shapes[Index].Name, Index);
			Shapes.RemoveAt(Index);
			return true;
		}
		return false;
	}

	bool Remove(FName ShapeName)
	{
		const int32 Index = GetIndexFromName(ShapeName);
		if (Index != INDEX_NONE)
		{
			Remove(Index);
			return true;
		}
		return false;
	}

	void Reset() { Shapes.Empty(); NameMap.Empty(); }

	bool IsEmpty() const { return Shapes.Num() == 0; }
	int32 Num() const { return Shapes.Num(); }

	// Collects the indices of every shape whose cook is stale. Only instantiated for shape types
	// that expose NeedsCook() (convex hulls, collision meshes); the simpler shape types never call it.
	TArray<int32> GetMeshIDsNeedingCook() const
	{
		TArray<int32> MeshesNeedingCook;
		for (auto It = Shapes.CreateConstIterator(); It; ++It)
		{
			if (It->NeedsCook())
			{
				MeshesNeedingCook.Add(It.GetIndex());
			}
		}
		return MeshesNeedingCook;
	}

	auto CreateIterator() const { return Shapes.CreateIterator(); }
	auto CreateConstIterator() const { return Shapes.CreateConstIterator(); }

	auto begin() { return Shapes.begin(); }
	auto end() { return Shapes.end(); }
	auto begin() const { return Shapes.begin(); }
	auto end() const  { return Shapes.end(); }

	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, struct FRealtimeMeshSimpleGeometry& SimpleGeometry);
	friend FArchive& operator<<(FArchive& Ar, struct FRealtimeMeshComplexGeometry& ComplexGeometry);
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSimpleGeometry
{
	GENERATED_BODY()

	FSimpleShapeSet<FRealtimeMeshCollisionSphere> Spheres;
	FSimpleShapeSet<FRealtimeMeshCollisionBox> Boxes;
	FSimpleShapeSet<FRealtimeMeshCollisionCapsule> Capsules;
	FSimpleShapeSet<FRealtimeMeshCollisionTaperedCapsule> TaperedCapsules;
	FSimpleShapeSet<FRealtimeMeshCollisionConvex> ConvexHulls;

	bool HasAnyShapes() const
	{
		return !Spheres.IsEmpty() || !Boxes.IsEmpty() || !Capsules.IsEmpty() || !TaperedCapsules.IsEmpty() || !ConvexHulls.IsEmpty();
	}
	
	// Only ConvexHulls carry cookable data.
	TArray<int32> GetMeshIDsNeedingCook() const
	{
		return ConvexHulls.GetMeshIDsNeedingCook();
	}

	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshSimpleGeometry& SimpleGeometry);
};



struct FRealtimeMeshCollisionMeshCookedUVData
{
	/** Index buffer, required to go from face index to UVs */
	TArray<RealtimeMesh::TIndex3<int32>> Triangles;
	/** Vertex positions, used to determine barycentric co-ords */
	TArray<FVector3f> Positions;
	/** UV channels for each vertex */
	TArray<TArray<FVector2f>> TexCoords;
	
	void FillFromTriMesh(const struct FRealtimeMeshCollisionMesh& TriMeshCollisionData);
	
	/** Get resource size of UV info */
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const
	{		
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(Triangles.GetAllocatedSize());
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(Positions.GetAllocatedSize());

		for (int32 ChannelIdx = 0; ChannelIdx < TexCoords.Num(); ChannelIdx++)
		{
			CumulativeResourceSize.AddDedicatedSystemMemoryBytes(TexCoords[ChannelIdx].GetAllocatedSize());
		}

		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(TexCoords.GetAllocatedSize());
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionMeshCookedUVData& UVInfo);
};

struct FRealtimeMeshCookedTriMeshData
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

	auto GetMesh() const { return TriMesh; }
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
	bool bFlipNormals;
	
	mutable TSharedPtr<FRealtimeMeshCookedTriMeshData> Cooked;
	
public:
	UPROPERTY(Category="RealtimeMesh|Collision", EditAnywhere, BlueprintReadWrite)
	FName Name;
	
	FRealtimeMeshCollisionMesh()
		: bFlipNormals(true)
	{ }
	
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
	
	bool NeedsCook() const { return !Cooked.IsValid(); }
	bool HasCookedMesh() const { return Cooked.IsValid() && Cooked->HasMesh(); }
	TSharedPtr<FRealtimeMeshCookedTriMeshData> GetCooked() const { return Cooked; }
	void ReleaseCooked() const { Cooked.Reset(); }

	
	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionMesh& Shape);
	friend class URealtimeMeshCollisionTools;
};


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshComplexGeometry
{
	GENERATED_BODY()

private:
	// Public façade over an FSimpleShapeSet of collision meshes. The member is deliberately named
	// `Meshes` so friend accessors (URealtimeMeshCollisionTools, the serializer) keep reaching it.
	FSimpleShapeSet<FRealtimeMeshCollisionMesh> Meshes;

public:

	int32 NumMeshes() const { return Meshes.Num(); }
	void Reset() { Meshes.Reset(); }

	int32 Add(const FRealtimeMeshCollisionMesh& InMesh) { return Meshes.Add(InMesh); }
	int32 Add(FRealtimeMeshCollisionMesh&& InMesh) { return Meshes.Add(MoveTemp(InMesh)); }

	bool Insert(int32 Index, const FRealtimeMeshCollisionMesh& InMesh) { return Meshes.Insert(Index, InMesh); }
	bool Insert(int32 Index, FRealtimeMeshCollisionMesh&& InMesh) { return Meshes.Insert(Index, MoveTemp(InMesh)); }

	FRealtimeMeshCollisionMesh& GetByIndex(int32 Index) { return Meshes.GetByIndex(Index); }
	const FRealtimeMeshCollisionMesh& GetByIndex(int32 Index) const { return Meshes.GetByIndex(Index); }

	int32 GetIndexFromName(FName MeshName) const { return Meshes.GetIndexFromName(MeshName); }

	bool GetByName(FName MeshName, FRealtimeMeshCollisionMesh& OutMesh) { return Meshes.GetByName(MeshName, OutMesh); }

	bool Update(int32 Index, const FRealtimeMeshCollisionMesh& InMesh) { return Meshes.Update(Index, InMesh); }
	bool Update(int32 Index, FRealtimeMeshCollisionMesh&& InMesh) { return Meshes.Update(Index, MoveTemp(InMesh)); }

	bool Remove(int32 Index) { return Meshes.Remove(Index); }
	bool Remove(FName MeshName) { return Meshes.Remove(MeshName); }

	TArray<int32> GetMeshIDsNeedingCook() const { return Meshes.GetMeshIDsNeedingCook(); }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshComplexGeometry& ComplexGeometry);
	friend class URealtimeMeshCollisionTools;
};

USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionInfo
{
	GENERATED_BODY()

	FRealtimeMeshSimpleGeometry SimpleGeometry;
	FRealtimeMeshComplexGeometry ComplexGeometry;
	FRealtimeMeshCollisionConfiguration Configuration;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionInfo& CollisionInfo);
};

inline void FRealtimeMeshCollisionMeshCookedUVData::FillFromTriMesh(const FRealtimeMeshCollisionMesh& TriMeshCollisionData)
{
	Positions = TriMeshCollisionData.GetVertices();
	Triangles = TriMeshCollisionData.GetTriangles();

	for (const auto& UVChannel : TriMeshCollisionData.GetTexCoords())
	{
		if (UVChannel.Num() == Positions.Num())
		{
			TexCoords.Add(UVChannel);
		}
	}		
}


