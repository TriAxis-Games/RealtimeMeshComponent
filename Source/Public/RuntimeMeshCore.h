// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine.h"
#include "Components/MeshComponent.h"
#include "RuntimeMeshProfiling.h"
#include "RuntimeMeshVersion.h"
#include "RuntimeMeshCore.generated.h"

class FRuntimeMeshVertexFactory;


template<typename Type, typename = void>
struct FVertexHasPositionComponent 
{ 
	enum
	{
		Value = false
	};
};

template<typename Type>
struct FVertexHasPositionComponent<Type, decltype(Type().Position, void())>
{
	enum
	{
		Value = true
	};
};



#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 12
/** Structure definition of a vertex */
using RuntimeMeshVertexStructure = FLocalVertexFactory::FDataType;
#else
/** Structure definition of a vertex */
using RuntimeMeshVertexStructure = FLocalVertexFactory::DataType;
#endif

#define RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, VertexType, Member, MemberType) \
	STRUCTMEMBER_VERTEXSTREAMCOMPONENT(&VertexBuffer, VertexType, Member, MemberType)

/* Update frequency for a section. Used to optimize for update or render speed*/
UENUM(BlueprintType)
enum class EUpdateFrequency
{
	/* Tries to skip recreating the scene proxy if possible. */
	Average UMETA(DisplayName = "Average"),
	/* Tries to skip recreating the scene proxy if possible and optimizes the buffers for frequent updates. */
	Frequent UMETA(DisplayName = "Frequent"),
	/* If the component is static it will try to use the static rendering path (this will force a recreate of the scene proxy) */
	Infrequent UMETA(DisplayName = "Infrequent")
};

/* Update frequency for a section. Used to optimize for update or render speed*/
enum class ESectionUpdateFlags
{
	None = 0x0,

	/** 
		This will use move-assignment when copying the supplied vertices/triangles into the section.
		This is faster as it doesn't require copying the data.

		CAUTION: This means that your copy of the arrays will be cleared!
	*/
	MoveArrays = 0x1,


	//CalculateNormalTangent = 0x2,
	
};
ENUM_CLASS_FLAGS(ESectionUpdateFlags)

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

	FRuntimeMeshTangent(float X, float Y, float Z)
		: TangentX(X, Y, Z)
		, bFlipTangentY(false)
	{}

	FRuntimeMeshTangent(FVector InTangentX, bool bInFlipTangentY)
		: TangentX(InTangentX)
		, bFlipTangentY(bInFlipTangentY)
	{}

	void AdjustNormal(FPackedNormal& Normal) const
	{
		Normal.Vector.W = bFlipTangentY ? 0 : 255;
	}
};





USTRUCT()
struct FRuntimeMeshCollisionSection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> VertexBuffer;

	UPROPERTY()
	TArray<int32> IndexBuffer;

	void Reset()
	{
		VertexBuffer.Empty();
		IndexBuffer.Empty();
	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionSection& Section)
	{
		Ar << Section.VertexBuffer;
		Ar << Section.IndexBuffer;
		return Ar;
	}
}; 

USTRUCT()
struct FRuntimeConvexCollisionSection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> VertexBuffer;

	UPROPERTY()
	FBox BoundingBox;

	void Reset()
	{
		VertexBuffer.Empty();
		BoundingBox.Init();
	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeConvexCollisionSection& Section)
	{
		Ar << Section.VertexBuffer;
		Ar << Section.BoundingBox;
		return Ar;
	}
};
