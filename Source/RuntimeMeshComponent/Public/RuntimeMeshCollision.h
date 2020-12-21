// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "RuntimeMeshCore.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Components/MeshComponent.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "RuntimeMeshCollision.generated.h"

class URuntimeMeshProvider;


USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionConvexMesh
{
	GENERATED_BODY()

public:
	FRuntimeMeshCollisionConvexMesh() : BoundingBox(ForceInit) { }
	FRuntimeMeshCollisionConvexMesh(const TArray<FVector>& InVertexBuffer)
		: VertexBuffer(InVertexBuffer)
		, BoundingBox(InVertexBuffer)
	{
	}
	FRuntimeMeshCollisionConvexMesh(TArray<FVector>&& InVertexBuffer)
		: VertexBuffer(InVertexBuffer)
		, BoundingBox(VertexBuffer)
	{
	}
	FRuntimeMeshCollisionConvexMesh(const TArray<FVector>& InVertexBuffer, const FBox& InBoundingBox)
		: VertexBuffer(InVertexBuffer)
		, BoundingBox(InBoundingBox)
	{
	}
	FRuntimeMeshCollisionConvexMesh(TArray<FVector>&& InVertexBuffer, const FBox& InBoundingBox)
		: VertexBuffer(InVertexBuffer)
		, BoundingBox(VertexBuffer)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Convex")
	TArray<FVector> VertexBuffer;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Convex")
	FBox BoundingBox;

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionConvexMesh& Section)
	{
		Ar << Section.VertexBuffer;
		Ar << Section.BoundingBox;
		return Ar;
	}
};


USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionSphere
{
	GENERATED_BODY()

public:
	/** Position of the sphere's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Sphere")
	FVector Center;

	/** Radius of the sphere */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Sphere")
	float Radius;

	FRuntimeMeshCollisionSphere()
		: Center(FVector::ZeroVector)
		, Radius(1)
	{

	}

	FRuntimeMeshCollisionSphere(float InRadius)
		: Center(FVector::ZeroVector)
		, Radius(InRadius)
	{

	}

	FRuntimeMeshCollisionSphere(const FVector& Center, float InRadius)
		: Center(Center)
		, Radius(InRadius)
	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionSphere& Sphere)
	{
		Ar << Sphere.Center;
		Ar << Sphere.Radius;
		return Ar;
	}
};


USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionBox
{
	GENERATED_BODY()

public:
	/** Position of the box's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Box")
	FVector Center;

	/** Rotation of the box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Box")
	FRotator Rotation;

	/** Extents of the box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Box")
	FVector Extents;


	FRuntimeMeshCollisionBox()
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(1, 1, 1)
	{

	}

	FRuntimeMeshCollisionBox(float s)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(s, s, s)
	{

	}

	FRuntimeMeshCollisionBox(float InX, float InY, float InZ)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(InX, InY, InZ)

	{

	}

	FRuntimeMeshCollisionBox(const FVector& InCenter, const FRotator& InRotation, float InX, float InY, float InZ)
		: Center(InCenter)
		, Rotation(InRotation)
		, Extents(InX, InY, InZ)

	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionBox& Box)
	{
		Ar << Box.Center;
		Ar << Box.Rotation;
		Ar << Box.Extents;
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionCapsule
{
	GENERATED_BODY()

public:
	/** Position of the capsule's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Capsule")
	FVector Center;

	/** Rotation of the capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Capsule")
	FRotator Rotation;

	/** Radius of the capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Capsule")
	float Radius;

	/** This is of line-segment ie. add Radius to both ends to find total length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Capsule")
	float Length;

	FRuntimeMeshCollisionCapsule()
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Radius(1), Length(1)

	{

	}

	FRuntimeMeshCollisionCapsule(float InRadius, float InLength)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Radius(InRadius), Length(InLength)
	{

	}

	FRuntimeMeshCollisionCapsule(const FVector& InCenter, const FRotator& InRotator, float InRadius, float InLength)
		: Center(InCenter)
		, Rotation(InRotator)
		, Radius(InRadius)
		, Length(InLength)
	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionCapsule& Capsule)
	{
		Ar << Capsule.Center;
		Ar << Capsule.Rotation;
		Ar << Capsule.Radius;
		Ar << Capsule.Length;
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionVertexStream
{
	GENERATED_USTRUCT_BODY()

private:
	TArray<FVector> Data;

public:
	FRuntimeMeshCollisionVertexStream() { }

	void SetNum(int32 NewNum, bool bAllowShrinking = true)
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

	void Reserve(int32 Number)
	{
		Data.Reserve(Number);
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

	friend class URuntimeMesh;

public:
	bool Serialize(FArchive& Ar)
	{
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshCollisionVertexStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshCollisionVertexStream>
{
	enum
	{
		WithSerializer = true
	};
};

static FArchive& operator<<(FArchive& Ar, FTriIndices& Tri)
{
	Ar << Tri.v0;
	Ar << Tri.v1;
	Ar << Tri.v2;
	return Ar;
}


USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionTriangleStream
{
	GENERATED_USTRUCT_BODY()

private:
	TArray<FTriIndices> Data;

public:
	FRuntimeMeshCollisionTriangleStream()
	{

	}

	//Sets a new number of triangles
	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum, bAllowShrinking);
	}

	//Returns the number of triangles
	int32 Num() const
	{
		return Data.Num();
	}

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack);
	}

	//Reserves a number of triangles
	void Reserve(int32 Number)
	{
		Data.Reserve(Number);
	}

	FORCEINLINE int32 Add(int32 IndexA, int32 IndexB, int32 IndexC)
	{
		FTriIndices NewTri;
		NewTri.v0 = IndexA;
		NewTri.v1 = IndexB;
		NewTri.v2 = IndexC;

		return Data.Add(NewTri);
	}

	FORCEINLINE void GetTriangleIndices(int32 TriangleIndex, int32& OutIndexA, int32& OutIndexB, int32& OutIndexC) const
	{
		const FTriIndices& Tri = Data[TriangleIndex];
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

	friend class URuntimeMesh;

public:


	bool Serialize(FArchive& Ar)
	{
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshCollisionTriangleStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshCollisionTriangleStream>
{
	enum
	{
		WithSerializer = true
	};
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionTexCoordStream
{
	GENERATED_USTRUCT_BODY()

private:
	TArray<TArray<FVector2D>> Data;

public:
	FRuntimeMeshCollisionTexCoordStream()
	{

	}

	void SetNumChannels(int32 NewNumChannels, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNumChannels, bAllowShrinking);
	}

	void SetNumCoords(int32 ChannelId, int32 NewNumCoords, bool bAllowShrinking)
	{
		Data[ChannelId].SetNum(NewNumCoords, bAllowShrinking);
	}

	void SetNum(int32 NewNumChannels, int32 NewNumCoords, bool bAllowShrinking = true)
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

	int32 NumTexCoords(int32 ChannelId) const
	{
		return Data[ChannelId].Num();
	}

	void Empty(int32 Slack = 0)
	{
		for (int32 Index = 0; Index < Data.Num(); Index++)
		{
			Data[Index].Empty(Slack);
		}
	}

	void Reserve(int32 NumChannels, int32 Number)
	{
		Data.SetNum(NumChannels, false);
		for (int32 Index = 0; Index < Data.Num(); Index++)
		{
			Data[Index].Reserve(Number);
		}
	}

	void EmptyChannel(int32 ChannelId, int32 Slack = 0)
	{
		Data[ChannelId].Empty(Slack);
	}

	FORCEINLINE int32 Add(int32 ChannelId, const FVector2D& NewTexCoord)
	{
		return Data[ChannelId].Add(NewTexCoord);
	}

	FORCEINLINE FVector2D GetTexCoord(int32 ChannelId, int32 TexCoordIndex) const
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

	friend class URuntimeMesh;

public:
	bool Serialize(FArchive& Ar)
	{
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshCollisionTexCoordStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshCollisionTexCoordStream>
{
	enum
	{
		WithSerializer = true
	};
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionMaterialIndexStream
{
	GENERATED_USTRUCT_BODY()

private:
	TArray<uint16> Data;

public:
	FRuntimeMeshCollisionMaterialIndexStream() { }

	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum, bAllowShrinking);
	}

	void SetNumZeroed(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNumZeroed(NewNum, bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num();
	}

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack);
	}

	void Reserve(int32 Number)
	{
		Data.Reserve(Number);
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

	friend class URuntimeMesh;

public:
	bool Serialize(FArchive& Ar)
	{
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshCollisionMaterialIndexStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshCollisionMaterialIndexStream>
{
	enum
	{
		WithSerializer = true
	};
};




USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionSettings
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Settings")
	bool bUseComplexAsSimple;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Settings")
	bool bUseAsyncCooking;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Settings")
	ERuntimeMeshCollisionCookingMode CookingMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Settings")
	TArray<FRuntimeMeshCollisionConvexMesh> ConvexElements;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Settings")
	TArray<FRuntimeMeshCollisionSphere> Spheres;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Settings")
	TArray<FRuntimeMeshCollisionBox> Boxes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|Settings")
	TArray<FRuntimeMeshCollisionCapsule> Capsules;

	FRuntimeMeshCollisionSettings();

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionSettings& Settings)
	{
		Ar << Settings.bUseComplexAsSimple;
		Ar << Settings.bUseAsyncCooking;
		Ar << Settings.CookingMode;

		Ar << Settings.ConvexElements;
		Ar << Settings.Spheres;
		Ar << Settings.Boxes;
		Ar << Settings.Capsules;
		return Ar;
	}
};


/* Source of a mesh face, whether it was collision or rendering */
UENUM(BlueprintType)
enum class ERuntimeMeshCollisionFaceSourceType : uint8
{
	/* Collision face was created by a collision specific source */
	Collision UMETA(DisplayName = "Collision"),
	/* Collision face was created by a renderable section */
	Renderable UMETA(DisplayName = "Renderable"),
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionSourceSectionInfo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	int32 StartIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	int32 EndIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	TWeakObjectPtr<URuntimeMeshProvider> SourceProvider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	int32 SectionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	ERuntimeMeshCollisionFaceSourceType SourceType;

	FRuntimeMeshCollisionSourceSectionInfo()
		: StartIndex(INDEX_NONE)
		, EndIndex(INDEX_NONE)
		, SourceProvider(nullptr)
		, SectionId(INDEX_NONE)
		, SourceType(ERuntimeMeshCollisionFaceSourceType::Collision)
	{

	}

	FRuntimeMeshCollisionSourceSectionInfo(int32 InStartIndex, int32 InEndIndex, TWeakObjectPtr<URuntimeMeshProvider> InSourceProvider, int32 InSectionId, ERuntimeMeshCollisionFaceSourceType InSourceType)
		: StartIndex(InStartIndex)
		, EndIndex(InEndIndex)
		, SourceProvider(InSourceProvider)
		, SectionId(InSectionId)
		, SourceType(InSourceType)
	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionSourceSectionInfo& Data)
	{
		Ar << Data.StartIndex;
		Ar << Data.EndIndex;

		Ar << Data.SourceProvider;
		Ar << Data.SectionId;
		Ar << Data.SourceType;
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionHitInfo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	TWeakObjectPtr<URuntimeMeshProvider> SourceProvider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	ERuntimeMeshCollisionFaceSourceType SourceType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	int32 SectionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	int32 FaceIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	UMaterialInterface* Material;

	FRuntimeMeshCollisionHitInfo()
		: SourceProvider(nullptr)
		, SourceType(ERuntimeMeshCollisionFaceSourceType::Collision)
		, SectionId(0)
		, FaceIndex(0)
		, Material(nullptr)
	{

	}
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionData
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	FRuntimeMeshCollisionVertexStream Vertices;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	FRuntimeMeshCollisionTriangleStream Triangles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	FRuntimeMeshCollisionTexCoordStream TexCoords;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	FRuntimeMeshCollisionMaterialIndexStream MaterialIndices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	TArray<FRuntimeMeshCollisionSourceSectionInfo> CollisionSources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	bool bFlipNormals;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	bool bDeformableMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	bool bFastCook;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	bool bDisableActiveEdgePrecompute;

	FRuntimeMeshCollisionData()
		: bFlipNormals(true)
		, bDeformableMesh(false)
		, bFastCook(false)
		, bDisableActiveEdgePrecompute(false)
	{

	}

	bool HasValidMeshData()
	{
		return Vertices.Num() >= 3 && Triangles.Num() >= 3;
	}

	void ReserveVertices(int32 Number, int32 NumTexCoordChannels = 1)
	{
		Vertices.Reserve(Number);
		TexCoords.Reserve(NumTexCoordChannels, Number);
		MaterialIndices.Reserve(Number);
	}


	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionData& Data)
	{
		Data.Vertices.Serialize(Ar);
		Data.Triangles.Serialize(Ar);
		Data.TexCoords.Serialize(Ar);
		Data.MaterialIndices.Serialize(Ar);

		Ar << Data.CollisionSources;

		Ar << Data.bFlipNormals;
		Ar << Data.bDeformableMesh;
		Ar << Data.bFastCook;
		Ar << Data.bDisableActiveEdgePrecompute;
		return Ar;
	}
};



USTRUCT()
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshAsyncBodySetupData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	UBodySetup* BodySetup;

	UPROPERTY()
	TArray<FRuntimeMeshCollisionSourceSectionInfo> CollisionSources;

	FRuntimeMeshAsyncBodySetupData()
		: BodySetup(nullptr)
	{
	}

	FRuntimeMeshAsyncBodySetupData(UBodySetup* InBodySetup, TArray<FRuntimeMeshCollisionSourceSectionInfo>&& InCollisionSources)
		: BodySetup(InBodySetup), CollisionSources(MoveTemp(InCollisionSources))
	{
	}
};


USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshRenderableCollisionData
{
	GENERATED_USTRUCT_BODY()

public:
	FRuntimeMeshRenderableCollisionData() { }
	FRuntimeMeshRenderableCollisionData(const struct FRuntimeMeshRenderableMeshData& InRenderable);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	FRuntimeMeshCollisionVertexStream Vertices;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	FRuntimeMeshCollisionTriangleStream Triangles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Collision|CollisionMesh")
	FRuntimeMeshCollisionTexCoordStream TexCoords;
};