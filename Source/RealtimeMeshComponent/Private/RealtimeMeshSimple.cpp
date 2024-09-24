// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshSimple.h"

#include "RealtimeMeshComponent.h"
#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshBuilder.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Async/Async.h"
#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "Mesh/RealtimeMeshAlgo.h"
#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Logging/MessageLog.h"
#endif

#define LOCTEXT_NAMESPACE "RealtimeMeshSimple"


namespace RealtimeMesh
{
	namespace Simple::Private
	{
		static thread_local bool bShouldDeferPolyGroupUpdates = false;		
	}	
	
	FRealtimeMeshSectionSimple::FRealtimeMeshSectionSimple(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey& InKey)
		: FRealtimeMeshSection(InSharedResources, InKey)
		  , bShouldCreateMeshCollision(false)
	{
	}

	FRealtimeMeshSectionSimple::~FRealtimeMeshSectionSimple()
	{
	}

	void FRealtimeMeshSectionSimple::SetShouldCreateCollision(FRealtimeMeshUpdateContext& UpdateContext, bool bNewShouldCreateMeshCollision)
	{
		if (bShouldCreateMeshCollision != bNewShouldCreateMeshCollision)
		{
			bShouldCreateMeshCollision = bNewShouldCreateMeshCollision;
			MarkCollisionDirty(UpdateContext);
		}
	}

	void FRealtimeMeshSectionSimple::UpdateStreamRange(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamRange& InRange)
	{
		auto ParentGroup = GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(UpdateContext);
		
		FRealtimeMeshSection::UpdateStreamRange(UpdateContext, InRange);		

		MarkBoundsDirtyIfNotOverridden(UpdateContext);
		MarkCollisionDirty(UpdateContext);
	}

	bool FRealtimeMeshSectionSimple::Serialize(FArchive& Ar)
	{
		const bool bResult = FRealtimeMeshSection::Serialize(Ar);

		Ar << bShouldCreateMeshCollision;

		return bResult;
	}

	void FRealtimeMeshSectionSimple::Reset(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMeshSection::Reset(UpdateContext);
		bShouldCreateMeshCollision = false;
		MarkCollisionDirty(UpdateContext);
	}

	void FRealtimeMeshSectionSimple::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMeshSection::FinalizeUpdate(UpdateContext);

		if (!HasOverrideBounds(UpdateContext))
		{
			auto& State = UpdateContext.GetState();
			bool bStreamsUpdated = State.StreamDirtyTree.HasDirtyStreams(Key);
			if (bStreamsUpdated)
			{
				const auto& StreamsUpdated = State.StreamDirtyTree.GetDirtyStreams(Key);
				bStreamsUpdated &= StreamsUpdated.Contains(FRealtimeMeshStreams::Position) || StreamsUpdated.Contains(FRealtimeMeshStreams::Triangles);
			}
			
			if (State.BoundsDirtyTree.IsDirty(Key) || bStreamsUpdated || State.StreamRangeDirtyTree.IsDirty(Key))
			{
				TOptional<FBoxSphereBounds3f> LocalBounds;

				if (const auto SectionGroup = GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(UpdateContext))
				{
					const auto Stream = SectionGroup->GetStream(UpdateContext, FRealtimeMeshStreams::Position);
					const auto SectionStreamRange = GetStreamRange(UpdateContext);
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
		}
	}

	void FRealtimeMeshSectionSimple::MarkCollisionDirty(FRealtimeMeshUpdateContext& UpdateContext) const
	{
		UpdateContext.GetState<FRealtimeMeshSimpleUpdateState>().CollisionGroupDirtySet.Flag(GetKey(UpdateContext).SectionGroup());		
	}

	FRealtimeMeshStreamRange FRealtimeMeshSectionGroupSimple::GetValidStreamRange(const FRealtimeMeshLockContext& LockContext) const
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

	const FRealtimeMeshStream* FRealtimeMeshSectionGroupSimple::GetStream(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshStreamKey StreamKey) const
	{
		return Streams.Find(StreamKey);
	}

	void FRealtimeMeshSectionGroupSimple::SetPolyGroupSectionHandler(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshPolyGroupConfigHandler& NewHandler)
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

	void FRealtimeMeshSectionGroupSimple::ClearPolyGroupSectionHandler(FRealtimeMeshUpdateContext& UpdateContext)
	{
		ConfigHandler = FRealtimeMeshPolyGroupConfigHandler::CreateSP(this, &FRealtimeMeshSectionGroupSimple::DefaultPolyGroupSectionHandler);
	}

	void FRealtimeMeshSectionGroupSimple::ProcessMeshData(const FRealtimeMeshLockContext& LockContext, TFunctionRef<void(const FRealtimeMeshStreamSet&)> ProcessFunc) const
	{
		ProcessFunc(Streams);
	}

	void FRealtimeMeshSectionGroupSimple::EditMeshData(FRealtimeMeshUpdateContext& UpdateContext, TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshStreamSet&)> EditFunc)
	{
		auto UpdatedStreams = EditFunc(Streams);

		for (const auto& UpdatedStream : UpdatedStreams)
		{
			if (const auto* Stream = Streams.Find(UpdatedStream))
			{
				FRealtimeMeshStream StreamCopy(*Stream);				
				FRealtimeMeshSectionGroup::CreateOrUpdateStream(UpdateContext, MoveTemp(StreamCopy));
			}
			else
			{				
				FMessageLog("RealtimeMesh").Error(
					FText::Format(LOCTEXT("EditMeshData_InvalidStream", "Unable to update stream {0} in mesh {1}"),
								  FText::FromString(UpdatedStream.ToString()), FText::FromName(SharedResources->GetMeshName())));
			}
		}
	}

	void FRealtimeMeshSectionGroupSimple::CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream)
	{
		// Replace the stored stream (We allow this to copy as we then pass the stream to the RT command queue)
		Streams.AddStream(Stream);
		
		// If this stream is a segments stream or polygon group stream lets update the sections
		if (bAutoCreateSectionsForPolygonGroups && !Simple::Private::bShouldDeferPolyGroupUpdates)
		{
			const bool bShouldCreateSingularSection = ShouldCreateSingularSection();
			
			if ((bShouldCreateSingularSection && Stream.GetStreamKey() == FRealtimeMeshStreams::Triangles) ||
				(!bShouldCreateSingularSection && (Stream.GetStreamKey() == FRealtimeMeshStreams::Triangles ||
					Stream.GetStreamKey() == FRealtimeMeshStreams::PolyGroups)))
			{
				UpdatePolyGroupSections(UpdateContext, false);
			}
			else if ((bShouldCreateSingularSection && Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyTriangles) ||
				(!bShouldCreateSingularSection && (Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyPolyGroups ||
					Stream.GetStreamKey() == FRealtimeMeshStreams::PolyGroups)))
			{
				UpdatePolyGroupSections(UpdateContext, true);
			}
		}
		
		FRealtimeMeshSectionGroup::CreateOrUpdateStream(UpdateContext, MoveTemp(Stream));
	}

	void FRealtimeMeshSectionGroupSimple::RemoveStream(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamKey& StreamKey)
	{
		// Replace the stored stream
		if (Streams.Remove(StreamKey) == 0)
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RemoveStreamInvalid", "Attempted to remove invalid stream {0} in Mesh:{1}"),
				              FText::FromString(StreamKey.ToString()), FText::FromName(SharedResources->GetMeshName())));
		}

		FRealtimeMeshSectionGroup::RemoveStream(UpdateContext, StreamKey);
	}

	void FRealtimeMeshSectionGroupSimple::SetAllStreams(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStreamSet&& InStreams)
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
		
		// Block auto update of material indices until all streams are set		
		// Defer updates for bulk changes like this
		Simple::Private::bShouldDeferPolyGroupUpdates = true;
		FRealtimeMeshSectionGroup::SetAllStreams(UpdateContext, MoveTemp(InStreams));
		Simple::Private::bShouldDeferPolyGroupUpdates = false;
		
		if (bWantsPolyGroupUpdate)
		{
			UpdatePolyGroupSections(UpdateContext, false);
		}
		else if (bWantsDepthOnlyPolyGroupUpdate)
		{
			UpdatePolyGroupSections(UpdateContext, true);
		}		
	}

	void FRealtimeMeshSectionGroupSimple::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext)
	{
		// We only send streams here, we rely on the base to send the sections
		Streams.ForEach([&](const FRealtimeMeshStream& Stream)
		{
			if (SharedResources->WantsStreamOnGPU(Stream.GetStreamKey()) && Stream.Num() > 0)
			{
				if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
				{
					FRealtimeMeshStream Copy(Stream);
					const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(Copy), EBufferUsageFlags::Static);
					UpdateData->CreateBufferAsyncIfPossible(UpdateContext);

					ProxyBuilder->AddSectionGroupTask(Key, [UpdateData](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
					{
						Proxy.CreateOrUpdateStream(RHICmdList, UpdateData);
					}, ShouldRecreateProxyOnChange(UpdateContext));
				}
			}
		});

		FRealtimeMeshSectionGroup::InitializeProxy(UpdateContext);
	}

	void FRealtimeMeshSectionGroupSimple::Reset(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Streams.Empty();
		FRealtimeMeshSectionGroup::Reset(UpdateContext);
	}

	bool FRealtimeMeshSectionGroupSimple::Serialize(FArchive& Ar)
	{
		const bool bResult = FRealtimeMeshSectionGroup::Serialize(Ar);

		if (ensure(bResult))
		{
			Ar << Streams;
		}

		return bResult;
	}

	bool FRealtimeMeshSectionGroupSimple::GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshCollisionMesh& CollisionMesh) const
	{
		bool bHasMeshData = false;
		for (const FRealtimeMeshSectionRef& Section : Sections)
		{
			const auto SimpleSection = StaticCastSharedRef<FRealtimeMeshSectionSimple>(Section);
			if (SimpleSection->HasCollision(LockContext))
			{
				URealtimeMeshCollisionTools::AppendStreamsToCollisionMesh(CollisionMesh, Streams, SimpleSection->GetConfig(LockContext).MaterialSlot,
					SimpleSection->GetStreamRange(LockContext).GetMinIndex() / REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE,
					SimpleSection->GetStreamRange(LockContext).NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE));
				bHasMeshData = true;
			}
		}

		return bHasMeshData;
	}

	void FRealtimeMeshSectionGroupSimple::UpdatePolyGroupSections(FRealtimeMeshUpdateContext& UpdateContext, bool bUpdateDepthOnly)
	{
		if (ShouldCreateSingularSection())
		{
			const FRealtimeMeshSectionKey PolyGroupKey = FRealtimeMeshSectionKey::CreateForPolyGroup(Key, 0);
												
			if (const auto Section = GetSectionAs<FRealtimeMeshSectionSimple>(UpdateContext, PolyGroupKey))
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
				// First update all the stream ranges
				for (const auto Range : Result.GetValue())
				{
					const FRealtimeMeshSectionKey PolyGroupKey = FRealtimeMeshSectionKey::CreateForPolyGroup(Key, Range.Key);
			
					if (const auto Section = GetSectionAs<FRealtimeMeshSectionSimple>(UpdateContext, PolyGroupKey))
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
						// Drop this section as it's empty.
						RemoveSection(UpdateContext, SectionKey);
					}
				}
			}
		}
	}

	FRealtimeMeshSectionConfig FRealtimeMeshSectionGroupSimple::DefaultPolyGroupSectionHandler(int32 PolyGroupIndex) const
	{
		return FRealtimeMeshSectionConfig(PolyGroupIndex);
	}

	bool FRealtimeMeshSectionGroupSimple::ShouldCreateSingularSection() const
	{
		return !Streams.Contains(FRealtimeMeshStreams::PolyGroups) && !Streams.Contains(FRealtimeMeshStreams::DepthOnlyPolyGroups) &&
			(Sections.Num() == 0 || (Sections.Num() == 1 && Sections.Contains(FRealtimeMeshSectionKey::CreateForPolyGroup(Key, 0))));
	}

	bool FRealtimeMeshLODSimple::GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshComplexGeometry& ComplexGeometry) const
	{
		bool bHasSectionData = false;
		for (const auto& SectionGroup : SectionGroups)
		{
			FRealtimeMeshCollisionMesh NewMesh;
			const bool bHadData = StaticCastSharedRef<FRealtimeMeshSectionGroupSimple>(SectionGroup)->GenerateComplexCollision(LockContext, NewMesh);
			if (bHadData)
			{
				ComplexGeometry.Add(MoveTemp(NewMesh));
				bHasSectionData = true;
			}
		}
		return bHasSectionData;
	}


	FRealtimeMeshRef FRealtimeMeshSharedResourcesSimple::CreateRealtimeMesh() const
	{
		return MakeShared<FRealtimeMeshSimple>(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()));
	}


	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey,
		const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
	{
		FRealtimeMeshUpdateBuilder UpdateBuilder;

		UpdateBuilder.AddLODTask<FRealtimeMeshLODSimple>(SectionGroupKey, [SectionGroupKey, InConfig](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODSimple& LOD)
		{
			LOD.CreateOrUpdateSectionGroup(UpdateContext, SectionGroupKey, InConfig);
		});

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
			[bShouldAutoCreateSectionsForPolyGroups](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionGroupSimple& SectionGroup)
		{
			SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(UpdateContext, bShouldAutoCreateSectionsForPolyGroups);
		});

		return UpdateBuilder.Commit(this->AsShared());
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData,
		const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
	{
		FRealtimeMeshUpdateBuilder UpdateBuilder;

		UpdateBuilder.AddLODTask<FRealtimeMeshLODSimple>(SectionGroupKey, [SectionGroupKey, InConfig](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODSimple& LOD)
		{
			LOD.CreateOrUpdateSectionGroup(UpdateContext, SectionGroupKey, InConfig);
		});

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
			[bShouldAutoCreateSectionsForPolyGroups, MeshData = MoveTemp(MeshData)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionGroupSimple& SectionGroup) mutable
		{
			SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(UpdateContext, bShouldAutoCreateSectionsForPolyGroups);
			SectionGroup.SetAllStreams(UpdateContext, MoveTemp(MeshData));
		});

		return UpdateBuilder.Commit(this->AsShared());
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData,
		const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
	{
		FRealtimeMeshStreamSet MeshDataCopy(MeshData);
		return CreateSectionGroup(SectionGroupKey, MoveTemp(MeshDataCopy), InConfig, bShouldAutoCreateSectionsForPolyGroups);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData)
	{
		FRealtimeMeshUpdateBuilder UpdateBuilder;

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
			[MeshData = MoveTemp(MeshData)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionGroupSimple& SectionGroup) mutable
		{
			SectionGroup.SetAllStreams(UpdateContext, MoveTemp(MeshData));
		});

		return UpdateBuilder.Commit(this->AsShared());
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData)
	{
		FRealtimeMeshStreamSet Copy(MeshData);
		return UpdateSectionGroup(SectionGroupKey, MoveTemp(Copy));
	}

	FRealtimeMeshCollisionConfiguration FRealtimeMeshSimple::GetCollisionConfig() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return CollisionConfig;
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CollisionConfig = InCollisionConfig;
		return MarkCollisionDirty();
	}

	FRealtimeMeshSimpleGeometry FRealtimeMeshSimple::GetSimpleGeometry() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return SimpleGeometry;
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		SimpleGeometry = InSimpleGeometry;
		return MarkCollisionDirty();
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::ClearCustomComplexMeshGeometry()
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		if (ComplexGeometry.NumMeshes() > 0)
		{
			ComplexGeometry.Reset();
			return MarkCollisionDirty();
		}

		return MakeFulfilledPromise<ERealtimeMeshCollisionUpdateResult>(ERealtimeMeshCollisionUpdateResult::Ignored).GetFuture();
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::SetCustomComplexMeshGeometry(FRealtimeMeshComplexGeometry&& InComplexMeshGeometry)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		ComplexGeometry = MoveTemp(InComplexMeshGeometry);
		return MarkCollisionDirty();		
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::SetCustomComplexMeshGeometry(const FRealtimeMeshComplexGeometry& InComplexMeshGeometry)
	{
		FRealtimeMeshComplexGeometry ComplexMeshGeometryCopy(InComplexMeshGeometry);
		return SetCustomComplexMeshGeometry(MoveTemp(ComplexMeshGeometryCopy));
	}

	void FRealtimeMeshSimple::ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshComplexGeometry&)> ProcessFunc) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		ProcessFunc(ComplexGeometry);
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshComplexGeometry&)> EditFunc)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		EditFunc(ComplexGeometry);
		return MarkCollisionDirty();
	}

	const FRealtimeMeshDistanceField& FRealtimeMeshSimple::GetDistanceField() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return DistanceField;
	}

	void FRealtimeMeshSimple::SetDistanceField(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshDistanceField&& InDistanceField)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		DistanceField = FRealtimeMeshDistanceField(InDistanceField);
		return FRealtimeMesh::SetDistanceField(UpdateContext, MoveTemp(InDistanceField));
	}

	void FRealtimeMeshSimple::ClearDistanceField(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		DistanceField = FRealtimeMeshDistanceField();
		return FRealtimeMesh::ClearDistanceField(UpdateContext);
	}

	const FRealtimeMeshCardRepresentation* FRealtimeMeshSimple::GetCardRepresentation(const FRealtimeMeshLockContext& LockContext) const
	{
		return CardRepresentation.Get();
	}

	void FRealtimeMeshSimple::SetCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CardRepresentation = MakeUnique<FRealtimeMeshCardRepresentation>(InCardRepresentation);
		return FRealtimeMesh::SetCardRepresentation(UpdateContext, MoveTemp(InCardRepresentation));
	}

	void FRealtimeMeshSimple::ClearCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CardRepresentation.Reset();
		FRealtimeMesh::ClearCardRepresentation(UpdateContext);
	}

	bool FRealtimeMeshSimple::GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshComplexGeometry& OutComplexGeometry) const
	{
		// Copy any custom complex geometry
		OutComplexGeometry = ComplexGeometry;
		
		// TODO: Allow other LOD to be used for collision?
		if (LODs.IsValidIndex(0))
		{
			return StaticCastSharedRef<FRealtimeMeshLODSimple>(LODs[0])->GenerateComplexCollision(LockContext, OutComplexGeometry);
		}
		return false;
	}

	void FRealtimeMeshSimple::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext) const
	{
		FRealtimeMesh::InitializeProxy(UpdateContext);
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([DistanceField = DistanceField](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetDistanceField(MoveTemp(DistanceField));
			});

			if (CardRepresentation)
			{
				FRealtimeMeshCardRepresentation CardRepresentationCopy(*CardRepresentation);
				ProxyBuilder->AddMeshTask([CardRepresentation = MoveTemp(CardRepresentationCopy)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
				{
					Proxy.SetCardRepresentation(MoveTemp(CardRepresentation));
				});
			}
		}
	}

	void FRealtimeMeshSimple::Reset(FRealtimeMeshUpdateContext& UpdateContext, bool bRemoveRenderProxy)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CollisionConfig = FRealtimeMeshCollisionConfiguration();
		SimpleGeometry = FRealtimeMeshSimpleGeometry();
		
		FRealtimeMesh::Reset(UpdateContext, bRemoveRenderProxy);

		// Default it back to a single LOD.
		InitializeLODs(UpdateContext, {FRealtimeMeshLODConfig()});
	}

	void FRealtimeMeshSimple::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		FRealtimeMesh::FinalizeUpdate(UpdateContext);
		
		if (UpdateContext.GetState<FRealtimeMeshSimpleUpdateState>().CollisionGroupDirtySet.HasAnyDirty())
		{
			MarkCollisionDirtyNoCallback();
		}
	}

	bool FRealtimeMeshSimple::Serialize(FArchive& Ar, URealtimeMesh* Owner)
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
			DistanceField.Serialize(Ar, Owner);

			bool bHasCardRepresentation = CardRepresentation.IsValid();
			Ar << bHasCardRepresentation;
			if (bHasCardRepresentation)
			{
				if (Ar.IsLoading() && !CardRepresentation.IsValid())
				{
					CardRepresentation = MakeUnique<FRealtimeMeshCardRepresentation>();
				}
				
				CardRepresentation->Serialize(Ar, Owner);				
			}
			else
			{
				CardRepresentation.Reset();
			}
		}

		if (Ar.IsLoading() && RenderProxy)
		{
			MarkCollisionDirtyNoCallback();
		}

		return bResult;
	}
	
	void FRealtimeMeshSimple::MarkCollisionDirtyNoCallback() const
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		MarkForEndOfFrameUpdate();

		if (!PendingCollisionPromise.IsValid())
		{
			PendingCollisionPromise = MakeShared<TPromise<ERealtimeMeshCollisionUpdateResult>>();
		}
	}
	
	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::MarkCollisionDirty() const
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
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

	void FRealtimeMeshSimple::ProcessEndOfFrameUpdates()
	{
		TSharedPtr<TPromise<ERealtimeMeshCollisionUpdateResult>> CollisionPromise;
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
			CollisionPromise = MoveTemp(PendingCollisionPromise);
			PendingCollisionPromise.Reset();
		}
		
		if (CollisionPromise.IsValid())
		{
			auto Promise = MoveTemp(*CollisionPromise);

			const int32 UpdateKey = GetNextCollisionUpdateVersion();
			
			const bool bAsyncCook = CollisionConfig.bUseAsyncCook;
			const ERealtimeMeshThreadType AllowedGenerationThread = bAsyncCook ? ERealtimeMeshThreadType::AsyncThread : ERealtimeMeshThreadType::GameThread;

			auto CollisionData = MakeShared<FRealtimeMeshCollisionInfo>();
			CollisionData->Configuration = CollisionConfig;
			CollisionData->SimpleGeometry = SimpleGeometry;
			
			auto ThisWeak = StaticCastWeakPtr<FRealtimeMeshSimple>(this->AsWeak());

			DoOnAllowedThread(AllowedGenerationThread, [ThisWeak, CollisionData, ResultPromise = MoveTemp(Promise), UpdateKey]() mutable
			{				
				if (const auto ThisShared = ThisWeak.Pin())
				{
					FRealtimeMeshAccessContext AccessContext(ThisShared.ToSharedRef());
					FRealtimeMeshComplexGeometry ComplexGeometry;
					
					if (ThisShared->GenerateComplexCollision(AccessContext, ComplexGeometry))
					{
						CollisionData->ComplexGeometry = MoveTemp(ComplexGeometry);
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


URealtimeMeshSimple::URealtimeMeshSimple(const FObjectInitializer& ObjectInitializer)
	: URealtimeMesh(ObjectInitializer)
{
	if (!IsTemplate())
	{
		Initialize(MakeShared<RealtimeMesh::FRealtimeMeshSharedResourcesSimple>());

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
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey,
	const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
{
	return GetMeshAs<FRealtimeMeshSimple>()->CreateSectionGroup(SectionGroupKey, InConfig, bShouldAutoCreateSectionsForPolyGroups);
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData,
	const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
{
	return GetMeshAs<FRealtimeMeshSimple>()->CreateSectionGroup(SectionGroupKey, MoveTemp(MeshData), InConfig, bShouldAutoCreateSectionsForPolyGroups);
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData,
	const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
{
	return GetMeshAs<FRealtimeMeshSimple>()->CreateSectionGroup(SectionGroupKey, MeshData, InConfig, bShouldAutoCreateSectionsForPolyGroups);
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData)
{
	return GetMeshAs<FRealtimeMeshSimple>()->UpdateSectionGroup(SectionGroupKey, MoveTemp(MeshData));
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData)
{
	return GetMeshAs<FRealtimeMeshSimple>()->UpdateSectionGroup(SectionGroupKey, MeshData);
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSection(const FRealtimeMeshSectionKey& SectionKey,
	const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionKey,
		[SectionKey, Config, StreamRange](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.CreateOrUpdateSection(UpdateContext, SectionKey, Config, StreamRange);
	});

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
		[bShouldCreateCollision](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionSimple& Section)
	{
		Section.SetShouldCreateCollision(UpdateContext, bShouldCreateCollision);			
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[Config, bShouldCreateCollision](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionSimple& Section)
	{
		Section.UpdateConfig(UpdateContext, Config);
		Section.SetShouldCreateCollision(UpdateContext, bShouldCreateCollision);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionRange(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[StreamRange](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionSimple& Section)
	{
		Section.UpdateStreamRange(UpdateContext, StreamRange);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

TArray<FRealtimeMeshSectionGroupKey> URealtimeMeshSimple::GetSectionGroups(const FRealtimeMeshLODKey& LODKey) const
{
	FRealtimeMeshAccessor Accessor;

	TArray<FRealtimeMeshSectionGroupKey> SectionGroups;
	
	Accessor.AddLODTask<FRealtimeMeshLODSimple>(LODKey,
	[&SectionGroups](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshLODSimple& LOD)
	{
		SectionGroups = LOD.GetSectionGroupKeys(LockContext).Array();
	});

	Accessor.Execute(GetMeshData());
	
	return SectionGroups;
}

TSharedPtr<FRealtimeMeshSectionGroupSimple> URealtimeMeshSimple::GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
{
	FRealtimeMeshAccessor Accessor;

	TSharedPtr<FRealtimeMeshSectionGroupSimple> FoundSectionGroup;
	
	Accessor.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&FoundSectionGroup](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		FoundSectionGroup = StaticCastSharedRef<FRealtimeMeshSectionGroupSimple, FRealtimeMeshSectionGroup>(const_cast<FRealtimeMeshSectionGroupSimple&>(SectionGroup).AsShared());
	});

	Accessor.Execute(GetMeshData());
	
	return FoundSectionGroup;
}

TArray<FRealtimeMeshSectionKey> URealtimeMeshSimple::GetSectionsInGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
{	
	FRealtimeMeshAccessor Accessor;

	TArray<FRealtimeMeshSectionKey> Sections;
	
	Accessor.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&Sections](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		Sections = SectionGroup.GetSectionKeys(LockContext).Array();
	});

	Accessor.Execute(GetMeshData());
	
	return Sections;
}


void URealtimeMeshSimple::ProcessMesh(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const TFunctionRef<void(const FRealtimeMeshStreamSet&)>& ProcessFunc) const
{	
	FRealtimeMeshAccessor Accessor;
	
	Accessor.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&ProcessFunc](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.ProcessMeshData(LockContext, ProcessFunc);
	});

	Accessor.Execute(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::EditMeshInPlace(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshStreamSet&)>& EditFunc)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&EditFunc](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.EditMeshData(UpdateContext, EditFunc);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

bool URealtimeMeshSimple::HasCustomComplexMeshGeometry() const
{
	return GetMeshAs<FRealtimeMeshSimple>()->HasCustomComplexMeshGeometry();
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::ClearCustomComplexMeshGeometry()
{
	return GetMeshAs<FRealtimeMeshSimple>()->ClearCustomComplexMeshGeometry();
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::SetCustomComplexMeshGeometry(FRealtimeMeshComplexGeometry&& InComplexMeshGeometry)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetCustomComplexMeshGeometry(MoveTemp(InComplexMeshGeometry));
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::SetCustomComplexMeshGeometry(const FRealtimeMeshComplexGeometry& InComplexMeshGeometry)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetCustomComplexMeshGeometry(InComplexMeshGeometry);
}

void URealtimeMeshSimple::ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshComplexGeometry&)> ProcessFunc) const
{
	return GetMeshAs<FRealtimeMeshSimple>()->ProcessCustomComplexMeshGeometry(ProcessFunc);
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshComplexGeometry&)> EditFunc)
{
	return GetMeshAs<FRealtimeMeshSimple>()->EditCustomComplexMeshGeometry(EditFunc);
}

void URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshSimpleCompletionCallback& OnComplete)
{
	if (!IsValid(MeshData))
	{
		(void)OnComplete.ExecuteIfBound(ERealtimeMeshProxyUpdateStatus::NoUpdate);
		return;
	}
	TFuture<ERealtimeMeshProxyUpdateStatus> Continuation = MeshData? CreateSectionGroup(SectionGroupKey, MeshData->GetStreamSet()) : CreateSectionGroup(SectionGroupKey);	
	Continuation.Next([OnComplete](ERealtimeMeshProxyUpdateStatus Status)
	{
		ensure(IsInGameThread());
		(void)OnComplete.ExecuteIfBound(Status);
	});
}

FRealtimeMeshSectionGroupKey URealtimeMeshSimple::CreateSectionGroupUnique(const FRealtimeMeshLODKey& LODKey, URealtimeMeshStreamSet* MeshData,
                                                                           const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	const FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(LODKey);
	CreateSectionGroup(SectionGroupKey, MeshData, CompletionCallback);
	return SectionGroupKey;
}

void URealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData,
                                             const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	if (MeshData)
	{
		UpdateSectionGroup(SectionGroupKey, MeshData->GetStreamSet()).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
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

void URealtimeMeshSimple::CreateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange,
	bool bShouldCreateCollision, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	CreateSection(SectionKey, Config, StreamRange, bShouldCreateCollision).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		(void)CompletionCallback.ExecuteIfBound(Status);
	});
}

void URealtimeMeshSimple::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision,
	const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	UpdateSectionConfig(SectionKey, Config, bShouldCreateCollision)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

void URealtimeMeshSimple::RemoveSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	RemoveSection(SectionKey)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

void URealtimeMeshSimple::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	RemoveSectionGroup(SectionGroupKey)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

void URealtimeMeshSimple::SetShouldAutoCreateSectionsForPolyGroups(const FRealtimeMeshSectionGroupKey& SectionGroupKey, bool bNewValue)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&bNewValue](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(UpdateContext, bNewValue);
	});
	
	UpdateBuilder.Commit(GetMeshData());
}

bool URealtimeMeshSimple::ShouldAutoCreateSectionsForPolygonGroups(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
{
	bool bShouldAutoCreateSections = true;
	
	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&bShouldAutoCreateSections](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		bShouldAutoCreateSections = SectionGroup.ShouldAutoCreateSectionsForPolygonGroups(LockContext);
	});
	Accessor.Execute(GetMeshData());
	
	return bShouldAutoCreateSections;
}

FRealtimeMeshSectionConfig URealtimeMeshSimple::GetSectionConfig(const FRealtimeMeshSectionKey& SectionKey) const
{
	FRealtimeMeshSectionConfig Config;
	
	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[&Config](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionSimple& Section)
	{
		Config = Section.GetConfig(LockContext);
	});
	Accessor.Execute(GetMeshData());
	
	return Config;
}

bool URealtimeMeshSimple::IsSectionVisible(const FRealtimeMeshSectionKey& SectionKey) const
{
	bool bIsVisible = false;
	
	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[&bIsVisible](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionSimple& Section)
	{
		bIsVisible = Section.IsVisible(LockContext);
	});
	Accessor.Execute(GetMeshData());
	
	return bIsVisible;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[bIsVisible](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionSimple& Section)
	{
		Section.SetVisibility(UpdateContext, bIsVisible);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

void URealtimeMeshSimple::SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	SetSectionVisibility(SectionKey, bIsVisible)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

bool URealtimeMeshSimple::IsSectionCastingShadow(const FRealtimeMeshSectionKey& SectionKey) const
{
	bool bIsCastingShadow = false;
	
	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[&bIsCastingShadow](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionSimple& Section)
	{
		bIsCastingShadow = Section.IsCastingShadow(LockContext);
	});
	Accessor.Execute(GetMeshData());
	
	return bIsCastingShadow;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[bCastShadow](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionSimple& Section)
	{
		Section.SetCastShadow(UpdateContext, bCastShadow);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

void URealtimeMeshSimple::SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	SetSectionCastShadow(SectionKey, bCastShadow)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionKey,
	[SectionKey](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.RemoveSection(UpdateContext, SectionKey);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddLODTask<FRealtimeMeshLODSimple>(SectionGroupKey,
	[SectionGroupKey](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODSimple& LOD)
	{
		LOD.RemoveSectionGroup(UpdateContext, SectionGroupKey);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

const FRealtimeMeshDistanceField& URealtimeMeshSimple::GetDistanceField() const
{
	return GetMeshAs<FRealtimeMeshSimple>()->GetDistanceField();
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField)
{
	FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetMeshAs<FRealtimeMeshSimple>()->SetDistanceField(UpdateContext, MoveTemp(InDistanceField));
	return UpdateContext.Commit();
}

void URealtimeMeshSimple::SetDistanceField(const FRealtimeMeshDistanceField& InDistanceField, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	FRealtimeMeshDistanceField Copy(InDistanceField);
	SetDistanceField(MoveTemp(Copy))
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::ClearDistanceField()
{
	FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetMeshAs<FRealtimeMeshSimple>()->ClearDistanceField(UpdateContext);
	return UpdateContext.Commit();
}

const FRealtimeMeshCardRepresentation* URealtimeMeshSimple::GetCardRepresentation(const FRealtimeMeshLockContext& LockContext) const
{
	return GetMeshAs<FRealtimeMeshSimple>()->GetCardRepresentation(LockContext);
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation)
{
	FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetMeshAs<FRealtimeMeshSimple>()->SetCardRepresentation(UpdateContext, MoveTemp(InCardRepresentation));
	return UpdateContext.Commit();
}

void URealtimeMeshSimple::SetCardRepresentation(const FRealtimeMeshCardRepresentation& InCardRepresentation, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	FRealtimeMeshCardRepresentation Copy(InCardRepresentation);
	SetCardRepresentation(MoveTemp(Copy))
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::ClearCardRepresentation()
{
	FRealtimeMeshUpdateContext UpdateContext(GetMesh());
	GetMeshAs<FRealtimeMeshSimple>()->ClearCardRepresentation(UpdateContext);
	return UpdateContext.Commit();
}

FRealtimeMeshCollisionConfiguration URealtimeMeshSimple::GetCollisionConfig() const
{
	return GetMeshAs<FRealtimeMeshSimple>()->GetCollisionConfig();
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetCollisionConfig(InCollisionConfig);
}

void URealtimeMeshSimple::SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig, const FRealtimeMeshSimpleCollisionCompletionCallback& CompletionCallback)
{
	SetCollisionConfig(InCollisionConfig)
		.Next([CompletionCallback](ERealtimeMeshCollisionUpdateResult Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

FRealtimeMeshSimpleGeometry URealtimeMeshSimple::GetSimpleGeometry() const
{
	return GetMeshAs<FRealtimeMeshSimple>()->GetSimpleGeometry();
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetSimpleGeometry(InSimpleGeometry);
}

void URealtimeMeshSimple::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry, const FRealtimeMeshSimpleCollisionCompletionCallback& CompletionCallback)
{
	SetSimpleGeometry(InSimpleGeometry)
		.Next([CompletionCallback](ERealtimeMeshCollisionUpdateResult Status)
		{
			(void)CompletionCallback.ExecuteIfBound(Status);
		});
}

void URealtimeMeshSimple::Reset(bool bCreateNewMeshData)
{
	Super::Reset(bCreateNewMeshData);
}

void URealtimeMeshSimple::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!IsTemplate())
	{
		StaticCastSharedPtr<FRealtimeMeshSimple>(MeshRef)->MarkCollisionDirtyNoCallback();
	}
}

void URealtimeMeshSimple::PostLoad()
{
	Super::PostLoad();
}

void URealtimeMeshSimple::PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph)
{
	Super::PostLoadSubobjects(OuterInstanceGraph);
}


#undef LOCTEXT_NAMESPACE
