// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshProxy.h"

#include "Data/RealtimeMeshShared.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshProxy::FRealtimeMeshProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources)
		: SharedResources(InSharedResources)
		  , ValidLODRange(TRange<int8>::Empty())
		  , bIsStateDirty(true)
	{
	}

	FRealtimeMeshProxy::~FRealtimeMeshProxy()
	{
		// The mesh proxy can only be safely destroyed from the rendering thread.
		// This is so that all the resources can be safely freed correctly.
		check(IsInRenderingThread());
		Reset();
	}

	ERHIFeatureLevel::Type FRealtimeMeshProxy::GetRHIFeatureLevel() const
	{
		// TODO: Probably shouldn't assume this as max all the time??
		return GMaxRHIFeatureLevel;
	}

	TRange<float> FRealtimeMeshProxy::GetScreenSizeRangeForLOD(const FRealtimeMeshLODKey& LODKey) const
	{
		return ScreenSizeRangeByLOD.IsValidIndex(LODKey) ? ScreenSizeRangeByLOD[LODKey] : TRange<float>(0.0f, 0.0f);
	}

	FRealtimeMeshLODProxyPtr FRealtimeMeshProxy::GetLOD(FRealtimeMeshLODKey LODKey) const
	{
		return LODs.IsValidIndex(LODKey) ? LODs[LODKey] : FRealtimeMeshLODProxyPtr();
	}

	void FRealtimeMeshProxy::AddLODIfNotExists(const FRealtimeMeshLODKey& LODKey)
	{
		check(IsInRenderingThread());

		if (!LODs.IsValidIndex(LODKey))
		{
			LODs.SetNum(LODKey.Index() + 1);
		}

		LODs[LODKey] = SharedResources->CreateLODProxy(LODKey);
		MarkStateDirty();
	}

	void FRealtimeMeshProxy::RemoveLOD(const FRealtimeMeshLODKey& LODKey)
	{
		check(IsInRenderingThread());

		if (LODs.IsValidIndex(LODKey))
		{
			LODs[LODKey].Reset();

			for (int32 Index = LODs.Num() - 1; Index >= 0; Index--)
			{
				if (!LODs[Index].IsValid())
				{
					LODs.SetNum(Index);
				}
				else
				{
					break;
				}
			}

			MarkStateDirty();
		}
	}

	void FRealtimeMeshProxy::CreateMeshBatches(int32 LODIndex, const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
	                                           const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const
	{
		const auto& LOD = LODs[LODIndex];
		LOD->CreateMeshBatches(Params, Materials, WireframeMaterial, DrawType, bForceAllDynamic);
	}

	bool FRealtimeMeshProxy::UpdatedCachedState(bool bShouldForceUpdate)
	{
		// Handle all LOD updates next
		for (const auto& LOD : LODs)
		{
			bIsStateDirty |= LOD->UpdateCachedState(bShouldForceUpdate);
		}

		if (!bIsStateDirty && !bShouldForceUpdate)
		{
			return false;
		}

		FRealtimeMeshDrawMask NewMask;
		TArray<TRange<float>> NewScreenSizeRangeByLOD;
		TRange<int8> NewValidLODRange = TRange<int8>::Empty();
		TRangeBound<float> MaxScreenSize = TNumericLimits<float>::Max();

		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			const auto& LOD = LODs[LODIndex];

			const auto LODDrawMask = LOD->GetDrawMask();
			NewMask |= LODDrawMask;

			TRangeBound<float> NewScreenSize = MaxScreenSize;
			if (LODDrawMask.HasAnyFlags())
			{
				NewScreenSize = LOD->GetScreenSize();
				if (NewScreenSize.GetValue() > MaxScreenSize.GetValue())
				{
					NewScreenSize = MaxScreenSize;
				}

				if (!NewValidLODRange.IsEmpty())
				{
					NewValidLODRange = TRange<int8>(NewValidLODRange.GetLowerBound(), TRangeBound<int8>::Inclusive(LODIndex));
				}
				else
				{
					NewValidLODRange = TRange<int8>(TRangeBound<int8>::Inclusive(LODIndex), TRangeBound<int8>::Inclusive(LODIndex));
				}
			}
			NewScreenSizeRangeByLOD.Add(TRange<float>(NewScreenSize, MaxScreenSize));
			MaxScreenSize = NewScreenSize;
		}

		const bool bStateChanged = DrawMask != NewMask || ScreenSizeRangeByLOD != NewScreenSizeRangeByLOD || ValidLODRange != NewValidLODRange;

		DrawMask = NewMask;
		ScreenSizeRangeByLOD = MoveTemp(NewScreenSizeRangeByLOD);
		ValidLODRange = NewValidLODRange;
		bIsStateDirty = false;

		check(!DrawMask.HasAnyFlags() || !ValidLODRange.IsEmpty());

		return bStateChanged;
	}

	void FRealtimeMeshProxy::MarkStateDirty()
	{
		bIsStateDirty = true;
	}

	void FRealtimeMeshProxy::Reset()
	{
		LODs.Empty();

		DrawMask = FRealtimeMeshDrawMask();
		ValidLODRange = TRange<int8>::Empty();
		ScreenSizeRangeByLOD.Empty();
		bIsStateDirty = false;
	}
}
