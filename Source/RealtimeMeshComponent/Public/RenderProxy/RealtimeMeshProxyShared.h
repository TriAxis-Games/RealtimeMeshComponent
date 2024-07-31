// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"
#include "MeshBatch.h"
#include "SceneManagement.h"

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
		FRealtimeMeshDrawMask() : MaskValue(ERealtimeMeshDrawMask::None)
		{
		}

		FRealtimeMeshDrawMask(ERealtimeMeshDrawMask InMask) : MaskValue(InMask)
		{
		}

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
		FORCEINLINE FRealtimeMeshDrawMask& operator|=(const FRealtimeMeshDrawMask& Other)
		{
			MaskValue |= Other.MaskValue;
			return *this;
		}

		FORCEINLINE FRealtimeMeshDrawMask operator&(const FRealtimeMeshDrawMask& Other) const { return FRealtimeMeshDrawMask(MaskValue & Other.MaskValue); }
		FORCEINLINE FRealtimeMeshDrawMask& operator&=(const FRealtimeMeshDrawMask& Other)
		{
			MaskValue &= Other.MaskValue;
			return *this;
		}
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBatchCreationParams
	{
		TFunction<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter;
		TFunction<FMeshBatch&()> BatchAllocator;
#if RHI_RAYTRACING
		TFunction<void(FMeshBatch&, float, const FRayTracingGeometry*)> BatchSubmitter;
#else
		TFunction<void(FMeshBatch&, float)> BatchSubmitter;
#endif

		FRHIUniformBuffer* UniformBuffer;

		TRange<float> ScreenSizeLimits;
		FLODMask LODMask;

		uint32 bIsMovable : 1;
		uint32 bIsLocalToWorldDeterminantNegative : 1;
		uint32 bCastRayTracedShadow : 1;
	};

	
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshResourceReferenceList
	{
	private:
		TSet<TSharedPtr<FRenderResource>> Resources;

	public:
		void AddResource(const TSharedRef<FRenderResource>& Resource)
		{
			Resources.Add(Resource);
		}
		
		void AddResource(const TSharedPtr<FRenderResource>& Resource)
		{
			Resources.Add(Resource);
		}

		void ClearReferences()
		{
			Resources.Empty();
		}
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshMaterialProxyMap
	{
	private:
		TSparseArray<FMaterialRenderProxy*> Materials;
		TBitArray<> MaterialSupportsDither;
	public:
		void SetMaterial(int32 Index, FMaterialRenderProxy* InMaterial)
		{
			Materials.Insert(Index, InMaterial);
		}
		void SetMaterialSupportsDither(int32 Index, bool bInSupportsDither)
		{
			if (MaterialSupportsDither.Num() < Index + 1)
			{
				MaterialSupportsDither.SetNum(Index + 1, false);
			}
			MaterialSupportsDither[Index] = bInSupportsDither;
		}

		FMaterialRenderProxy* GetMaterial(int32 Index) const
		{
			return Materials.IsValidIndex(Index)? Materials[Index] : nullptr;
		}

		bool GetMaterialSupportsDither(int32 Index) const
		{
			return MaterialSupportsDither.IsValidIndex(Index)? MaterialSupportsDither[Index] : false;
		}

		void Reset()
		{
			Materials.Empty();
			MaterialSupportsDither.Empty();
		}
	};
	
}
