// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshSimple.h"

#include "RealtimeMeshComponent.h"
#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshBuilder.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Async/Async.h"
#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "Mesh/RealtimeMeshPolyGroupUtils.h"
#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "RealtimeMeshSimple"

using namespace RealtimeMesh;

namespace RealtimeMesh
{
	// -------- FRealtimeMeshBufferSetSimple --------

	void FRealtimeMeshBufferSetSimple::SetPolyGroupSectionHandler(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshPolyGroupConfigHandler& NewHandler)
	{
		if (NewHandler.IsBound())
		{
			ConfigHandler = NewHandler;
		}
		else
		{
			ClearPolyGroupSectionHandler(UpdateContext);
		}
	}

	void FRealtimeMeshBufferSetSimple::ClearPolyGroupSectionHandler(FRealtimeMeshUpdateContext& UpdateContext)
	{
		ConfigHandler = FRealtimeMeshPolyGroupConfigHandler::CreateSP(this, &FRealtimeMeshBufferSetSimple::DefaultPolyGroupSectionHandler);
	}

	void FRealtimeMeshBufferSetSimple::ProcessMeshData(const FRealtimeMeshLockContext& LockContext, TFunctionRef<void(const FRealtimeMeshStreamSet&)> ProcessFunc) const
	{
		ProcessFunc(Streams);
	}

	void FRealtimeMeshBufferSetSimple::EditMeshData(FRealtimeMeshUpdateContext& UpdateContext, TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshStreamSet&)> EditFunc)
	{
		auto UpdatedStreams = EditFunc(Streams);

		for (const auto& UpdatedStream : UpdatedStreams)
		{
			if (const auto* Stream = Streams.Find(UpdatedStream))
			{
				FRealtimeMeshStream StreamCopy(*Stream);
				// EditMeshData is an in-place edit: the CPU stream storage is already updated
				// above by EditFunc, so route through the fast attribute-update path. Eligible
				// vertex streams on a Dynamic buffer set become in-place GPU updates; anything
				// else (topology, count change, Static buffer set) falls back to a full update.
				FRealtimeMeshBufferSet::FastUpdateStream(UpdateContext, MoveTemp(StreamCopy), FInt32Range::Empty());
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(
					FText::Format(LOCTEXT("EditMeshData_InvalidStream", "Unable to update stream {0} in mesh {1}"),
								  FText::FromString(UpdatedStream.ToString()), FText::FromName(Context->GetMeshName())));
			}
		}
	}

	void FRealtimeMeshBufferSetSimple::EditMeshDataRanged(FRealtimeMeshUpdateContext& UpdateContext, TFunctionRef<TMap<FRealtimeMeshStreamKey, FInt32Range>(FRealtimeMeshStreamSet&)> EditFunc)
	{
		const TMap<FRealtimeMeshStreamKey, FInt32Range> UpdatedStreams = EditFunc(Streams);

		for (const TPair<FRealtimeMeshStreamKey, FInt32Range>& Updated : UpdatedStreams)
		{
			if (const auto* Stream = Streams.Find(Updated.Key))
			{
				// The in-place upload path (CanUpdateStreamInPlace/UpdateInPlace) requires a
				// FULL-size stream: eligibility checks NumElements == GPUBuffer.Num(), and the
				// copy reads/writes at absolute byte offset ElementOffset*Stride into both the
				// source and the destination buffer. A range-sliced (smaller) copy would fail
				// eligibility and force the slow reallocating fallback, so we still need a full
				// copy here (Streams is persistent CPU storage and can't be moved out of).
				FRealtimeMeshStream StreamCopy(*Stream);
				FRealtimeMeshBufferSet::FastUpdateStream(UpdateContext, MoveTemp(StreamCopy), Updated.Value);
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(
					FText::Format(LOCTEXT("EditMeshData_InvalidStream", "Unable to update stream {0} in mesh {1}"),
								  FText::FromString(Updated.Key.ToString()), FText::FromName(Context->GetMeshName())));
			}
		}
	}

	void FRealtimeMeshBufferSetSimple::CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream)
	{
		const FRealtimeMeshStreamKey IncomingKey = Stream.GetStreamKey();
		FRealtimeMeshBufferSetManaged::CreateOrUpdateStream(UpdateContext, MoveTemp(Stream));

		if (bAutoCreateSectionsForPolygonGroups && !bDeferPolyGroupUpdates)
		{
			const bool bShouldCreateSingularSection = ShouldCreateSingularSection();

			if ((bShouldCreateSingularSection && IncomingKey == FRealtimeMeshStreams::Triangles) ||
				(!bShouldCreateSingularSection && (IncomingKey == FRealtimeMeshStreams::Triangles ||
					IncomingKey == FRealtimeMeshStreams::PolyGroups)))
			{
				UpdatePolyGroupSections(UpdateContext, false);
			}
			else if ((bShouldCreateSingularSection && IncomingKey == FRealtimeMeshStreams::DepthOnlyTriangles) ||
				(!bShouldCreateSingularSection && (IncomingKey == FRealtimeMeshStreams::DepthOnlyPolyGroups ||
					IncomingKey == FRealtimeMeshStreams::PolyGroups)))
			{
				UpdatePolyGroupSections(UpdateContext, true);
			}
		}
	}

	void FRealtimeMeshBufferSetSimple::SetAllStreams(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStreamSet&& InStreams)
	{
		bool bWantsPolyGroupUpdate = false;
		bool bWantsDepthOnlyPolyGroupUpdate = false;
		if (bAutoCreateSectionsForPolygonGroups)
		{
			if (InStreams.Contains(FRealtimeMeshStreams::PolyGroups) ||
				InStreams.Contains(FRealtimeMeshStreams::PolyGroupSegments) ||
				InStreams.Contains(FRealtimeMeshStreams::Triangles))
			{
				bWantsPolyGroupUpdate = true;
			}
			if (InStreams.Contains(FRealtimeMeshStreams::DepthOnlyPolyGroups) ||
				InStreams.Contains(FRealtimeMeshStreams::DepthOnlyPolyGroupSegments) ||
				InStreams.Contains(FRealtimeMeshStreams::DepthOnlyTriangles))
			{
				bWantsDepthOnlyPolyGroupUpdate = true;
			}
		}

		{
			TGuardValue<bool> Guard(bDeferPolyGroupUpdates, true);
			FRealtimeMeshBufferSetManaged::SetAllStreams(UpdateContext, MoveTemp(InStreams));
		}

		if (bWantsPolyGroupUpdate)
		{
			UpdatePolyGroupSections(UpdateContext, false);
		}
		else if (bWantsDepthOnlyPolyGroupUpdate)
		{
			UpdatePolyGroupSections(UpdateContext, true);
		}
	}

	const FRealtimeMeshSectionKey& FRealtimeMeshBufferSetSimple::GetPolyGroupSectionKey(int32 PolyGroupIndex) const
	{
		if (const FRealtimeMeshSectionKey* Existing = PolyGroupSectionKeyCache.Find(PolyGroupIndex))
		{
			return *Existing;
		}
		return PolyGroupSectionKeyCache.Add(PolyGroupIndex, FRealtimeMeshSectionKey::CreateForPolyGroup(Key, PolyGroupIndex));
	}

	void FRealtimeMeshBufferSetSimple::UpdatePolyGroupSections(FRealtimeMeshUpdateContext& UpdateContext, bool bUpdateDepthOnly)
	{
		if (ShouldCreateSingularSection())
		{
			const FRealtimeMeshSectionKey PolyGroupKey = GetPolyGroupSectionKey(0);

			if (const auto Section = GetSectionAs<FRealtimeMeshSectionManaged>(UpdateContext, PolyGroupKey))
			{
				Section->UpdateStreamRange(UpdateContext, GetValidStreamRange(UpdateContext));
			}
			else
			{
				const FRealtimeMeshSectionConfig SectionConfig = ConfigHandler.IsBound()
					? ConfigHandler.Execute(0)
					: FRealtimeMeshSectionConfig(0);

				CreateOrUpdateSection(UpdateContext, PolyGroupKey, SectionConfig, GetValidStreamRange(UpdateContext));
			}
		}
		else
		{
			auto Result = bUpdateDepthOnly
				? RealtimeMeshAlgo::GetStreamRangesFromPolyGroupsDepthOnly(Streams)
				: RealtimeMeshAlgo::GetStreamRangesFromPolyGroups(Streams);

			if (Result)
			{
				for (const auto Range : Result.GetValue())
				{
					const FRealtimeMeshSectionKey PolyGroupKey = GetPolyGroupSectionKey(Range.Key);

					if (const auto Section = GetSectionAs<FRealtimeMeshSectionManaged>(UpdateContext, PolyGroupKey))
					{
						Section->UpdateStreamRange(UpdateContext, Range.Value);
					}
					else
					{
						const FRealtimeMeshSectionConfig SectionConfig = ConfigHandler.IsBound()
							? ConfigHandler.Execute(Range.Key)
							: FRealtimeMeshSectionConfig(Range.Key);
						CreateOrUpdateSection(UpdateContext, PolyGroupKey, SectionConfig, Range.Value);
					}
				}

				for (FRealtimeMeshSectionKey SectionKey : GetSectionKeys(UpdateContext))
				{
					check(Sections.Contains(SectionKey));
					const auto& Section = *Sections.Find(SectionKey);
					if (Section->GetKey(UpdateContext).IsPolyGroupKey() && (Section->GetStreamRange(UpdateContext).Vertices.IsEmpty() || Section->GetStreamRange(UpdateContext).Indices.IsEmpty()))
					{
						RemoveSection(UpdateContext, SectionKey);
					}
				}
			}
		}
	}

	FRealtimeMeshSectionConfig FRealtimeMeshBufferSetSimple::DefaultPolyGroupSectionHandler(int32 PolyGroupIndex) const
	{
		return FRealtimeMeshSectionConfig(PolyGroupIndex);
	}

	bool FRealtimeMeshBufferSetSimple::ShouldCreateSingularSection() const
	{
		return !Streams.Contains(FRealtimeMeshStreams::PolyGroups) && !Streams.Contains(FRealtimeMeshStreams::DepthOnlyPolyGroups) &&
			(Sections.Num() == 0 || (Sections.Num() == 1 && Sections.Contains(GetPolyGroupSectionKey(0))));
	}


	// -------- FRealtimeMeshSimple --------

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey,
		const FRealtimeMeshBufferSetConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
	{
		FRealtimeMeshUpdateBuilder UpdateBuilder;

		UpdateBuilder.AddLODTask<FRealtimeMeshLODManaged>(SectionGroupKey, [SectionGroupKey, InConfig](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODManaged& LOD)
		{
			LOD.CreateOrUpdateSectionGroup(UpdateContext, SectionGroupKey, InConfig);
		});

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
			[bShouldAutoCreateSectionsForPolyGroups](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetSimple& SectionGroup)
		{
			SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(UpdateContext, bShouldAutoCreateSectionsForPolyGroups);
		});

		return UpdateBuilder.Commit(this->AsShared());
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData,
		const FRealtimeMeshBufferSetConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
	{
		FRealtimeMeshUpdateBuilder UpdateBuilder;

		UpdateBuilder.AddLODTask<FRealtimeMeshLODManaged>(SectionGroupKey, [SectionGroupKey, InConfig](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODManaged& LOD)
		{
			LOD.CreateOrUpdateSectionGroup(UpdateContext, SectionGroupKey, InConfig);
		});

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
			[bShouldAutoCreateSectionsForPolyGroups, MeshData = MoveTemp(MeshData)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetSimple& SectionGroup) mutable
		{
			SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(UpdateContext, bShouldAutoCreateSectionsForPolyGroups);
			SectionGroup.SetAllStreams(UpdateContext, MoveTemp(MeshData));
		});

		return UpdateBuilder.Commit(this->AsShared());
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData,
		const FRealtimeMeshBufferSetConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
	{
		FRealtimeMeshStreamSet MeshDataCopy(MeshData);
		return CreateSectionGroup(SectionGroupKey, MoveTemp(MeshDataCopy), InConfig, bShouldAutoCreateSectionsForPolyGroups);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData)
	{
		FRealtimeMeshUpdateBuilder UpdateBuilder;

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
			[MeshData = MoveTemp(MeshData)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetSimple& SectionGroup) mutable
		{
			SectionGroup.SetAllStreams(UpdateContext, MoveTemp(MeshData));
		});

		return UpdateBuilder.Commit(this->AsShared());
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData)
	{
		FRealtimeMeshStreamSet Copy(MeshData);
		return UpdateSectionGroup(SectionGroupKey, MoveTemp(Copy));
	}
}


// -------- URealtimeMeshSimple --------

URealtimeMeshSimple::URealtimeMeshSimple(const FObjectInitializer& ObjectInitializer)
	: URealtimeMeshManaged(ObjectInitializer)
{
	if (!IsTemplate())
	{
		const auto SR = MakeShared<RealtimeMesh::FRealtimeMeshContext>();
		Initialize(SR, MakeShared<RealtimeMesh::FRealtimeMeshSimple>(SR));

		FRealtimeMeshUpdateContext UpdateContext(GetMeshData());
		MeshRef->InitializeLODs(UpdateContext, RealtimeMesh::TFixedLODArray<FRealtimeMeshLODConfig>{FRealtimeMeshLODConfig()});
	}
}

URealtimeMeshSimple* URealtimeMeshSimple::InitializeRealtimeMeshSimple(URealtimeMeshComponent* Owner)
{
	if (IsValid(Owner))
	{
		return Owner->InitializeRealtimeMesh<URealtimeMeshSimple>();
	}
	return nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey,
	const FRealtimeMeshBufferSetConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
{
	return GetMeshAs<FRealtimeMeshSimple>()->CreateSectionGroup(SectionGroupKey, InConfig, bShouldAutoCreateSectionsForPolyGroups);
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData,
	const FRealtimeMeshBufferSetConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
{
	return GetMeshAs<FRealtimeMeshSimple>()->CreateSectionGroup(SectionGroupKey, MoveTemp(MeshData), InConfig, bShouldAutoCreateSectionsForPolyGroups);
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData,
	const FRealtimeMeshBufferSetConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
{
	return GetMeshAs<FRealtimeMeshSimple>()->CreateSectionGroup(SectionGroupKey, MeshData, InConfig, bShouldAutoCreateSectionsForPolyGroups);
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData)
{
	return GetMeshAs<FRealtimeMeshSimple>()->UpdateSectionGroup(SectionGroupKey, MoveTemp(MeshData));
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData)
{
	return GetMeshAs<FRealtimeMeshSimple>()->UpdateSectionGroup(SectionGroupKey, MeshData);
}

TSharedPtr<FRealtimeMeshBufferSetSimple> URealtimeMeshSimple::GetBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey) const
{
	// WARNING: The returned shared pointer is extracted while the accessor read-lock is held, but the
	// lock is released when this function returns. Dereferencing or mutating the buffer set after return
	// races with concurrent mesh updates. Callers must only touch the result on the game thread and treat
	// it as short-lived. For a lock-safe alternative, prefer ProcessSectionGroup() (below), which invokes
	// your callback with the buffer set while the accessor lock is still held; also see
	// ProcessMesh()/EditMeshInPlace() which keep work inside the lock scope.
	// (API-L14: return type is public API; a weak-ptr change would break existing callers — left as-is.)
	FRealtimeMeshAccessor Accessor;

	TSharedPtr<FRealtimeMeshBufferSetSimple> FoundSectionGroup;

	Accessor.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
	[&FoundSectionGroup](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetSimple& SectionGroup)
	{
		FoundSectionGroup = StaticCastSharedRef<FRealtimeMeshBufferSetSimple, FRealtimeMeshBufferSet>(const_cast<FRealtimeMeshBufferSetSimple&>(SectionGroup).AsShared());
	});

	Accessor.Execute(GetMeshData());

	return FoundSectionGroup;
}

void URealtimeMeshSimple::ProcessBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey, TFunctionRef<void(const FRealtimeMeshBufferSetSimple&)> ProcessFunc) const
{
	// Lock-safe counterpart to GetSectionGroup(): ProcessFunc is invoked with the buffer set while the
	// accessor read-lock/lock-context is still held, so the reference passed to it never escapes the lock
	// scope. If the requested section group does not exist, the accessor task simply never runs and
	// ProcessFunc is not called.
	FRealtimeMeshAccessor Accessor;

	Accessor.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
	[&ProcessFunc](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetSimple& SectionGroup)
	{
		ProcessFunc(SectionGroup);
	});

	Accessor.Execute(GetMeshData());
}

void URealtimeMeshSimple::ProcessMesh(const FRealtimeMeshBufferSetKey& SectionGroupKey, const TFunctionRef<void(const FRealtimeMeshStreamSet&)>& ProcessFunc) const
{
	FRealtimeMeshAccessor Accessor;

	Accessor.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
	[&ProcessFunc](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetSimple& SectionGroup)
	{
		SectionGroup.ProcessMeshData(LockContext, ProcessFunc);
	});

	Accessor.Execute(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::EditMeshInPlace(const FRealtimeMeshBufferSetKey& SectionGroupKey, const TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshStreamSet&)>& EditFunc)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
	[&EditFunc](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetSimple& SectionGroup)
	{
		SectionGroup.EditMeshData(UpdateContext, EditFunc);
	});

	return UpdateBuilder.Commit(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::EditMeshInPlaceRanged(const FRealtimeMeshBufferSetKey& SectionGroupKey, const TFunctionRef<TMap<FRealtimeMeshStreamKey, FInt32Range>(FRealtimeMeshStreamSet&)>& EditFunc)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
	[&EditFunc](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetSimple& SectionGroup)
	{
		SectionGroup.EditMeshDataRanged(UpdateContext, EditFunc);
	});

	return UpdateBuilder.Commit(GetMeshData());
}

void URealtimeMeshSimple::CreateBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshManagedCompletionCallback& OnComplete)
{
	if (!IsValid(MeshData))
	{
		(void)OnComplete.ExecuteIfBound(ERealtimeMeshProxyUpdateStatus::NoUpdate);
		return;
	}
	// MeshData is guaranteed valid past the early-out, so no null/empty-key fallback branch is needed.
	TFuture<ERealtimeMeshProxyUpdateStatus> Continuation = CreateBufferSet(SectionGroupKey, MeshData->GetStreamSet());
	Continuation.Next([OnComplete](ERealtimeMeshProxyUpdateStatus Status)
	{
		ensure(IsInGameThread());
		(void)OnComplete.ExecuteIfBound(Status);
	});
}

FRealtimeMeshBufferSetKey URealtimeMeshSimple::CreateBufferSetUnique(const FRealtimeMeshLODKey& LODKey, URealtimeMeshStreamSet* MeshData,
                                                                           const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	const FRealtimeMeshBufferSetKey SectionGroupKey = FRealtimeMeshBufferSetKey::CreateUnique(LODKey);
	CreateSectionGroup(SectionGroupKey, MeshData, CompletionCallback);
	return SectionGroupKey;
}

void URealtimeMeshSimple::UpdateBufferSet(const FRealtimeMeshBufferSetKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData,
                                             const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	// NOTE (IDIOM-009): the null-MeshData handling here (fall through to an
	// FEditorScriptExecutionGuard-wrapped callback) intentionally differs from
	// CreateSectionGroup's early-out above. Normalizing either side would change
	// observable BP behavior (guard/log differences), so the divergence is
	// intentional-as-shipped.
	if (MeshData)
	{
		UpdateBufferSet(SectionGroupKey, MeshData->GetStreamSet()).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
	}
	else
	{
		FEditorScriptExecutionGuard ScriptGuard;
		if (CompletionCallback.IsBound())
		{
			(void)CompletionCallback.ExecuteIfBound(ERealtimeMeshProxyUpdateStatus::NoUpdate);
		}
	}
}

void URealtimeMeshSimple::SetShouldAutoCreateSectionsForPolyGroups(const FRealtimeMeshBufferSetKey& SectionGroupKey, bool bNewValue)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
	[&bNewValue](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetSimple& SectionGroup)
	{
		SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(UpdateContext, bNewValue);
	});

	UpdateBuilder.Commit(GetMeshData());
}

bool URealtimeMeshSimple::ShouldAutoCreateSectionsForPolygonGroups(const FRealtimeMeshBufferSetKey& SectionGroupKey) const
{
	bool bShouldAutoCreateSections = true;

	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionGroupTask<FRealtimeMeshBufferSetSimple>(SectionGroupKey,
	[&bShouldAutoCreateSections](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetSimple& SectionGroup)
	{
		bShouldAutoCreateSections = SectionGroup.ShouldAutoCreateSectionsForPolygonGroups(LockContext);
	});
	Accessor.Execute(GetMeshData());

	return bShouldAutoCreateSections;
}


#undef LOCTEXT_NAMESPACE

// --- Deprecated SectionGroup-terminology UFUNCTION shims (see the *BufferSet forms) ---

void URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshManagedCompletionCallback& OnComplete)
{
	CreateBufferSet(SectionGroupKey, MeshData, OnComplete);
}

FRealtimeMeshBufferSetKey URealtimeMeshSimple::CreateSectionGroupUnique(const FRealtimeMeshLODKey& LODKey, URealtimeMeshStreamSet* MeshData,
	const FRealtimeMeshManagedCompletionCallback& OnComplete)
{
	return CreateBufferSetUnique(LODKey, MeshData, OnComplete);
}

void URealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData,
	const FRealtimeMeshManagedCompletionCallback& OnComplete)
{
	UpdateBufferSet(SectionGroupKey, MeshData, OnComplete);
}
