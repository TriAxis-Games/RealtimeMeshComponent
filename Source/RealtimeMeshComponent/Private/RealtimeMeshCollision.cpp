// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshCollision.h"
#include "PhysicsEngine/BodySetup.h"
#include "Interface_CollisionDataProviderCore.h"
#include "RealtimeMeshCore.h"


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

static FArchive& operator<<(FArchive& Ar, FTriIndices& Indices)
{
	Ar << Indices.v0 << Indices.v1 << Indices.v2;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshTriMeshData& MeshData)
{
	Ar << MeshData.Vertices;
	Ar << MeshData.Triangles;
	Ar << MeshData.Materials;
	Ar << MeshData.UVs;
	return Ar;
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


// Sphere Functions

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
		BodyConvex.VertexData = Convex.Vertices;
		BodyConvex.SetTransform(FTransform(Convex.Rotation.Quaternion(), Convex.Center));
		BodyConvex.UpdateElemBox();
		BodyConvex.SetContributeToMass(Convex.bContributesToMass);
		BodyConvex.SetName(Convex.Name);
	}
}

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
