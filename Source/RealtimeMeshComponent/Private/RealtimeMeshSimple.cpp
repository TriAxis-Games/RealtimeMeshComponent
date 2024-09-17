// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshSimple.h"
#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshBuilder.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Async/Async.h"
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
		SharedResources->OnStreamChanged().AddRaw(this, &FRealtimeMeshSectionSimple::HandleStreamsChanged);
	}

	FRealtimeMeshSectionSimple::~FRealtimeMeshSectionSimple()
	{
		SharedResources->OnStreamChanged().RemoveAll(this);
	}

	void FRealtimeMeshSectionSimple::SetShouldCreateCollision(bool bNewShouldCreateMeshCollision)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		if (bShouldCreateMeshCollision != bNewShouldCreateMeshCollision)
		{
			bShouldCreateMeshCollision = bNewShouldCreateMeshCollision;
			MarkBoundsDirtyIfNotOverridden();
			// We know the flag changed here so we may need to remove our collision, so force the issue
			MarkCollisionDirtyIfNecessary(true);
		}
	}

	void FRealtimeMeshSectionSimple::UpdateStreamRange(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshStreamRange& InRange)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		auto ParentGroup = GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>();
		
		FRealtimeMeshSection::UpdateStreamRange(ProxyBuilder, InRange);		
		
		MarkBoundsDirtyIfNotOverridden();
		MarkCollisionDirtyIfNecessary();
	}

	bool FRealtimeMeshSectionSimple::Serialize(FArchive& Ar)
	{
		const bool bResult = FRealtimeMeshSection::Serialize(Ar);

		Ar << bShouldCreateMeshCollision;

		return bResult;
	}

	void FRealtimeMeshSectionSimple::Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		bShouldCreateMeshCollision = false;
		FRealtimeMeshSection::Reset(ProxyBuilder);
	}

	void FRealtimeMeshSectionSimple::HandleStreamsChanged(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshChangeType ChangeType) const
	{
		if (!bShouldCreateMeshCollision || !Key.IsPartOf(SectionGroupKey))
		{
			return;
		}

		if (StreamKey == FRealtimeMeshStreams::Position || StreamKey == FRealtimeMeshStreams::TexCoords || StreamKey == FRealtimeMeshStreams::Triangles)
		{
			MarkBoundsDirtyIfNotOverridden();
			MarkCollisionDirtyIfNecessary();
		}
	}

	FBoxSphereBounds3f FRealtimeMeshSectionSimple::CalculateBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		TOptional<FBoxSphereBounds3f> LocalBounds;

		if (const auto SectionGroup = GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>())
		{
			const auto Stream = SectionGroup->GetStream(FRealtimeMeshStreamKey(
				ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::PositionStreamName));
			if (Stream && GetStreamRange().NumVertices() > 0 && GetStreamRange().GetMaxVertex() < Stream->Num())
			{
				const auto TestLayout = GetRealtimeMeshBufferLayout<FVector3f>();
				if (Stream->GetLayout() == TestLayout)
				{
					const FVector3f* Points = Stream->GetData<FVector3f>() + GetStreamRange().GetMinVertex();
					LocalBounds = FBoxSphereBounds3f(Points, GetStreamRange().NumVertices());
				}
			}
		}

		// Set the bounds if we made one, or default them
		return LocalBounds.IsSet() ? LocalBounds.GetValue() : FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f));
	}

	void FRealtimeMeshSectionSimple::MarkCollisionDirtyIfNecessary(bool bForceUpdate) const
	{
		auto SimpleSharedResources = StaticCastSharedRef<FRealtimeMeshSharedResourcesSimple>(SharedResources);
		auto SimpleOwner = StaticCastSharedPtr<FRealtimeMeshSimple>(SimpleSharedResources->GetOwner());
		
		if ((bShouldCreateMeshCollision || bForceUpdate) && !SimpleOwner->HasCustomComplexMeshGeometry())
		{
			SimpleSharedResources->BroadcastCollisionDataChanged();
		}
	}

	FRealtimeMeshStreamRange FRealtimeMeshSectionGroupSimple::GetValidStreamRange() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

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

	const FRealtimeMeshStream* FRealtimeMeshSectionGroupSimple::GetStream(FRealtimeMeshStreamKey StreamKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Streams.Find(StreamKey);
	}

	void FRealtimeMeshSectionGroupSimple::SetPolyGroupSectionHandler(const FRealtimeMeshPolyGroupConfigHandler& NewHandler)
	{
		if (NewHandler.IsBound())
		{
			ConfigHandler = NewHandler;
		}
		else
		{
			ClearPolyGroupSectionHandler();
		}
	}

	void FRealtimeMeshSectionGroupSimple::ClearPolyGroupSectionHandler()
	{
		ConfigHandler = FRealtimeMeshPolyGroupConfigHandler::CreateSP(this, &FRealtimeMeshSectionGroupSimple::DefaultPolyGroupSectionHandler);
	}

	void FRealtimeMeshSectionGroupSimple::ProcessMeshData(TFunctionRef<void(const FRealtimeMeshStreamSet&)> ProcessFunc) const
	{		
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		ProcessFunc(Streams);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroupSimple::EditMeshData(TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshStreamSet&)> EditFunc)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);

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

		return UpdateContext.Commit();
	}
	
	void FRealtimeMeshSectionGroupSimple::CreateOrUpdateStream(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshStream&& Stream)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

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
				UpdatePolyGroupSections(ProxyBuilder, false);
			}
			else if ((bShouldCreateSingularSection && Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyTriangles) ||
				(!bShouldCreateSingularSection && (Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyPolyGroups ||
					Stream.GetStreamKey() == FRealtimeMeshStreams::PolyGroups)))
			{
				UpdatePolyGroupSections(ProxyBuilder, true);
			}
		}
		
		FRealtimeMeshSectionGroup::CreateOrUpdateStream(ProxyBuilder, MoveTemp(Stream));
	}

	void FRealtimeMeshSectionGroupSimple::RemoveStream(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshStreamKey& StreamKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		// Replace the stored stream
		if (Streams.Remove(StreamKey) == 0)
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RemoveStreamInvalid", "Attempted to remove invalid stream {0} in Mesh:{1}"),
				              FText::FromString(StreamKey.ToString()), FText::FromName(SharedResources->GetMeshName())));
		}

		FRealtimeMeshSectionGroup::RemoveStream(ProxyBuilder, StreamKey);
	}

	void FRealtimeMeshSectionGroupSimple::SetAllStreams(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshStreamSet&& InStreams)
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
		FRealtimeMeshSectionGroup::SetAllStreams(ProxyBuilder, MoveTemp(InStreams));
		Simple::Private::bShouldDeferPolyGroupUpdates = false;
		
		if (bWantsPolyGroupUpdate)
		{
			UpdatePolyGroupSections(ProxyBuilder, false);
		}
		else if (bWantsDepthOnlyPolyGroupUpdate)
		{
			UpdatePolyGroupSections(ProxyBuilder, true);
		}		
	}

	void FRealtimeMeshSectionGroupSimple::InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		// We only send streams here, we rely on the base to send the sections
		Streams.ForEach([&](const FRealtimeMeshStream& Stream)
		{
			if (ProxyBuilder && SharedResources->WantsStreamOnGPU(Stream.GetStreamKey()) && Stream.Num() > 0)
			{
				const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(Stream);
				UpdateData->ConfigureBuffer(EBufferUsageFlags::Static, true);

				ProxyBuilder.AddSectionGroupTask(Key, [UpdateData](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.CreateOrUpdateStream(RHICmdList, UpdateData);
				}, ShouldRecreateProxyOnChange());
			}
		});

		FRealtimeMeshSectionGroup::InitializeProxy(ProxyBuilder);
	}

	void FRealtimeMeshSectionGroupSimple::Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Streams.Empty();
		FRealtimeMeshSectionGroup::Reset(ProxyBuilder);
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

	bool FRealtimeMeshSectionGroupSimple::GenerateComplexCollision(FRealtimeMeshCollisionMesh& CollisionMesh)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		bool bHasMeshData = false;
		for (const FRealtimeMeshSectionRef& Section : Sections)
		{
			const auto SimpleSection = StaticCastSharedRef<FRealtimeMeshSectionSimple>(Section);
			if (SimpleSection->HasCollision())
			{
				URealtimeMeshCollisionTools::AppendStreamsToCollisionMesh(CollisionMesh, Streams, SimpleSection->GetConfig().MaterialSlot,
					SimpleSection->GetStreamRange().GetMinIndex() / REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE,
					SimpleSection->GetStreamRange().NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE));
				bHasMeshData = true;
			}
		}

		return bHasMeshData;
	}

	void FRealtimeMeshSectionGroupSimple::UpdatePolyGroupSections(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, bool bUpdateDepthOnly)
	{
		if (ShouldCreateSingularSection())
		{
			const FRealtimeMeshSectionKey PolyGroupKey = FRealtimeMeshSectionKey::CreateForPolyGroup(Key, 0);
												
			if (const auto Section = GetSectionAs<FRealtimeMeshSectionSimple>(PolyGroupKey))
			{				
				Section->UpdateStreamRange(ProxyBuilder, GetValidStreamRange());
			}
			else
			{
				const FRealtimeMeshSectionConfig SectionConfig = ConfigHandler.IsBound()
					? ConfigHandler.Execute(0)
					: FRealtimeMeshSectionConfig(0);
				
				CreateOrUpdateSection(ProxyBuilder, PolyGroupKey, SectionConfig, GetValidStreamRange());
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
			
					if (const auto Section = GetSectionAs<FRealtimeMeshSectionSimple>(PolyGroupKey))
					{
						Section->UpdateStreamRange(ProxyBuilder, Range.Value);
					}
					else
					{
						const FRealtimeMeshSectionConfig SectionConfig = ConfigHandler.IsBound()
							? ConfigHandler.Execute(Range.Key)
							: FRealtimeMeshSectionConfig(Range.Key);				
						CreateOrUpdateSection(ProxyBuilder, PolyGroupKey, SectionConfig, Range.Value);
					}
				}

				for (FRealtimeMeshSectionKey SectionKey : GetSectionKeys())
				{
					check(Sections.Contains(SectionKey));
					const auto& Section = *Sections.Find(SectionKey);
					if (Section->GetKey().IsPolyGroupKey() && (Section->GetStreamRange().Vertices.IsEmpty() || Section->GetStreamRange().Indices.IsEmpty()))
					{
						// Drop this section as it's empty.
						RemoveSection(ProxyBuilder, SectionKey);
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

	bool FRealtimeMeshLODSimple::GenerateComplexCollision(FRealtimeMeshComplexGeometry& ComplexGeometry)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		bool bHasSectionData = false;
		for (const auto& SectionGroup : SectionGroups)
		{
			FRealtimeMeshCollisionMesh NewMesh;
			const bool bHadData = StaticCastSharedRef<FRealtimeMeshSectionGroupSimple>(SectionGroup)->GenerateComplexCollision(NewMesh);
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

		UpdateBuilder.AddLODTask<FRealtimeMeshLODSimple>(SectionGroupKey, [SectionGroupKey, InConfig](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshLODSimple& LOD)
		{
			LOD.CreateOrUpdateSectionGroup(ProxyBuilder, SectionGroupKey, InConfig);
		});

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
			[bShouldAutoCreateSectionsForPolyGroups](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionGroupSimple& SectionGroup)
		{
			SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(bShouldAutoCreateSectionsForPolyGroups);
		});

		return UpdateBuilder.Commit(this->AsShared());
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData,
		const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups)
	{
		FRealtimeMeshUpdateBuilder UpdateBuilder;

		UpdateBuilder.AddLODTask<FRealtimeMeshLODSimple>(SectionGroupKey, [SectionGroupKey, InConfig](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshLODSimple& LOD)
		{
			LOD.CreateOrUpdateSectionGroup(ProxyBuilder, SectionGroupKey, InConfig);
		});

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
			[bShouldAutoCreateSectionsForPolyGroups, MeshData = MoveTemp(MeshData)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionGroupSimple& SectionGroup) mutable
		{
			SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(bShouldAutoCreateSectionsForPolyGroups);
			SectionGroup.SetAllStreams(ProxyBuilder, MoveTemp(MeshData));
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
			[MeshData = MoveTemp(MeshData)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionGroupSimple& SectionGroup) mutable
		{
			SectionGroup.SetAllStreams(ProxyBuilder, MoveTemp(MeshData));
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

	void FRealtimeMeshSimple::SetDistanceField(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshDistanceField&& InDistanceField)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		DistanceField = FRealtimeMeshDistanceField(InDistanceField);
		return FRealtimeMesh::SetDistanceField(ProxyBuilder, MoveTemp(InDistanceField));
	}

	void FRealtimeMeshSimple::ClearDistanceField(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		DistanceField = FRealtimeMeshDistanceField();
		return FRealtimeMesh::ClearDistanceField(ProxyBuilder);
	}

	const FRealtimeMeshCardRepresentation* FRealtimeMeshSimple::GetCardRepresentation() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return CardRepresentation.Get();
	}

	void FRealtimeMeshSimple::SetCardRepresentation(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CardRepresentation = MakeUnique<FRealtimeMeshCardRepresentation>(InCardRepresentation);
		return FRealtimeMesh::SetCardRepresentation(ProxyBuilder, MoveTemp(InCardRepresentation));
	}

	void FRealtimeMeshSimple::ClearCardRepresentation(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CardRepresentation.Reset();
		FRealtimeMesh::ClearCardRepresentation(ProxyBuilder);
	}

	bool FRealtimeMeshSimple::GenerateComplexCollision(FRealtimeMeshComplexGeometry& OutComplexGeometry)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		// Copy any custom complex geometry
		OutComplexGeometry = ComplexGeometry;
		
		// TODO: Allow other LOD to be used for collision?
		if (LODs.IsValidIndex(0))
		{
			return StaticCastSharedRef<FRealtimeMeshLODSimple>(LODs[0])->GenerateComplexCollision(OutComplexGeometry);
		}
		return false;
	}

	void FRealtimeMeshSimple::InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder) const
	{
		FRealtimeMesh::InitializeProxy(ProxyBuilder);
		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([DistanceField = DistanceField](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetDistanceField(MoveTemp(DistanceField));
			});

			if (CardRepresentation)
			{
				FRealtimeMeshCardRepresentation CardRepresentationCopy(*CardRepresentation);
				ProxyBuilder.AddMeshTask([CardRepresentation = MoveTemp(CardRepresentationCopy)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
				{
					Proxy.SetCardRepresentation(MoveTemp(CardRepresentation));
				});
			}
		}
	}

	void FRealtimeMeshSimple::Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, bool bRemoveRenderProxy)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CollisionConfig = FRealtimeMeshCollisionConfiguration();
		SimpleGeometry = FRealtimeMeshSimpleGeometry();
		
		FRealtimeMesh::Reset(ProxyBuilder, bRemoveRenderProxy);

		// Default it back to a single LOD.
		InitializeLODs(ProxyBuilder, {FRealtimeMeshLODConfig()});
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

	void FRealtimeMeshSimple::MarkForEndOfFrameUpdate() const
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		SharedResources->GetEndOfFrameRequestHandler().ExecuteIfBound();
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

	void FRealtimeMeshSimple::MarkCollisionDirtyNoCallback() const
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		MarkForEndOfFrameUpdate();

		if (!PendingCollisionPromise.IsValid())
		{
			PendingCollisionPromise = MakeShared<TPromise<ERealtimeMeshCollisionUpdateResult>>();
		}
	}

	void FRealtimeMeshSimple::ProcessEndOfFrameUpdates()
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		if (PendingCollisionPromise.IsValid())
		{
			const bool bAsyncCook = CollisionConfig.bUseAsyncCook;

			auto CollisionData = MakeShared<FRealtimeMeshCollisionInfo>();
			CollisionData->Configuration = CollisionConfig;
			CollisionData->SimpleGeometry = SimpleGeometry;
			
			auto ThisWeak = TWeakPtr<FRealtimeMeshSimple>(StaticCastSharedRef<FRealtimeMeshSimple>(this->AsShared()));

			const bool bGenerateOnOtherThread = bAsyncCook && IsInGameThread();

			auto GenerationPromise = MakeShared<TPromise<TSharedPtr<FRealtimeMeshCollisionInfo>>>();

			auto GenerateMeshDataLambda = [ThisWeak, GenerationPromise, CollisionData]() mutable
			{
				FRealtimeMeshComplexGeometry ComplexGeometry;
				if (const auto ThisShared = ThisWeak.Pin())
				{
					if (ThisShared->GenerateComplexCollision(ComplexGeometry))
					{
						CollisionData->ComplexGeometry = MoveTemp(ComplexGeometry);
					}
				}
				GenerationPromise->SetValue(CollisionData);
			};


			if (bGenerateOnOtherThread)
			{
				AsyncTask(ENamedThreads::AnyThread, MoveTemp(GenerateMeshDataLambda));
			}
			else
			{
				GenerateMeshDataLambda();
			}

			GenerationPromise->GetFuture().Next([ThisWeak, ResultPromise = PendingCollisionPromise](const TSharedPtr<FRealtimeMeshCollisionInfo>& CollisionData) mutable
			{
				auto SendCollisionUpdate = [ThisWeak, ResultPromise, CollisionData]() mutable
				{
					if (const auto ThisShared = ThisWeak.Pin())
					{
						ThisShared->UpdateCollision(MoveTemp(*CollisionData))
						          .Next([ResultPromise](ERealtimeMeshCollisionUpdateResult Result)
						          {
							          ResultPromise->SetValue(Result);
						          });
					}
					else
					{
						ResultPromise->SetValue(ERealtimeMeshCollisionUpdateResult::Ignored);
					}
				};

				if (!IsInGameThread())
				{
					AsyncTask(ENamedThreads::GameThread, MoveTemp(SendCollisionUpdate));
				}
				else
				{
					SendCollisionUpdate();
				}
			});

			PendingCollisionPromise.Reset();
		}
		FRealtimeMesh::ProcessEndOfFrameUpdates();
	}
}


URealtimeMeshSimple::URealtimeMeshSimple(const FObjectInitializer& ObjectInitializer)
	: URealtimeMesh(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Initialize(MakeShared<RealtimeMesh::FRealtimeMeshSharedResourcesSimple>());
		MeshRef->InitializeLODs(RealtimeMesh::TFixedLODArray<FRealtimeMeshLODConfig>{FRealtimeMeshLODConfig()});
	}
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
		[SectionKey, Config, StreamRange](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.CreateOrUpdateSection(ProxyBuilder, SectionKey, Config, StreamRange);
	});

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
		[bShouldCreateCollision](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionSimple& Section)
	{
		Section.SetShouldCreateCollision(bShouldCreateCollision);			
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[Config, bShouldCreateCollision](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionSimple& Section)
	{
		Section.UpdateConfig(ProxyBuilder, Config);
		Section.SetShouldCreateCollision(bShouldCreateCollision);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionRange(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[StreamRange](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionSimple& Section)
	{
		Section.UpdateStreamRange(ProxyBuilder, StreamRange);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

TArray<FRealtimeMeshSectionGroupKey> URealtimeMeshSimple::GetSectionGroups(const FRealtimeMeshLODKey& LODKey) const
{
	FRealtimeMeshAccessor Accessor;

	TArray<FRealtimeMeshSectionGroupKey> SectionGroups;
	
	Accessor.AddLODTask<FRealtimeMeshLODSimple>(LODKey,
	[&SectionGroups](const FRealtimeMeshLODSimple& LOD)
	{
		SectionGroups = LOD.GetSectionGroupKeys().Array();
	});

	Accessor.Execute(GetMeshData());
	
	return SectionGroups;
}

TSharedPtr<FRealtimeMeshSectionGroupSimple> URealtimeMeshSimple::GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
{
	FRealtimeMeshAccessor Accessor;

	TSharedPtr<FRealtimeMeshSectionGroupSimple> FoundSectionGroup;
	
	Accessor.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&FoundSectionGroup](const FRealtimeMeshSectionGroupSimple& SectionGroup)
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
	[&Sections](const FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		Sections = SectionGroup.GetSectionKeys().Array();
	});

	Accessor.Execute(GetMeshData());
	
	return Sections;
}


void URealtimeMeshSimple::ProcessMesh(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const TFunctionRef<void(const FRealtimeMeshStreamSet&)>& ProcessFunc) const
{	
	FRealtimeMeshAccessor Accessor;
	
	Accessor.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&ProcessFunc](const FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.ProcessMeshData(ProcessFunc);
	});

	Accessor.Execute(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::EditMeshInPlace(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshStreamSet&)>& EditFunc)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&EditFunc](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.EditMeshData(EditFunc);
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

void URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData,
                                             const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	if (!IsValid(MeshData))
	{
		ensure(IsInGameThread());
		//check(IsValid(MeshData));
		CompletionCallback.ExecuteIfBound(ERealtimeMeshProxyUpdateStatus::NoUpdate);
		return;
	}
	TFuture<ERealtimeMeshProxyUpdateStatus> Continuation = MeshData? CreateSectionGroup(SectionGroupKey, MeshData->GetStreamSet()) : CreateSectionGroup(SectionGroupKey);	
	Continuation.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		ensure(IsInGameThread());
		CompletionCallback.ExecuteIfBound(Status);
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
			CompletionCallback.ExecuteIfBound(Status);
		});
	}
	else
	{
		FEditorScriptExecutionGuard ScriptGuard;
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.ExecuteIfBound(ERealtimeMeshProxyUpdateStatus::NoUpdate);
		}
	}
}

void URealtimeMeshSimple::CreateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange,
	bool bShouldCreateCollision, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	CreateSection(SectionKey, Config, StreamRange, bShouldCreateCollision).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		CompletionCallback.ExecuteIfBound(Status);
	});
}

void URealtimeMeshSimple::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision,
	const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	UpdateSectionConfig(SectionKey, Config, bShouldCreateCollision)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			CompletionCallback.ExecuteIfBound(Status);
		});
}

void URealtimeMeshSimple::RemoveSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	RemoveSection(SectionKey)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			CompletionCallback.ExecuteIfBound(Status);
		});
}

void URealtimeMeshSimple::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	RemoveSectionGroup(SectionGroupKey)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			CompletionCallback.ExecuteIfBound(Status);
		});
}

void URealtimeMeshSimple::SetShouldAutoCreateSectionsForPolyGroups(const FRealtimeMeshSectionGroupKey& SectionGroupKey, bool bNewValue)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&bNewValue](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.SetShouldAutoCreateSectionsForPolyGroups(bNewValue);
	});
	
	UpdateBuilder.Commit(GetMeshData());
}

bool URealtimeMeshSimple::ShouldAutoCreateSectionsForPolygonGroups(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
{
	bool bShouldAutoCreateSections = true;
	
	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionGroupKey,
	[&bShouldAutoCreateSections](const FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		bShouldAutoCreateSections = SectionGroup.ShouldAutoCreateSectionsForPolygonGroups();
	});
	Accessor.Execute(GetMeshData());
	
	return bShouldAutoCreateSections;
}

FRealtimeMeshSectionConfig URealtimeMeshSimple::GetSectionConfig(const FRealtimeMeshSectionKey& SectionKey) const
{
	FRealtimeMeshSectionConfig Config;
	
	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[&Config](const FRealtimeMeshSectionSimple& Section)
	{
		Config = Section.GetConfig();
	});
	Accessor.Execute(GetMeshData());
	
	return Config;
}

bool URealtimeMeshSimple::IsSectionVisible(const FRealtimeMeshSectionKey& SectionKey) const
{
	bool bIsVisible = false;
	
	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[&bIsVisible](const FRealtimeMeshSectionSimple& Section)
	{
		bIsVisible = Section.IsVisible();
	});
	Accessor.Execute(GetMeshData());
	
	return bIsVisible;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[bIsVisible](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionSimple& Section)
	{
		Section.SetVisibility(bIsVisible);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

void URealtimeMeshSimple::SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	SetSectionVisibility(SectionKey, bIsVisible)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			CompletionCallback.ExecuteIfBound(Status);
		});
}

bool URealtimeMeshSimple::IsSectionCastingShadow(const FRealtimeMeshSectionKey& SectionKey) const
{
	bool bIsCastingShadow = false;
	
	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[&bIsCastingShadow](const FRealtimeMeshSectionSimple& Section)
	{
		bIsCastingShadow = Section.IsCastingShadow();
	});
	Accessor.Execute(GetMeshData());
	
	return bIsCastingShadow;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionTask<FRealtimeMeshSectionSimple>(SectionKey,
	[bCastShadow](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionSimple& Section)
	{
		Section.SetCastShadow(bCastShadow);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

void URealtimeMeshSimple::SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	SetSectionCastShadow(SectionKey, bCastShadow)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			CompletionCallback.ExecuteIfBound(Status);
		});
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddSectionGroupTask<FRealtimeMeshSectionGroupSimple>(SectionKey,
	[SectionKey](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionGroupSimple& SectionGroup)
	{
		SectionGroup.RemoveSection(ProxyBuilder, SectionKey);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
{
	FRealtimeMeshUpdateBuilder UpdateBuilder;

	UpdateBuilder.AddLODTask<FRealtimeMeshLODSimple>(SectionGroupKey,
	[SectionGroupKey](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshLODSimple& LOD)
	{
		LOD.RemoveSectionGroup(ProxyBuilder, SectionGroupKey);
	});
	
	return UpdateBuilder.Commit(GetMeshData());
}

const FRealtimeMeshDistanceField& URealtimeMeshSimple::GetDistanceField() const
{
	return GetMeshAs<FRealtimeMeshSimple>()->GetDistanceField();
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetDistanceField(MoveTemp(InDistanceField));
}

void URealtimeMeshSimple::SetDistanceField(const FRealtimeMeshDistanceField& InDistanceField, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	FRealtimeMeshDistanceField Copy(InDistanceField);
	SetDistanceField(MoveTemp(Copy))
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			CompletionCallback.ExecuteIfBound(Status);
		});
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::ClearDistanceField()
{
	return GetMeshAs<FRealtimeMeshSimple>()->ClearDistanceField();
}

const FRealtimeMeshCardRepresentation* URealtimeMeshSimple::GetCardRepresentation() const
{
	return GetMeshAs<FRealtimeMeshSimple>()->GetCardRepresentation();
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetCardRepresentation(MoveTemp(InCardRepresentation));
}

void URealtimeMeshSimple::SetCardRepresentation(const FRealtimeMeshCardRepresentation& InCardRepresentation, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	FRealtimeMeshCardRepresentation Copy(InCardRepresentation);
	SetCardRepresentation(MoveTemp(Copy))
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			CompletionCallback.ExecuteIfBound(Status);
		});
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::ClearCardRepresentation()
{
	return GetMeshAs<FRealtimeMeshSimple>()->ClearCardRepresentation();
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
			CompletionCallback.ExecuteIfBound(Status);
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
			CompletionCallback.ExecuteIfBound(Status);
		});
}

void URealtimeMeshSimple::Reset(bool bCreateNewMeshData)
{
	Super::Reset(bCreateNewMeshData);
}

void URealtimeMeshSimple::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	StaticCastSharedPtr<FRealtimeMeshSimple>(MeshRef)->MarkCollisionDirtyNoCallback();
}



#undef LOCTEXT_NAMESPACE
