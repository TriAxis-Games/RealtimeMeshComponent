// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshCore.h"
#include "RuntimeMeshSection.h"

//////////////////////////////////////////////////////////////////////////
//	
//	This file contains a generic vertex structure capable of efficiently representing a vertex 
//	with any combination of position, normal, tangent, color, and 0-8 uv channels.
//
//	To get around an issue with MSVC and potentially other compilers not performing
//	empty base class optimizations (EBO) to children in multiple inheritance, 
//	this vertex is built via a tree of inheritance using partial specializations.
//
//	At each tier of this tree partial specialization will choose which components 
//	we need to add in, thereby removing entire inherited classes when we don't need them.
//
//  Structure:
//											  FRuntimeMeshVertex
//											/					\
//	FRuntimeMeshPositionNormalTangentComponentCombiner			FRuntimeMeshColorUVComponentCombiner
//					/					\								/					\
//	FRuntimeMeshPositionComponent		 \					FRuntimeMeshColorComponent		 \
//										  \													  \
//						FRuntimeMeshNormalTangentComponents							FRuntimeMeshUVComponents
//
//
//
//
//
//	Example use: (This defines a vertex with all components and 1 UV with default precision for normal/tangent and UV)
//
//	using MyVertex = FRuntimeMeshVertex<true, true, true, true, 1, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::Default>;
//
//	MyVertex Vertex;
//	Vertex.Position = FVector(0,0,0);
//	Vertex.Normal = FVector(0,0,0);
//	Vertex.UV0 = FVector2D(0,0);
//
//
//////////////////////////////////////////////////////////////////////////


template<int32 TextureChannels, bool HalfPrecisionUVs, bool HasPositionComponent>
RuntimeMeshVertexStructure CreateVertexStructure(const FVertexBuffer& VertexBuffer);


//////////////////////////////////////////////////////////////////////////
// Texture Component Type Selector
//////////////////////////////////////////////////////////////////////////

enum class ERuntimeMeshVertexUVType
{
	Default = 1,
	HighPrecision = 2,
};

template<ERuntimeMeshVertexUVType UVType>
struct FRuntimeMeshVertexUVsTypeSelector;

template<>
struct FRuntimeMeshVertexUVsTypeSelector<ERuntimeMeshVertexUVType::Default>
{
	typedef FVector2DHalf UVsType;
	static const EVertexElementType VertexElementType1Channel = VET_Half2;
	static const EVertexElementType VertexElementType2Channel = VET_Half4;

};

template<>
struct FRuntimeMeshVertexUVsTypeSelector<ERuntimeMeshVertexUVType::HighPrecision>
{
	typedef FVector2D UVsType;
	static const EVertexElementType VertexElementType1Channel = VET_Float2;
	static const EVertexElementType VertexElementType2Channel = VET_Float4;
};

//////////////////////////////////////////////////////////////////////////
// Texture Component
//////////////////////////////////////////////////////////////////////////

/* Defines the UV coordinates for a vertex (Defaulted to 0 channels) */
template<int32 TextureChannels, typename UVType> struct FRuntimeMeshUVComponents
{
	static_assert(TextureChannels >= 0 && TextureChannels <= 8, "You must have between 0 and 8 (inclusive) UV channels");
};

/* Defines the UV coordinates for a vertex (Specialized to 0 channels) */
template<typename UVType> struct FRuntimeMeshUVComponents<0, UVType>
{
	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) { }
};

/* Defines the UV coordinates for a vertex (Specialized to 1 channels) */
template<typename UVType> struct FRuntimeMeshUVComponents<1, UVType>
{
	UVType UV0;

	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) : UV0(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0) : UV0(InUV0) { }
};

/* Defines the UV coordinates for a vertex (Specialized to 2 channels) */
template<typename UVType> struct FRuntimeMeshUVComponents<2, UVType>
{
	UVType UV0;
	UVType UV1;

	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) : UV0(0, 0), UV1(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0) : UV0(InUV0), UV1(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1) : UV0(InUV0), UV1(InUV1) { }
};

/* Defines the UV coordinates for a vertex (Specialized to 3 channels) */
template<typename UVType> struct FRuntimeMeshUVComponents<3, UVType>
{
	UVType UV0;
	UVType UV1;
	UVType UV2;

	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) :
		UV0(0, 0), UV1(0, 0), UV2(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0) :
		UV0(InUV0), UV1(0, 0), UV2(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1) :
		UV0(InUV0), UV1(InUV1), UV2(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2) { }
};

/* Defines the UV coordinates for a vertex (Specialized to 4 channels) */
template<typename UVType> struct FRuntimeMeshUVComponents<4, UVType>
{
	UVType UV0;
	UVType UV1;
	UVType UV2;
	UVType UV3;

	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) :
		UV0(0, 0), UV1(0, 0), UV2(0, 0), UV3(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0) :
		UV0(InUV0), UV1(0, 0), UV2(0, 0), UV3(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1) :
		UV0(InUV0), UV1(InUV1), UV2(0, 0), UV3(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3) { }
};

/* Defines the UV coordinates for a vertex (Specialized to 5 channels) */
template<typename UVType> struct FRuntimeMeshUVComponents<5, UVType>
{
	UVType UV0;
	UVType UV1;
	UVType UV2;
	UVType UV3;
	UVType UV4;

	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) :
		UV0(0, 0), UV1(0, 0), UV2(0, 0), UV3(0, 0), UV4(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0) :
		UV0(InUV0), UV1(0, 0), UV2(0, 0), UV3(0, 0), UV4(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1) :
		UV0(InUV0), UV1(InUV1), UV2(0, 0), UV3(0, 0), UV4(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(0, 0), UV4(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4) : UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4) { }
};

/* Defines the UV coordinates for a vertex (Specialized to 6 channels, Half Precision) */
template<typename UVType> struct FRuntimeMeshUVComponents<6, UVType>
{
	UVType UV0;
	UVType UV1;
	UVType UV2;
	UVType UV3;
	UVType UV4;
	UVType UV5;

	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) :
		UV0(0, 0), UV1(0, 0), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0) :
		UV0(InUV0), UV1(0, 0), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1) :
		UV0(InUV0), UV1(InUV1), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(0, 0), UV4(0, 0), UV5(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(0, 0), UV5(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4, const FVector2D& InUV5) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(InUV5) { }
};

/* Defines the UV coordinates for a vertex (Specialized to 7 channels) */
template<typename UVType> struct FRuntimeMeshUVComponents<7, UVType>
{
	UVType UV0;
	UVType UV1;
	UVType UV2;
	UVType UV3;
	UVType UV4;
	UVType UV5;
	UVType UV6;

	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) :
		UV0(0, 0), UV1(0, 0), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0), UV6(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0) :
		UV0(InUV0), UV1(0, 0), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0), UV6(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1) :
		UV0(InUV0), UV1(InUV1), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0), UV6(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(0, 0), UV4(0, 0), UV5(0, 0), UV6(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(0, 0), UV5(0, 0), UV6(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(0, 0), UV6(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4, const FVector2D& InUV5) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(InUV5), UV6(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4, const FVector2D& InUV5, const FVector2D& InUV6) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(InUV5), UV6(InUV6) { }
};

/* Defines the UV coordinates for a vertex (Specialized to 8 channels) */
template<typename UVType> struct FRuntimeMeshUVComponents<8, UVType>
{
	UVType UV0;
	UVType UV1;
	UVType UV2;
	UVType UV3;
	UVType UV4;
	UVType UV5;
	UVType UV6;
	UVType UV7;

	FRuntimeMeshUVComponents() { }
	FRuntimeMeshUVComponents(EForceInit) :
		UV0(0, 0), UV1(0, 0), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0), UV6(0, 0), UV7(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0) :
		UV0(InUV0), UV1(0, 0), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0), UV6(0, 0), UV7(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1) :
		UV0(InUV0), UV1(InUV1), UV2(0, 0), UV3(0, 0), UV4(0, 0), UV5(0, 0), UV6(0, 0), UV7(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(0, 0), UV4(0, 0), UV5(0, 0), UV6(0, 0), UV7(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(0, 0), UV5(0, 0), UV6(0, 0), UV7(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(0, 0), UV6(0, 0), UV7(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4, const FVector2D& InUV5) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(InUV5), UV6(0, 0), UV7(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4, const FVector2D& InUV5, const FVector2D& InUV6) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(InUV5), UV6(InUV6), UV7(0, 0) { }
	FRuntimeMeshUVComponents(const FVector2D& InUV0, const FVector2D& InUV1, const FVector2D& InUV2, const FVector2D& InUV3,
		const FVector2D& InUV4, const FVector2D& InUV5, const FVector2D& InUV6, const FVector2D& InUV7) :
		UV0(InUV0), UV1(InUV1), UV2(InUV2), UV3(InUV3), UV4(InUV4), UV5(InUV5), UV6(InUV6), UV7(InUV7) { }
};


//////////////////////////////////////////////////////////////////////////
// Tangent Basis Component Type Selector
//////////////////////////////////////////////////////////////////////////

enum class ERuntimeMeshVertexTangentBasisType
{
	Default = 1,
	HighPrecision = 2,
};

template<ERuntimeMeshVertexTangentBasisType TangentBasisType>
struct FRuntimeMeshVertexTangentTypeSelector;

template<>
struct FRuntimeMeshVertexTangentTypeSelector<ERuntimeMeshVertexTangentBasisType::Default>
{
	typedef FPackedNormal TangentType;
	static const EVertexElementType VertexElementType = VET_PackedNormal;
};

template<>
struct FRuntimeMeshVertexTangentTypeSelector<ERuntimeMeshVertexTangentBasisType::HighPrecision>
{
	typedef FPackedRGBA16N TangentType;
	static const EVertexElementType VertexElementType = VET_UShort4N;
};


//////////////////////////////////////////////////////////////////////////
// Tangent Basis Components
//////////////////////////////////////////////////////////////////////////

template<bool WantsNormal, bool WantsTangent, typename TangentType>
struct FRuntimeMeshNormalTangentComponents;

template<typename TangentType>
struct FRuntimeMeshNormalTangentComponents<true, false, TangentType>
{
	TangentType Normal;

	FRuntimeMeshNormalTangentComponents() { }
	FRuntimeMeshNormalTangentComponents(EForceInit) : Normal(FVector4(0.0f, 0.0f, 1.0f, 1.0f)) { }

	void SetNormalAndTangent(const FVector& InNormal, const FRuntimeMeshTangent& InTangent)
	{
		Normal = InNormal;

		InTangent.AdjustNormal(Normal);
	}

	void SetNormalAndTangent(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		Normal = InTangentZ;

		// store determinant of basis in w component of normal vector
		Normal.Vector.W = GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ) < 0.0f ? 0 : 255;
	}
};

template<typename TangentType>
struct FRuntimeMeshNormalTangentComponents<false, true, TangentType>
{
	TangentType Tangent;

	FRuntimeMeshNormalTangentComponents() { }
	FRuntimeMeshNormalTangentComponents(EForceInit) : Tangent(FVector(1.0f, 0.0f, 0.0f)) { }

	void SetNormalAndTangent(const FVector& InNormal, const FRuntimeMeshTangent& InTangent)
	{
		Tangent = InTangent.TangentX;
	}

	void SetNormalAndTangent(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		Tangent = InTangentX;
	}
};

template<typename TangentType>
struct FRuntimeMeshNormalTangentComponents<true, true, TangentType>
{
	TangentType Normal;
	TangentType Tangent;

	FRuntimeMeshNormalTangentComponents() { }
	FRuntimeMeshNormalTangentComponents(EForceInit) : Normal(FVector4(0.0f, 0.0f, 1.0f, 1.0f)), Tangent(FVector(1.0f, 0.0f, 0.0f)) { }

	void SetNormalAndTangent(const FVector& InNormal, const FRuntimeMeshTangent& InTangent)
	{
		Normal = InNormal;
		Tangent = InTangent.TangentX;

		InTangent.AdjustNormal(Normal);
	}
	
	void SetNormalAndTangent(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		Normal = InTangentZ;
		Tangent = InTangentX;

		// store determinant of basis in w component of normal vector
		Normal.Vector.W = GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ) < 0.0f ? 0 : 255;
	}
};



//////////////////////////////////////////////////////////////////////////
// Position Component
//////////////////////////////////////////////////////////////////////////

template<bool WantsPosition>
struct FRuntimeMeshPositionComponent;

template<>
struct FRuntimeMeshPositionComponent<true>
{
	FVector Position;

	FRuntimeMeshPositionComponent() { }
	FRuntimeMeshPositionComponent(EForceInit) : Position(0.0f, 0.0f, 0.0f) { }
};



//////////////////////////////////////////////////////////////////////////
// Color Component
//////////////////////////////////////////////////////////////////////////

template<bool WantsColor>
struct FRuntimeMeshColorComponent;

template<>
struct FRuntimeMeshColorComponent<true>
{
	FColor Color;

	FRuntimeMeshColorComponent() { }
	FRuntimeMeshColorComponent(EForceInit) : Color(FColor::White) { }
};




//////////////////////////////////////////////////////////////////////////
// Position Normal Tangent Combiner
//////////////////////////////////////////////////////////////////////////

template<bool WantsPosition, bool WantsNormal, bool WantsTangent, typename TangentBasisType>
struct FRuntimeMeshPositionNormalTangentComponentCombiner :
	public FRuntimeMeshPositionComponent<WantsPosition>,
	public FRuntimeMeshNormalTangentComponents<WantsNormal, WantsTangent, TangentBasisType>
{
	FRuntimeMeshPositionNormalTangentComponentCombiner() { }
	FRuntimeMeshPositionNormalTangentComponentCombiner(EForceInit)
		: FRuntimeMeshPositionComponent<WantsPosition>(EForceInit::ForceInit)
		, FRuntimeMeshNormalTangentComponents<WantsNormal, WantsTangent, TangentBasisType>(EForceInit::ForceInit)
	{ }
};

template<bool WantsPosition, typename TangentBasisType>
struct FRuntimeMeshPositionNormalTangentComponentCombiner<WantsPosition, false, false, TangentBasisType> :
	public FRuntimeMeshPositionComponent<WantsPosition>
{
	FRuntimeMeshPositionNormalTangentComponentCombiner() { }
	FRuntimeMeshPositionNormalTangentComponentCombiner(EForceInit)
		: FRuntimeMeshPositionComponent<WantsPosition>(EForceInit::ForceInit)
	{ }
};

template<bool WantsNormal, bool WantsTangent, typename TangentBasisType>
struct FRuntimeMeshPositionNormalTangentComponentCombiner<false, WantsNormal, WantsTangent, TangentBasisType> :
	public FRuntimeMeshNormalTangentComponents<WantsNormal, WantsTangent, TangentBasisType>
{
	FRuntimeMeshPositionNormalTangentComponentCombiner() { }
	FRuntimeMeshPositionNormalTangentComponentCombiner(EForceInit)
		: FRuntimeMeshNormalTangentComponents<WantsNormal, WantsTangent, TangentBasisType>(EForceInit::ForceInit)
	{ }
};

template<typename TangentBasisType>
struct FRuntimeMeshPositionNormalTangentComponentCombiner<false, false, false, TangentBasisType>;



//////////////////////////////////////////////////////////////////////////
// Color UV Combiner
//////////////////////////////////////////////////////////////////////////

template<bool WantsColor, int32 NumWantedUVChannels, typename UVType>
struct FRuntimeMeshColorUVComponentCombiner :
	public FRuntimeMeshColorComponent<WantsColor>,
	public FRuntimeMeshUVComponents<NumWantedUVChannels, UVType>
{
	FRuntimeMeshColorUVComponentCombiner() { }
	FRuntimeMeshColorUVComponentCombiner(EForceInit)
		: FRuntimeMeshColorComponent<WantsColor>(EForceInit::ForceInit)
		, FRuntimeMeshUVComponents<NumWantedUVChannels, UVType>(EForceInit::ForceInit)
	{ }
};

template<int32 NumWantedUVChannels, typename UVType>
struct FRuntimeMeshColorUVComponentCombiner<false, NumWantedUVChannels, UVType> :
	public FRuntimeMeshUVComponents<NumWantedUVChannels, UVType>
{
	FRuntimeMeshColorUVComponentCombiner() { }
	FRuntimeMeshColorUVComponentCombiner(EForceInit)
		: FRuntimeMeshUVComponents<NumWantedUVChannels, UVType>(EForceInit::ForceInit)
	{ }
};

template<bool WantsColor, typename UVType>
struct FRuntimeMeshColorUVComponentCombiner<WantsColor, 0, UVType> :
	public FRuntimeMeshColorComponent<WantsColor>
{
	FRuntimeMeshColorUVComponentCombiner() { }
	FRuntimeMeshColorUVComponentCombiner(EForceInit)
		: FRuntimeMeshColorComponent<WantsColor>(EForceInit::ForceInit)
	{ }
};

template<typename UVType>
struct FRuntimeMeshColorUVComponentCombiner<false, 0, UVType>;







//////////////////////////////////////////////////////////////////////////
// Template Vertex Type Info Structure
//////////////////////////////////////////////////////////////////////////

template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels,
	ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
struct FRuntimeMeshVertexTypeInfo_GenericVertex : public FRuntimeMeshVertexTypeInfo
{
	FRuntimeMeshVertexTypeInfo_GenericVertex() :
		FRuntimeMeshVertexTypeInfo(
			FString::Printf(TEXT("RuntimeMeshVertex<%d, %d, %d, %d, %d, %d, %d>"), WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, (int32)NormalTangentType, (int32)UVType),
			FGuid(0x00FFEB44, 0x31094597, 0x93918032, 0x015678C3)) { }

	static uint32 ComputeRuntimeMeshVertexTemplateTypeID(bool bWantsPosition, bool bWantsNormal, bool bWantsTangent, bool bWantsColor, int32 NumUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentBasisType, ERuntimeMeshVertexUVType UVChannelsType)
	{
		uint32 TypeID = 0;
		TypeID = (TypeID << 1) | (bWantsPosition ? 1 : 0);
		TypeID = (TypeID << 1) | (bWantsNormal ? 1 : 0);
		TypeID = (TypeID << 1) | (bWantsTangent ? 1 : 0);
		TypeID = (TypeID << 2) | (NormalTangentBasisType == ERuntimeMeshVertexTangentBasisType::HighPrecision ? 1 : 0);
		TypeID = (TypeID << 1) | (bWantsColor ? 1 : 0);
		TypeID = (TypeID << 8) | (NumUVChannels & 0xFF);
		TypeID = (TypeID << 2) | (UVChannelsType == ERuntimeMeshVertexUVType::HighPrecision ? 1 : 0);
		return TypeID;
	}

	const uint32 TemplateTypeID = ComputeRuntimeMeshVertexTemplateTypeID(WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType);

	virtual bool EqualsAdvanced(const FRuntimeMeshVertexTypeInfo* Other) const
	{
		const FRuntimeMeshVertexTypeInfo_GenericVertex* OtherGenericVertex = static_cast<const FRuntimeMeshVertexTypeInfo_GenericVertex*>(Other);

		return TemplateTypeID == OtherGenericVertex->TemplateTypeID;
	}
};





//////////////////////////////////////////////////////////////////////////
// Template Vertex
//////////////////////////////////////////////////////////////////////////

// This version uses both sub combiners since there's at least 1 thing we need from both.
template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels,
ERuntimeMeshVertexTangentBasisType NormalTangentType = ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType UVType = ERuntimeMeshVertexUVType::Default>
struct FRuntimeMeshVertex :
	public FRuntimeMeshPositionNormalTangentComponentCombiner<WantsPosition, WantsNormal, WantsTangent, typename FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::TangentType>,
	public FRuntimeMeshColorUVComponentCombiner<WantsColor, NumWantedUVChannels, typename FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType>
{
	// Make sure something is enabled
	static_assert((WantsPosition || WantsNormal || WantsTangent || WantsColor || NumWantedUVChannels > 0), "Invalid configuration... You must have at least 1 component enabled.");

	// Type Info
	static const FRuntimeMeshVertexTypeInfo_GenericVertex<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType> TypeInfo;
	
	// Typedef self
	using SelfType = FRuntimeMeshVertex<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType>;

	// Get vertex structure
	static RuntimeMeshVertexStructure GetVertexStructure(const FVertexBuffer& VertexBuffer)
	{
	 	return FRuntimemeshVertexStructureHelper::CreateVertexStructure<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType>(VertexBuffer);
	}

	FRuntimeMeshVertex() { }
	FRuntimeMeshVertex(EForceInit)
		: FRuntimeMeshPositionNormalTangentComponentCombiner<WantsPosition, WantsNormal, WantsTangent, typename FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::TangentType>(EForceInit::ForceInit)
		, FRuntimeMeshColorUVComponentCombiner<WantsColor, NumWantedUVChannels, typename FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType>(EForceInit::ForceInit)
	{ }
};

// This version only uses the position/normal/tangent combiner as we don't need anything from the other
template<bool WantsPosition, bool WantsNormal, bool WantsTangent, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
struct FRuntimeMeshVertex<WantsPosition, WantsNormal, WantsTangent, false, 0, NormalTangentType, UVType> :
	public FRuntimeMeshPositionNormalTangentComponentCombiner<WantsPosition, WantsNormal, WantsTangent, typename FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::TangentType>
{
	// Type Info
	static const FRuntimeMeshVertexTypeInfo_GenericVertex<WantsPosition, WantsNormal, WantsTangent, false, 0, NormalTangentType, UVType> TypeInfo;

	// Typedef self
	using SelfType = FRuntimeMeshVertex<WantsPosition, WantsNormal, WantsTangent, false, 0, NormalTangentType, UVType>;

	// Get vertex structure
	static RuntimeMeshVertexStructure GetVertexStructure(const FVertexBuffer& VertexBuffer)
	{
		return FRuntimemeshVertexStructureHelper::CreateVertexStructure<WantsPosition, WantsNormal, WantsTangent, false, 0, NormalTangentType, UVType>(VertexBuffer);
	}

	FRuntimeMeshVertex() { }
	FRuntimeMeshVertex(EForceInit)
		: FRuntimeMeshPositionNormalTangentComponentCombiner<WantsPosition, WantsNormal, WantsTangent, typename FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::TangentType>(EForceInit::ForceInit)
	{ }
};

// This version only uses the color/uv combiner as we don't need anything from the other
template<bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
struct FRuntimeMeshVertex<false, false, false, WantsColor, NumWantedUVChannels, NormalTangentType, UVType> :
	public FRuntimeMeshColorUVComponentCombiner<WantsColor, NumWantedUVChannels, typename FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType>
{
	// Type Info
	static const FRuntimeMeshVertexTypeInfo_GenericVertex<false, false, false, WantsColor, NumWantedUVChannels, NormalTangentType, UVType> TypeInfo;

	// Typedef self
	using SelfType = FRuntimeMeshVertex<false, false, false, WantsColor, NumWantedUVChannels, NormalTangentType, UVType>;

	// Get vertex structure
	static RuntimeMeshVertexStructure GetVertexStructure(const FVertexBuffer& VertexBuffer)
	{
		return FRuntimemeshVertexStructureHelper::CreateVertexStructure<false, false, false, WantsColor, NumWantedUVChannels, NormalTangentType, UVType>(VertexBuffer);
	}

	FRuntimeMeshVertex() { }
	FRuntimeMeshVertex(EForceInit)
		: FRuntimeMeshColorUVComponentCombiner<WantsColor, NumWantedUVChannels, typename FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType>(EForceInit::ForceInit)
	{ }
};



// Type Info Definition
template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
const FRuntimeMeshVertexTypeInfo_GenericVertex<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType>
	FRuntimeMeshVertex<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType>::TypeInfo;

// Type Info Definition
template<bool WantsPosition, bool WantsNormal, bool WantsTangent, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
const FRuntimeMeshVertexTypeInfo_GenericVertex<WantsPosition, WantsNormal, WantsTangent, false, 0, NormalTangentType, UVType>
	FRuntimeMeshVertex<WantsPosition, WantsNormal, WantsTangent, false, 0, NormalTangentType, UVType>::TypeInfo;

// Type Info Definition
template<bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
const FRuntimeMeshVertexTypeInfo_GenericVertex<false, false, false, WantsColor, NumWantedUVChannels, NormalTangentType, UVType>
	FRuntimeMeshVertex<false, false, false, WantsColor, NumWantedUVChannels, NormalTangentType, UVType>::TypeInfo;




//////////////////////////////////////////////////////////////////////////
// Macros to create a custom vertex type based on the generic vertex and implement some common constructors
//////////////////////////////////////////////////////////////////////////



#define RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION_true Position = FVector(0.0f, 0.0f, 0.0f);
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION_false 
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION(HasPosition) RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION_##HasPosition

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL_true Normal = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL_false 
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL(HasNormal) RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL_##HasNormal

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT_true Tangent = FVector(1.0f, 0.0f, 0.0f);
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT_false 
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(HasTangent) RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT_##HasTangent

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR_true Color = FColor::White;
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR_false 
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(HasColor) RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR_##HasColor

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_0

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_1 \
	UV0 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_2 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_1 \
	UV1 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_3 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_2 \
	UV2 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_4 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_3 \
	UV3 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_5 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_4 \
	UV4 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_6 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_5 \
	UV5 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_7 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_6 \
	UV6 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_8 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_7 \
	UV7 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNELS(NumChannels) RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_##NumChannels




#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_0
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_1 , const FVector2D& InUV0 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_2 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_1 , const FVector2D& InUV1 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_3 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_2 , const FVector2D& InUV2 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_4 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_3 , const FVector2D& InUV3 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_5 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_4 , const FVector2D& InUV4 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_6 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_5 , const FVector2D& InUV5 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_7 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_6 , const FVector2D& InUV6 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_8 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_7 , const FVector2D& InUV7 = FVector2D::ZeroVector

#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(NumChannels) RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_##NumChannels




#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_0

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_1 \
	UV0 = InUV0;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_2 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_1 \
	UV1 = InUV1;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_3 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_2 \
	UV2 = InUV2;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_4 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_3 \
	UV3 = InUV3;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_5 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_4 \
	UV4 = InUV4;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_6 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_5 \
	UV5 = InUV5;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_7 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_6 \
	UV6 = InUV6;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_8 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_7 \
	UV7 = InUV7;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(NumChannels) RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_##NumChannels



#define RUNTIMEMESH_VERTEX_PARAMETER_POSITION_true const FVector& InPosition, 
#define RUNTIMEMESH_VERTEX_PARAMETER_POSITION_false
#define RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) RUNTIMEMESH_VERTEX_PARAMETER_POSITION_##NeedsPosition

#define RUNTIMEMESH_VERTEX_INIT_POSITION_true Position = InPosition;
#define RUNTIMEMESH_VERTEX_INIT_POSITION_false
#define RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition) RUNTIMEMESH_VERTEX_INIT_POSITION_##NeedsPosition


// PreProcessor IF with pass through for all the constructor arguments
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, Condition, IfTrue) \
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF_##Condition(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, IfTrue)
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF_false(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, IfTrue)
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF_true(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, IfTrue) IfTrue


// Implementation of Position only Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(const FVector& InPosition)							\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL(NeedsNormal)			\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(NeedsTangent)		\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)			\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNELS(UVChannelCount)	\
	}

// Defines the Position Constuctor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)								\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsPosition,				\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)				\
	)

// Implementation of Position/Normal Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InNormal)	\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		Normal = InNormal;											\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(NeedsTangent)		\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)			\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNELS(UVChannelCount)	\
	}

// Defines the Position/Normal Constuctor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)						\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,				\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
	)

// Implementation of Position/Color Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FColor& InColor)	\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL(NeedsNormal)			\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(NeedsTangent)		\
		Color = InColor;											\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNELS(UVChannelCount)	\
	}

// Defines the Position/Color Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)						\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsColor,				\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
	)








// Implementation of Position/Normal/Tangent Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InNormal, const FRuntimeMeshTangent& InTangent RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))	\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		Normal = InNormal;											\
		Tangent = InTangent.TangentX;								\
		InTangent.AdjustNormal(Normal);								\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)			\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)			\
	}

// Defines the Position/Normal/Tangent Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)						\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,						\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,					\
			RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
		)																																															\
	)


// Implementation of Position/TangentX/TangentY/TangentZ Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))	\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		SetNormalAndTangent(InTangentX, InTangentY, InTangentZ);	\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)			\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)			\
	}

// Defines the Position/TangentX/TangentY/TangentZ Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)						\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,									\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,								\
			RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
		)																																																		\
	)







// Implementation of Position/Normal/Tangent Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InNormal, const FRuntimeMeshTangent& InTangent, const FColor& InColor RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))	\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		Normal = InNormal;											\
		Tangent = InTangent.TangentX;								\
		InTangent.AdjustNormal(Normal);								\
		Color = InColor;											\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)			\
	}

// Defines the Position/Normal/Tangent Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)									\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,									\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,								\
			RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsColor,								\
				RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
			)																																																		\
		)																																																			\
	)


// Implementation of Position/TangentX/TangentY/TangentZ Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ, const FColor& InColor RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))	\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		SetNormalAndTangent(InTangentX, InTangentY, InTangentZ);	\
		Color = InColor;											\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)			\
	}

// Defines the Position/TangentX/TangentY/TangentZ Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)								\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,											\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,										\
			RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsColor,										\
				RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
			)																																																				\
		)																																																					\
	)










#define DECLARE_RUNTIME_MESH_VERTEX(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)												\
	struct VertexName : public FRuntimeMeshVertex<NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType>											\
	{																																															\
		typedef FRuntimeMeshVertex<NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType> Super;													\
																																																\
		VertexName() { }																																										\
																																																\
		VertexName(EForceInit) : Super(EForceInit::ForceInit) { }																																\
																																																\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)									\
																																																\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)							\
																																																\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)							\
																																																\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)					\
																																																\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
																																																\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)				\
																																																\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
																																																\
	};		


// Define FRuntimeMeshVertexSimple

//////////////////////////////////////////////////////////////////////////
// Name Vertex Configurations
//////////////////////////////////////////////////////////////////////////

/** Simple vertex with 1 UV channel */
DECLARE_RUNTIME_MESH_VERTEX(FRuntimeMeshVertexSimple, true, true, true, true, 1, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision)

/** Simple vertex with 2 UV channels */
DECLARE_RUNTIME_MESH_VERTEX(FRuntimeMeshVertexDualUV, true, true, true, true, 2, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision)

/** Simple vertex with 1 UV channel and NO position component (Meant to be used with separate position buffer) */
DECLARE_RUNTIME_MESH_VERTEX(FRuntimeMeshVertexNoPosition, false, true, true, true, 1, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision)

/** Simple vertex with 2 UV channels and NO position component (Meant to be used with separate position buffer) */
DECLARE_RUNTIME_MESH_VERTEX(FRuntimeMeshVertexNoPositionDualUV, false, true, true, true, 2, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision)



//////////////////////////////////////////////////////////////////////////
// Vertex Structure Generator
//////////////////////////////////////////////////////////////////////////

struct FRuntimemeshVertexStructureHelper
{
	//////////////////////////////////////////////////////////////////////////
	// Position Component
	//////////////////////////////////////////////////////////////////////////
	template<typename RuntimeVertexType, bool WantsPosition>
	struct FRuntimeMeshPositionComponentVertexStructure
	{
		static void AddComponent(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.PositionComponent = RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, Position, VET_Float3);
		}
	};

	template<typename RuntimeVertexType>
	struct FRuntimeMeshPositionComponentVertexStructure<RuntimeVertexType, false>
	{
		static void AddComponent(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Normal/Tangent Components
	//////////////////////////////////////////////////////////////////////////
	template<typename RuntimeVertexType, bool WantsNormal, bool WantsTangent, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct FRuntimeMeshNormalTangentComponentVertexStructure
	{
		static void AddComponent(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TangentBasisComponents[1] = RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, Normal,
				FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::VertexElementType);
			VertexStructure.TangentBasisComponents[0] = RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, Tangent,
				FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::VertexElementType);
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct FRuntimeMeshNormalTangentComponentVertexStructure<RuntimeVertexType, true, false, NormalTangentType>
	{
		static void AddComponent(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TangentBasisComponents[1] = RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, Normal,
				FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::VertexElementType);
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct FRuntimeMeshNormalTangentComponentVertexStructure<RuntimeVertexType, false, true, NormalTangentType>
	{
		static void AddComponent(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TangentBasisComponents[0] = RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, Tangent,
				FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::VertexElementType);
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct FRuntimeMeshNormalTangentComponentVertexStructure<RuntimeVertexType, false, false, NormalTangentType>
	{
		static void AddComponent(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Color Component
	//////////////////////////////////////////////////////////////////////////
	template<typename RuntimeVertexType, bool WantsColor>
	struct FRuntimeMeshColorComponentVertexStructure
	{
		static void AddComponent(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.ColorComponent = RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, Color, VET_Color);
		}
	};

	template<typename RuntimeVertexType>
	struct FRuntimeMeshColorComponentVertexStructure<RuntimeVertexType, false>
	{
		static void AddComponent(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
		}
	};


	//////////////////////////////////////////////////////////////////////////
	// UV Components
	//////////////////////////////////////////////////////////////////////////
	template<typename RuntimeVertexType, int32 NumWantedUVChannels, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, 1, UVType>
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType1Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, 2, UVType>
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
		}
	};
	
	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, 3, UVType>
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{

			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType1Channel));
		}
	};
	
	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, 4, UVType>
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
		}
	};
	
	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, 5, UVType>
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV4, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType1Channel));
		}
	};
	
	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, 6, UVType>
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV4, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, 7, UVType>
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV4, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV6, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType1Channel));
		}
	};
	
	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, 8, UVType>
	{
		static void AddChannels(const FVertexBuffer& VertexBuffer, RuntimeMeshVertexStructure& VertexStructure)
		{
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV4, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.TextureCoordinates.Add(RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, RuntimeVertexType, UV6, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
		}
	};



	//////////////////////////////////////////////////////////////////////////
	// Vertex Structure Helper
	//////////////////////////////////////////////////////////////////////////
	template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels, 
		ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
	static RuntimeMeshVertexStructure CreateVertexStructure(const FVertexBuffer& VertexBuffer)
	{
		typedef FRuntimeMeshVertex<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType> RuntimeVertexType;

		RuntimeMeshVertexStructure VertexStructure;

		// Add Position component if necessary
		FRuntimeMeshPositionComponentVertexStructure<RuntimeVertexType, WantsPosition>::AddComponent(VertexBuffer, VertexStructure);

		// Add normal and tangent components if necessary
		FRuntimeMeshNormalTangentComponentVertexStructure<RuntimeVertexType, WantsNormal, WantsTangent, NormalTangentType>::AddComponent(VertexBuffer, VertexStructure);

		// Add color component if necessary
		FRuntimeMeshColorComponentVertexStructure<RuntimeVertexType, WantsColor>::AddComponent(VertexBuffer, VertexStructure);

		// Add all texture channels
		FRuntimeMeshTextureChannelsVertexStructure<RuntimeVertexType, NumWantedUVChannels, UVType>::AddChannels(VertexBuffer, VertexStructure);

		return VertexStructure;
	}
};




//////////////////////////////////////////////////////////////////////////
// Vertex Section Instantiator
//////////////////////////////////////////////////////////////////////////

// This is meant to support serialization where it can create any variation of the vertex.
// This has the unfortunate side-effect of instantiating every variation of the template.

struct FRuntimeMeshVertexSectionInstantiator
{
	//////////////////////////////////////////////////////////////////////////
	// Type Creator (Only creates type if it's not an empty vertex)
	//////////////////////////////////////////////////////////////////////////
private:
	template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
	static typename TEnableIf<(WantsPosition || WantsNormal || WantsTangent || WantsColor || (NumWantedUVChannels > 0)), FRuntimeMeshSectionInterface*>::Type CreateSection_Internal(bool bInNeedsPositionOnlyBuffer)
	{
		return new FRuntimeMeshSection<FRuntimeMeshVertex<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels,
			NormalTangentType, UVType>>(bInNeedsPositionOnlyBuffer);
	}

	template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType>
	static typename TEnableIf<!(WantsPosition || WantsNormal || WantsTangent || WantsColor || (NumWantedUVChannels > 0)), FRuntimeMeshSectionInterface*>::Type CreateSection_Internal(bool bInNeedsPositionOnlyBuffer)
	{
		checkNoEntry();
		return nullptr; 
	}


public:





	//////////////////////////////////////////////////////////////////////////
	// UV Type Selection
	//////////////////////////////////////////////////////////////////////////
	template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct FRuntimeMeshVertexInstantiator_UVType
	{
		static FRuntimeMeshSectionInterface* CreateSection(ERuntimeMeshVertexUVType UVType, bool bInNeedsPositionOnlyBuffer)
		{
			switch (UVType)
			{
			case ERuntimeMeshVertexUVType::Default:
				return CreateSection_Internal<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels,
					NormalTangentType, ERuntimeMeshVertexUVType::Default>(bInNeedsPositionOnlyBuffer);
			case ERuntimeMeshVertexUVType::HighPrecision:
				return CreateSection_Internal<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels,
					NormalTangentType, ERuntimeMeshVertexUVType::HighPrecision>(bInNeedsPositionOnlyBuffer);
			}
			checkNoEntry();
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Tangent Type Selection
	//////////////////////////////////////////////////////////////////////////
	template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels>
	struct FRuntimeMeshVertexInstantiator_NormalTangentType
	{
		static FRuntimeMeshSectionInterface* CreateSection(ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType, bool bInNeedsPositionOnlyBuffer)
		{
			switch (NormalTangentType)
			{
			case ERuntimeMeshVertexTangentBasisType::Default:
				return FRuntimeMeshVertexInstantiator_UVType<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, 
					ERuntimeMeshVertexTangentBasisType::Default>::CreateSection(UVType, bInNeedsPositionOnlyBuffer);
			case ERuntimeMeshVertexTangentBasisType::HighPrecision:
				return FRuntimeMeshVertexInstantiator_UVType<WantsPosition, WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, 
					ERuntimeMeshVertexTangentBasisType::HighPrecision>::CreateSection(UVType, bInNeedsPositionOnlyBuffer);
			}
			checkNoEntry();
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Num UV Channels Selection
	//////////////////////////////////////////////////////////////////////////
	template<bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor>
	struct FRuntimeMeshVertexInstantiator_NumWantedUVChannels
	{
		static FRuntimeMeshSectionInterface* CreateSection(int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType, bool bInNeedsPositionOnlyBuffer)
		{
			switch (NumWantedUVChannels)
			{
			case 0:
				return FRuntimeMeshVertexInstantiator_NormalTangentType<WantsPosition, WantsNormal, WantsTangent, WantsColor, 0>::CreateSection(NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			case 1:
				return FRuntimeMeshVertexInstantiator_NormalTangentType<WantsPosition, WantsNormal, WantsTangent, WantsColor, 1>::CreateSection(NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			case 2:
				return FRuntimeMeshVertexInstantiator_NormalTangentType<WantsPosition, WantsNormal, WantsTangent, WantsColor, 2>::CreateSection(NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			case 3:
				return FRuntimeMeshVertexInstantiator_NormalTangentType<WantsPosition, WantsNormal, WantsTangent, WantsColor, 3>::CreateSection(NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			case 4:
				return FRuntimeMeshVertexInstantiator_NormalTangentType<WantsPosition, WantsNormal, WantsTangent, WantsColor, 4>::CreateSection(NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			case 5:
				return FRuntimeMeshVertexInstantiator_NormalTangentType<WantsPosition, WantsNormal, WantsTangent, WantsColor, 5>::CreateSection(NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			case 6:
				return FRuntimeMeshVertexInstantiator_NormalTangentType<WantsPosition, WantsNormal, WantsTangent, WantsColor, 6>::CreateSection(NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			case 7:
				return FRuntimeMeshVertexInstantiator_NormalTangentType<WantsPosition, WantsNormal, WantsTangent, WantsColor, 7>::CreateSection(NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			}
			checkNoEntry();
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Wants Color Selection
	//////////////////////////////////////////////////////////////////////////
	template<bool WantsPosition, bool WantsNormal, bool WantsTangent>
	struct FRuntimeMeshVertexInstantiator_WantsColor
	{
		static FRuntimeMeshSectionInterface* CreateSection(bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType, bool bInNeedsPositionOnlyBuffer)
		{
			if (WantsColor)
			{
				return FRuntimeMeshVertexInstantiator_NumWantedUVChannels<WantsPosition, WantsNormal, WantsTangent, true>::CreateSection(NumWantedUVChannels, NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			}
			else
			{
				return FRuntimeMeshVertexInstantiator_NumWantedUVChannels<WantsPosition, WantsNormal, WantsTangent, false>::CreateSection(NumWantedUVChannels, NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			}
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Wants Tangent Selection
	//////////////////////////////////////////////////////////////////////////
	template<bool WantsPosition, bool WantsNormal>
	struct FRuntimeMeshVertexInstantiator_WantsTangent
	{
		static FRuntimeMeshSectionInterface* CreateSection(bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType, bool bInNeedsPositionOnlyBuffer)
		{
			if (WantsTangent)
			{
				return FRuntimeMeshVertexInstantiator_WantsColor<WantsPosition, WantsNormal, true>::CreateSection(WantsColor, NumWantedUVChannels, NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			}
			else
			{
				return FRuntimeMeshVertexInstantiator_WantsColor<WantsPosition, WantsNormal, false>::CreateSection(WantsColor, NumWantedUVChannels, NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			}
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Wants Normal Selection
	//////////////////////////////////////////////////////////////////////////
	template<bool WantsPosition>
	struct FRuntimeMeshVertexInstantiator_WantsNormal
	{
		static FRuntimeMeshSectionInterface* CreateSection(bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType, bool bInNeedsPositionOnlyBuffer)
		{
			if (WantsNormal)
			{
				return FRuntimeMeshVertexInstantiator_WantsTangent<WantsPosition, true>::CreateSection(WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			}
			else
			{
				return FRuntimeMeshVertexInstantiator_WantsTangent<WantsPosition, false>::CreateSection(WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
			}
		}
	};


	//////////////////////////////////////////////////////////////////////////
	// Vertex Section Instantiator
	//////////////////////////////////////////////////////////////////////////
	static FRuntimeMeshSectionInterface* CreateVertexStructure(bool WantsPosition, bool WantsNormal, bool WantsTangent, bool WantsColor, int32 NumWantedUVChannels, ERuntimeMeshVertexTangentBasisType NormalTangentType, ERuntimeMeshVertexUVType UVType, bool bInNeedsPositionOnlyBuffer)
	{
		// We do position selection right here
		if (WantsPosition)
		{
			return FRuntimeMeshVertexInstantiator_WantsNormal<true>::CreateSection(WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
		}
		else
		{
			return FRuntimeMeshVertexInstantiator_WantsNormal<false>::CreateSection(WantsNormal, WantsTangent, WantsColor, NumWantedUVChannels, NormalTangentType, UVType, bInNeedsPositionOnlyBuffer);
		}
	}
};











/** Section meant to support the old style interface for creating/updating sections */
template <int32 TextureChannels, bool HalfPrecisionUVs>
struct FRuntimeMeshSectionInternal :
	public FRuntimeMeshSection<FRuntimeMeshVertex<true, true, true, true, TextureChannels, 
		ERuntimeMeshVertexTangentBasisType::Default, HalfPrecisionUVs? ERuntimeMeshVertexUVType::Default : ERuntimeMeshVertexUVType::HighPrecision>>
{
public:

	typedef FRuntimeMeshVertex<true, true, true, true, TextureChannels,
		ERuntimeMeshVertexTangentBasisType::Default, HalfPrecisionUVs ? ERuntimeMeshVertexUVType::Default : ERuntimeMeshVertexUVType::HighPrecision> VertexType;


	template<typename Type, bool HasSecondUV>
	struct FUVSetter
	{
		FORCEINLINE static void Set(Type& Vertex, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, int32 VertexIndex, bool bShouldDefault)
		{
			if (UV0.Num() > VertexIndex)
			{
				Vertex.UV0 = UV0[VertexIndex];
			}
			else if (bShouldDefault)
			{
				Vertex.UV0 = FVector2D(0, 0);
			}
		}

		FORCEINLINE static void Serialize(FArchive& Ar, Type& Vertex)
		{
			Ar << Vertex.UV0;
		}
	};

	template<typename Type>
	struct FUVSetter<Type, true>
	{
		FORCEINLINE static void Set(Type& Vertex, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, int32 VertexIndex, bool bShouldDefault)
		{
			if (UV0.Num() > VertexIndex)
			{
				Vertex.UV0 = UV0[VertexIndex];
			}
			else if (bShouldDefault)
			{
				Vertex.UV0 = FVector2D(0, 0);
			}

			if (UV1.Num() > VertexIndex)
			{
				Vertex.UV1 = UV1[VertexIndex];
			}
			else if (bShouldDefault)
			{
				Vertex.UV1 = FVector2D(0, 0);
			}
		}

		FORCEINLINE static void Serialize(FArchive& Ar, Type& Vertex)
		{
			Ar << Vertex.UV0;
			Ar << Vertex.UV1;
		}
	};


	typedef FRuntimeMeshSection<VertexType> Super;


	FRuntimeMeshSectionInternal(bool bWantsSeparatePositionBuffer /*Ignored for this section type*/) : Super(false) { }
	virtual ~FRuntimeMeshSectionInternal() override { }

	virtual bool UpdateVertexBufferInternal(const TArray<FVector>& Positions, const TArray<FVector>& Normals, const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors) override
	{
		int32 NewVertexCount = (Positions.Num() > 0) ? Positions.Num() : Super::VertexBuffer.Num();
		int32 OldVertexCount = FMath::Min(Super::VertexBuffer.Num(), NewVertexCount);

		// Check existence of data components
		const bool HasPositions = Positions.Num() == NewVertexCount;
		
		// Size the vertex buffer correctly
		if (NewVertexCount != Super::VertexBuffer.Num())
		{
			Super::VertexBuffer.SetNumZeroed(NewVertexCount);
		}

		// Clear the bounding box if we have new positions
		if (HasPositions)
		{
			Super::LocalBoundingBox.Init();
		}
		
		// Loop through existing range to update data
		for (int32 VertexIdx = 0; VertexIdx < OldVertexCount; VertexIdx++)
		{
			auto& Vertex = Super::VertexBuffer[VertexIdx];

			// Update position and bounding box
			if (Positions.Num() == NewVertexCount)
			{
				Vertex.Position = Positions[VertexIdx];
				Super::LocalBoundingBox += Vertex.Position;
			}

			// see if we have a new normal and/or tangent
			bool HasNormal = Normals.Num() > VertexIdx;
			bool HasTangent = Tangents.Num() > VertexIdx;

			// Update normal and tangent together
			if (HasNormal && HasTangent)
			{
				Vertex.Normal = Normals[VertexIdx];
				Vertex.Normal.Vector.W = Tangents[VertexIdx].bFlipTangentY ? 0 : 255;
				Vertex.Tangent = Tangents[VertexIdx].TangentX;
			}
			// Else update only normal keeping the W component 
			else if (HasNormal)
			{
				float W = Vertex.Normal.Vector.W;
				Vertex.Normal = Normals[VertexIdx];
				Vertex.Normal.Vector.W = W;
			}
			// Else update tangent updating the normals W component
			else if (HasTangent)
			{
				Vertex.Tangent = Tangents[VertexIdx].TangentX;
				Vertex.Normal.Vector.W = Tangents[VertexIdx].bFlipTangentY ? 0 : 255;
			}

			// Update color
			if (Colors.Num() > VertexIdx)
			{
				Vertex.Color = Colors[VertexIdx];
			}

			// Set the UVs
			FUVSetter<VertexType, (TextureChannels > 1)>::Set(Vertex, UV0, UV1, VertexIdx, false);
		}

		// Loop through additional range to add new data
		for (int32 VertexIdx = OldVertexCount; VertexIdx < NewVertexCount; VertexIdx++)
		{
			auto& Vertex = Super::VertexBuffer[VertexIdx];

			// Set position
			Vertex.Position = Positions[VertexIdx];
			// Update bounding box
			Super::LocalBoundingBox += Vertex.Position;

			// see if we have a new normal and/or tangent
			bool HasNormal = Normals.Num() > VertexIdx;
			bool HasTangent = Tangents.Num() > VertexIdx;

			// Set normal and tangent both
			if (HasNormal && HasTangent)
			{
				Vertex.Normal = Normals[VertexIdx];
				Vertex.Normal.Vector.W = Tangents[VertexIdx].bFlipTangentY ? 0 : 255;
				Vertex.Tangent = Tangents[VertexIdx].TangentX;
			}
			// Set normal and default tangent
			else if (HasNormal)
			{
				Vertex.Normal = Normals[VertexIdx];
				Vertex.Normal.Vector.W = 255;
				Vertex.Tangent = FVector(1.0f, 0.0f, 0.0f);
			}
			// Default normal and set tangent
			else if (HasTangent)
			{
				Vertex.Normal = FVector(0.0f, 0.0f, 1.0f);
				Vertex.Normal.Vector.W = Tangents[VertexIdx].bFlipTangentY ? 0 : 255;
				Vertex.Tangent = Tangents[VertexIdx].TangentX;
			}
			// Default normal and tangent
			else
			{
				Vertex.Normal = FVector(0.0f, 0.0f, 1.0f);
				Vertex.Normal.Vector.W = 255;
				Vertex.Tangent = FVector(1.0f, 0.0f, 0.0f);
			}

			// Set color or default 
			Vertex.Color = Colors.Num() > VertexIdx ? Colors[VertexIdx] : FColor::White;

			// Set UVs or default
			FUVSetter<VertexType, (TextureChannels > 1)>::Set(Vertex, UV0, UV1, VertexIdx, true);
		}

		return true;
	}

	virtual void GetInternalVertexComponents(int32& NumUVChannels, bool& WantsHalfPrecisionUVs) override
	{
		NumUVChannels = TextureChannels;
		WantsHalfPrecisionUVs = HalfPrecisionUVs;
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
	
		int32 VertexBufferLength = Super::VertexBuffer.Num();
		Ar << VertexBufferLength;
		if (Ar.IsLoading())
		{
			Super::VertexBuffer.SetNum(VertexBufferLength);
		}

		for (int32 Index = 0; Index < VertexBufferLength; Index++)
		{
			auto& Vertex = Super::VertexBuffer[Index];

			Ar << Vertex.Position;
			Ar << Vertex.Normal;
			Ar << Vertex.Tangent;
			Ar << Vertex.Color;
			FUVSetter<VertexType, (TextureChannels > 1)>::Serialize(Ar, Vertex);
		}
	}

};