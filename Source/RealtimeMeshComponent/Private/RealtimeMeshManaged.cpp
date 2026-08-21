// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshManaged.h"

#include "RealtimeMeshComponent.h"
#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshBuilder.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Async/Async.h"
#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "RealtimeMeshManaged"

using namespace RealtimeMesh;

namespace RealtimeMesh
{
	// -------- FRealtimeMeshSectionManaged --------

	FRealtimeMeshSectionManaged::FRealtimeMeshSectionManaged(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshSectionKey& InKey)
		: FRealtimeMeshSection(InContext, InKey)
		, bShouldCreateMeshCollision(false)
	{
	}

	void FRealtimeMeshSectionManaged::SetShouldCreateCollision(FRealtimeMeshUpdateContext& UpdateContext, bool bNewShouldCreateMeshCollision)
	{
		if (bShouldCreateMeshCollision != bNewShouldCreateMeshCollision)
		{
			bShouldCreateMeshCollision = bNewShouldCreateMeshCollision;
			MarkCollisionDirty(UpdateContext);
		}
	}

	bool FRealtimeMeshSectionManaged::Serialize(FArchive& Ar)
	{
		const bool bResult = FRealtimeMeshSection::Serialize(Ar);
		Ar << bShouldCreateMeshCollision;
		return bResult;
	}

	void FRealtimeMeshSectionManaged::Reset(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMeshSection::Reset(UpdateContext);
		bShouldCreateMeshCollision = false;
		MarkCollisionDirty(UpdateContext);
	}

	void FRealtimeMeshSectionManaged::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMeshSection::FinalizeUpdate(UpdateContext);

		auto& State = UpdateContext.GetState();
		bool bStreamsUpdated = State.StreamDirtyTree.HasDirtyStreams(Key);
		bool bCollisionStreamsUpdated = false;
		if (bStreamsUpdated)
		{
			const auto& StreamsUpdated = State.StreamDirtyTree.GetDirtyStreams(Key);
			bStreamsUpdated = StreamsUpdated.Contains(FRealtimeMeshStreams::Position) || StreamsUpdated.Contains(FRealtimeMeshStreams::Triangles);
			// Collision extraction also consumes TexCoords (UV-from-hit-results support), so a
			// UV-only edit must still re-cook collision even though it cannot affect bounds.
			bCollisionStreamsUpdated = bStreamsUpdated || StreamsUpdated.Contains(FRealtimeMeshStreams::TexCoords);
		}

		const bool bStreamRangeUpdated = State.StreamRangeDirtyTree.IsDirty(Key);

		// Collision re-cook is driven from the actual data that feeds it — the position/triangle/
		// texcoord stream content or the section's stream range — rather than unconditionally on
		// every UpdateStreamRange. Only collision-enabled sections flag the group dirty, so
		// collision-free meshes updated per frame no longer re-cook, while genuine geometry changes
		// (including same-range content edits) still do.
		if (bShouldCreateMeshCollision && (bCollisionStreamsUpdated || bStreamRangeUpdated))
		{
			MarkCollisionDirty(UpdateContext);
		}

		if (HasOverrideBounds(UpdateContext))
		{
			return;
		}

		if (!State.BoundsDirtyTree.IsDirty(Key) && !bStreamsUpdated && !bStreamRangeUpdated)
		{
			return;
		}

		TOptional<FBoxSphereBounds3f> LocalBounds;

		if (const auto SectionGroup = GetSectionGroupAs<FRealtimeMeshBufferSetManaged>(UpdateContext))
		{
			const FRealtimeMeshStream* Stream = SectionGroup->GetStream(UpdateContext, FRealtimeMeshStreams::Position);
			const FRealtimeMeshStreamRange SectionStreamRange = GetStreamRange(UpdateContext);
			if (Stream && SectionStreamRange.NumVertices() > 0 && SectionStreamRange.GetMaxVertex() < Stream->Num())
			{
				if (Stream->GetLayout() == GetRealtimeMeshBufferLayout<FVector3f>())
				{
					const FVector3f* Points = Stream->GetData<FVector3f>() + SectionStreamRange.GetMinVertex();
					LocalBounds = FBoxSphereBounds3f(Points, SectionStreamRange.NumVertices());
				}
			}
		}

		UpdateCalculatedBounds(UpdateContext, LocalBounds);
		State.BoundsDirtyTree.Flag(Key.SectionGroup());
	}

	void FRealtimeMeshSectionManaged::MarkCollisionDirty(FRealtimeMeshUpdateContext& UpdateContext) const
	{
		UpdateContext.GetState<FRealtimeMeshManagedUpdateState>().CollisionGroupDirtySet.Flag(GetKey(UpdateContext).SectionGroup());
	}


	// -------- FRealtimeMeshBufferSetManaged --------

	FRealtimeMeshStreamRange FRealtimeMeshBufferSetManaged::GetValidStreamRange(const FRealtimeMeshLockContext& LockContext) const
	{
		FRealtimeMeshStreamRange StreamRange;

		if (const FRealtimeMeshStream* Stream = Streams.Find(FRealtimeMeshStreams::Position))
		{
			StreamRange.Vertices = FInt32Range(0, Stream->Num());
		}

		if (const FRealtimeMeshStream* Stream = Streams.Find(FRealtimeMeshStreams::Triangles))
		{
			StreamRange.Indices = FInt32Range(0, Stream->Num() * Stream->GetNumElements());
		}

		return StreamRange;
	}

	const FRealtimeMeshStream* FRealtimeMeshBufferSetManaged::GetStream(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshStreamKey StreamKey) const
	{
		return Streams.Find(StreamKey);
	}

	void FRealtimeMeshBufferSetManaged::CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream)
	{
		Streams.AddStream(Stream);
		FRealtimeMeshBufferSet::CreateOrUpdateStream(UpdateContext, MoveTemp(Stream));
	}

	void FRealtimeMeshBufferSetManaged::FastUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream, const FInt32Range& ElementRange)
	{
		Streams.AddStream(Stream);
		FRealtimeMeshBufferSet::FastUpdateStream(UpdateContext, MoveTemp(Stream), ElementRange);
	}

	void FRealtimeMeshBufferSetManaged::RemoveStream(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamKey& StreamKey)
	{
		if (Streams.Remove(StreamKey) == 0)
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RemoveStreamInvalid", "Attempted to remove invalid stream {0} in Mesh:{1}"),
				              FText::FromString(StreamKey.ToString()), FText::FromName(Context->GetMeshName())));
		}
		FRealtimeMeshBufferSet::RemoveStream(UpdateContext, StreamKey);
	}

	void FRealtimeMeshBufferSetManaged::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Streams.ForEach([&](const FRealtimeMeshStream& Stream)
		{
			if (Context->WantsStreamOnGPU(Stream.GetStreamKey()) && Stream.Num() > 0)
			{
				if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
				{
					FRealtimeMeshStream Copy(Stream);
					const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(Copy), GetGPUBufferUsageFlags());
					UpdateData->CreateBufferAsyncIfPossible(UpdateContext);

					ProxyBuilder->AddSectionGroupTask(Key, [UpdateData](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& Proxy)
					{
						Proxy.CreateOrUpdateStream(RHICmdList, UpdateData);
					}, ShouldRecreateProxyOnChange(UpdateContext));
				}
			}
		});

		FRealtimeMeshBufferSet::InitializeProxy(UpdateContext);
	}

	void FRealtimeMeshBufferSetManaged::Reset(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Streams.Empty();
		FRealtimeMeshBufferSet::Reset(UpdateContext);
	}

	bool FRealtimeMeshBufferSetManaged::Serialize(FArchive& Ar)
	{
		const bool bResult = FRealtimeMeshBufferSet::Serialize(Ar);
		if (ensure(bResult))
		{
			Ar << Streams;
		}

		// The base FRealtimeMeshBufferSet keeps a TSet<FRealtimeMeshStreamKey> key
		// registry (FRealtimeMeshBufferSet::Streams) that CPU read-back APIs
		// (HasStreams/GetStreamKeys) consult. During normal runtime the managed
		// CreateOrUpdateStream keeps it in sync by delegating to
		// FRealtimeMeshBufferSet::CreateOrUpdateStream, which does
		// `Streams.FindOrAdd(StreamKey)`. The raw-Serialize load path above only
		// repopulates the managed payload set (this->Streams) and never touches the
		// base registry, leaving it empty until a re-commit / proxy init rebuilds it.
		// Replay the runtime registration here so read-back works immediately after
		// load: register each loaded payload stream's key into the base set exactly
		// as CreateOrUpdateStream would. This runs only on load and is idempotent —
		// FindOrAdd on a TSet is a no-op for keys already present.
		if (Ar.IsLoading())
		{
			for (const FRealtimeMeshStreamKey& StreamKey : Streams.GetStreamKeys())
			{
				FRealtimeMeshBufferSet::Streams.FindOrAdd(StreamKey);
			}
		}

		return bResult;
	}

	bool FRealtimeMeshBufferSetManaged::GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshCollisionMesh& CollisionMesh) const
	{
		bool bHasMeshData = false;
		for (const FRealtimeMeshSectionRef& Section : Sections)
		{
			const auto ManagedSection = StaticCastSharedRef<FRealtimeMeshSectionManaged>(Section);
			if (ManagedSection->HasCollision(LockContext))
			{
				URealtimeMeshCollisionTools::AppendStreamsToCollisionMesh(CollisionMesh, Streams, ManagedSection->GetConfig(LockContext).MaterialSlot,
					ManagedSection->GetStreamRange(LockContext).GetMinIndex() / REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE,
					ManagedSection->GetStreamRange(LockContext).NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE));
				bHasMeshData = true;
			}
		}
		return bHasMeshData;
	}


	// -------- FRealtimeMeshLODManaged --------

	bool FRealtimeMeshLODManaged::GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshComplexGeometry& ComplexGeometry) const
	{
		bool bHasSectionData = false;
		for (const auto& SectionGroup : SectionGroups)
		{
			FRealtimeMeshCollisionMesh NewMesh;
			const bool bHadData = StaticCastSharedRef<FRealtimeMeshBufferSetManaged>(SectionGroup)->GenerateComplexCollision(LockContext, NewMesh);
			if (bHadData)
			{
				ComplexGeometry.Add(MoveTemp(NewMesh));
				bHasSectionData = true;
			}
		}
		return bHasSectionData;
	}


	// -------- FRealtimeMeshManaged --------

	FRealtimeMeshSectionRef FRealtimeMeshManaged::CreateSection(const FRealtimeMeshSectionKey& InKey) const
	{
		return MakeShared<FRealtimeMeshSectionManaged>(Context, InKey);
	}

	FRealtimeMeshSectionGroupRef FRealtimeMeshManaged::CreateSectionGroup(const FRealtimeMeshBufferSetKey& InKey) const
	{
		return MakeShared<FRealtimeMeshBufferSetManaged>(Context, InKey);
	}

	FRealtimeMeshLODRef FRealtimeMeshManaged::CreateLOD(const FRealtimeMeshLODKey& InKey) const
	{
		return MakeShared<FRealtimeMeshLODManaged>(Context, InKey);
	}

	FRealtimeMeshUpdateStateRef FRealtimeMeshManaged::CreateUpdateState() const
	{
		return MakeShared<FRealtimeMeshManagedUpdateState>();
	}



	FRealtimeMeshManaged::~FRealtimeMeshManaged()
	{
		if (PendingCollisionPromise)
		{
			// The final reference to this mesh can be dropped on any thread, but chained
			// BP/collision completion delegates expect to run on the game thread. Move the
			// promise out and fulfil it there. DoOnGameThread runs the callable in place when
			// already on the GT, otherwise schedules it via AsyncTask(GameThread).
			DoOnGameThread([Promise = MoveTemp(PendingCollisionPromise)]() mutable
			{
				Promise->SetValue(ERealtimeMeshCollisionUpdateResult::Ignored);
			});
		}
	}

	FRealtimeMeshCollisionConfiguration FRealtimeMeshManaged::GetCollisionConfig() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
		return CollisionConfig;
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshManaged::SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		CollisionConfig = InCollisionConfig;
		return MarkCollisionDirty();
	}

	FRealtimeMeshSimpleGeometry FRealtimeMeshManaged::GetSimpleGeometry() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
		return SimpleGeometry;
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshManaged::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		SimpleGeometry = InSimpleGeometry;
		return MarkCollisionDirty();
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshManaged::ClearCustomComplexMeshGeometry()
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		if (ComplexGeometry.NumMeshes() > 0)
		{
			ComplexGeometry.Reset();
			return MarkCollisionDirty();
		}

		return MakeFulfilledPromise<ERealtimeMeshCollisionUpdateResult>(ERealtimeMeshCollisionUpdateResult::Ignored).GetFuture();
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshManaged::SetCustomComplexMeshGeometry(FRealtimeMeshComplexGeometry&& InComplexMeshGeometry)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		ComplexGeometry = MoveTemp(InComplexMeshGeometry);
		return MarkCollisionDirty();
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshManaged::SetCustomComplexMeshGeometry(const FRealtimeMeshComplexGeometry& InComplexMeshGeometry)
	{
		FRealtimeMeshComplexGeometry ComplexMeshGeometryCopy(InComplexMeshGeometry);
		return SetCustomComplexMeshGeometry(MoveTemp(ComplexMeshGeometryCopy));
	}

	void FRealtimeMeshManaged::ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshComplexGeometry&)> ProcessFunc) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
		ProcessFunc(ComplexGeometry);
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshManaged::EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshComplexGeometry&)> EditFunc)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		EditFunc(ComplexGeometry);
		return MarkCollisionDirty();
	}

	bool FRealtimeMeshManaged::HasNaniteResources(const FRealtimeMeshLockContext& LockContext) const
	{
		return NaniteResources && NaniteResources->HasValidData();
	}

	void FRealtimeMeshManaged::SetNaniteResources(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshNaniteResourcesPtr&& InNaniteResources)
	{
		// Nanite resource-sharing: initialize the incoming instance ONCE here (game thread) and keep it as
		// the single live registration for this mesh, rather than storing a never-initialized "master" and
		// deep-cloning + re-registering it for every consumer. Replacing the member drops this mesh's
		// reference to the previous instance; any still-live proxy versions / scene proxies keep it alive
		// until their last reference drops, at which point the custom deleter releases the old registration
		// (same copy-on-write lifetime as the distance-field / card snapshots).
		if (InNaniteResources.IsValid() && InNaniteResources->HasValidData())
		{
			FGCScopeGuard GCGuard;
			InNaniteResources->InitResources(Context->GetOwningMesh());
			NaniteResources = MakeShareableNaniteResources(MoveTemp(InNaniteResources));
		}
		else
		{
			NaniteResources.Reset();
		}

		MarkBoundsDirtyIfNotOverridden(UpdateContext);

		if (NaniteResources.IsValid() && NaniteResources->HasValidData())
		{
			// Hand the shared live instance to the proxy by refcount bump (no clone, already initialized).
			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->SetHasNaniteData(true);
				ProxyBuilder->AddMeshTask([Resources = NaniteResources](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
				{
					Proxy.SetNaniteResources_RT(Resources);
				}, true);
			}
		}
		else
		{
			FRealtimeMesh::ClearNaniteResources(UpdateContext);
		}
	}

	void FRealtimeMeshManaged::ClearNaniteResources(FRealtimeMeshUpdateContext& UpdateContext)
	{
		NaniteResources.Reset();
		MarkBoundsDirtyIfNotOverridden(UpdateContext);
		FRealtimeMesh::ClearNaniteResources(UpdateContext);
	}

	void FRealtimeMeshManaged::ProcessDistanceField(TFunctionRef<void(const FRealtimeMeshDistanceField&)> ProcessFunc) const
	{
		// API-L8: invoke the callback while the read guard is held so the reference the
		// caller sees cannot be swapped out from under it (fixes the former TOCTOU where
		// GetDistanceField() returned a reference after releasing its read guard).
		FRealtimeMeshScopeGuardRead ScopeGuard(Context->GetGuard());
		ProcessFunc(*DistanceField);
	}

	void FRealtimeMeshManaged::SetDistanceField(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshDistanceField&& InDistanceField)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		// Copy-on-write: allocate a fresh immutable snapshot rather than mutating the live
		// one (an in-flight proxy-recreate task may still be holding the previous snapshot).
		DistanceField = MakeShared<const FRealtimeMeshDistanceField>(InDistanceField);
		return FRealtimeMesh::SetDistanceField(UpdateContext, MoveTemp(InDistanceField));
	}

	void FRealtimeMeshManaged::ClearDistanceField(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		// Copy-on-write: swap in a fresh empty snapshot instead of clearing in place.
		DistanceField = MakeShared<const FRealtimeMeshDistanceField>();
		return FRealtimeMesh::ClearDistanceField(UpdateContext);
	}

	const FRealtimeMeshCardRepresentation* FRealtimeMeshManaged::GetCardRepresentation(const FRealtimeMeshLockContext& LockContext) const
	{
		return CardRepresentation.Get();
	}

	void FRealtimeMeshManaged::SetCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		// Copy-on-write: allocate a fresh immutable snapshot rather than mutating the live one.
		CardRepresentation = MakeShared<const FRealtimeMeshCardRepresentation>(InCardRepresentation);
		return FRealtimeMesh::SetCardRepresentation(UpdateContext, MoveTemp(InCardRepresentation));
	}

	void FRealtimeMeshManaged::ClearCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		CardRepresentation.Reset();
		FRealtimeMesh::ClearCardRepresentation(UpdateContext);
	}

	bool FRealtimeMeshManaged::GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshComplexGeometry& OutComplexGeometry) const
	{
		OutComplexGeometry = ComplexGeometry;

		// TODO: Allow other LOD to be used for collision?
		if (LODs.IsValidIndex(0))
		{
			return StaticCastSharedRef<FRealtimeMeshLODManaged>(LODs[0])->GenerateComplexCollision(LockContext, OutComplexGeometry);
		}
		return false;
	}

	void FRealtimeMeshManaged::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext) const
	{
		FRealtimeMesh::InitializeProxy(UpdateContext);
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			// API-L8: capture the shared immutable snapshots by refcount bump instead of
			// deep-copying the DF / card payload on every proxy recreate.
			ProxyBuilder->AddMeshTask([DistanceFieldSnapshot = DistanceField](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
			{
				Proxy.SetDistanceField(DistanceFieldSnapshot);
			});

			if (CardRepresentation)
			{
				ProxyBuilder->AddMeshTask([CardRepresentationSnapshot = CardRepresentation.ToSharedRef()](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
				{
					Proxy.SetCardRepresentation(CardRepresentationSnapshot);
				});
			}

			if (NaniteResources.IsValid() && NaniteResources->HasValidData())
			{
				// Nanite resource-sharing: hand the already-initialized shared instance to the new proxy
				// version by refcount bump — no clone, no re-registration, so streamed-in detail pages are
				// retained across a proxy recreate (they used to be dropped and had to re-stream).
				ProxyBuilder->SetHasNaniteData(true);
				ProxyBuilder->AddMeshTask([Resources = NaniteResources](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
				{
					Proxy.SetNaniteResources_RT(Resources);
				}, true);
			}
		}
	}

	void FRealtimeMeshManaged::ResetInternal(FRealtimeMeshUpdateContext& UpdateContext, bool bRemoveRenderProxy)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());
		CollisionConfig = FRealtimeMeshCollisionConfiguration();
		SimpleGeometry = FRealtimeMeshSimpleGeometry();

		FRealtimeMesh::ResetInternal(UpdateContext, bRemoveRenderProxy);

		InitializeLODs(UpdateContext, {FRealtimeMeshLODConfig()});
	}

	void FRealtimeMeshManaged::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMesh::FinalizeUpdate(UpdateContext);

		if (UpdateContext.GetState<FRealtimeMeshManagedUpdateState>().CollisionGroupDirtySet.HasAnyDirty())
		{
			MarkCollisionDirtyNoCallback();
		}

		if (UpdateContext.GetState().bNeedsBoundsUpdate && !Bounds.HasUserSetBounds())
		{
			TOptional<FBoxSphereBounds3f> NewBounds;

			if (NaniteResources && NaniteResources->HasValidData())
			{
				NewBounds = FBoxSphereBounds3f(NaniteResources->GetBounds());
			}

			if (NewBounds && Bounds.HasComputedBounds())
			{
				NewBounds = *NewBounds + Bounds.GetComputedBounds();
			}

			if (NewBounds)
			{
				Bounds.SetComputedBounds(*NewBounds);
			}
		}
	}

	bool FRealtimeMeshManaged::Serialize(FArchive& Ar, URealtimeMesh* Owner)
	{
		const bool bResult = FRealtimeMesh::Serialize(Ar, Owner);

		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::SimpleMeshStoresCollisionConfig)
		{
			Ar << CollisionConfig;
			Ar << SimpleGeometry;
		}

		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::SimpleMeshStoresCustomComplexCollision)
		{
			if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::CollisionOverhaul)
			{
				Ar << ComplexGeometry;
			}
			else
			{
				check(Ar.IsLoading())
				FRealtimeMeshStreamSet ComplexMeshGeometry;
				int32 NumStreams = ComplexMeshGeometry.Num();
				Ar << NumStreams;

				ComplexMeshGeometry.Empty();
				for (int32 Index = 0; Index < NumStreams; Index++)
				{
					FRealtimeMeshStreamKey StreamKey;
					Ar << StreamKey;
					FRealtimeMeshStream Stream;
					Ar << Stream;
					Stream.SetStreamKey(StreamKey);

					ComplexMeshGeometry.AddStream(MoveTemp(Stream));
				}

				FRealtimeMeshCollisionMesh CollisionMesh;
				URealtimeMeshCollisionTools::AppendStreamsToCollisionMesh(CollisionMesh, ComplexMeshGeometry, 0);
				ComplexGeometry.Add(MoveTemp(CollisionMesh));
			}
		}

		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::DistanceFieldAndCardRepresentationSupport)
		{
			// API-L8: DistanceField / CardRepresentation are now immutable shared snapshots.
			// FRealtimeMeshDistanceField::Serialize is non-const and (on save) mutates the
			// FByteBulkData, so we serialize through a temp: on load we build a fresh snapshot
			// and swap it in; on save/count we serialize a copy so a live snapshot (possibly
			// pinned by an in-flight proxy-recreate task) is never mutated. Bytes are identical.
			if (Ar.IsLoading())
			{
				FRealtimeMeshDistanceField DistanceFieldTemp;
				DistanceFieldTemp.Serialize(Ar, Owner);
				DistanceField = MakeShared<const FRealtimeMeshDistanceField>(MoveTemp(DistanceFieldTemp));
			}
			else
			{
				FRealtimeMeshDistanceField DistanceFieldTemp(*DistanceField);
				DistanceFieldTemp.Serialize(Ar, Owner);
			}

			bool bHasCardRepresentation = CardRepresentation.IsValid();
			Ar << bHasCardRepresentation;
			if (bHasCardRepresentation)
			{
				if (Ar.IsLoading())
				{
					FRealtimeMeshCardRepresentation CardRepresentationTemp;
					CardRepresentationTemp.Serialize(Ar, Owner);
					CardRepresentation = MakeShared<const FRealtimeMeshCardRepresentation>(MoveTemp(CardRepresentationTemp));
				}
				else
				{
					FRealtimeMeshCardRepresentation CardRepresentationTemp(*CardRepresentation);
					CardRepresentationTemp.Serialize(Ar, Owner);
				}
			}
			else
			{
				CardRepresentation.Reset();
			}
		}

		// NOTE: Do NOT trigger a collision rebuild here on load. During archive
		// load RenderProxy is never valid (it is created lazily), so the old
		// `Ar.IsLoading() && RenderProxy` gate was dead code. The load-time
		// collision rebuild is now kicked off from URealtimeMeshManaged::PostLoad(),
		// which mirrors PostDuplicate. Adding a trigger here as well would
		// double-mark collision dirty on load.

		return bResult;
	}

	void FRealtimeMeshManaged::MarkCollisionDirtyNoCallback() const
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context);
		MarkForEndOfFrameUpdate();

		if (!PendingCollisionPromise.IsValid())
		{
			PendingCollisionPromise = MakeShared<TPromise<ERealtimeMeshCollisionUpdateResult>>();
		}
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshManaged::MarkCollisionDirty() const
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context);
		MarkForEndOfFrameUpdate();

		if (!PendingCollisionPromise.IsValid())
		{
			PendingCollisionPromise = MakeShared<TPromise<ERealtimeMeshCollisionUpdateResult>>();
			return PendingCollisionPromise->GetFuture();
		}

		const auto NewPendingPromise = MakeShared<TPromise<ERealtimeMeshCollisionUpdateResult>>();
		auto NewReturnPromise = MakeShared<TPromise<ERealtimeMeshCollisionUpdateResult>>();

		NewPendingPromise->GetFuture().Next([PendingPromise = PendingCollisionPromise, ReturnPromise = NewReturnPromise](ERealtimeMeshCollisionUpdateResult Result)
		{
			PendingPromise->SetValue(Result);
			ReturnPromise->SetValue(Result);
		});

		PendingCollisionPromise = NewPendingPromise;
		return NewReturnPromise->GetFuture();
	}

	void FRealtimeMeshManaged::ProcessEndOfFrameUpdates()
	{
		TSharedPtr<TPromise<ERealtimeMeshCollisionUpdateResult>> CollisionPromise;
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(Context);
			CollisionPromise = MoveTemp(PendingCollisionPromise);
			PendingCollisionPromise.Reset();
		}

		if (CollisionPromise.IsValid())
		{
			auto Promise = MoveTemp(*CollisionPromise);

			const int32 UpdateKey = GetNextCollisionUpdateVersion();

			auto CollisionData = MakeShared<FRealtimeMeshCollisionInfo>();

			// Read CollisionConfig/SimpleGeometry under the guard: the setters take the
			// write guard from any thread, so reading them unguarded here would be a torn read.
			bool bAsyncCook;
			{
				FRealtimeMeshScopeGuardRead ScopeGuard(Context);
				bAsyncCook = CollisionConfig.bUseAsyncCook;
				CollisionData->Configuration = CollisionConfig;
				CollisionData->SimpleGeometry = SimpleGeometry;
			}

			const ERealtimeMeshThreadType AllowedGenerationThread = bAsyncCook ? ERealtimeMeshThreadType::AsyncThread : ERealtimeMeshThreadType::GameThread;

			auto ThisWeak = StaticCastWeakPtr<FRealtimeMeshManaged>(this->AsWeak());

			DoOnAllowedThread(AllowedGenerationThread, [ThisWeak, CollisionData, ResultPromise = MoveTemp(Promise), UpdateKey]() mutable
			{
				if (const auto ThisShared = ThisWeak.Pin())
				{
					// Scope the access context (and thus the mesh read lock) to just the geometry
					// extraction. UpdateCollision runs the synchronous Chaos cook on the independent
					// copied CollisionData below, so it must not hold the mesh lock — otherwise a
					// long cook blocks the next game-thread write for its whole duration.
					{
						FRealtimeMeshAccessContext AccessContext(ThisShared.ToSharedRef());
						FRealtimeMeshComplexGeometry NewComplexGeometry;

						if (ThisShared->GenerateComplexCollision(AccessContext, NewComplexGeometry))
						{
							CollisionData->ComplexGeometry = MoveTemp(NewComplexGeometry);
						}
					}

					auto CollisionUpdateFuture = ThisShared->UpdateCollision(MoveTemp(*CollisionData), UpdateKey);

					ContinueOnGameThread(MoveTemp(CollisionUpdateFuture), [ResultPromise = MoveTemp(ResultPromise)](TFuture<ERealtimeMeshCollisionUpdateResult>&& Result) mutable
					{
						ResultPromise.EmplaceValue(Result.Get());
					});
				}
				else
				{
					DoOnGameThread([ResultPromise = MoveTemp(ResultPromise)]() mutable
					{
						ResultPromise.EmplaceValue(ERealtimeMeshCollisionUpdateResult::Ignored);
					});
				}
			});
		}
		FRealtimeMesh::ProcessEndOfFrameUpdates();
	}
}


namespace
{
	// DUP-011: shared BP-callback continuation. Every UObject-facing wrapper below tails its
	// TFuture result into the bound dynamic delegate with the same
	// .Next([Cb](Status){ (void)Cb.ExecuteIfBound(Status); }). Collapse that tail onto one
	// helper templated over the status + delegate type. Thread/continuation semantics are
	// unchanged — .Next stays .Next and the delegate is still captured by value.
	template <typename TStatus, typename TDelegate>
	void ExecuteCallbackOnCompletion(TFuture<TStatus>&& Future, const TDelegate& Callback)
	{
		Future.Next([Callback](TStatus Status)
		{
			(void)Callback.ExecuteIfBound(Status);
		});
	}
}


URealtimeMeshManaged::URealtimeMeshManaged(const FObjectInitializer& ObjectInitializer)
	: URealtimeMesh(ObjectInitializer)
	, bShouldSerializeMeshData(true)
{
	// Abstract — concrete leaves call Initialize() with their own shared resources.
}

void URealtimeMeshManaged::Serialize(FArchive& Ar)
{
	// Bypass URealtimeMesh::Serialize and write the byte layout that older Managed
	// asset versions used (UObject bytes → custom-ver-gated bShouldSerializeMeshData
	// → optional mesh data). Calling Super here would also write the mesh data
	// unconditionally and we'd end up writing it twice.
	UObject::Serialize(Ar);

	if (!IsTemplate())
	{
		Ar.UsingCustomVersion(RealtimeMesh::FRealtimeMeshVersion::GUID);

		bool bShouldSerializeData = bShouldSerializeMeshData;
		if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::SupportOptionalDataSerialization)
		{
			Ar << bShouldSerializeData;
			if (Ar.IsLoading())
			{
				bShouldSerializeMeshData = bShouldSerializeData;
			}
		}
		else
		{
			bShouldSerializeData = true;
		}

		if (bShouldSerializeData)
		{
			GetMesh()->Serialize(Ar, this);
		}
	}
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::CreateSection(const FRealtimeMeshSectionKey& SectionKey,
	const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetManaged>(SectionKey,
		[SectionKey, Config, StreamRange](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetManaged& SectionGroup)
	{
		SectionGroup.CreateOrUpdateSection(UpdateContext, SectionKey, Config, StreamRange);
	});

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
		[bShouldCreateCollision](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionManaged& Section)
	{
		Section.SetShouldCreateCollision(UpdateContext, bShouldCreateCollision);
	});

	return UpdateBuilder.Commit(GetManagedMeshData());
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
	[Config, bShouldCreateCollision](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionManaged& Section)
	{
		Section.UpdateConfig(UpdateContext, Config);
		Section.SetShouldCreateCollision(UpdateContext, bShouldCreateCollision);
	});

	return UpdateBuilder.Commit(GetManagedMeshData());
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::UpdateSectionRange(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
	[StreamRange](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionManaged& Section)
	{
		Section.UpdateStreamRange(UpdateContext, StreamRange);
	});

	return UpdateBuilder.Commit(GetManagedMeshData());
}

TArray<FRealtimeMeshBufferSetKey> URealtimeMeshManaged::GetBufferSets(const FRealtimeMeshLODKey& LODKey) const
{
	FRealtimeMeshAccessor Accessor;

	TArray<FRealtimeMeshBufferSetKey> SectionGroups;

	Accessor.AddLODTask<FRealtimeMeshLODManaged>(LODKey,
	[&SectionGroups](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshLODManaged& LOD)
	{
		SectionGroups = LOD.GetSectionGroupKeys(LockContext).Array();
	});

	Accessor.Execute(GetManagedMeshData());

	return SectionGroups;
}

TArray<FRealtimeMeshLODKey> URealtimeMeshManaged::GetLODs() const
{
	FRealtimeMeshAccessor Accessor;

	TArray<FRealtimeMeshLODKey> LODKeys;

	Accessor.AddMeshTask([&LODKeys](const FRealtimeMeshLockContext& LockContext, const FRealtimeMesh& Mesh)
	{
		const int32 NumLODs = Mesh.GetNumLODs(LockContext);
		LODKeys.Reserve(NumLODs);

		for (int32 LODIndex = 0; LODIndex < NumLODs; LODIndex++)
		{
			LODKeys.Add(FRealtimeMeshLODKey(LODIndex));
		}
	});

	Accessor.Execute(GetManagedMeshData());

	return LODKeys;
}

TArray<FRealtimeMeshSectionKey> URealtimeMeshManaged::GetSectionsInBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey)
{
	FRealtimeMeshAccessor Accessor;

	TArray<FRealtimeMeshSectionKey> Sections;

	Accessor.AddSectionGroupTask<FRealtimeMeshBufferSetManaged>(BufferSetKey,
	[&Sections](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetManaged& SectionGroup)
	{
		Sections = SectionGroup.GetSectionKeys(LockContext).Array();
	});

	Accessor.Execute(GetManagedMeshData());

	return Sections;
}

void URealtimeMeshManaged::CreateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange,
	bool bShouldCreateCollision, const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	ExecuteCallbackOnCompletion(CreateSection(SectionKey, Config, StreamRange, bShouldCreateCollision), CompletionCallback);
}

void URealtimeMeshManaged::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision,
	const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	ExecuteCallbackOnCompletion(UpdateSectionConfig(SectionKey, Config, bShouldCreateCollision), CompletionCallback);
}

void URealtimeMeshManaged::RemoveSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	ExecuteCallbackOnCompletion(RemoveSection(SectionKey), CompletionCallback);
}

void URealtimeMeshManaged::RemoveBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	ExecuteCallbackOnCompletion(RemoveBufferSet(BufferSetKey), CompletionCallback);
}

TArray<FRealtimeMeshBufferSetKey> URealtimeMeshManaged::GetSectionGroups(const FRealtimeMeshLODKey& LODKey) const
{
	return GetBufferSets(LODKey);
}

TArray<FRealtimeMeshSectionKey> URealtimeMeshManaged::GetSectionsInGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey)
{
	return GetSectionsInBufferSet(SectionGroupKey);
}

void URealtimeMeshManaged::RemoveSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshManagedCompletionCallback& OnComplete)
{
	RemoveBufferSet(SectionGroupKey, OnComplete);
}

FRealtimeMeshSectionConfig URealtimeMeshManaged::GetSectionConfig(const FRealtimeMeshSectionKey& SectionKey) const
{
	FRealtimeMeshSectionConfig Config;

	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
	[&Config](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionManaged& Section)
	{
		Config = Section.GetConfig(LockContext);
	});
	Accessor.Execute(GetManagedMeshData());

	return Config;
}

bool URealtimeMeshManaged::IsSectionVisible(const FRealtimeMeshSectionKey& SectionKey) const
{
	bool bIsVisible = false;

	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
	[&bIsVisible](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionManaged& Section)
	{
		bIsVisible = Section.IsVisible(LockContext);
	});
	Accessor.Execute(GetManagedMeshData());

	return bIsVisible;
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
	[bIsVisible](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionManaged& Section)
	{
		Section.SetVisibility(UpdateContext, bIsVisible);
	});

	return UpdateBuilder.Commit(GetManagedMeshData());
}

void URealtimeMeshManaged::SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible, const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	ExecuteCallbackOnCompletion(SetSectionVisibility(SectionKey, bIsVisible), CompletionCallback);
}

bool URealtimeMeshManaged::IsSectionCastingShadow(const FRealtimeMeshSectionKey& SectionKey) const
{
	bool bIsCastingShadow = false;

	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
	[&bIsCastingShadow](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionManaged& Section)
	{
		bIsCastingShadow = Section.IsCastingShadow(LockContext);
	});
	Accessor.Execute(GetManagedMeshData());

	return bIsCastingShadow;
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
	[bCastShadow](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionManaged& Section)
	{
		Section.SetCastShadow(UpdateContext, bCastShadow);
	});

	return UpdateBuilder.Commit(GetManagedMeshData());
}

void URealtimeMeshManaged::SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow, const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	ExecuteCallbackOnCompletion(SetSectionCastShadow(SectionKey, bCastShadow), CompletionCallback);
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetManaged>(SectionKey,
	[SectionKey](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetManaged& SectionGroup)
	{
		SectionGroup.RemoveSection(UpdateContext, SectionKey);
	});

	return UpdateBuilder.Commit(GetManagedMeshData());
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::RemoveBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddLODTask<FRealtimeMeshLODManaged>(BufferSetKey,
	[BufferSetKey](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODManaged& LOD)
	{
		LOD.RemoveSectionGroup(UpdateContext, BufferSetKey);
	});

	return UpdateBuilder.Commit(GetManagedMeshData());
}

void URealtimeMeshManaged::ProcessDistanceField(TFunctionRef<void(const FRealtimeMeshDistanceField&)> ProcessFunc) const
{
	return GetManagedMeshData()->ProcessDistanceField(ProcessFunc);
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField)
{
	FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetManagedMeshData()->SetDistanceField(UpdateContext, MoveTemp(InDistanceField));
	return UpdateContext.Commit();
}

void URealtimeMeshManaged::SetDistanceField(const FRealtimeMeshDistanceField& InDistanceField, const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	FRealtimeMeshDistanceField Copy(InDistanceField);
	ExecuteCallbackOnCompletion(SetDistanceField(MoveTemp(Copy)), CompletionCallback);
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::ClearDistanceField()
{
	FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetManagedMeshData()->ClearDistanceField(UpdateContext);
	return UpdateContext.Commit();
}

const FRealtimeMeshCardRepresentation* URealtimeMeshManaged::GetCardRepresentation(const FRealtimeMeshLockContext& LockContext) const
{
	return GetManagedMeshData()->GetCardRepresentation(LockContext);
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation)
{
	FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetManagedMeshData()->SetCardRepresentation(UpdateContext, MoveTemp(InCardRepresentation));
	return UpdateContext.Commit();
}

void URealtimeMeshManaged::SetCardRepresentation(const FRealtimeMeshCardRepresentation& InCardRepresentation, const FRealtimeMeshManagedCompletionCallback& CompletionCallback)
{
	FRealtimeMeshCardRepresentation Copy(InCardRepresentation);
	ExecuteCallbackOnCompletion(SetCardRepresentation(MoveTemp(Copy)), CompletionCallback);
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshManaged::ClearCardRepresentation()
{
	FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetManagedMeshData()->ClearCardRepresentation(UpdateContext);
	return UpdateContext.Commit();
}

bool URealtimeMeshManaged::HasCustomComplexMeshGeometry() const
{
	return GetManagedMeshData()->HasCustomComplexMeshGeometry();
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshManaged::ClearCustomComplexMeshGeometry()
{
	return GetManagedMeshData()->ClearCustomComplexMeshGeometry();
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshManaged::SetCustomComplexMeshGeometry(FRealtimeMeshComplexGeometry&& InComplexMeshGeometry)
{
	return GetManagedMeshData()->SetCustomComplexMeshGeometry(MoveTemp(InComplexMeshGeometry));
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshManaged::SetCustomComplexMeshGeometry(const FRealtimeMeshComplexGeometry& InComplexMeshGeometry)
{
	return GetManagedMeshData()->SetCustomComplexMeshGeometry(InComplexMeshGeometry);
}

void URealtimeMeshManaged::ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshComplexGeometry&)> ProcessFunc) const
{
	return GetManagedMeshData()->ProcessCustomComplexMeshGeometry(ProcessFunc);
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshManaged::EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshComplexGeometry&)> EditFunc)
{
	return GetManagedMeshData()->EditCustomComplexMeshGeometry(EditFunc);
}

FRealtimeMeshCollisionConfiguration URealtimeMeshManaged::GetCollisionConfig() const
{
	return GetManagedMeshData()->GetCollisionConfig();
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshManaged::SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig)
{
	return GetManagedMeshData()->SetCollisionConfig(InCollisionConfig);
}

void URealtimeMeshManaged::SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig, const FRealtimeMeshManagedCollisionCompletionCallback& CompletionCallback)
{
	ExecuteCallbackOnCompletion(SetCollisionConfig(InCollisionConfig), CompletionCallback);
}

FRealtimeMeshSimpleGeometry URealtimeMeshManaged::GetSimpleGeometry() const
{
	return GetManagedMeshData()->GetSimpleGeometry();
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshManaged::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry)
{
	return GetManagedMeshData()->SetSimpleGeometry(InSimpleGeometry);
}

void URealtimeMeshManaged::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry, const FRealtimeMeshManagedCollisionCompletionCallback& CompletionCallback)
{
	ExecuteCallbackOnCompletion(SetSimpleGeometry(InSimpleGeometry), CompletionCallback);
}

void URealtimeMeshManaged::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!IsTemplate())
	{
		StaticCastSharedPtr<FRealtimeMeshManaged>(MeshRef)->MarkCollisionDirtyNoCallback();
	}
}

void URealtimeMeshManaged::PostLoad()
{
	Super::PostLoad();

	// BodySetup is Transient, so after load OnRegister -> UpdateCollision runs with an
	// unbuilt BodySetup and a saved mesh has no collision until the first runtime edit.
	// Mark collision dirty here (mirroring PostDuplicate) so the end-of-frame update
	// cooks the serialized CollisionConfig/SimpleGeometry/complex geometry once on load.
	if (!IsTemplate())
	{
		GetManagedMeshData()->MarkCollisionDirtyNoCallback();
	}
}

#undef LOCTEXT_NAMESPACE
