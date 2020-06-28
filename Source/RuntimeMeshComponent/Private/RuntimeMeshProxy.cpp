// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshProxy.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMesh.h"

DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Initialize LDOs - RenderThread"), STAT_RuntimeMeshProxy_InitializeLODs_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Clear LOD - RenderThread"), STAT_RuntimeMeshProxy_ClearLOD_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Create/Update Section - RenderThread"), STAT_RuntimeMeshProxy_CreateUpdateSection_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Update Section Mesh - RenderThread"), STAT_RuntimeMeshProxy_UpdateSection_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Clear All Sections - RenderThread"), STAT_RuntimeMeshProxy_ClearAllSections_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Clear Section - RenderThread"), STAT_RuntimeMeshProxy_ClearSection_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Remove All Sections - RenderThread"), STAT_RuntimeMeshProxy_RemoveAllSections_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Remove Section - RenderThread"), STAT_RuntimeMeshProxy_RemoveSection_RT, STATGROUP_RuntimeMesh);

FRuntimeMeshProxy::FRuntimeMeshProxy()
	: bShouldRender(false)
	, bShouldRenderStatic(false)
	, bShouldRenderDynamic(false)
	, bShouldRenderShadow(false)
	, MinAvailableLOD(INDEX_NONE)
	, MaxAvailableLOD(INDEX_NONE)
	, CurrentForcedLOD(INDEX_NONE)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): Created"), FPlatformTLS::GetCurrentThreadId());
}

FRuntimeMeshProxy::~FRuntimeMeshProxy()
{
	// The mesh proxy can only be safely destroyed from the rendering thread.
	// This is so that all the resources can be safely freed correctly.
	check(IsInRenderingThread());
}


float FRuntimeMeshProxy::GetScreenSize(int32 LODIndex) const
{
	if (LODIndex >= MinAvailableLOD && LODIndex <= MaxAvailableLOD)
	{
		return LODs[LODIndex].ScreenSize;
	}
	return 0.0f;
}

void FRuntimeMeshProxy::QueueForUpdate()
{
	// TODO: Is this really necessary. Enqueueing render commands fails sometimes when not called from game thread
	// We need to correctly support asynchronus submission 

	if (!IsQueuedForUpdate.AtomicSet(true)/* && GRenderingThread && !GIsRenderingThreadSuspended.Load(EMemoryOrder::Relaxed)*/)
	{
		ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_Update)([this](FRHICommandListImmediate& RHICmdList)
			{
				FlushPendingUpdates();
			});
	}
}

void FRuntimeMeshProxy::FlushPendingUpdates()
{
	check(IsInRenderingThread());
	IsQueuedForUpdate.AtomicSet(false);

	TFunction<void()> Cmd;
	while (PendingUpdates.Dequeue(Cmd))
	{
		Cmd();
	}
}






void FRuntimeMeshProxy::ResetProxy_GameThread()
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ResetProxy_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this]()
		{
			ResetProxy_RenderThread();
		});
	QueueForUpdate();
}

void FRuntimeMeshProxy::InitializeLODs_GameThread(const TArray<FRuntimeMeshLODProperties>& InProperties)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): InitializeLODs_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, InProperties]()
		{
			InitializeLODs_RenderThread(InProperties);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::ClearAllSectionsForLOD_GameThread(int32 LODIndex)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ClearAllSectionsForLOD_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex]()
		{
			ClearAllSectionsForLOD_RenderThread(LODIndex);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::RemoveAllSectionsForLOD_GameThread(int32 LODIndex)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): RemoveAllSectionsForLOD_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex]()
		{
			RemoveAllSectionsForLOD_RenderThread(LODIndex);
		});
	QueueForUpdate();
}

void FRuntimeMeshProxy::CreateOrUpdateSection_GameThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): CreateOrUpdateSection_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex, SectionId, InProperties, bShouldReset]()
		{
			CreateOrUpdateSection_RenderThread(LODIndex, SectionId, InProperties, bShouldReset);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::SetSectionsForLOD_GameThread(int32 LODIndex, const TMap<int32, FRuntimeMeshSectionProperties>& InProperties, bool bShouldReset)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): SetSectionsForLOD_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex, InProperties, bShouldReset]()
		{
			SetSectionsForLOD_RenderThread(LODIndex, InProperties, bShouldReset);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::UpdateSectionMesh_GameThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshSectionUpdateData>& MeshData)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): UpdateSectionMesh_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex, SectionId, MeshData]()
		{
			UpdateSectionMesh_RenderThread(LODIndex, SectionId, MeshData);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::UpdateMultipleSectionsMesh_GameThread(int32 LODIndex, const TMap<int32, TSharedPtr<FRuntimeMeshSectionUpdateData>>& MeshData)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): UpdateMultipleSectionsMesh_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex, MeshData]()
		{
			UpdateMultipleSectionsMesh_RenderThread(LODIndex, MeshData);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::ClearSection_GameThread(int32 LODIndex, int32 SectionId)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ClearSection_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex, SectionId]()
		{
			ClearSection_RenderThread(LODIndex, SectionId);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::RemoveSection_GameThread(int32 LODIndex, int32 SectionId)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): RemoveSection_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex, SectionId]()
		{
			RemoveSection_RenderThread(LODIndex, SectionId);
		});
	QueueForUpdate();
}


void FRuntimeMeshProxy::ResetProxy_RenderThread()
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ResetProxy_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

	LODs.Empty();
}

void FRuntimeMeshProxy::InitializeLODs_RenderThread(const TArray<FRuntimeMeshLODProperties>& InProperties)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): InitializeLODs_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

	LODs.SetNum(InProperties.Num());
	for (int32 LODIndex = 0; LODIndex < InProperties.Num(); LODIndex++)
	{
		auto& LOD = LODs[LODIndex];
		auto& LODProperties = InProperties[LODIndex];

		LOD.ScreenSize = LODProperties.ScreenSize;
		LOD.bShouldMergeStaticSections = LODProperties.bShouldMergeStaticSectionBuffers;

		LOD.bShouldRender = false;
		LOD.bShouldRenderStatic = false;
		LOD.bShouldRenderDynamic = false;
	}
}

void FRuntimeMeshProxy::ClearAllSectionsForLOD_RenderThread(int32 LODIndex)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ClearAllSectionsForLOD_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

}

void FRuntimeMeshProxy::RemoveAllSectionsForLOD_RenderThread(int32 LODIndex)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): RemoveAllSectionsForLOD_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];
		LOD.Sections.Empty();
		UpdateRenderState();
	}
	else
	{
		// Invalid LOD
	}
}

void FRuntimeMeshProxy::CreateOrUpdateSection_RenderThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): CreateOrUpdateSection_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];

		FRuntimeMeshSectionProxy& Section = LOD.Sections.FindOrAdd(SectionId);

		if (bShouldReset)
		{
			ClearSection(Section);
		}

		Section.Buffers = MakeShared<FRuntimeMeshSectionProxyBuffers>(InProperties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Frequent, false);
		Section.Buffers->InitResource();

		Section.UpdateFrequency = InProperties.UpdateFrequency;
		Section.bIsVisible = InProperties.bIsVisible;
		Section.bIsMainPassRenderable = InProperties.bIsMainPassRenderable;
		Section.bCastsShadow = InProperties.bCastsShadow;
		Section.bForceOpaque = InProperties.bForceOpaque;

		UpdateRenderState();
	}
	else
	{
		// Invalid LOD
	}
}

void FRuntimeMeshProxy::SetSectionsForLOD_RenderThread(int32 LODIndex, const TMap<int32, FRuntimeMeshSectionProperties>& InProperties, bool bShouldReset)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): SetSectionsForLOD_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];

		// Remove old sections
		TArray<int32> SectionIds;
		LOD.Sections.GetKeys(SectionIds);

		for (int32 SectionId : SectionIds)
		{
			if (!InProperties.Contains(SectionId))
			{
				LOD.Sections.Remove(SectionId);
			}
		}

		for (auto& NewSectionEntry : InProperties)
		{
			int32 SectionId = NewSectionEntry.Key;
			FRuntimeMeshSectionProperties SectionProperties = NewSectionEntry.Value;

			FRuntimeMeshSectionProxy& Section = LOD.Sections.FindOrAdd(SectionId);

			if (bShouldReset)
			{
				ClearSection(Section);
			}

			Section.Buffers = MakeShared<FRuntimeMeshSectionProxyBuffers>(SectionProperties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Frequent, false);
			Section.Buffers->InitResource();

			Section.bIsVisible = SectionProperties.bIsVisible;
			Section.bIsMainPassRenderable = SectionProperties.bIsMainPassRenderable;
			Section.bCastsShadow = SectionProperties.bCastsShadow;
			Section.bForceOpaque = SectionProperties.bForceOpaque;
		}

		UpdateRenderState();
	}
	else
	{
		// Invalid LOD
	}
}

void FRuntimeMeshProxy::UpdateSectionMesh_RenderThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshSectionUpdateData>& MeshData)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): UpdateSectionMesh_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());


	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];

		auto* FoundSection = LOD.Sections.Find(SectionId);
		if (FoundSection)
		{
			ApplyMeshToSection(*FoundSection, MoveTemp(*MeshData));

			UpdateRenderState();
		}
		else
		{
			// Invalid section
		}
	}
	else
	{
		// Invalid LOD
	}
}

void FRuntimeMeshProxy::UpdateMultipleSectionsMesh_RenderThread(int32 LODIndex, const TMap<int32, TSharedPtr<FRuntimeMeshSectionUpdateData>>& MeshDatas)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): UpdateMultipleSectionsMesh_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];

		for (auto& NewSectionMesh : MeshDatas)
		{
			int32 SectionId = NewSectionMesh.Key;
			const TSharedPtr<FRuntimeMeshSectionUpdateData>& MeshData = NewSectionMesh.Value;

			auto* FoundSection = LOD.Sections.Find(SectionId);
			if (FoundSection)
			{
				ApplyMeshToSection(*FoundSection, MoveTemp(*MeshData));
			}
			else
			{
				// Invalid section
			}
		}

		UpdateRenderState();
	}
	else
	{
		// Invalid LOD
	}
}

void FRuntimeMeshProxy::ClearSection_RenderThread(int32 LODIndex, int32 SectionId)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ClearSection_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];

		FRuntimeMeshSectionProxy* FoundSection = LOD.Sections.Find(SectionId);
		if (FoundSection)
		{
			ClearSection(*FoundSection);
			UpdateRenderState();
		}
		else
		{
			// Invalid section
		}
	}
	else
	{
		// Invalid LOD
	}
}

void FRuntimeMeshProxy::RemoveSection_RenderThread(int32 LODIndex, int32 SectionId)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): RemoveSection_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());
	check(IsInRenderingThread());

	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];

		if (LOD.Sections.Remove(SectionId))
		{
			UpdateRenderState();
		}
		else
		{
			// Invalid section
		}
	}
	else
	{
		// Invalid LOD
	}
}

void FRuntimeMeshProxy::UpdateRenderState()
{
	bShouldRender = false;
	bShouldRenderStatic = false;
	bShouldRenderDynamic = false;
	bShouldRenderShadow = false;

	for (int32 Index = 0; Index < LODs.Num(); Index++)
	{
		FRuntimeMeshLODData& LOD = LODs[Index];

		LOD.UpdateState();

		bShouldRender |= LOD.bShouldRender;
		bShouldRenderStatic |= LOD.bShouldRender;
		bShouldRenderDynamic |= LOD.bShouldRenderDynamic;
		bShouldRenderShadow |= LOD.bShouldRenderShadow;

		if (LOD.bShouldRender)
		{
			if (MinAvailableLOD == INDEX_NONE)
			{
				MinAvailableLOD = Index;
			}

			MaxAvailableLOD = FMath::Max<int32>(MaxAvailableLOD, Index);
		}
	}
}

void FRuntimeMeshProxy::ClearSection(FRuntimeMeshSectionProxy& Section)
{
	Section.FirstIndex = 0;
	Section.NumTriangles = 0;
	Section.MinVertexIndex = 0;
	Section.MaxVertexIndex = 0;
	Section.bIsValid = false;

	Section.bHasAdjacencyInfo = false;
	Section.bHasDepthOnlyIndices = false;
	Section.bHasReversedIndices = false;
	Section.bHasReversedDepthOnlyIndices = false;
	Section.bHasRayTracingGeometry = false;

	Section.Buffers.Reset();
}

void FRuntimeMeshProxy::ApplyMeshToSection(FRuntimeMeshSectionProxy& Section, FRuntimeMeshSectionUpdateData&& MeshData)
{
	if (Section.UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent)
	{
		Section.Buffers = MakeShared<FRuntimeMeshSectionProxyBuffers>(false, false);
		Section.Buffers->InitResource();
	}
	FRuntimeMeshSectionProxyBuffers& Buffers = *Section.Buffers.Get();


	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ApplyMeshToSection Called"), FPlatformTLS::GetCurrentThreadId());


	// Update all buffers data

	/* Todo: Make this batch count a little more accurate to what is required*/
	TRHIResourceUpdateBatcher<16> Batcher;


	MeshData.CreateRHIBuffers<true>(false);
	Buffers.ApplyRHIReferences(MeshData, Batcher);
	Batcher.Flush();

	Section.FirstIndex = 0;
	Section.NumTriangles = MeshData.Triangles.GetNumElements() / 3;
	Section.MinVertexIndex = 0;
	Section.MaxVertexIndex = MeshData.Positions.GetNumElements();



	Section.UpdateState();
		

	if (Section.CanRender())
	{
		FLocalVertexFactory::FDataType DataType;
		Buffers.PositionBuffer.Bind(DataType);
		Buffers.TangentsBuffer.Bind(DataType);
		Buffers.UVsBuffer.Bind(DataType);
		Buffers.ColorBuffer.Bind(DataType);
		Buffers.VertexFactory.Init(DataType);
		Buffers.VertexFactory.InitResource();
	}
	else
	{
		Buffers.VertexFactory.ReleaseResource();
	}


	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ApplyMeshToSection Finished"), FPlatformTLS::GetCurrentThreadId());
}
