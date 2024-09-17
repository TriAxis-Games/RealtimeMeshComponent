// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interface_CollisionDataProviderCore.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RealtimeMeshCollision.h"
#include "RealtimeMeshCollisionLibrary.generated.h"


UCLASS()
class REALTIMEMESHCOMPONENT_API URealtimeMeshCollisionTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Collision")
	static bool FindCollisionUVRealtimeMesh(const struct FHitResult& Hit, int32 UVChannel, FVector2D& UV);

	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Collision")
	static void CookConvexHull(FRealtimeMeshCollisionConvex& ConvexHull);

	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh|Collision")
	static void CookComplexMesh(FRealtimeMeshCollisionMesh& CollisionMesh);
	
	static void CopySimpleGeometryToBodySetup(const FRealtimeMeshSimpleGeometry& SimpleGeom, UBodySetup* BodySetup);
	static void CopyComplexGeometryToBodySetup(const FRealtimeMeshComplexGeometry& ComplexGeom, UBodySetup* BodySetup, TArray<FRealtimeMeshCollisionMeshCookedUVData>& OutUVData);

	
	static bool AppendStreamsToCollisionMesh(FRealtimeMeshCollisionMesh& CollisionMesh, const RealtimeMesh::FRealtimeMeshStreamSet& Streams, int32 MaterialIndex);
	static bool AppendStreamsToCollisionMesh(FRealtimeMeshCollisionMesh& CollisionMesh, const RealtimeMesh::FRealtimeMeshStreamSet& Streams, int32 MaterialIndex, int32 FirstTriangle, int32 TriangleCount);
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





