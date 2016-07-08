// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine.h"
#include "Components/MeshComponent.h"
#include "RuntimeMeshProfiling.h"
#include "RuntimeMeshVersion.h"
#include "Runtime/Launch/Resources/Version.h"
#include "RuntimeMeshCore.generated.h"

class FRuntimeMeshVertexFactory;


/* Helper for determining if a struct has a member of type "FVector" with the name "Position" */
template<typename T> struct FVertexHasPositionComponent {
	struct Fallback { FVector Position; };
	struct Derived : T, Fallback { };

	template<typename C, C> struct ChT;

	template<typename C> static char(&f(ChT<FVector Fallback::*, &C::Position>*))[1];
	template<typename C> static char(&f(...))[2];

	static bool const Value = sizeof(f<Derived>(0)) == 2;
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

	void AdjustNormal(FPackedRGBA16N& Normal) const
	{
		Normal.W = bFlipTangentY ? 0 : 65535;
	}
};

/* The different buffers within the Runtime Mesh Component */
enum class ERuntimeMeshBuffer
{
	None = 0x0,
	Vertices = 0x1,
	Triangles = 0x2,
	Positions = 0x4
};
ENUM_CLASS_FLAGS(ERuntimeMeshBuffer)


#if WITH_IMPROVED_PHYSX_COOKER_CONTROL

/* Hint for the collision cooker */
UENUM(BlueprintType)
enum class ERuntimeMeshComponentCookingHint : uint8
{
	Speed UMETA(DisplayName = "Speed"),
	Average UMETA(DisplayName = "Average"),
	Quality UMETA(DisplayName = "Quality"),
};

#endif

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






struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexTypeInfo
{
	FString TypeName;
	FGuid TypeGuid;

	FRuntimeMeshVertexTypeInfo(FString Name, FGuid Guid) : TypeName(Name), TypeGuid(Guid) { }

	bool Equals(const FRuntimeMeshVertexTypeInfo* Other) const
	{
		if (TypeGuid != Other->TypeGuid)
		{
			return false;
		}

		return EqualsAdvanced(Other);
	}

	template<typename Type>
	void EnsureEquals() const
	{
		if (!Equals(&Type::TypeInfo))
		{
			ThrowMismatchException(Type::TypeInfo.TypeName);
		}
	}

protected:

	void ThrowMismatchException(const FString& OtherName) const
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Vertex Type Mismatch: %s  and  %s"), *TypeName, *OtherName);
	}

	/*
	*	This is called only if the Guids match. This is meant to due further
	*	checking like in the case of the generic vertex, how it's configured.
	*/
	virtual bool EqualsAdvanced(const FRuntimeMeshVertexTypeInfo* Other) const { return true; }
};



#define DECLARE_RUNTIMEMESH_VERTEXTYPEINFO_SIMPLE(TypeName, Guid) \
	struct FRuntimeMeshVertexTypeInfo_##TypeName : public FRuntimeMeshVertexTypeInfo \
	{ \
		FRuntimeMeshVertexTypeInfo_##TypeName() : FRuntimeMeshVertexTypeInfo(TEXT(#TypeName), Guid) { } \
	}; \
	static const FRuntimeMeshVertexTypeInfo_##TypeName TypeInfo;

#define DEFINE_RUNTIMEMESH_VERTEXTYPEINFO(TypeName) \
	const  TypeName::FRuntimeMeshVertexTypeInfo_##TypeName TypeName::TypeInfo = TypeName::FRuntimeMeshVertexTypeInfo_##TypeName();


