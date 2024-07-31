// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshCollision.h"
#include "PhysicsEngine/BodySetup.h"
#include "Interface_CollisionDataProviderCore.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshComponentModule.h"
#include "RealtimeMeshCore.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Mesh/RealtimeMeshBuilder.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "PhysicsEngine/PhysicsSettings.h"


namespace RealtimeMesh::CollisionHelpers
{
	static void AddToNameMap(TMap<FName, int32>& NameMap, const FName& Name, int32 Index)
	{
		if (Name != NAME_None)
		{
			if (!NameMap.Contains(Name))
			{
				NameMap.Add(Name, Index);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Name %s already exists in the map"), *Name.ToString());
			}
		}
	}

	static int32 GetIndexFromNameMap(const TMap<FName, int32>& NameMap, const FName& Name)
	{
		if (const int32* FoundEntry = NameMap.Find(Name))
		{
			return *FoundEntry;
		}
		UE_LOG(LogTemp, Warning, TEXT("Name %s does not exist in the map"), *Name.ToString());
		return INDEX_NONE;
	}

	static void RemoveIndexFromNameMap(TMap<FName, int32>& NameMap, const FName& Name, int32 Index)
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
					UE_LOG(LogTemp, Warning, TEXT("Name %s does not match the index %d"), *Name.ToString(), Index);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Name %s does not exist in the map"), *Name.ToString());
			}
		}
	}
}

using namespace RealtimeMesh::CollisionHelpers;

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConfiguration& Config)
{
	Ar << Config.bUseComplexAsSimpleCollision;
	Ar << Config.bUseAsyncCook;
	Ar << Config.bShouldFastCookMeshes;

	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::CollisionUpdateFlowRestructure)
	{
		Ar << Config.bFlipNormals;
		Ar << Config.bDeformableMesh;
	}
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionShape& Shape)
{
	Ar << Shape.Name;
	Ar << Shape.Center;
	Ar << Shape.Rotation;
	Ar << Shape.bContributesToMass;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionSphere& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.Radius;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionBox& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.Extents;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionCapsule& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.Radius;
	Ar << Shape.Length;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionTaperedCapsule& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.RadiusA;
	Ar << Shape.RadiusB;
	Ar << Shape.Length;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConvex& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.Vertices;
	Ar << Shape.BoundingBox;
	return Ar;
}

// This is added in 5.4 but only for editor use, so we add it any other time.
#if RMC_ENGINE_BELOW_5_4 || !WITH_EDITORONLY_DATA
static FArchive& operator<<(FArchive& Ar, FTriIndices& Indices)
{
	Ar << Indices.v0 << Indices.v1 << Indices.v2;
	return Ar;
}
#endif

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionMesh& MeshData)
{
	Ar << MeshData.Vertices;
	Ar << MeshData.Triangles;
	Ar << MeshData.Materials;
	Ar << MeshData.TexCoords;
	return Ar;
}


// FRealtimeMeshCollisionConvex Functions

TSharedPtr<FRealtimeMeshCookedConvexMeshData> FRealtimeMeshCollisionConvex::Cook() const
{
	if (Cooked)
	{
		return Cooked;
	}
	
	using namespace Chaos;
	auto BuildConvexFromVerts = [this](const bool bMirrored) -> FConvex*
	{
		const int32 NumHullVerts = Vertices.Num();
		if(NumHullVerts == 0)
		{
			return nullptr;
		}

		// Calculate the margin to apply to the convex - it depends on overall dimensions
		FAABB3 Bounds = FAABB3::EmptyAABB();
		for(int32 VertIndex = 0; VertIndex < NumHullVerts; ++VertIndex)
		{
			const FVector& HullVert = Vertices[VertIndex];
			Bounds.GrowToInclude(HullVert);
		}

		// Create the corner vertices for the convex
		TArray<FConvex::FVec3Type> ConvexVertices;
		ConvexVertices.SetNumZeroed(NumHullVerts);

		for(int32 VertIndex = 0; VertIndex < NumHullVerts; ++VertIndex)
		{
			const FVector& HullVert = Vertices[VertIndex];
			ConvexVertices[VertIndex] = FConvex::FVec3Type(bMirrored ? -HullVert.X : HullVert.X, HullVert.Y, HullVert.Z);
		}

		// Margin is always zero on convex shapes - they are intended to be instanced
		return new FConvex(ConvexVertices, 0.0f);
	};

	FConvexPtr NonMirrored = BuildConvexFromVerts(false);
	Cooked = MakeShared<FRealtimeMeshCookedConvexMeshData>(NonMirrored);
	return Cooked;
}



// Sphere Functions
int32 FRealtimeMeshSimpleGeometry::AddSphere(const FRealtimeMeshCollisionSphere& InSphere)
{
	const int32 NewIndex = Spheres.Add(InSphere);
	AddToNameMap(SpheresNameMap, InSphere.Name, NewIndex);
	return NewIndex;
}

bool FRealtimeMeshSimpleGeometry::InsertSphere(int32 Index, const FRealtimeMeshCollisionSphere& InSphere)
{
	if (!Spheres.IsValidIndex(Index) && Index >= 0)
	{
		Spheres.Insert(Index, InSphere);
		AddToNameMap(SpheresNameMap, InSphere.Name, Index);
		return true;
	}
	return false;
}

int32 FRealtimeMeshSimpleGeometry::GetSphereIndexByName(FName SphereName) const
{
	return GetIndexFromNameMap(SpheresNameMap, SphereName);
}

bool FRealtimeMeshSimpleGeometry::GetSphereByName(FName SphereName, FRealtimeMeshCollisionSphere& OutSphere)
{
	const int32 Index = GetSphereIndexByName(SphereName);
	if (Index != INDEX_NONE)
	{
		OutSphere = Spheres[Index];
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::UpdateSphere(int32 Index, const FRealtimeMeshCollisionSphere& InSphere)
{
	if (Spheres.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(SpheresNameMap, Spheres[Index].Name, Index);
		Spheres[Index] = InSphere;
		AddToNameMap(SpheresNameMap, InSphere.Name, Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveSphere(int32 Index)
{
	if (Spheres.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(SpheresNameMap, Spheres[Index].Name, Index);
		Spheres.RemoveAt(Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveSphere(FName SphereName)
{
	const int32 Index = GetSphereIndexByName(SphereName);
	if (Index != INDEX_NONE)
	{
		RemoveSphere(Index);
		return true;
	}
	return false;
}


// Box Functions
int32 FRealtimeMeshSimpleGeometry::AddBox(const FRealtimeMeshCollisionBox& InBox)
{
	const int32 NewIndex = Boxes.Add(InBox);
	AddToNameMap(BoxesNameMap, InBox.Name, NewIndex);
	return NewIndex;
}

bool FRealtimeMeshSimpleGeometry::InsertBox(int32 Index, const FRealtimeMeshCollisionBox& InBox)
{
	if (!Boxes.IsValidIndex(Index) && Index >= 0)
	{
		Boxes.Insert(Index, InBox);
		AddToNameMap(BoxesNameMap, InBox.Name, Index);
		return true;
	}
	return false;
}

int32 FRealtimeMeshSimpleGeometry::GetBoxIndexByName(FName BoxName) const
{
	return GetIndexFromNameMap(BoxesNameMap, BoxName);
}

bool FRealtimeMeshSimpleGeometry::GetBoxByName(FName BoxName, FRealtimeMeshCollisionBox& OutBox)
{
	const int32 Index = GetBoxIndexByName(BoxName);
	if (Index != INDEX_NONE)
	{
		OutBox = Boxes[Index];
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::UpdateBox(int32 Index, const FRealtimeMeshCollisionBox& InBox)
{
	if (Boxes.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(BoxesNameMap, Boxes[Index].Name, Index);
		Boxes[Index] = InBox;
		AddToNameMap(BoxesNameMap, InBox.Name, Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveBox(int32 Index)
{
	if (Boxes.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(BoxesNameMap, Boxes[Index].Name, Index);
		Boxes.RemoveAt(Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveBox(FName BoxName)
{
	const int32 Index = GetBoxIndexByName(BoxName);
	if (Index != INDEX_NONE)
	{
		RemoveBox(Index);
		return true;
	}
	return false;
}

// Capsule Functions

int32 FRealtimeMeshSimpleGeometry::AddCapsule(const FRealtimeMeshCollisionCapsule& InCapsule)
{
	const int32 NewIndex = Capsules.Add(InCapsule);
	AddToNameMap(CapsulesNameMap, InCapsule.Name, NewIndex);
	return NewIndex;
}

bool FRealtimeMeshSimpleGeometry::InsertCapsule(int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule)
{
	if (!Capsules.IsValidIndex(Index) && Index >= 0)
	{
		Capsules.Insert(Index, InCapsule);
		AddToNameMap(CapsulesNameMap, InCapsule.Name, Index);
		return true;
	}
	return false;
}

int32 FRealtimeMeshSimpleGeometry::GetCapsuleIndexByName(FName CapsuleName) const
{
	return GetIndexFromNameMap(CapsulesNameMap, CapsuleName);
}

bool FRealtimeMeshSimpleGeometry::GetCapsuleByName(FName CapsuleName, FRealtimeMeshCollisionCapsule& OutCapsule)
{
	const int32 Index = GetCapsuleIndexByName(CapsuleName);
	if (Index != INDEX_NONE)
	{
		OutCapsule = Capsules[Index];
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::UpdateCapsule(int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule)
{
	if (Capsules.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(CapsulesNameMap, Capsules[Index].Name, Index);
		Capsules[Index] = InCapsule;
		AddToNameMap(CapsulesNameMap, InCapsule.Name, Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveCapsule(int32 Index)
{
	if (Capsules.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(CapsulesNameMap, Capsules[Index].Name, Index);
		Capsules.RemoveAt(Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveCapsule(FName CapsuleName)
{
	const int32 Index = GetCapsuleIndexByName(CapsuleName);
	if (Index != INDEX_NONE)
	{
		RemoveCapsule(Index);
		return true;
	}
	return false;
}

// Tapered Capsule Functions

int32 FRealtimeMeshSimpleGeometry::AddTaperedCapsule(const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule)
{
	const int32 NewIndex = TaperedCapsules.Add(InTaperedCapsule);
	AddToNameMap(TaperedCapsulesNameMap, InTaperedCapsule.Name, NewIndex);
	return NewIndex;
}

bool FRealtimeMeshSimpleGeometry::InsertTaperedCapsule(int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule)
{
	if (!TaperedCapsules.IsValidIndex(Index) && Index >= 0)
	{
		TaperedCapsules.Insert(Index, InTaperedCapsule);
		AddToNameMap(TaperedCapsulesNameMap, InTaperedCapsule.Name, Index);
		return true;
	}
	return false;
}

int32 FRealtimeMeshSimpleGeometry::GetTaperedCapsuleIndexByName(FName TaperedCapsuleName) const
{
	return GetIndexFromNameMap(TaperedCapsulesNameMap, TaperedCapsuleName);
}

bool FRealtimeMeshSimpleGeometry::GetTaperedCapsuleByName(FName TaperedCapsuleName, FRealtimeMeshCollisionTaperedCapsule& OutTaperedCapsule)
{
	const int32 Index = GetTaperedCapsuleIndexByName(TaperedCapsuleName);
	if (Index != INDEX_NONE)
	{
		OutTaperedCapsule = TaperedCapsules[Index];
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::UpdateTaperedCapsule(int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule)
{
	if (TaperedCapsules.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(TaperedCapsulesNameMap, TaperedCapsules[Index].Name, Index);
		TaperedCapsules[Index] = InTaperedCapsule;
		AddToNameMap(TaperedCapsulesNameMap, InTaperedCapsule.Name, Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveTaperedCapsule(int32 Index)
{
	if (TaperedCapsules.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(TaperedCapsulesNameMap, TaperedCapsules[Index].Name, Index);
		TaperedCapsules.RemoveAt(Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveTaperedCapsule(FName TaperedCapsuleName)
{
	const int32 Index = GetTaperedCapsuleIndexByName(TaperedCapsuleName);
	if (Index != INDEX_NONE)
	{
		RemoveTaperedCapsule(Index);
		return true;
	}
	return false;
}

// Convex Hull Functions

int32 FRealtimeMeshSimpleGeometry::AddConvexHull(const FRealtimeMeshCollisionConvex& InConvexHull)
{
	const int32 NewIndex = ConvexHulls.Add(InConvexHull);
	AddToNameMap(ConvexHullsNameMap, InConvexHull.Name, NewIndex);
	return NewIndex;
}

bool FRealtimeMeshSimpleGeometry::InsertConvexHull(int32 Index, const FRealtimeMeshCollisionConvex& InConvexHull)
{
	if (!ConvexHulls.IsValidIndex(Index) && Index >= 0)
	{
		ConvexHulls.Insert(Index, InConvexHull);
		AddToNameMap(ConvexHullsNameMap, InConvexHull.Name, Index);
		return true;
	}
	return false;
}

int32 FRealtimeMeshSimpleGeometry::GetConvexHullIndexByName(FName ConvexHullName) const
{
	return GetIndexFromNameMap(ConvexHullsNameMap, ConvexHullName);
}

bool FRealtimeMeshSimpleGeometry::GetConvexHullByName(FName ConvexHullName, FRealtimeMeshCollisionConvex& OutConvexHull)
{
	const int32 Index = GetConvexHullIndexByName(ConvexHullName);
	if (Index != INDEX_NONE)
	{
		OutConvexHull = ConvexHulls[Index];
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::UpdateConvexHull(int32 Index, const FRealtimeMeshCollisionConvex& InConvexHull)
{
	if (ConvexHulls.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(ConvexHullsNameMap, ConvexHulls[Index].Name, Index);
		ConvexHulls[Index] = InConvexHull;
		AddToNameMap(ConvexHullsNameMap, InConvexHull.Name, Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveConvexHull(int32 Index)
{
	if (ConvexHulls.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(ConvexHullsNameMap, ConvexHulls[Index].Name, Index);
		ConvexHulls.RemoveAt(Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshSimpleGeometry::RemoveConvexHull(FName ConvexHullName)
{
	const int32 Index = GetConvexHullIndexByName(ConvexHullName);
	if (Index != INDEX_NONE)
	{
		RemoveConvexHull(Index);
		return true;
	}
	return false;
}

TArray<int32> FRealtimeMeshSimpleGeometry::GetMeshIDsNeedingCook() const
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

void FRealtimeMeshSimpleGeometry::CookHull(int32 Index) const
{
	if (ConvexHulls.IsValidIndex(Index))
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		ConvexHulls[Index].Cook();
	}
}


// FRealtimeMeshSimpleGeometry Functions

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSimpleGeometry& SimpleGeometry)
{
	Ar << SimpleGeometry.Spheres;
	Ar << SimpleGeometry.Boxes;
	Ar << SimpleGeometry.Capsules;
	Ar << SimpleGeometry.TaperedCapsules;
	Ar << SimpleGeometry.ConvexHulls;

	// Rebuild name maps on load
	if (Ar.IsLoading())
	{
		SimpleGeometry.SpheresNameMap.Empty();
		for (TSparseArray<FRealtimeMeshCollisionSphere>::TConstIterator It(SimpleGeometry.Spheres); It; ++It)
		{
			AddToNameMap(SimpleGeometry.SpheresNameMap, It->Name, It.GetIndex());
		}

		SimpleGeometry.BoxesNameMap.Empty();
		for (TSparseArray<FRealtimeMeshCollisionBox>::TConstIterator It(SimpleGeometry.Boxes); It; ++It)
		{
			AddToNameMap(SimpleGeometry.BoxesNameMap, It->Name, It.GetIndex());
		}

		SimpleGeometry.CapsulesNameMap.Empty();
		for (TSparseArray<FRealtimeMeshCollisionCapsule>::TConstIterator It(SimpleGeometry.Capsules); It; ++It)
		{
			AddToNameMap(SimpleGeometry.CapsulesNameMap, It->Name, It.GetIndex());
		}

		SimpleGeometry.TaperedCapsulesNameMap.Empty();
		for (TSparseArray<FRealtimeMeshCollisionTaperedCapsule>::TConstIterator It(SimpleGeometry.TaperedCapsules); It; ++It)
		{
			AddToNameMap(SimpleGeometry.TaperedCapsulesNameMap, It->Name, It.GetIndex());
		}

		SimpleGeometry.ConvexHullsNameMap.Empty();
		for (TSparseArray<FRealtimeMeshCollisionConvex>::TConstIterator It(SimpleGeometry.ConvexHulls); It; ++It)
		{
			AddToNameMap(SimpleGeometry.ConvexHullsNameMap, It->Name, It.GetIndex());
		}
	}

	return Ar;
}

void FRealtimeMeshSimpleGeometry::CopyToBodySetup(UBodySetup* BodySetup) const
{
	for (const auto& Sphere : Spheres)
	{
		auto& BodySphere = BodySetup->AggGeom.SphereElems.AddDefaulted_GetRef();
		BodySphere.Radius = Sphere.Radius;
		BodySphere.Center = Sphere.Center;
		BodySphere.SetContributeToMass(Sphere.bContributesToMass);
		BodySphere.SetName(Sphere.Name);
	}

	for (const auto& Box : Boxes)
	{
		auto& BodyBox = BodySetup->AggGeom.BoxElems.AddDefaulted_GetRef();
		BodyBox.X = Box.Extents.X;
		BodyBox.Y = Box.Extents.Y;
		BodyBox.Z = Box.Extents.Z;
		BodyBox.Center = Box.Center;
		BodyBox.Rotation = Box.Rotation;
		BodyBox.SetContributeToMass(Box.bContributesToMass);
		BodyBox.SetName(Box.Name);
	}

	for (const auto& Capsule : Capsules)
	{
		auto& BodyCapsule = BodySetup->AggGeom.SphylElems.AddDefaulted_GetRef();
		BodyCapsule.Radius = Capsule.Radius;
		BodyCapsule.Length = Capsule.Length;
		BodyCapsule.Center = Capsule.Center;
		BodyCapsule.Rotation = Capsule.Rotation;
		BodyCapsule.SetContributeToMass(Capsule.bContributesToMass);
		BodyCapsule.SetName(Capsule.Name);
	}

	for (const auto& TaperedCapsule : TaperedCapsules)
	{
		auto& BodyTaperedCapsule = BodySetup->AggGeom.TaperedCapsuleElems.AddDefaulted_GetRef();
		BodyTaperedCapsule.Radius0 = TaperedCapsule.RadiusA;
		BodyTaperedCapsule.Radius1 = TaperedCapsule.RadiusB;
		BodyTaperedCapsule.Length = TaperedCapsule.Length;
		BodyTaperedCapsule.Center = TaperedCapsule.Center;
		BodyTaperedCapsule.Rotation = TaperedCapsule.Rotation;
		BodyTaperedCapsule.SetContributeToMass(TaperedCapsule.bContributesToMass);
		BodyTaperedCapsule.SetName(TaperedCapsule.Name);
	}

	for (const auto& Convex : ConvexHulls)
	{
		auto& BodyConvex = BodySetup->AggGeom.ConvexElems.AddDefaulted_GetRef();
		BodyConvex.VertexData = Convex.GetVertices();
		BodyConvex.SetTransform(FTransform(Convex.Rotation.Quaternion(), Convex.Center));
		BodyConvex.UpdateElemBox();
		BodyConvex.SetContributeToMass(Convex.bContributesToMass);
		BodyConvex.SetName(Convex.Name);

		if (Convex.HasCookedMesh())
		{
			const auto CookedMeshData = Convex.GetCooked();
			auto MeshData = CookedMeshData->GetNonMirrored();
			BodyConvex.SetConvexMeshObject(MoveTemp(MeshData));
		}
	}
}

void FRealtimeMeshCollisionMeshCookedUVData::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const
{
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(Triangles.GetAllocatedSize());
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(Positions.GetAllocatedSize());

	for (int32 ChannelIdx = 0; ChannelIdx < TexCoords.Num(); ChannelIdx++)
	{
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(TexCoords[ChannelIdx].GetAllocatedSize());
	}

	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(TexCoords.GetAllocatedSize());
}

void FRealtimeMeshCollisionMeshCookedUVData::FillFromTriMesh(const FRealtimeMeshCollisionMesh& TriMeshCollisionData)
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


// FRealtimeMeshCollisionMesh Functions

bool FRealtimeMeshCollisionMesh::Append(const RealtimeMesh::FRealtimeMeshStreamSet& Streams, int32 MaterialIndex)
{
	using namespace RealtimeMesh;

	const auto PositionStream = Streams.Find(FRealtimeMeshStreams::Position);
	const auto TriangleStream = Streams.Find(FRealtimeMeshStreams::Triangles);
	const auto TexCoordsStream = Streams.Find(FRealtimeMeshStreamKey(FRealtimeMeshStreams::TexCoords));

	if (!PositionStream || !TriangleStream)
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("Unable to append collision vertices: missing position or triangle stream."));
		return false;
	}

	if (PositionStream->Num() < 3 || (TriangleStream->Num() * TriangleStream->GetNumElements()) < 3)
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("Unable to append collision vertices: not enough elements in streams."));
		return false;
	}

	if (!PositionStream->CanConvertTo<FVector3f>())
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("Unable to append collision vertices: position stream not convertible to FVector3f"));
		return false;
	}
	
	const int32 StartVertexIndex = Vertices.Num();

	// Copy in the vertices
	PositionStream->CopyTo(Vertices);

	if (UPhysicsSettings::Get()->bSupportUVFromHitResults)
	{
		if (TexCoordsStream)
		{
			// We use the max number of UV channels across the appended data.
			// We simply zero-fill the unallocated channel sections.
			const int32 NumTexCoordStreams = FMath::Max(TexCoords.Num(), TexCoordsStream->GetNumElements());		
			TexCoords.SetNum(NumTexCoordStreams);
			for (int32 Index = 0; Index < NumTexCoordStreams; Index++)
			{
				TexCoords[Index].SetNumZeroed(Vertices.Num());
			}
			
			for (int32 ChannelIndex = 0; ChannelIndex < TexCoordsStream->GetNumElements(); ChannelIndex++)
			{
				TRealtimeMeshStridedStreamBuilder<const FVector2f, void> UVData(*TexCoordsStream, ChannelIndex);
				const int32 NumUVsToCopy = FMath::Min(UVData.Num(), PositionStream->Num());
				auto& CollisionUVChannel = TexCoords[ChannelIndex];
				CollisionUVChannel.SetNumUninitialized(NumUVsToCopy);
			
				for (int32 TexCoordIdx = 0; TexCoordIdx < NumUVsToCopy; TexCoordIdx++)
				{
					CollisionUVChannel[TexCoordIdx] = UVData[TexCoordIdx];
				}

				// Make sure the uv data is the same length as the position data
				if (PositionStream->Num() > UVData.Num())
				{
					CollisionUVChannel.SetNumZeroed(PositionStream->Num());
				}
			}
		}
		else
		{
			// Zero fill data to vertex data length

			if (TexCoords.Num() < 1)
			{
				TexCoords.SetNum(1);
			}
			for (int32 Index = 0; Index < TexCoords.Num(); Index++)
			{
				TexCoords[Index].SetNumZeroed(Vertices.Num());
			}
		}
	}

	TRealtimeMeshStreamBuilder<const TIndex3<uint32>, void> TrianglesData(*TriangleStream);
	Triangles.Reserve(Triangles.Num() + TrianglesData.Num());
	Materials.Reserve(Triangles.Num());

	for (int32 TriIdx = 0; TriIdx < TrianglesData.Num(); TriIdx++)
	{
		TIndex3<int32>& Tri = Triangles.AddDefaulted_GetRef();
		Tri.V0 = TrianglesData[TriIdx].GetElement(0).GetValue() + StartVertexIndex;
		Tri.V1 = TrianglesData[TriIdx].GetElement(1).GetValue() + StartVertexIndex;
		Tri.V2 = TrianglesData[TriIdx].GetElement(2).GetValue() + StartVertexIndex;

		Materials.Add(MaterialIndex);
	}
	
	return true;
}

bool FRealtimeMeshCollisionMesh::Append(const RealtimeMesh::FRealtimeMeshStreamSet& Streams, int32 MaterialIndex, int32 FirstTriangle, int32 TriangleCount)
{
	using namespace RealtimeMesh;

	const auto PositionStream = Streams.Find(FRealtimeMeshStreams::Position);
	const auto TriangleStream = Streams.Find(FRealtimeMeshStreams::Triangles);
	const auto TexCoordsStream = Streams.Find(FRealtimeMeshStreamKey(FRealtimeMeshStreams::TexCoords));

	if (!PositionStream || !TriangleStream)
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("Unable to append collision vertices: missing position or triangle stream."));
		return false;
	}

	if (PositionStream->Num() < 3 || (TriangleStream->Num() * TriangleStream->GetNumElements()) < 3)
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("Unable to append collision vertices: not enough elements in streams."));
		return false;
	}

	if (!PositionStream->CanConvertTo<FVector3f>())
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("Unable to append collision vertices: position stream not convertible to FVector3f"));
		return false;
	}

	TMap<uint32, uint32> VertexRemap;
	const int32 OriginalVertexCount = Vertices.Num();
	
	TRealtimeMeshStreamBuilder<const TIndex3<uint32>, void> TrianglesData(*TriangleStream);
	Triangles.Reserve(Triangles.Num() + TriangleCount);
	Materials.Reserve(Triangles.Num() + TriangleCount);

	// Make sure materials is same length as triangles
	Materials.SetNumZeroed(Triangles.Num());


	int32 NewVertexIndex = OriginalVertexCount;
	const auto RemapVertex = [&](int32 Index)
	{
		if (const auto* Found = VertexRemap.Find(Index))
		{
			return *Found;
		}

		return VertexRemap.FindOrAdd(Index) = NewVertexIndex++;		
	};
	
	for (int32 TriIdx = 0; TriIdx < TriangleCount; TriIdx++)
	{
		TIndex3<int32>& Tri = Triangles.AddDefaulted_GetRef();
		Tri.V0 = RemapVertex(TrianglesData[TriIdx + FirstTriangle].GetElement(0).GetValue());
		Tri.V1 = RemapVertex(TrianglesData[TriIdx + FirstTriangle].GetElement(1).GetValue());
		Tri.V2 = RemapVertex(TrianglesData[TriIdx + FirstTriangle].GetElement(2).GetValue());

		Materials.Add(MaterialIndex);
	}	
	

	// Copy in the vertices
	Vertices.SetNum(OriginalVertexCount + VertexRemap.Num());
	const auto CopyVertices = [&]<typename DataType>()
	{		
		TRealtimeMeshStreamBuilder<const FVector3f, DataType> Positions(*PositionStream);
		for (const auto& Pair : VertexRemap)
		{
			checkSlow(Pair.Value >= OriginalVertexCount && Pair.Value < CurrentVertexIndex);
			
			Vertices[Pair.Value] = Positions[Pair.Key % Positions.Num()];
		}
	};

	if (PositionStream->IsOfType<FVector3f>())
	{
		CopyVertices.template operator()<FVector3f>();
	}
	else
	{
		CopyVertices.template operator()<void>();		
	}

	if (UPhysicsSettings::Get()->bSupportUVFromHitResults)
	{
		if (TexCoordsStream)
		{
			// We use the max number of UV channels across the appended data.
			// We simply zero-fill the unallocated channel sections.
			const int32 NumTexCoordStreams = FMath::Max(TexCoords.Num(), TexCoordsStream->GetNumElements());		
			TexCoords.SetNum(NumTexCoordStreams);
			for (int32 Index = 0; Index < NumTexCoordStreams; Index++)
			{
				TexCoords[Index].SetNumZeroed(Vertices.Num());
			}
			
			const auto CopyUVs = [&]<typename DataType>(int32 ChannelIndex)
			{		
				TRealtimeMeshStridedStreamBuilder<const FVector2f, DataType> UVData(*TexCoordsStream, ChannelIndex);
				auto& CollisionUVChannel = TexCoords[ChannelIndex];
				CollisionUVChannel.SetNumUninitialized(OriginalVertexCount + FMath::Min(VertexRemap.Num(), UVData.Num()));

				for (const auto& Pair : VertexRemap)
				{
					checkSlow(Pair.Value >= OriginalVertexCount && Pair.Value < CurrentVertexIndex);
			
					CollisionUVChannel[Pair.Value] = UVData[Pair.Key % UVData.Num()];
				}

				// Make sure the uv data is the same length as the position data
				if (PositionStream->Num() > UVData.Num())
				{
					CollisionUVChannel.SetNumZeroed(PositionStream->Num());
				}
			};

			for (int32 ChannelIndex = 0; ChannelIndex < TexCoordsStream->GetNumElements(); ChannelIndex++)
			{				
				if (TexCoordsStream->GetElementType() == GetRealtimeMeshDataElementType<FVector2f>())
				{
					CopyUVs.template operator()<FVector2f>(ChannelIndex);
				}
				else if (TexCoordsStream->GetElementType() == GetRealtimeMeshDataElementType<FVector2DHalf>())
				{
					CopyUVs.template operator()<FVector2DHalf>(ChannelIndex);		
				}
				else
				{
					CopyUVs.template operator()<void>(ChannelIndex);		
				}
			}

			for (int32 ChannelIndex = TexCoordsStream->GetNumElements(); ChannelIndex < TexCoords.Num(); ChannelIndex++)
			{
				TexCoords[ChannelIndex].SetNumZeroed(Vertices.Num());
			}			
		}
		else
		{
			// Zero fill data to vertex data length

			if (TexCoords.Num() < 1)
			{
				TexCoords.SetNum(1);
			}
			for (int32 Index = 0; Index < TexCoords.Num(); Index++)
			{
				TexCoords[Index].SetNumZeroed(Vertices.Num());
			}
		}
	}
	
	return true;	
}

TSharedPtr<FRealtimeMeshCookedTriMeshData> FRealtimeMeshCollisionMesh::Cook(bool bFlipNormals) const
{
	constexpr bool EnableMeshClean = false;
	
	if(Vertices.Num() == 0)
	{
		Cooked = MakeShared<FRealtimeMeshCookedTriMeshData>();
		return Cooked;
	}

	TArray<FVector3f> FinalVerts = Vertices;

	// Push indices into one flat array
	TArray<int32> FinalIndices;
	FinalIndices.Reserve(Triangles.Num() * 3);
	for(const RealtimeMesh::TIndex3<int32>& Tri : Triangles)
	{
		// NOTE: This is where the Winding order of the triangles are changed to be consistent throughout the rest of the physics engine
		// After this point we should have clockwise (CW) winding in left handed (LH) coordinates (or equivalently CCW in RH)
		// This is the opposite convention followed in most of the unreal engine
		FinalIndices.Add(bFlipNormals ? Tri.V1 : Tri.V0);
		FinalIndices.Add(bFlipNormals ? Tri.V0 : Tri.V1);
		FinalIndices.Add(Tri.V2);
	}

	/*if(EnableMeshClean)
	{
		Chaos::CleanTrimesh(FinalVerts, FinalIndices, &OutFaceRemap, &OutVertexRemap);
	}*/

	// Build particle list #BG Maybe allow TParticles to copy vectors?
	Chaos::FTriangleMeshImplicitObject::ParticlesType TriMeshParticles;
	TriMeshParticles.AddParticles(FinalVerts.Num());

	const int32 NumVerts = FinalVerts.Num();
	for(int32 VertIndex = 0; VertIndex < NumVerts; ++VertIndex)
	{
		TriMeshParticles.SetX(VertIndex, FinalVerts[VertIndex]);
	}

	TArray<int32> OutVertexRemap;
	TArray<int32> OutFaceRemap;

	// Build chaos triangle list. #BGTODO Just make the clean function take these types instead of double copying
	auto LambdaHelper = [this, &FinalVerts, &FinalIndices, &TriMeshParticles, &OutFaceRemap, &OutVertexRemap](auto& Triangles)
	{
		const int32 NumTriangles = FinalIndices.Num() / 3;
		bool bHasMaterials = Materials.Num() > 0;
		TArray<uint16> MaterialIndices;

		if(bHasMaterials)
		{
			MaterialIndices.Reserve(NumTriangles);
		}

		// Need to rebuild face remap array, in case there are any invalid triangles
		TArray<int32> OldFaceRemap = MoveTemp(OutFaceRemap);
		OutFaceRemap.Reserve(OldFaceRemap.Num());

		Triangles.Reserve(NumTriangles);
		for(int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
		{
			// Only add this triangle if it is valid
			const int32 BaseIndex = TriangleIndex * 3;
			const bool bIsValidTriangle = Chaos::FConvexBuilder::IsValidTriangle(
				FinalVerts[FinalIndices[BaseIndex]],
				FinalVerts[FinalIndices[BaseIndex + 1]],
				FinalVerts[FinalIndices[BaseIndex + 2]]);

			// TODO: Figure out a proper way to handle this. Could these edges get sewn together? Is this important?
			//if (ensureMsgf(bIsValidTriangle, TEXT("FChaosDerivedDataCooker::BuildTriangleMeshes(): Trimesh attempted cooked with invalid triangle!")));
			if(bIsValidTriangle)
			{
				Triangles.Add(Chaos::TVector<int32, 3>(FinalIndices[BaseIndex], FinalIndices[BaseIndex + 1], FinalIndices[BaseIndex + 2]));
				OutFaceRemap.Add(OldFaceRemap.IsEmpty()? TriangleIndex : OldFaceRemap[TriangleIndex]);

				if(bHasMaterials)
				{
					if(EnableMeshClean)
					{
						if(!ensure(OldFaceRemap.IsValidIndex(TriangleIndex)))
						{
							MaterialIndices.Empty();
							bHasMaterials = false;
						}
						else
						{
							const int32 OriginalIndex = OldFaceRemap[TriangleIndex];

							if(ensure(Materials.IsValidIndex(OriginalIndex)))
							{
								MaterialIndices.Add(Materials[OriginalIndex]);
							}
							else
							{
								MaterialIndices.Empty();
								bHasMaterials = false;
							}
						}
					}
					else
					{
						if(ensure(Materials.IsValidIndex(TriangleIndex)))
						{
							MaterialIndices.Add(Materials[TriangleIndex]);
						}
						else
						{
							MaterialIndices.Empty();
							bHasMaterials = false;
						}
					}
				}
			}
		}

		TUniquePtr<TArray<int32>> OutFaceRemapPtr = MakeUnique<TArray<int32>>(OutFaceRemap);
		TUniquePtr<TArray<int32>> OutVertexRemapPtr = Chaos::TriMeshPerPolySupport ? MakeUnique<TArray<int32>>(OutVertexRemap) : nullptr;
		Chaos::FTriangleMeshImplicitObjectPtr CookedMesh = new Chaos::FTriangleMeshImplicitObject(MoveTemp(TriMeshParticles), MoveTemp(Triangles), MoveTemp(MaterialIndices), MoveTemp(OutFaceRemapPtr), MoveTemp(OutVertexRemapPtr));

		
		
		// Propagate remapped indices from the FTriangleMeshImplicitObject back to the remap array
		const auto& TriangleMeshRef = *CookedMesh.GetReference();
		for (int32 TriangleIndex = 0; TriangleIndex < OutFaceRemap.Num(); TriangleIndex++)
		{
			OutFaceRemap[TriangleIndex] = TriangleMeshRef.GetExternalFaceIndexFromInternal(TriangleIndex);
		}
		
		FRealtimeMeshCollisionMeshCookedUVData UVInfo;
		if (UPhysicsSettings::Get()->bSupportUVFromHitResults)
		{
			UVInfo.FillFromTriMesh(*this);
		}
		
		Cooked = MakeShared<FRealtimeMeshCookedTriMeshData>(CookedMesh,
			MoveTemp(OutVertexRemap), MoveTemp(OutFaceRemap), MoveTemp(UVInfo));
	};

	if(FinalVerts.Num() < TNumericLimits<uint16>::Max())
	{
		TArray<Chaos::TVector<uint16, 3>> TrianglesSmallIdx;
		LambdaHelper(TrianglesSmallIdx);
	}
	else
	{
		TArray<Chaos::TVector<int32, 3>> TrianglesLargeIdx;
		LambdaHelper(TrianglesLargeIdx);
	}

	return Cooked;
}






int32 FRealtimeMeshComplexGeometry::AddMesh(const FRealtimeMeshCollisionMesh& InMesh)
{
	const int32 NewIndex = Meshes.Add(InMesh);
	AddToNameMap(MeshesNameMap, InMesh.Name, NewIndex);
	return NewIndex;	
}

int32 FRealtimeMeshComplexGeometry::AddMesh(FRealtimeMeshCollisionMesh&& InMesh)
{
	const int32 NewIndex = Meshes.Add(MoveTemp(InMesh));
	AddToNameMap(MeshesNameMap, InMesh.Name, NewIndex);
	return NewIndex;	
}

bool FRealtimeMeshComplexGeometry::InsertMesh(int32 Index, const FRealtimeMeshCollisionMesh& InMesh)
{
	if (!Meshes.IsValidIndex(Index) && Index >= 0)
	{
		Meshes.Insert(Index, InMesh);
		AddToNameMap(MeshesNameMap, InMesh.Name, Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshComplexGeometry::InsertMesh(int32 Index, FRealtimeMeshCollisionMesh&& InMesh)
{
	if (!Meshes.IsValidIndex(Index) && Index >= 0)
	{
		Meshes.Insert(Index, MoveTemp(InMesh));
		AddToNameMap(MeshesNameMap, InMesh.Name, Index);
		return true;
	}
	return false;
}

int32 FRealtimeMeshComplexGeometry::GetMeshIndexByName(FName MeshName) const
{
	return GetIndexFromNameMap(MeshesNameMap, MeshName);
}

bool FRealtimeMeshComplexGeometry::GetMeshByName(FName MeshName, FRealtimeMeshCollisionMesh& OutMesh)
{
	const int32 Index = GetMeshIndexByName(MeshName);
	if (Index != INDEX_NONE)
	{
		OutMesh = Meshes[Index];
		return true;
	}
	return false;
}

bool FRealtimeMeshComplexGeometry::UpdateMesh(int32 Index, const FRealtimeMeshCollisionMesh& InMesh)
{
	if (Meshes.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(MeshesNameMap, Meshes[Index].Name, Index);
		Meshes[Index] = InMesh;
		AddToNameMap(MeshesNameMap, InMesh.Name, Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshComplexGeometry::UpdateMesh(int32 Index, FRealtimeMeshCollisionMesh&& InMesh)
{
	if (Meshes.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(MeshesNameMap, Meshes[Index].Name, Index);
		Meshes[Index] = MoveTemp(InMesh);
		AddToNameMap(MeshesNameMap, InMesh.Name, Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshComplexGeometry::RemoveMesh(int32 Index)
{
	if (Meshes.IsValidIndex(Index))
	{
		RemoveIndexFromNameMap(MeshesNameMap, Meshes[Index].Name, Index);
		Meshes.RemoveAt(Index);
		return true;
	}
	return false;
}

bool FRealtimeMeshComplexGeometry::RemoveMesh(FName MeshName)
{
	const int32 Index = GetMeshIndexByName(MeshName);
	if (Index != INDEX_NONE)
	{
		RemoveMesh(Index);
		return true;
	}
	return false;
}

TArray<int32> FRealtimeMeshComplexGeometry::GetMeshIDsNeedingCook() const
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

void FRealtimeMeshComplexGeometry::CookMesh(int32 Index) const
{
	if (Meshes.IsValidIndex(Index))
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		Meshes[Index].Cook();
	}
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshComplexGeometry& ComplexGeometry)
{
	Ar << ComplexGeometry.Meshes;

	// Rebuild name maps on load
	if (Ar.IsLoading())
	{
		ComplexGeometry.MeshesNameMap.Empty();
		for (TSparseArray<FRealtimeMeshCollisionMesh>::TConstIterator It(ComplexGeometry.Meshes); It; ++It)
		{
			AddToNameMap(ComplexGeometry.MeshesNameMap, It->Name, It.GetIndex());
		}
	}

	return Ar;
}

void FRealtimeMeshComplexGeometry::CopyToBodySetup(UBodySetup* BodySetup, TArray<FRealtimeMeshCollisionMeshCookedUVData>& OutUVData) const
{
	for (const auto& Mesh : Meshes)
	{
		if (Mesh.HasCookedMesh())
		{
			const auto CookedMeshData = Mesh.GetCooked();
			auto MeshData = CookedMeshData->GetMesh();

#if RMC_ENGINE_ABOVE_5_4
			BodySetup->TriMeshGeometries.Add(MeshData);
#else
			BodySetup->ChaosTriMeshes.Add(MeshData);
#endif
			/*NewBodySetup->bSupportUVsAndFaceRemap;
			NewBodySetup->FaceRemap = PendingCollisionUpdate->TriMeshData.Cook();
			NewBodySetup->UVInfo;*/
			BodySetup->bCreatedPhysicsMeshes = true;

			OutUVData.Add(CookedMeshData->GetUVInfo());
		}
	}	
}















// Sphere Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddSphere(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   const FRealtimeMeshCollisionSphere& InSphere, int32& OutIndex)
{
	OutIndex = SimpleGeometry.AddSphere(InSphere);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertSphere(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, const FRealtimeMeshCollisionSphere& InSphere, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.InsertSphere(Index, InSphere);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetSphereByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                         FName SphereName, bool& OutSuccess, FRealtimeMeshCollisionSphere& OutSphere)
{
	OutSuccess = SimpleGeometry.GetSphereByName(SphereName, OutSphere);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateSphere(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, const FRealtimeMeshCollisionSphere& InSphere, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.UpdateSphere(Index, InSphere);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveSphere(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveSphere(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveSphereByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                            FName SphereName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveSphere(SphereName);
	return SimpleGeometry;
}


// Box Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddBox(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                const FRealtimeMeshCollisionBox& InBox, int32& OutIndex)
{
	OutIndex = SimpleGeometry.AddBox(InBox);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertBox(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   int32 Index, const FRealtimeMeshCollisionBox& InBox, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.InsertBox(Index, InBox);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetBoxByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      FName BoxName, bool& OutSuccess, FRealtimeMeshCollisionBox& OutBox)
{
	OutSuccess = SimpleGeometry.GetBoxByName(BoxName, OutBox);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateBox(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   int32 Index, const FRealtimeMeshCollisionBox& InBox, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.UpdateBox(Index, InBox);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveBox(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveBox(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveBoxByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                         FName BoxName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveBox(BoxName);
	return SimpleGeometry;
}


// Capsule Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                    const FRealtimeMeshCollisionCapsule& InCapsule, int32& OutIndex)
{
	OutIndex = SimpleGeometry.AddCapsule(InCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                       int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.InsertCapsule(Index, InCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetCapsuleByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                          FName CapsuleName, bool& OutSuccess, FRealtimeMeshCollisionCapsule& OutCapsule)
{
	OutSuccess = SimpleGeometry.GetCapsuleByName(CapsuleName, OutCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                       int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.UpdateCapsule(Index, InCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                       int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveCapsule(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveCapsuleByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                             FName CapsuleName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveCapsule(CapsuleName);
	return SimpleGeometry;
}


// Tapered Capsule Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddTaperedCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                           const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule, int32& OutIndex)
{
	OutIndex = SimpleGeometry.AddTaperedCapsule(InTaperedCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertTaperedCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                              int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule,
                                                                                              bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.InsertTaperedCapsule(Index, InTaperedCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetTaperedCapsuleByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                                 FName TaperedCapsuleName, bool& OutSuccess,
                                                                                                 FRealtimeMeshCollisionTaperedCapsule& OutTaperedCapsule)
{
	OutSuccess = SimpleGeometry.GetTaperedCapsuleByName(TaperedCapsuleName, OutTaperedCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateTaperedCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                              int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule,
                                                                                              bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.UpdateTaperedCapsule(Index, InTaperedCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveTaperedCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                              int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveTaperedCapsule(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveTaperedCapsuleByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                                    FName TaperedCapsuleName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveTaperedCapsule(TaperedCapsuleName);
	return SimpleGeometry;
}


// Convex Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddConvex(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   const FRealtimeMeshCollisionConvex& InConvex, int32& OutIndex)
{
	OutIndex = SimpleGeometry.AddConvexHull(InConvex);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertConvex(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, const FRealtimeMeshCollisionConvex& InConvex, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.InsertConvexHull(Index, InConvex);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetConvexByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                         FName ConvexName, bool& OutSuccess, FRealtimeMeshCollisionConvex& OutConvex)
{
	OutSuccess = SimpleGeometry.GetConvexHullByName(ConvexName, OutConvex);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateConvex(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, const FRealtimeMeshCollisionConvex& InConvex, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.UpdateConvexHull(Index, InConvex);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveConvex(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveConvexHull(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveConvexByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                            FName ConvexName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.RemoveConvexHull(ConvexName);
	return SimpleGeometry;
}

bool URealtimeMeshCollisionTools::FindCollisionUVRealtimeMesh(const FHitResult& Hit, int32 UVChannel, FVector2D& UV)
{
	bool bSuccess = false;
	
	if (UPhysicsSettings::Get()->bSupportUVFromHitResults)
	{
		URealtimeMeshComponent* HitComp = Cast<URealtimeMeshComponent>(Hit.Component.Get());
		if (HitComp)
		{
			UBodySetup* BodySetup = HitComp->GetBodySetup();
			URealtimeMesh* RealtimeMesh = HitComp->GetRealtimeMesh();
			if (BodySetup && RealtimeMesh)
			{
				const FVector LocalHitPos = HitComp->GetComponentToWorld().InverseTransformPosition(Hit.Location);
				bSuccess = RealtimeMesh->CalcTexCoordAtLocation(LocalHitPos, Hit.ElementIndex, Hit.FaceIndex, UVChannel, UV);
			}
		}
	}

	return bSuccess;
}


/*
TSharedPtr<Chaos::FTriangleMeshImplicitObject> FRealtimeMeshCollisionCooker::CookTriMesh(const FRealtimeMeshTriMeshData& TriMeshData)
{
	using namespace Chaos;
	
	auto& Vertices = TriMeshData.GetVertices();
	const auto& Triangles = TriMeshData.GetTriangles();
	const auto& Materials = TriMeshData.GetMaterials();
	const auto& TexCoords = TriMeshData.GetUVs();

	auto CookTriMeshHelper = [&]<typename IdxType>() mutable -> TSharedPtr<FTriangleMeshImplicitObject>
	{
		TParticles<FRealSingle, 3> Particles;
		Particles.AddParticles(Vertices.Num());

		for (int32 Idx = 0; Idx < Vertices.Num(); Idx++)
		{
			Particles.SetX(Idx, Vertices[Idx]);
		}

		TArray<TVector<IdxType, 3>> CollisionTriangles;
		CollisionTriangles.Reserve(Triangles.Num());

		TArray<uint16> CollisionMaterials;

		for (int32 Idx = 0; Idx < Triangles.Num(); Idx++)
		{
			const TVector<int32, 3> Triangle(
				Triangles[Idx].v0,
				Triangles[Idx].v1,
				Triangles[Idx].v2);

			if (!FConvexBuilder::IsValidTriangle(
				Particles.GetX(Triangle.X),
				Particles.GetX(Triangle.Y),
				Particles.GetX(Triangle.Z)))
			{
				continue;
			}

			CollisionTriangles.Add(Triangle);
			CollisionMaterials.Add(Materials[Idx]);
		}

		if (CollisionTriangles.IsEmpty())
		{
			return nullptr;
		}
		//FTriangleMeshImplicitObject TriObj(MoveTemp(Particles), MoveTemp(Triangles), TArray<uint16>(FaceMaterials), BVH, nullptr, nullptr, true))

		// This is slow cook
		return MakeShared<FTriangleMeshImplicitObject>(MoveTemp(Particles), MoveTemp(CollisionTriangles), MoveTemp(CollisionMaterials), nullptr, nullptr, true);


	};

	return TriMeshData.GetVertices().Num() > TNumericLimits<uint16>::Max()? CookTriMeshHelper.operator()<int32>() : CookTriMeshHelper.operator()<uint16>();
}
*/

