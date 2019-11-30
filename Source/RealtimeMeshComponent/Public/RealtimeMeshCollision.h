// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Components/MeshComponent.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshCollision.generated.h"


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionConvexMesh
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionConvex)
	TArray<FVector> VertexBuffer;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionConvex)
	FBox BoundingBox;

	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshCollisionConvexMesh& Section)
	{
		Ar << Section.VertexBuffer;
		Ar << Section.BoundingBox;
		return Ar;
	}
};


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionSphere
{
	GENERATED_USTRUCT_BODY()

	/** Position of the sphere's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionSphere)
	FVector Center;

	/** Radius of the sphere */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionSphere)
	float Radius;

	FRealtimeMeshCollisionSphere()
		: Center(FVector::ZeroVector)
		, Radius(1)
	{

	}

	FRealtimeMeshCollisionSphere(float r)
		: Center(FVector::ZeroVector)
		, Radius(r)
	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshCollisionSphere& Sphere)
	{
		Ar << Sphere.Center;
		Ar << Sphere.Radius;
		return Ar;
	}
};


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionBox
{
	GENERATED_USTRUCT_BODY()

	/** Position of the box's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionBox)
	FVector Center;

	/** Rotation of the box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionBox)
	FRotator Rotation;

	/** Extents of the box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionBox)
	FVector Extents;


	FRealtimeMeshCollisionBox()
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(1, 1, 1)
	{

	}

	FRealtimeMeshCollisionBox(float s)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(s, s, s)
	{

	}

	FRealtimeMeshCollisionBox(float InX, float InY, float InZ)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(InX, InY, InZ)

	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshCollisionBox& Box)
	{
		Ar << Box.Center;
		Ar << Box.Rotation;
		Ar << Box.Extents;
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshCollisionCapsule
{
	GENERATED_USTRUCT_BODY()

	/** Position of the capsule's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionCapsule)
	FVector Center;

	/** Rotation of the capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionCapsule)
	FRotator Rotation;

	/** Radius of the capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionCapsule)
	float Radius;

	/** This is of line-segment ie. add Radius to both ends to find total length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionCapsule)
	float Length;

	FRealtimeMeshCollisionCapsule()
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Radius(1), Length(1)

	{

	}

	FRealtimeMeshCollisionCapsule(float InRadius, float InLength)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Radius(InRadius), Length(InLength)
	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshCollisionCapsule& Capsule)
	{
		Ar << Capsule.Center;
		Ar << Capsule.Rotation;
		Ar << Capsule.Radius;
		Ar << Capsule.Length;
		return Ar;
	}
};

struct FRealtimeMeshCollisionVertexStream
{
private:
	TArray<FVector> Data;

public:
	FRealtimeMeshCollisionVertexStream() { }

	void SetNum(int32 NewNum, bool bAllowShrinking)
	{
		Data.SetNum(NewNum, bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num();
	}

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack);
	}

	FORCEINLINE int32 Add(const FVector& InPosition)
	{
		return Data.Add(InPosition);
	}

	FORCEINLINE const FVector& GetPosition(int32 Index) const
	{
		return Data[Index];
	}

	FORCEINLINE void SetPosition(int32 Index, const FVector& NewPosition)
	{
		Data[Index] = NewPosition;
	}

private:
	TArray<FVector>&& TakeContents()
	{
		return MoveTemp(Data);
	}

	friend class URealtimeMesh;
};

struct FRealtimeMeshCollisionTriangleStream
{
private:
	TArray<FTriIndices> Data;

public:
	FRealtimeMeshCollisionTriangleStream()
	{

	}

	void SetNum(int32 NewNum, bool bAllowShrinking)
	{
		Data.SetNum(NewNum, bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num();
	}

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack);
	}

	FORCEINLINE int32 Add(int32 IndexA, int32 IndexB, int32 IndexC)
	{
		FTriIndices NewTri;
		NewTri.v0 = IndexA;
		NewTri.v1 = IndexB;
		NewTri.v2 = IndexC;

		return Data.Add(NewTri);
	}

	FORCEINLINE void GetTriangleIndices(int32 TriangleIndex, int32& OutIndexA, int32& OutIndexB, int32& OutIndexC)
	{
		FTriIndices& Tri = Data[TriangleIndex];
		OutIndexA = Tri.v0;
		OutIndexB = Tri.v1;
		OutIndexC = Tri.v2;
	}

	FORCEINLINE void SetTriangleIndices(int32 TriangleIndex, int32 IndexA, int32 IndexB, int32 IndexC)
	{
		FTriIndices& Tri = Data[TriangleIndex];
		Tri.v0 = IndexA;
		Tri.v1 = IndexB;
		Tri.v2 = IndexC;
	}

private:
	TArray<FTriIndices>&& TakeContents()
	{
		return MoveTemp(Data);
	}

	friend class URealtimeMesh;
};

struct FRealtimeMeshCollisionTexCoordStream
{
private:
	TArray<TArray<FVector2D>> Data;

public:
	FRealtimeMeshCollisionTexCoordStream()
	{

	}

	void SetNumChannels(int32 NewNumChannels, bool bAllowShrinking)
	{
		Data.SetNum(NewNumChannels, bAllowShrinking);
	}

	void SetNumCoords(int32 ChannelId, int32 NewNumCoords, bool bAllowShrinking)
	{
		Data[ChannelId].SetNum(NewNumCoords, bAllowShrinking);
	}

	void SetNum(int32 NewNumChannels, int32 NewNumCoords, bool bAllowShrinking)
	{
		Data.SetNum(NewNumChannels, bAllowShrinking);
		for (int32 Index = 0; Index < Data.Num(); Index++)
		{
			Data[Index].SetNum(NewNumCoords, bAllowShrinking);
		}
	}

	int32 NumChannels() const
	{
		return Data.Num();
	}

	int32 NumTexCoords(int32 ChannelId)
	{
		return Data[ChannelId].Num();
	}

	void EmptyChannel(int32 ChannelId, int32 Slack = 0)
	{
		Data[ChannelId].Empty(Slack);
	}

	FORCEINLINE int32 Add(int32 ChannelId, const FVector2D& NewTexCoord)
	{
		return Data[ChannelId].Add(NewTexCoord);
	}

	FORCEINLINE FVector2D GetTexCoord(int32 ChannelId, int32 TexCoordIndex)
	{
		return Data[ChannelId][TexCoordIndex];
	}

	FORCEINLINE void SetTexCoord(int32 ChannelId, int32 TexCoordIndex, const FVector2D& NewTexCoord)
	{
		Data[ChannelId][TexCoordIndex] = NewTexCoord;
	}
private:
	TArray<TArray<FVector2D>>&& TakeContents()
	{
		return MoveTemp(Data);
	}

	friend class URealtimeMesh;
};

struct FRealtimeMeshCollisionMaterialIndexStream
{
private:
	TArray<uint16> Data;

public:
	FRealtimeMeshCollisionMaterialIndexStream() { }

	void SetNum(int32 NewNum, bool bAllowShrinking)
	{
		Data.SetNum(NewNum, bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num();
	}

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack);
	}

	FORCEINLINE int32 Add(uint16 NewMaterialIndex)
	{
		return Data.Add(NewMaterialIndex);
	}

	FORCEINLINE uint16 GetMaterialIndex(int32 Index) const
	{
		return Data[Index];
	}

	FORCEINLINE void SetMaterialIndex(int32 Index, uint16 NewMaterialIndex)
	{
		Data[Index] = NewMaterialIndex;
	}

private:
	TArray<uint16>&& TakeContents()
	{
		return MoveTemp(Data);
	}

	friend class URealtimeMesh;
};




USTRUCT(BlueprintType)
struct FRealtimeMeshCollisionSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollision)
	bool bUseComplexAsSimple;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollision)
	bool bUseAsyncCooking;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollision)
	ERealtimeMeshCollisionCookingMode CookingMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionShapes)
	TArray<FRealtimeMeshCollisionConvexMesh> ConvexElements;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionShapes)
	TArray<FRealtimeMeshCollisionSphere> Spheres;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionShapes)
	TArray<FRealtimeMeshCollisionBox> Boxes; 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RealtimeMeshCollisionShapes)
	TArray<FRealtimeMeshCollisionCapsule> Capsules;

	FRealtimeMeshCollisionSettings()
		: bUseComplexAsSimple(true)
		, bUseAsyncCooking(false)
		, CookingMode(ERealtimeMeshCollisionCookingMode::CollisionPerformance)
	{

	}
};

struct FRealtimeMeshCollisionData
{
	FRealtimeMeshCollisionVertexStream Vertices;
	FRealtimeMeshCollisionTriangleStream Triangles;
	FRealtimeMeshCollisionTexCoordStream TexCoords;
	FRealtimeMeshCollisionMaterialIndexStream MaterialIndices;

	bool bFlipNormals;
	bool bDeformableMesh;
	bool bFastCook;
	bool bDisableActiveEdgePrecompute;

	FRealtimeMeshCollisionData()
		: bFlipNormals(false)
		, bDeformableMesh(false)
		, bFastCook(false)
		, bDisableActiveEdgePrecompute(false)
	{

	}
};