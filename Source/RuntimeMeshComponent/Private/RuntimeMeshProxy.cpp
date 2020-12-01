// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshProxy.h"
#include "CoreMinimal.h"
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

#define RMC_LOG_VERBOSE(Format, ...) \
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("[RMSP:%d Mesh:%d Thread:%d]: " Format), GetUniqueID(), ParentMeshId, FPlatformTLS::GetCurrentThreadId(), ##__VA_ARGS__);

FRuntimeMeshProxy::FRuntimeMeshProxy(uint32 InParentMeshId)
	: bShouldRender(false)
	, bShouldRenderStatic(false)
	, bShouldRenderDynamic(false)
	, bShouldRenderShadow(false)
	, MinAvailableLOD(INDEX_NONE)
	, MaxAvailableLOD(INDEX_NONE)
	, CurrentForcedLOD(INDEX_NONE)
	, ParentMeshId(InParentMeshId)
{
	RMC_LOG_VERBOSE("Created for RM:%d", InParentMeshId);
}

FRuntimeMeshProxy::~FRuntimeMeshProxy()
{
	// The mesh proxy can only be safely destroyed from the rendering thread.
	// This is so that all the resources can be safely freed correctly.
	check(IsInRenderingThread());

	RMC_LOG_VERBOSE("Destroyed");

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
		TWeakPtr<FRuntimeMeshProxy, ESPMode::ThreadSafe> ThisWeakRef = this->AsShared();

		ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_Update)([ThisWeakRef](FRHICommandListImmediate& RHICmdList)
			{
				auto Pinned = ThisWeakRef.Pin();
				if (Pinned)
				{
					Pinned->FlushPendingUpdates();
				}
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
	RMC_LOG_VERBOSE("ResetProxy_GameThread");
	PendingUpdates.Enqueue([this]()
		{
			ResetProxy_RenderThread();
		});
	QueueForUpdate();
}

void FRuntimeMeshProxy::InitializeLODs_GameThread(const TArray<FRuntimeMeshLODProperties>& InProperties)
{
	RMC_LOG_VERBOSE("InitializeLODs_GameThread");
	PendingUpdates.Enqueue([this, InProperties]()
		{
			InitializeLODs_RenderThread(InProperties);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::ClearAllSectionsForLOD_GameThread(int32 LODIndex)
{
	RMC_LOG_VERBOSE("ClearAllSectionsForLOD_GameThread: LOD:%d", LODIndex);
	PendingUpdates.Enqueue([this, LODIndex]()
		{
			ClearAllSectionsForLOD_RenderThread(LODIndex);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::RemoveAllSectionsForLOD_GameThread(int32 LODIndex)
{
	RMC_LOG_VERBOSE("RemoveAllSectionsForLOD_GameThread: LOD:%d", LODIndex);
	PendingUpdates.Enqueue([this, LODIndex]()
		{
			RemoveAllSectionsForLOD_RenderThread(LODIndex);
		});
	QueueForUpdate();
}

void FRuntimeMeshProxy::CreateOrUpdateSection_GameThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset)
{
	RMC_LOG_VERBOSE("CreateOrUpdateSection_GameThread: LOD:%d Section:%d Reset:%d", LODIndex, SectionId, bShouldReset);
	PendingUpdates.Enqueue([this, LODIndex, SectionId, InProperties, bShouldReset]()
		{
			CreateOrUpdateSection_RenderThread(LODIndex, SectionId, InProperties, bShouldReset);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::SetSectionsForLOD_GameThread(int32 LODIndex, const TMap<int32, FRuntimeMeshSectionProperties>& InProperties, bool bShouldReset)
{
	RMC_LOG_VERBOSE("SetSectionsForLOD_GameThread: LOD:%d Reset:%d", LODIndex, bShouldReset);
	PendingUpdates.Enqueue([this, LODIndex, InProperties, bShouldReset]()
		{
			SetSectionsForLOD_RenderThread(LODIndex, InProperties, bShouldReset);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::UpdateSectionMesh_GameThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshSectionUpdateData>& MeshData)
{
	RMC_LOG_VERBOSE("UpdateSectionMesh_GameThread: LOD:%d Section:%d", LODIndex, SectionId);
	PendingUpdates.Enqueue([this, LODIndex, SectionId, MeshData]()
		{
			UpdateSectionMesh_RenderThread(LODIndex, SectionId, MeshData);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::UpdateMultipleSectionsMesh_GameThread(int32 LODIndex, const TMap<int32, TSharedPtr<FRuntimeMeshSectionUpdateData>>& MeshData)
{
	RMC_LOG_VERBOSE("UpdateMultipleSectionsMesh_GameThread: LOD:%d", LODIndex);
	PendingUpdates.Enqueue([this, LODIndex, MeshData]()
		{
			UpdateMultipleSectionsMesh_RenderThread(LODIndex, MeshData);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::ClearSection_GameThread(int32 LODIndex, int32 SectionId)
{
	RMC_LOG_VERBOSE("ClearSection_GameThread: LOD:%d Section:%d", LODIndex, SectionId);
	PendingUpdates.Enqueue([this, LODIndex, SectionId]()
		{
			ClearSection_RenderThread(LODIndex, SectionId);
		});
	QueueForUpdate();
}
void FRuntimeMeshProxy::RemoveSection_GameThread(int32 LODIndex, int32 SectionId)
{
	RMC_LOG_VERBOSE("RemoveSection_GameThread: LOD:%d Section:%d", LODIndex, SectionId);
	PendingUpdates.Enqueue([this, LODIndex, SectionId]()
		{
			RemoveSection_RenderThread(LODIndex, SectionId);
		});
	QueueForUpdate();
}


void FRuntimeMeshProxy::ResetProxy_RenderThread()
{
	RMC_LOG_VERBOSE("ResetProxy_RenderThread");
	check(IsInRenderingThread());

	bShouldRender = false;
	bShouldRenderStatic = false;
	bShouldRenderDynamic = false;
	bShouldRenderShadow = false;

	MinAvailableLOD = INDEX_NONE;
	MaxAvailableLOD = INDEX_NONE;

	CurrentForcedLOD = INDEX_NONE;

	LODs.Empty();

}

void FRuntimeMeshProxy::InitializeLODs_RenderThread(const TArray<FRuntimeMeshLODProperties>& InProperties)
{
	RMC_LOG_VERBOSE("InitializeLODs_RenderThread");
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
	RMC_LOG_VERBOSE("ClearAllSectionsForLOD_RenderThread: LOD:%d", LODIndex);
	check(IsInRenderingThread());

	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];
		for (auto& SectionEntry : LOD.Sections)
		{
			ClearSection(SectionEntry.Value);			
		}
		UpdateRenderState();
	}
	else
	{
		// Invalid LOD
	}
}

void FRuntimeMeshProxy::RemoveAllSectionsForLOD_RenderThread(int32 LODIndex)
{
	RMC_LOG_VERBOSE("RemoveAllSectionsForLOD_RenderThread: LOD:%d", LODIndex);
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
	RMC_LOG_VERBOSE("CreateOrUpdateSection_RenderThread: LOD:%d Section:%d Reset:%d", LODIndex, SectionId, bShouldReset);
	check(IsInRenderingThread());

	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];

		FRuntimeMeshSectionProxy& Section = LOD.Sections.FindOrAdd(SectionId);

		if (bShouldReset)
		{
			ClearSection(Section);
		}

		Section.UpdateFrequency = InProperties.UpdateFrequency;
		Section.MaterialSlot = InProperties.MaterialSlot;

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
	RMC_LOG_VERBOSE("SetSectionsForLOD_RenderThread: LOD:%d Reset:%d", LODIndex, bShouldReset);
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

			Section.UpdateFrequency = SectionProperties.UpdateFrequency;
			Section.MaterialSlot = SectionProperties.MaterialSlot;

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
	RMC_LOG_VERBOSE("UpdateSectionMesh_RenderThread: LOD:%d Section:%d", LODIndex, SectionId);
	check(IsInRenderingThread());


	if (LODs.IsValidIndex(LODIndex))
	{
		auto& LOD = LODs[LODIndex];

		auto* FoundSection = LOD.Sections.Find(SectionId);
		if (FoundSection)
		{
			ApplyMeshToSection(LODIndex, SectionId, *FoundSection, MoveTemp(*MeshData));

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
	RMC_LOG_VERBOSE("UpdateMultipleSectionsMesh_RenderThread: LOD:%d", LODIndex);
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
				ApplyMeshToSection(LODIndex, SectionId, *FoundSection, MoveTemp(*MeshData));
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
	RMC_LOG_VERBOSE("ClearSection_RenderThread: LOD:%d Section:%d", LODIndex, SectionId);
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
	RMC_LOG_VERBOSE("RemoveSection_RenderThread: LOD:%d Section:%d", LODIndex, SectionId);
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
	Section.bHasRayTracingGeometry = false;

	Section.Buffers.Reset();
}

void FRuntimeMeshProxy::ApplyMeshToSection(int32 LODIndex, int32 SectionId, FRuntimeMeshSectionProxy& Section, FRuntimeMeshSectionUpdateData&& MeshData)
{
	RMC_LOG_VERBOSE("ApplyMeshToSection called");

	bool bShouldRecreateBuffers = !Section.Buffers.IsValid() || Section.UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent;

	// This creates the RHI buffers if they weren't previously created by another thread
	MeshData.CreateRHIBuffers<true>(Section.UpdateFrequency == ERuntimeMeshUpdateFrequency::Frequent);
		
	TRHIResourceUpdateBatcher<16> Batcher;

	if (bShouldRecreateBuffers)
	{
		Section.Buffers = MakeShared<FRuntimeMeshSectionProxyBuffers>(false, false);
		Section.Buffers->InitFromRHIReferences(MeshData, Batcher);
	}
	else
	{
		Section.Buffers->ApplyRHIReferences(MeshData, Batcher);
	}

	Batcher.Flush();

	// Update the state
	Section.FirstIndex = 0;
	Section.NumTriangles = MeshData.Triangles.GetNumElements() / 3;
	Section.MinVertexIndex = 0;
	Section.MaxVertexIndex = MeshData.Positions.GetNumElements();
	Section.UpdateState();
	
	if (Section.CanRender())
	{
		FRuntimeMeshSectionProxyBuffers& Buffers = *Section.Buffers.Get();

		FLocalVertexFactory::FDataType DataType;
		Buffers.PositionBuffer.Bind(DataType);
		Buffers.TangentsBuffer.Bind(DataType);
		Buffers.UVsBuffer.Bind(DataType);
		Buffers.ColorBuffer.Bind(DataType);
		Buffers.VertexFactory.Init(DataType);
		Buffers.VertexFactory.InitResource();

		// TODO: Should this use any lod selection logic for ray tracing?
		// We only use LOD 0 for ray tracing at this time
		if (LODIndex == 0)
		{
			Buffers.UpdateRayTracingGeometry();
		}
	}
	else
	{
		FRuntimeMeshSectionProxyBuffers& Buffers = *Section.Buffers.Get();

		Buffers.VertexFactory.ReleaseResource();
#if RHI_RAYTRACING
		Buffers.RayTracingGeometry.ReleaseResource();
#endif
	}

	check(!Section.CanRender() || Section.Buffers->VertexFactory.IsInitialized());
}


#undef RMC_LOG_VERBOSE