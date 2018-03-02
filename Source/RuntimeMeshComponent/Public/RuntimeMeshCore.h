// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine.h"
#include "Components/MeshComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Stats/Stats.h"
#include "RuntimeMeshCore.generated.h"

DECLARE_STATS_GROUP(TEXT("RuntimeMesh"), STATGROUP_RuntimeMesh, STATCAT_Advanced);


#define RUNTIMEMESH_MAXTEXCOORDS MAX_TEXCOORDS



// Custom version for runtime mesh serialization
namespace FRuntimeMeshVersion
{
	enum Type
	{
		Initial = 0,
		TemplatedVertexFix = 1,
		SerializationOptional = 2,
		DualVertexBuffer = 3,
		SerializationV2 = 4,

		// This was a total overhaul of the component, and with it the serialization
		RuntimeMeshComponentV3 = 5,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version
	const static FGuid GUID = FGuid(0xEE48714B, 0x8A2C4652, 0x98BE40E6, 0xCB7EF0E6);
};


class FRuntimeMeshVertexFactory;
struct FRuntimeMeshVertexStreamStructure;

 template<typename T>
 struct FRuntimeMeshIndexTraits
 {
 	enum { IsValidIndexType = false };
 	enum { Is32Bit = false };
 };
 
 template<>
 struct FRuntimeMeshIndexTraits<uint16>
 {
 	enum { IsValidIndexType = true };
 	enum { Is32Bit = false };
 };
 
 template<>
 struct FRuntimeMeshIndexTraits<uint32>
 {
 	enum { IsValidIndexType = true };
 	enum { Is32Bit = true };
 };
 
 template<>
 struct FRuntimeMeshIndexTraits<int32>
 {
 	enum { IsValidIndexType = true };
 	enum { Is32Bit = true };
 };


enum class ERuntimeMeshBuffersToUpdate : uint8
{
	None = 0x0,
	VertexBuffer0 = 0x1,
	VertexBuffer1 = 0x2,
	VertexBuffer2 = 0x4,

	AllVertexBuffers = VertexBuffer0 | VertexBuffer1 | VertexBuffer2,

	IndexBuffer = 0x8,
	AdjacencyIndexBuffer = 0x10
};
ENUM_CLASS_FLAGS(ERuntimeMeshBuffersToUpdate);

/* Mobility status for a RuntimeMeshComponent */
UENUM(BlueprintType)
enum class ERuntimeMeshMobility : uint8
{
	Movable,
	Stationary,
	Static
};

/* Update frequency for a section. Used to optimize for update or render speed*/
UENUM(BlueprintType)
enum class EUpdateFrequency : uint8
{
	/* Tries to skip recreating the scene proxy if possible. */
	Average UMETA(DisplayName = "Average"),
	/* Tries to skip recreating the scene proxy if possible and optimizes the buffers for frequent updates. */
	Frequent UMETA(DisplayName = "Frequent"),
	/* If the component is static it will try to use the static rendering path (this will force a recreate of the scene proxy) */
	Infrequent UMETA(DisplayName = "Infrequent")
};

/* Control flags for update actions */
enum class ESectionUpdateFlags
{
	None = 0x0,

	// 	/** 
	// 	*	This will use move-assignment when copying the supplied vertices/triangles into the section.
	// 	*	This is faster as it doesn't require copying the data.
	// 	*
	// 	*	CAUTION: This means that your copy of the arrays will be cleared!
	// 	*/
	// 	MoveArrays = 0x1,

	/**
	*	Should the normals and tangents be calculated automatically?
	*	To do this manually see RuntimeMeshLibrary::CalculateTangentsForMesh()
	*	This version calculates smooth tangents, so it will smooth across vertices sharing position
	*/
	CalculateNormalTangent = 0x2,

	/**
	*	Should the normals and tangents be calculated automatically?
	*	To do this manually see RuntimeMeshLibrary::CalculateTangentsForMesh()
	*	This version calculates hard tangents, so it will not smooth across vertices sharing position
	*/
	CalculateNormalTangentHard = 0x4,

	/**
	*	Should the tessellation indices be calculated to support tessellation?
	*	To do this manually see RuntimeMeshLibrary::GenerateTessellationIndexBuffer()
	*/
	CalculateTessellationIndices = 0x8,

};
ENUM_CLASS_FLAGS(ESectionUpdateFlags)

/*
*	Configuration flag for the collision cooking to prioritize cooking speed or collision performance.
*/
UENUM(BlueprintType)
enum class ERuntimeMeshCollisionCookingMode : uint8
{
	/*
	*	Favors runtime collision performance of cooking speed.
	*	This means that cooking a new mesh will be slower, but collision will be faster.
	*/
	CollisionPerformance UMETA(DisplayName = "Collision Performance"),

	/*
	*	Favors cooking speed over collision performance.
	*	This means that cooking a new mesh will be faster, but collision will be slower.
	*/
	CookingPerformance UMETA(DisplayName = "Cooking Performance"),
};



/**
*	Struct used to specify a tangent vector for a vertex
*	The Y tangent is computed from the cross product of the vertex normal (Tangent Z) and the TangentX member.
*/
USTRUCT(BlueprintType)
struct FRuntimeMeshTangent
{
	GENERATED_USTRUCT_BODY()

	/** Direction of X tangent for this vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	FVector TangentX;

	/** Bool that indicates whether we should flip the Y tangent when we compute it using cross product */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	bool bFlipTangentY;

	FRuntimeMeshTangent()
		: TangentX(1.f, 0.f, 0.f)
		, bFlipTangentY(false)
	{}

	FRuntimeMeshTangent(float X, float Y, float Z, bool bInFlipTangentY = false)
		: TangentX(X, Y, Z)
		, bFlipTangentY(bInFlipTangentY)
	{}

	FRuntimeMeshTangent(FVector InTangentX, bool bInFlipTangentY = false)
		: TangentX(InTangentX)
		, bFlipTangentY(bInFlipTangentY)
	{}

	void ModifyNormal(FVector4& Normal) const { Normal.W = bFlipTangentY ? -1 : 1; }
	void ModifyNormal(FPackedNormal& Normal) const { Normal.Vector.W = bFlipTangentY ? 0 : MAX_uint8; }
	void ModifyNormal(FPackedRGBA16N& Normal) const { Normal.W = bFlipTangentY ? 0 : MAX_uint16; }
};



struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexStreamStructureElement
{
	uint8 Offset;
	uint8 Stride;
	EVertexElementType Type;

	FRuntimeMeshVertexStreamStructureElement() : Offset(-1), Stride(-1), Type(EVertexElementType::VET_None) { }
	FRuntimeMeshVertexStreamStructureElement(uint8 InOffset, uint8 InStride, EVertexElementType InType)
		: Offset(InOffset), Stride(InStride), Type(InType) { }

	bool IsValid() const { return Offset >= 0 && Stride >= 0 && Type != EVertexElementType::VET_None; }

	bool operator==(const FRuntimeMeshVertexStreamStructureElement& Other) const;

	bool operator!=(const FRuntimeMeshVertexStreamStructureElement& Other) const;

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexStreamStructureElement& Element)
	{
		Ar << Element.Offset;
		Ar << Element.Stride;

		int32 TypeValue = static_cast<int32>(Element.Type);
		Ar << TypeValue;
		Element.Type = static_cast<EVertexElementType>(TypeValue);

		return Ar;
	}
};

#define RUNTIMEMESH_VERTEXSTREAMCOMPONENT(VertexType, Member, MemberType) \
	FRuntimeMeshVertexStreamStructureElement(STRUCT_OFFSET(VertexType,Member),sizeof(VertexType),MemberType)

struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexStreamStructure
{
	FRuntimeMeshVertexStreamStructureElement Position;
	FRuntimeMeshVertexStreamStructureElement Normal;
	FRuntimeMeshVertexStreamStructureElement Tangent;
	FRuntimeMeshVertexStreamStructureElement Color;
	TArray<FRuntimeMeshVertexStreamStructureElement, TInlineAllocator<RUNTIMEMESH_MAXTEXCOORDS>> UVs;

	bool operator==(const FRuntimeMeshVertexStreamStructure& Other) const;

	bool operator!=(const FRuntimeMeshVertexStreamStructure& Other) const;

	bool HasAnyElements() const;

	bool HasUVs() const;

	uint8 CalculateStride() const;

	bool IsValid() const;

	bool HasNoOverlap(const FRuntimeMeshVertexStreamStructure& Other) const;

	static bool ValidTripleStream(const FRuntimeMeshVertexStreamStructure& Stream1, const FRuntimeMeshVertexStreamStructure& Stream2, const FRuntimeMeshVertexStreamStructure& Stream3);

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexStreamStructure& Structure)
	{
		Ar << Structure.Position;
		Ar << Structure.Normal;
		Ar << Structure.Tangent;
		Ar << Structure.Color;
		Ar << Structure.UVs;

		return Ar;
	}
};


struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshNullVertex
{
	static const FRuntimeMeshVertexStreamStructure GetVertexStructure()
	{
		return FRuntimeMeshVertexStreamStructure();
	}
};

template<typename VertexType>
inline FRuntimeMeshVertexStreamStructure GetStreamStructure()
{
	return VertexType::GetVertexStructure();
}

template<>
inline FRuntimeMeshVertexStreamStructure GetStreamStructure<FVector>()
{
	FRuntimeMeshVertexStreamStructure Structure;
	Structure.Position = FRuntimeMeshVertexStreamStructureElement(0, sizeof(FVector), VET_Float3);
	return Structure;
}

template<>
inline FRuntimeMeshVertexStreamStructure GetStreamStructure<FColor>()
{
	FRuntimeMeshVertexStreamStructure Structure;
	Structure.Color = FRuntimeMeshVertexStreamStructureElement(0, sizeof(FColor), VET_Color);
	return Structure;
}





struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshLockProvider
{
	virtual ~FRuntimeMeshLockProvider() { }
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
};



struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshNullLockProvider : public FRuntimeMeshLockProvider
{
	FRuntimeMeshNullLockProvider() { check(IsInGameThread()); }
	virtual ~FRuntimeMeshNullLockProvider() { }
	virtual void Lock() override { check(IsInGameThread()); }
	virtual void Unlock() override { check(IsInGameThread()); }

	static FRuntimeMeshLockProvider* CreateLock() { return new FRuntimeMeshNullLockProvider(); }
};


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshScopeLock
{

private:
	// Holds the synchronization object to aggregate and scope manage.
	FRuntimeMeshLockProvider* SynchObject;

public:

	/**
	* Constructor that performs a lock on the synchronization object
	*
	* @param InSynchObject The synchronization object to manage
	*/
	FRuntimeMeshScopeLock(const FRuntimeMeshLockProvider* InSyncObject)
		: SynchObject(const_cast<FRuntimeMeshLockProvider*>(InSyncObject))
	{
		check(SynchObject);
		SynchObject->Lock();
	}

	FRuntimeMeshScopeLock(const TUniquePtr<FRuntimeMeshLockProvider>& InSyncObject)
		: SynchObject(InSyncObject.Get())
	{
		check(SynchObject);
		SynchObject->Lock();
	}

	/** Destructor that performs a release on the synchronization object. */
	~FRuntimeMeshScopeLock()
	{
		check(SynchObject);
		SynchObject->Unlock();
	}
private:

	/** Default constructor (hidden on purpose). */
	FRuntimeMeshScopeLock();

	/** Copy constructor( hidden on purpose). */
	FRuntimeMeshScopeLock(const FRuntimeMeshScopeLock& InScopeLock);

	/** Assignment operator (hidden on purpose). */
	FRuntimeMeshScopeLock& operator=(FRuntimeMeshScopeLock& InScopeLock)
	{
		return *this;
	}
};