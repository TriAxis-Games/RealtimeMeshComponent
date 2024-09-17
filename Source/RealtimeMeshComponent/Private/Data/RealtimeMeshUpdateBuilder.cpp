// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RealtimeMeshComponentModule.h"

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshUpdateBuilder::Commit(const TSharedRef<FRealtimeMesh>& Mesh)
	{
		FRealtimeMeshUpdateContext UpdateContext(Mesh);

		for (auto& Task : Tasks)
		{
			Task(UpdateContext, *Mesh);
		}

		return UpdateContext.Commit();		
	}

	void FRealtimeMeshUpdateBuilder::AddMeshTask(TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMesh&)>&& Function)
	{
		Tasks.Add(MoveTemp(Function));
	}

	void FRealtimeMeshUpdateBuilder::AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMeshLOD&)>&& Function)
	{
		AddMeshTask([LODKey, Func = MoveTemp(Function)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMesh& Mesh)
		{
			const FRealtimeMeshLODPtr LOD = Mesh.GetLOD(LODKey);

			if (ensure(LOD.IsValid()))
			{
				Func(ProxyBuilder, *LOD.Get());
			}
			else
			{
				UE_LOG(RealtimeMeshLog, Error, TEXT("Failed to find LOD %s"), *LODKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshUpdate_LODTask", "RealtimeMeshUpdate_LODTask: Failed to find LOD {0}"),
					FText::FromString(LODKey.ToString())));				
			}
		});
	}

	void FRealtimeMeshUpdateBuilder::AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMeshSectionGroup&)>&& Function)
	{
		AddLODTask(SectionGroupKey.LOD(), [SectionGroupKey, Func = MoveTemp(Function)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshLOD& LOD)
		{
			const FRealtimeMeshSectionGroupPtr SectionGroup = LOD.GetSectionGroup(SectionGroupKey);

			if (ensure(SectionGroup.IsValid()))
			{
				Func(ProxyBuilder, *SectionGroup.Get());
			}
			else
			{
				UE_LOG(RealtimeMeshLog, Error, TEXT("Failed to find SectionGroup %s"), *SectionGroupKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshUpdate_SectionGroupTask", "RealtimeMeshUpdate_SectionGroupTask: Failed to find SectionGroup {0}"),
					FText::FromString(SectionGroupKey.ToString())));				
			}
		});
	}

	void FRealtimeMeshUpdateBuilder::AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMeshSection&)>&& Function)
	{
		AddSectionGroupTask(SectionKey.SectionGroup(), [SectionKey, Func = MoveTemp(Function)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroup& SectionGroup)
		{
			const FRealtimeMeshSectionPtr Section = SectionGroup.GetSection(SectionKey);

			if (ensure(Section.IsValid()))
			{
				Func(ProxyBuilder, *Section.Get());
			}
			else
			{
				UE_LOG(RealtimeMeshLog, Error, TEXT("Failed to find Section %s"), *SectionKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshUpdate_Section", "RealtimeMeshUpdate_Section: Failed to find Section {0}"),
					FText::FromString(SectionKey.ToString())));	
			}
		});
	}









	void FRealtimeMeshAccessor::Execute(const TSharedRef<const FRealtimeMesh>& Mesh)
	{
		for (auto& Task : Tasks)
		{
			Task(*Mesh);
		}	
	}

	void FRealtimeMeshAccessor::AddMeshTask(TUniqueFunction<void(const FRealtimeMesh&)>&& Function)
	{
		Tasks.Add(MoveTemp(Function));
	}

	void FRealtimeMeshAccessor::AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(const FRealtimeMeshLOD&)>&& Function)
	{
		AddMeshTask([LODKey, Func = MoveTemp(Function)](const FRealtimeMesh& Mesh)
		{
			const FRealtimeMeshLODPtr LOD = Mesh.GetLOD(LODKey);

			if (ensure(LOD.IsValid()))
			{
				Func(*LOD.Get());
			}
			else
			{
				UE_LOG(RealtimeMeshLog, Error, TEXT("Failed to find LOD %s"), *LODKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshAccessor_LODTask", "RealtimeMeshAccessor_LODTask: Failed to find LOD {0}"),
					FText::FromString(LODKey.ToString())));				
			}
		});
	}

	void FRealtimeMeshAccessor::AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(const FRealtimeMeshSectionGroup&)>&& Function)
	{
		AddLODTask(SectionGroupKey.LOD(), [SectionGroupKey, Func = MoveTemp(Function)](const FRealtimeMeshLOD& LOD)
		{
			const FRealtimeMeshSectionGroupPtr SectionGroup = LOD.GetSectionGroup(SectionGroupKey);

			if (ensure(SectionGroup.IsValid()))
			{
				Func(*SectionGroup.Get());
			}
			else
			{
				UE_LOG(RealtimeMeshLog, Error, TEXT("Failed to find SectionGroup %s"), *SectionGroupKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshAccessor_SectionGroupTask", "RealtimeMeshAccessor_SectionGroupTask: Failed to find SectionGroup {0}"),
					FText::FromString(SectionGroupKey.ToString())));				
			}
		});
	}

	void FRealtimeMeshAccessor::AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(const FRealtimeMeshSection&)>&& Function)
	{
		AddSectionGroupTask(SectionKey.SectionGroup(), [SectionKey, Func = MoveTemp(Function)](const FRealtimeMeshSectionGroup& SectionGroup)
		{
			const FRealtimeMeshSectionPtr Section = SectionGroup.GetSection(SectionKey);

			if (ensure(Section.IsValid()))
			{
				Func(*Section.Get());
			}
			else
			{
				UE_LOG(RealtimeMeshLog, Error, TEXT("Failed to find Section %s"), *SectionKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshAccessor_Section", "RealtimeMeshAccessor_Section: Failed to find Section {0}"),
					FText::FromString(SectionKey.ToString())));	
			}
		});
	}
}
