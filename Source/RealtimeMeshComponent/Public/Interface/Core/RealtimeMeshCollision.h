// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshDataTypes.h"
#include "Chaos/TriangleMeshImplicitObject.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshStreamSet;
}

class UBodySetup;

enum class ERealtimeMeshCollisionUpdateResult : uint8
{
	Unknown,
	Updated,
	Ignored,
	Error,
};

struct FRealtimeMeshCollisionConfiguration
{
	bool bUseComplexAsSimpleCollision;
	bool bUseAsyncCook;
	bool bShouldFastCookMeshes;
	bool bFlipNormals;
	bool bDeformableMesh;	
	bool bMergeAllMeshes;
	
	FRealtimeMeshCollisionConfiguration()
		: bUseComplexAsSimpleCollision(true)
		, bUseAsyncCook(true)
		, bShouldFastCookMeshes(false)
		, bFlipNormals(false)
		, bDeformableMesh(false)
		, bMergeAllMeshes(false)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConfiguration& Config);
};


struct FRealtimeMeshCollisionShape
{
	FName Name;
	FVector Center;
	FRotator Rotation;
	bool bContributesToMass;

	FRealtimeMeshCollisionShape()
		: Name(NAME_None)
		, Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, bContributesToMass(true)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionShape& Shape);
};


struct FRealtimeMeshCollisionSphere : public FRealtimeMeshCollisionShape
{
	float Radius;
	
	FRealtimeMeshCollisionSphere()
		: Radius(0.5f)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionSphere& Shape);
};

struct FRealtimeMeshCollisionBox : public FRealtimeMeshCollisionShape
{
	FVector Extents;
	
	FRealtimeMeshCollisionBox(const FVector& InExtents = FVector(0.5f, 0.5f, 0.5f))
		: Extents(InExtents)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionBox& Shape);
};


struct FRealtimeMeshCollisionCapsule : public FRealtimeMeshCollisionShape
{
	float Radius;
	float Length;
	
	FRealtimeMeshCollisionCapsule()
		: Radius(0.5f)
		, Length(0.5f)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionCapsule& Shape);
};

struct FRealtimeMeshCollisionTaperedCapsule : public FRealtimeMeshCollisionShape
{
	float RadiusA;
	float RadiusB;
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
#if RMC_ENGINE_ABOVE_5_4
	Chaos::FConvexPtr CookedMesh;
#else
	TSharedPtr<Chaos::FConvex> CookedMesh;
#endif
public:
#if RMC_ENGINE_ABOVE_5_4
	FRealtimeMeshCookedConvexMeshData(const Chaos::FConvexPtr& InNonMirrored)
		: CookedMesh(InNonMirrored) { }	
#else
	FRealtimeMeshCookedConvexMeshData(const TSharedPtr<Chaos::FConvex>& InNonMirrored)
		: CookedMesh(InNonMirrored) { }	
#endif
	bool HasNonMirrored() const { return CookedMesh.IsValid(); }
	auto GetNonMirrored() const { return CookedMesh; }
};

struct FRealtimeMeshCollisionConvex : public FRealtimeMeshCollisionShape
{
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


template<typename ShapeType>
struct REALTIMEMESHCOMPONENT_INTERFACE_API FSimpleShapeSet
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
	
	auto CreateIterator() const { return Shapes.CreateIterator(); }
	auto CreateConstIterator() const { return Shapes.CreateConstIterator(); }

	auto begin() { return Shapes.begin(); }
	auto end() { return Shapes.end(); }
	auto begin() const { return Shapes.begin(); }
	auto end() const  { return Shapes.end(); }
	
	friend FArchive& operator<<(FArchive& Ar, FSimpleShapeSet& ShapeSet);
	friend FArchive& operator<<(FArchive& Ar, struct FRealtimeMeshSimpleGeometry& SimpleGeometry);
};

struct REALTIMEMESHCOMPONENT_INTERFACE_API FRealtimeMeshSimpleGeometry
{
	FSimpleShapeSet<FRealtimeMeshCollisionSphere> Spheres;
	FSimpleShapeSet<FRealtimeMeshCollisionBox> Boxes;
	FSimpleShapeSet<FRealtimeMeshCollisionCapsule> Capsules;
	FSimpleShapeSet<FRealtimeMeshCollisionTaperedCapsule> TaperedCapsules;
	FSimpleShapeSet<FRealtimeMeshCollisionConvex> ConvexHulls;

	
	TArray<int32> GetMeshIDsNeedingCook() const
	{
		TArray<int32> MeshesNeedingCook;
		for (auto It = ConvexHulls.CreateConstIterator(); It; ++It)
		{
			if (It->NeedsCook())
			{
				MeshesNeedingCook.Add(It.GetIndex());
			}
		}
		return MeshesNeedingCook;		
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshSimpleGeometry& SimpleGeometry);
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
#if RMC_ENGINE_ABOVE_5_4
	Chaos::FTriangleMeshImplicitObjectPtr TriMesh;
#else
	TSharedPtr<Chaos::FTriangleMeshImplicitObject> TriMesh;
#endif
	TArray<int32> VertexRemap;
	TArray<int32> FaceRemap;
	FRealtimeMeshCollisionMeshCookedUVData UVInfo;
public:
	
	FRealtimeMeshCookedTriMeshData()
		: TriMesh(nullptr)
	{ }
	
#if RMC_ENGINE_ABOVE_5_4
	FRealtimeMeshCookedTriMeshData(const Chaos::FTriangleMeshImplicitObjectPtr& InTriMesh,
#else
	FRealtimeMeshCookedTriMeshData(const TSharedPtr<Chaos::FTriangleMeshImplicitObject>& InTriMesh,
#endif
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

struct REALTIMEMESHCOMPONENT_INTERFACE_API FRealtimeMeshCollisionMesh
{
private:	
	TArray<FVector3f> Vertices;
	TArray<RealtimeMesh::TIndex3<int32>> Triangles;
	TArray<uint16> Materials;
	TArray<TArray<FVector2f>> TexCoords;
	bool bFlipNormals;
	
	mutable TSharedPtr<FRealtimeMeshCookedTriMeshData> Cooked;
	
public:
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


struct REALTIMEMESHCOMPONENT_INTERFACE_API FRealtimeMeshComplexGeometry
{
private:
	TSparseArray<FRealtimeMeshCollisionMesh> Meshes;
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
public:

	int32 NumMeshes() const { return Meshes.Num(); }
	void Reset() { Meshes.Empty(); NameMap.Empty(); }

	int32 Add(const FRealtimeMeshCollisionMesh& InMesh)
	{
		const int32 NewIndex = Meshes.Add(InMesh);
		AddToNameMap(InMesh.Name, NewIndex);
		return NewIndex;
	}

	int32 Add(FRealtimeMeshCollisionMesh&& InMesh)
	{
		const int32 NewIndex = Meshes.Add(MoveTemp(InMesh));
		AddToNameMap(InMesh.Name, NewIndex);
		return NewIndex;
	}

	bool Insert(int32 Index, const FRealtimeMeshCollisionMesh& InMesh)
	{
		if (!Meshes.IsValidIndex(Index) && Index >= 0)
		{
			Meshes.Insert(Index, InMesh);
			AddToNameMap(InMesh.Name, Index);
			return true;
		}
		return false;
	}

	bool Insert(int32 Index, FRealtimeMeshCollisionMesh&& InMesh)
	{
		if (!Meshes.IsValidIndex(Index) && Index >= 0)
		{
			Meshes.Insert(Index, MoveTemp(InMesh));
			AddToNameMap(Meshes[Index].Name, Index);
			return true;
		}
		return false;
	}

	FRealtimeMeshCollisionMesh& GetByIndex(int32 Index)
	{
		return Meshes[Index];
	}

	const FRealtimeMeshCollisionMesh& GetByIndex(int32 Index) const
	{
		return Meshes[Index];
	}

	int32 GetIndexFromName(FName MeshName) const
	{
		if (const int32* FoundEntry = NameMap.Find(MeshName))
		{
			return *FoundEntry;
		}
		UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Name %s does not exist in the map"), *MeshName.ToString());
		return INDEX_NONE;
	}

	bool GetByName(FName MeshName, FRealtimeMeshCollisionMesh& OutMesh)
	{
		const int32 Index = GetIndexFromName(MeshName);
		if (Index != INDEX_NONE)
		{
			OutMesh = Meshes[Index];
			return true;
		}
		return false;
	}

	bool Update(int32 Index, const FRealtimeMeshCollisionMesh& InMesh)
	{
		if (Meshes.IsValidIndex(Index))
		{
			RemoveIndexFromNameMap(Meshes[Index].Name, Index);
			Meshes[Index] = InMesh;
			AddToNameMap(InMesh.Name, Index);
			return true;
		}
		return false;
	}

	bool Update(int32 Index, FRealtimeMeshCollisionMesh&& InMesh)
	{
		if (Meshes.IsValidIndex(Index))
		{
			RemoveIndexFromNameMap(Meshes[Index].Name, Index);
			Meshes[Index] = MoveTemp(InMesh);
			AddToNameMap(Meshes[Index].Name, Index);
			return true;
		}
		return false;
	}

	bool Remove(int32 Index)
	{
		if (Meshes.IsValidIndex(Index))
		{
			RemoveIndexFromNameMap(Meshes[Index].Name, Index);
			Meshes.RemoveAt(Index);
			return true;
		}
		return false;
	}

	bool Remove(FName MeshName)
	{
		const int32 Index = GetIndexFromName(MeshName);
		if (Index != INDEX_NONE)
		{
			Remove(Index);
			return true;
		}
		return false;
	}

	TArray<int32> GetMeshIDsNeedingCook() const
	{
		TArray<int32> MeshesNeedingCook;
		for (auto It = Meshes.CreateConstIterator(); It; ++It)
		{
			if (It->NeedsCook())
			{
				MeshesNeedingCook.Add(It.GetIndex());
			}
		}
		return MeshesNeedingCook;
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshComplexGeometry& ComplexGeometry);
	friend class URealtimeMeshCollisionTools;
};

struct FRealtimeMeshCollisionInfo
{
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


namespace RealtimeMesh
{
	class IRealtimeMeshCollisionTools_v0 : public IModularFeature
	{
	public:
		static FName GetModularFeatureName()
		{
			static FName FeatureName = TEXT("IRealtimeMeshCollisionTools_v0");
			return FeatureName;
		}

		
		virtual bool FindCollisionUVRealtimeMesh(const struct FHitResult& Hit, int32 UVChannel, FVector2D& UV) const = 0;
		virtual void CookConvexHull(FRealtimeMeshCollisionConvex& ConvexHull) const = 0;
		virtual void CookComplexMesh(FRealtimeMeshCollisionMesh& CollisionMesh) const = 0;
	};

}


