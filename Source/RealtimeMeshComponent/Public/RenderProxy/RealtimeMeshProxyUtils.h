// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxy.h"


namespace RealtimeMesh
{	
	enum class ERealtimeMeshDrawMask : uint8
	{
		None = 0x0,
		DrawStatic = 0x1,
		DrawDynamic = 0x2,
		DrawMainPass = 0x4,
		DrawShadowPass = 0x8,
		DrawDepthPass = 0x10,


		DrawPassMask = DrawStatic | DrawDynamic,
	};
	ENUM_CLASS_FLAGS(ERealtimeMeshDrawMask);


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshDrawMask
	{
	private:
		ERealtimeMeshDrawMask MaskValue;

	public:
		FRealtimeMeshDrawMask() : MaskValue(ERealtimeMeshDrawMask::None) { }
		FRealtimeMeshDrawMask(ERealtimeMeshDrawMask InMask) : MaskValue(InMask) { }

		FORCEINLINE void SetFlag(ERealtimeMeshDrawMask InFlag) { MaskValue |= InFlag; }
		FORCEINLINE bool IsSet(ERealtimeMeshDrawMask InFlag) const { return EnumHasAllFlags(MaskValue, InFlag); }
		FORCEINLINE bool IsAnySet(ERealtimeMeshDrawMask InFlag) const { return EnumHasAnyFlags(MaskValue, InFlag); }
		FORCEINLINE bool HasAnyFlags() const { return MaskValue != ERealtimeMeshDrawMask::None; }
		FORCEINLINE void RemoveFlag(ERealtimeMeshDrawMask InFlag) { EnumRemoveFlags(MaskValue, InFlag); }
		
		FORCEINLINE bool ShouldRenderMainPass() const { return EnumHasAllFlags(MaskValue, ERealtimeMeshDrawMask::DrawMainPass); }
		FORCEINLINE bool IsStaticSection() const { return EnumHasAllFlags(MaskValue, ERealtimeMeshDrawMask::DrawStatic); }

		FORCEINLINE bool ShouldRender() const { return MaskValue != ERealtimeMeshDrawMask::None; }
		FORCEINLINE bool ShouldRenderStaticPath() const { return ShouldRender() && ShouldRenderMainPass() && IsStaticSection(); }
		FORCEINLINE bool ShouldRenderDynamicPath() const { return ShouldRender() && ShouldRenderMainPass() && !IsStaticSection(); }
		FORCEINLINE bool ShouldRenderShadow() const { return ShouldRender() && EnumHasAllFlags(MaskValue, ERealtimeMeshDrawMask::DrawShadowPass); }

		FORCEINLINE bool operator==(const FRealtimeMeshDrawMask& Other) const { return MaskValue == Other.MaskValue; }
		FORCEINLINE bool operator!=(const FRealtimeMeshDrawMask& Other) const { return MaskValue != Other.MaskValue; }

		FORCEINLINE FRealtimeMeshDrawMask operator|(const FRealtimeMeshDrawMask& Other) const { return FRealtimeMeshDrawMask(MaskValue | Other.MaskValue); }
		FORCEINLINE FRealtimeMeshDrawMask& operator|=(const FRealtimeMeshDrawMask& Other) { MaskValue |= Other.MaskValue; return *this; }
		
		FORCEINLINE FRealtimeMeshDrawMask operator&(const FRealtimeMeshDrawMask& Other) const { return FRealtimeMeshDrawMask(MaskValue & Other.MaskValue); }
		FORCEINLINE FRealtimeMeshDrawMask& operator&=(const FRealtimeMeshDrawMask& Other) { MaskValue &= Other.MaskValue; return *this; }
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBatchCreationParams
	{
		TSharedRef<const FRealtimeMeshSectionGroupProxy>& SectionGroup;
		TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter;
		TFunctionRef<void(FMeshBatch&, float)> BatchSubmitter;
		TFunctionRef<FMeshBatch&()> BatchAllocator;

		FRHIUniformBuffer* UniformBuffer;
		const FMaterialRenderProxy* Material;

		FLODMask LODMask;
		TRange<float>& ScreenSizeLimits;
		uint32 bIsMovable : 1;
		uint32 bIsLocalToWorldDeterminantNegative : 1;
		uint32 bIsWireframe : 1;
		uint32 bSupportsDithering : 1;
		uint32 bCastRayTracedShadow : 1;
	};

	
}
