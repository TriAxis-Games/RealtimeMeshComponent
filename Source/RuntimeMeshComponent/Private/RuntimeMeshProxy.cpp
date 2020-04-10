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

FRuntimeMeshProxy::FRuntimeMeshProxy(ERHIFeatureLevel::Type InFeatureLevel)
	: FeatureLevel(InFeatureLevel)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): Created"), FPlatformTLS::GetCurrentThreadId());
}

FRuntimeMeshProxy::~FRuntimeMeshProxy()
{
	// The mesh proxy can only be safely destroyed from the rendering thread.
	// This is so that all the resources can be safely freed correctly.
	check(IsInRenderingThread());
}

float FRuntimeMeshProxy::GetScreenSize(int32 LODIndex)
{
	if (LODIndex >= LODs.Num())
	{
		return 0;
	}

	return LODs[LODIndex]->GetMaxScreenSize();
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

void FRuntimeMeshProxy::QueueForUpdate()
{
	if (!IsQueuedForUpdate.AtomicSet(true))
	{
 		ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_Update)(
  		[this](FRHICommandListImmediate& RHICmdList)
		{
			FlushPendingUpdates();
		}
		);
	}
}

void FRuntimeMeshProxy::FlushPendingUpdates()
{
	IsQueuedForUpdate.AtomicSet(false);

	TFunction<void()> Cmd;
	while (PendingUpdates.Dequeue(Cmd))
	{
		Cmd();
	}
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

void FRuntimeMeshProxy::CreateOrUpdateSection_GameThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): CreateOrUpdateSection_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex, SectionId, InProperties, bShouldReset]()
		{
			CreateOrUpdateSection_RenderThread(LODIndex, SectionId, InProperties, bShouldReset);
		});
	QueueForUpdate();
}

void FRuntimeMeshProxy::UpdateSectionMesh_GameThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): UpdateSectionMesh_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex, SectionId, MeshData]()
		{
			UpdateSectionMesh_RenderThread(LODIndex, SectionId, MeshData);
		});
	QueueForUpdate();
}

void FRuntimeMeshProxy::ClearAllSections_GameThread(int32 LODIndex)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ClearAllSections_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex]()
		{
			ClearAllSections_RenderThread(LODIndex);
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

void FRuntimeMeshProxy::RemoveAllSections_GameThread(int32 LODIndex)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): RemoveAllSections_GameThread Called"), FPlatformTLS::GetCurrentThreadId());
	PendingUpdates.Enqueue([this, LODIndex]()
		{
			RemoveAllSections_RenderThread(LODIndex);
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
	//SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_RemoveAllSections_RT);

	check(IsInRenderingThread());
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ResetProxy_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());

	for (int32 Index = 0; Index < LODs.Num(); Index++)
	{
		LODs[Index].Reset();
	}
}

void FRuntimeMeshProxy::InitializeLODs_RenderThread(const TArray<FRuntimeMeshLODProperties>& InProperties)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_InitializeLODs_RT);
	check(IsInRenderingThread());
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): InitializeLODs_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());

	// Remove whatever might have been there
	LODs.Empty();

	for (int32 LODIndex = 0; LODIndex < InProperties.Num(); LODIndex++)
	{
		LODs.Add(MakeShareable(new FRuntimeMeshLODProxy(FeatureLevel, InProperties[LODIndex]), FRuntimeMeshRenderThreadDeleter<FRuntimeMeshLODProxy>()));

	}	
}

void FRuntimeMeshProxy::CreateOrUpdateSection_RenderThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties, bool bShouldReset)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_CreateUpdateSection_RT);

	check(IsInRenderingThread());
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): CreateOrUpdateSection_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());

	LODs[LODIndex]->CreateOrUpdateSection_RenderThread(SectionId, InProperties, bShouldReset);

}

void FRuntimeMeshProxy::UpdateSectionMesh_RenderThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_UpdateSection_RT);

	check(IsInRenderingThread());
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): UpdateSectionMesh_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());

	LODs[LODIndex]->UpdateSectionMesh_RenderThread(SectionId, *MeshData);
}

void FRuntimeMeshProxy::ClearAllSections_RenderThread(int32 LODIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_ClearAllSections_RT);

	check(IsInRenderingThread());
	check(LODIndex >= 0 && LODIndex < RUNTIMEMESH_MAXLODS);
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ClearAllSections_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());

	LODs[LODIndex]->ClearAllSections_RenderThread();
}

void FRuntimeMeshProxy::ClearSection_RenderThread(int32 LODIndex, int32 SectionId)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_ClearSection_RT);

	check(IsInRenderingThread());
	check(LODIndex >= 0 && LODIndex < RUNTIMEMESH_MAXLODS);
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): ClearSection_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());

	LODs[LODIndex]->ClearSection_RenderThread(SectionId);
}

void FRuntimeMeshProxy::RemoveAllSections_RenderThread(int32 LODIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_RemoveAllSections_RT);

	check(IsInRenderingThread());
	check(LODIndex >= 0 && LODIndex < RUNTIMEMESH_MAXLODS);
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): RemoveAllSections_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());

	LODs[LODIndex]->RemoveAllSections_RenderThread();
}

void FRuntimeMeshProxy::RemoveSection_RenderThread(int32 LODIndex, int32 SectionId)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_RemoveSection_RT);

	check(IsInRenderingThread());
	check(LODIndex >= 0 && LODIndex < RUNTIMEMESH_MAXLODS);
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMP(%d): RemoveSection_RenderThread Called"), FPlatformTLS::GetCurrentThreadId());

	LODs[LODIndex]->RemoveSection_RenderThread(SectionId);
}


