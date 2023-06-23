// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshProxy.h"

#include "RenderProxy/RealtimeMeshLODProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshProxy::FRealtimeMeshProxy(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh)
		: ClassFactory(InClassFactory)
		, MeshWeak(InMesh)
		, ValidLODRange(TRange<int8>::Empty())
		, IsQueuedForUpdate(false)
		, bIsStateDirty(false)
	{
	}

	FRealtimeMeshProxy::~FRealtimeMeshProxy()
	{
		// The mesh proxy can only be safely destroyed from the rendering thread.
		// This is so that all the resources can be safely freed correctly.
		check(IsInRenderingThread());
	}

	void FRealtimeMeshProxy::InitializeRenderThreadResources(const FRealtimeMeshProxyInitializationParametersRef& InitParams)
	{
		LODs.Reserve(InitParams->LODs.Num());
		for (int32 LODIndex = 0; LODIndex < InitParams->LODs.Num(); LODIndex++)
		{
			LODs.Add(ClassFactory->CreateLODProxy(this->AsShared(), FRealtimeMeshLODKey(LODIndex), InitParams->LODs[LODIndex]));
		}
	}

	ERHIFeatureLevel::Type FRealtimeMeshProxy::GetRHIFeatureLevel() const
	{
		// TODO: Probably shouldn't assume this as max all the time??
		return GMaxRHIFeatureLevel;
	}

	FRealtimeMeshLODProxyPtr FRealtimeMeshProxy::GetLOD(FRealtimeMeshLODKey LODKey) const
	{
		return LODs.IsValidIndex(FRealtimeMeshKeyHelpers::GetLODIndex(LODKey))
			? LODs[FRealtimeMeshKeyHelpers::GetLODIndex(LODKey)]
			: FRealtimeMeshLODProxyPtr();
	}

	TRange<float> FRealtimeMeshProxy::GetScreenSizeLimits(FRealtimeMeshLODKey LODKey) const
	{
		return ScreenSizeRangeByLOD.IsValidIndex(FRealtimeMeshKeyHelpers::GetLODIndex(LODKey))
			? ScreenSizeRangeByLOD[FRealtimeMeshKeyHelpers::GetLODIndex(LODKey)]
			: TRange<float>(0.0f, 0.0f);
	}


	void FRealtimeMeshProxy::InitializeLODs(const TFixedLODArray<FRealtimeMeshLODProxyInitializationParametersRef>& LODConfigs)
	{
		check(IsInRenderingThread());

		LODs.Empty(LODConfigs.Num());
		for (int32 LODIndex = 0; LODIndex < LODConfigs.Num(); LODIndex++)
		{
			LODs.Add(ClassFactory->CreateLODProxy(this->AsShared(), FRealtimeMeshLODKey(LODIndex), LODConfigs[LODIndex]));
		}			
	}

	void FRealtimeMeshProxy::AddLOD(const FRealtimeMeshLODKey& NewLODKey, const FRealtimeMeshLODProxyInitializationParametersRef& Config)
	{
		check(IsInRenderingThread());
		check(LODs.Num() == FRealtimeMeshKeyHelpers::GetLODIndex(NewLODKey));

		LODs.Add(ClassFactory->CreateLODProxy(this->AsShared(), NewLODKey, Config));
		MarkStateDirty();		
	}

	void FRealtimeMeshProxy::RemoveTrailingLOD()
	{
		check(IsInRenderingThread());
		check(LODs.Num() > 1);

		LODs.RemoveAt(LODs.Num() - 1);
		MarkStateDirty();		
	}

	void FRealtimeMeshProxy::CreateMeshBatches(int32 LODIndex, const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
		const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const
	{
		const auto& LOD = LODs[LODIndex];		
		LOD->CreateMeshBatches(Params, Materials, WireframeMaterial, DrawType, bForceAllDynamic);
	}

	void FRealtimeMeshProxy::EnqueueRenderingCommand(TUniqueFunction<void(const FRealtimeMeshProxyRef&)>&& InCommand)
	{
		PendingUpdates.Enqueue(MoveTemp(InCommand));
			
		if (!IsQueuedForUpdate.AtomicSet(true))
		{
			FRealtimeMeshProxyWeakPtr ThisWeakRef = this->AsShared();

			ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_Update)([ThisWeakRef](FRHICommandListImmediate&)
			{
				if (const auto Pinned = ThisWeakRef.Pin())
				{
					Pinned->HandleUpdates();
				}
			});
		}
	}

	void FRealtimeMeshProxy::MarkStateDirty()
	{
		bIsStateDirty = true;
	}

	bool FRealtimeMeshProxy::HandleUpdates()
	{
		bool bHadUpdates = false;
		
		// Flush all the pending render thread tasks
		IsQueuedForUpdate.AtomicSet(false);

		const FRealtimeMeshProxyRef ThisRef = this->AsShared();
		TUniqueFunction<void(const FRealtimeMeshProxyRef&)> Cmd;
		while (PendingUpdates.Dequeue(Cmd))
		{
			Cmd(ThisRef);
			bHadUpdates = true;
		}

		if (!bHadUpdates)
		{
			return false;
		}
		
		// Handle all LOD updates next
		for (const auto& LOD : LODs)
		{
			bIsStateDirty |= LOD->HandleUpdates();
		}

		if (!bIsStateDirty)
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
					NewValidLODRange.SetUpperBound(TRangeBound<int8>::Inclusive(LODIndex));
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

	void FRealtimeMeshProxy::Reset()
	{
		IsQueuedForUpdate.AtomicSet(false);
		PendingUpdates.Empty();
		LODs.Empty();

		DrawMask = FRealtimeMeshDrawMask();
		ValidLODRange = TRange<int8>::Empty();
		ScreenSizeRangeByLOD.Empty();
		bIsStateDirty = false;
	}
}
