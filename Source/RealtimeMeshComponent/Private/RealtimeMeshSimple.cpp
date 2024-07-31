// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshSimple.h"
#include "RealtimeMeshCore.h"
#include "Mesh/RealtimeMeshBuilder.h"
#include "Mesh/RealtimeMeshSimpleData.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Async/Async.h"
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
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		if (bShouldCreateMeshCollision != bNewShouldCreateMeshCollision)
		{
			bShouldCreateMeshCollision = bNewShouldCreateMeshCollision;
			MarkBoundsDirtyIfNotOverridden();
			// We know the flag changed here so we may need to remove our collision, so force the issue
			MarkCollisionDirtyIfNecessary(true);
		}
	}

	void FRealtimeMeshSectionSimple::UpdateStreamRange(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamRange& InRange)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		auto ParentGroup = GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>();
		
		FRealtimeMeshSection::UpdateStreamRange(Commands, InRange);		
		
		MarkBoundsDirtyIfNotOverridden();
		MarkCollisionDirtyIfNecessary();
	}

	bool FRealtimeMeshSectionSimple::Serialize(FArchive& Ar)
	{
		const bool bResult = FRealtimeMeshSection::Serialize(Ar);

		Ar << bShouldCreateMeshCollision;

		return bResult;
	}

	void FRealtimeMeshSectionSimple::Reset(FRealtimeMeshProxyCommandBatch& Commands)
	{
		bShouldCreateMeshCollision = false;
		FRealtimeMeshSection::Reset(Commands);
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
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		auto UpdatedStreams = EditFunc(Streams);

		for (const auto& UpdatedStream : UpdatedStreams)
		{
			if (const auto* Stream = Streams.Find(UpdatedStream))
			{
				FRealtimeMeshStream StreamCopy(*Stream);				
				FRealtimeMeshSectionGroup::CreateOrUpdateStream(Commands, MoveTemp(StreamCopy));
			}
			else
			{				
				FMessageLog("RealtimeMesh").Error(
					FText::Format(LOCTEXT("EditMeshData_InvalidStream", "Unable to update stream {0} in mesh {1}"),
								  FText::FromString(UpdatedStream.ToString()), FText::FromName(SharedResources->GetMeshName())));
			}
		}

		return Commands.Commit();
	}
	
	void FRealtimeMeshSectionGroupSimple::CreateOrUpdateStream(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshStream&& Stream)
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
				UpdatePolyGroupSections(Commands, false);
			}
			else if ((bShouldCreateSingularSection && Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyTriangles) ||
				(!bShouldCreateSingularSection && (Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyPolyGroups ||
					Stream.GetStreamKey() == FRealtimeMeshStreams::PolyGroups)))
			{
				UpdatePolyGroupSections(Commands, true);
			}
		}
		
		FRealtimeMeshSectionGroup::CreateOrUpdateStream(Commands, MoveTemp(Stream));
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroupSimple::UpdateFromSimpleMesh(const FRealtimeMeshSimpleMeshData& MeshData)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);
		UpdateFromSimpleMesh(Commands, MeshData);
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroupSimple::UpdateFromSimpleMesh(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSimpleMeshData& MeshData)
	{
		if (MeshData.Positions.Num() < 3)
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("UpdateFromSimpleMeshData_InvalidVertexCount", "Invalid vertex count {0} for mesh {1}"),
				MeshData.Positions.Num(), FText::FromName(SharedResources->GetMeshName())));
			return;
		}
		if (MeshData.Triangles.Num() < 3)
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("UpdateFromSimpleMeshData_InvalidTriangleCount", "Invalid triangle count {0} for mesh {1}"),
				MeshData.Positions.Num(), FText::FromName(SharedResources->GetMeshName())));
			return;
		}

		FRealtimeMeshStreamSet StreamSet;
		StreamSet.AddStream(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>());


		StreamSet.AddStream(FRealtimeMeshStreams::Tangents,
			MeshData.bUseHighPrecisionTangents? GetRealtimeMeshBufferLayout<TRealtimeMeshTangents<FPackedRGBA16N>>() : GetRealtimeMeshBufferLayout<TRealtimeMeshTangents<FPackedNormal>>());

		const int32 NumUVChannels = MeshData.UV3.Num() > 0 ? 4 : MeshData.UV2.Num() > 0 ? 3 : MeshData.UV1.Num() > 0 ? 2 : 1;
		StreamSet.AddStream(FRealtimeMeshStreams::TexCoords,
			MeshData.bUseHighPrecisionTexCoords?
				FRealtimeMeshBufferLayout(GetRealtimeMeshDataElementType<FVector2f>(), NumUVChannels) :
				FRealtimeMeshBufferLayout(GetRealtimeMeshDataElementType<FVector2DHalf>(), NumUVChannels));

		StreamSet.AddStream(FRealtimeMeshStreams::Color, GetRealtimeMeshBufferLayout<FColor>());

		StreamSet.AddStream(FRealtimeMeshStreams::Triangles, GetRealtimeMeshBufferLayout<TIndex3<uint32>>());
			//MeshData.Positions.Num() > TNumericLimits<uint16>::Max()? GetRealtimeMeshBufferLayout<TIndex3<uint32>>() : GetRealtimeMeshBufferLayout<TIndex3<uint16>>());

		if (MeshData.MaterialIndex.Num() > 0)
		{
			StreamSet.AddStream(FRealtimeMeshStreams::PolyGroups, MeshData.MaterialIndex.Num() > TNumericLimits<uint16>::Max()? GetRealtimeMeshBufferLayout<uint32>() : GetRealtimeMeshBufferLayout<uint16>());
		}

		MeshData.CopyToStreamSet(StreamSet, false);		
		SetAllStreams(Commands, MoveTemp(StreamSet));
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	void FRealtimeMeshSectionGroupSimple::RemoveStream(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamKey& StreamKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		// Replace the stored stream
		if (Streams.Remove(StreamKey) == 0)
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RemoveStreamInvalid", "Attempted to remove invalid stream {0} in Mesh:{1}"),
				              FText::FromString(StreamKey.ToString()), FText::FromName(SharedResources->GetMeshName())));
		}

		FRealtimeMeshSectionGroup::RemoveStream(Commands, StreamKey);
	}

	void FRealtimeMeshSectionGroupSimple::SetAllStreams(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshStreamSet&& InStreams)
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
		FRealtimeMeshSectionGroup::SetAllStreams(Commands, MoveTemp(InStreams));
		Simple::Private::bShouldDeferPolyGroupUpdates = false;
		
		if (bWantsPolyGroupUpdate)
		{
			UpdatePolyGroupSections(Commands, false);
		}
		else if (bWantsDepthOnlyPolyGroupUpdate)
		{
			UpdatePolyGroupSections(Commands, true);
		}		
	}

	void FRealtimeMeshSectionGroupSimple::InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		// We only send streams here, we rely on the base to send the sections
		Streams.ForEach([&](const FRealtimeMeshStream& Stream)
		{
			if (Commands && SharedResources->WantsStreamOnGPU(Stream.GetStreamKey()) && Stream.Num() > 0)
			{
				const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(Stream);
				UpdateData->ConfigureBuffer(EBufferUsageFlags::Static, true);

				Commands.AddSectionGroupTask(Key, [UpdateData](FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.CreateOrUpdateStream(UpdateData);
				}, ShouldRecreateProxyOnStreamChange());
			}
		});

		FRealtimeMeshSectionGroup::InitializeProxy(Commands);
	}

	void FRealtimeMeshSectionGroupSimple::Reset(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Streams.Empty();
		FRealtimeMeshSectionGroup::Reset(Commands);
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
				CollisionMesh.Append(Streams, SimpleSection->GetConfig().MaterialSlot,
					SimpleSection->GetStreamRange().GetMinIndex() / REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE,
					SimpleSection->GetStreamRange().NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE));
				bHasMeshData = true;
			}
		}

		return bHasMeshData;
	}

	void FRealtimeMeshSectionGroupSimple::UpdatePolyGroupSections(FRealtimeMeshProxyCommandBatch& Commands, bool bUpdateDepthOnly)
	{
		if (ShouldCreateSingularSection())
		{
			const FRealtimeMeshSectionKey PolyGroupKey = FRealtimeMeshSectionKey::CreateForPolyGroup(Key, 0);
												
			if (const auto Section = GetSectionAs<FRealtimeMeshSectionSimple>(PolyGroupKey))
			{				
				Section->UpdateStreamRange(Commands, GetValidStreamRange());
			}
			else
			{
				const FRealtimeMeshSectionConfig SectionConfig = ConfigHandler.IsBound()
					? ConfigHandler.Execute(0)
					: FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0);
				
				CreateOrUpdateSection(Commands, PolyGroupKey, SectionConfig, GetValidStreamRange());
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
						Section->UpdateStreamRange(Commands, Range.Value);
					}
					else
					{
						const FRealtimeMeshSectionConfig SectionConfig = ConfigHandler.IsBound()
							? ConfigHandler.Execute(Range.Key)
							: FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, Range.Key);				
						CreateOrUpdateSection(Commands, PolyGroupKey, SectionConfig, Range.Value);
					}
				}

				for (FRealtimeMeshSectionKey SectionKey : GetSectionKeys())
				{
					check(Sections.Contains(SectionKey));
					const auto& Section = *Sections.Find(SectionKey);
					if (Section->GetKey().IsPolyGroupKey() && (Section->GetStreamRange().Vertices.IsEmpty() || Section->GetStreamRange().Indices.IsEmpty()))
					{
						// Drop this section as it's empty.
						RemoveSection(Commands, SectionKey);
					}
				}
			}
		}
	}

	FRealtimeMeshSectionConfig FRealtimeMeshSectionGroupSimple::DefaultPolyGroupSectionHandler(int32 PolyGroupIndex) const
	{
		return FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, PolyGroupIndex);
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
				ComplexGeometry.AddMesh(MoveTemp(NewMesh));
				bHasSectionData = true;
			}
		}
		return bHasSectionData;
	}


	FRealtimeMeshRef FRealtimeMeshSharedResourcesSimple::CreateRealtimeMesh() const
	{
		return MakeShared<FRealtimeMeshSimple>(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()));
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

	void FRealtimeMeshSimple::SetDistanceField(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshDistanceField&& InDistanceField)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		DistanceField = FRealtimeMeshDistanceField(InDistanceField);
		return FRealtimeMesh::SetDistanceField(Commands, MoveTemp(InDistanceField));
	}

	void FRealtimeMeshSimple::ClearDistanceField(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		DistanceField = FRealtimeMeshDistanceField();
		return FRealtimeMesh::ClearDistanceField(Commands);
	}

	const FRealtimeMeshCardRepresentation* FRealtimeMeshSimple::GetCardRepresentation() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return CardRepresentation.Get();
	}

	void FRealtimeMeshSimple::SetCardRepresentation(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CardRepresentation = MakeUnique<FRealtimeMeshCardRepresentation>(InCardRepresentation);
		return FRealtimeMesh::SetCardRepresentation(Commands, MoveTemp(InCardRepresentation));
	}

	void FRealtimeMeshSimple::ClearCardRepresentation(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CardRepresentation.Reset();
		FRealtimeMesh::ClearCardRepresentation(Commands);
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

	void FRealtimeMeshSimple::InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands) const
	{
		FRealtimeMesh::InitializeProxy(Commands);
		if (Commands)
		{
			Commands.AddMeshTask([DistanceField = DistanceField](FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetDistanceField(MoveTemp(DistanceField));
			});

			if (CardRepresentation)
			{
				FRealtimeMeshCardRepresentation CardRepresentationCopy(*CardRepresentation);
				Commands.AddMeshTask([CardRepresentation = MoveTemp(CardRepresentationCopy)](FRealtimeMeshProxy& Proxy) mutable
				{
					Proxy.SetCardRepresentation(MoveTemp(CardRepresentation));
				});
			}
		}
	}

	void FRealtimeMeshSimple::Reset(FRealtimeMeshProxyCommandBatch& Commands, bool bRemoveRenderProxy)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		CollisionConfig = FRealtimeMeshCollisionConfiguration();
		SimpleGeometry = FRealtimeMeshSimpleGeometry();
		
		FRealtimeMesh::Reset(Commands, bRemoveRenderProxy);

		// Default it back to a single LOD.
		InitializeLODs(Commands, {FRealtimeMeshLODConfig()});
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
				CollisionMesh.Append(ComplexMeshGeometry, 0);
				ComplexGeometry.AddMesh(MoveTemp(CollisionMesh));
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
	Initialize(MakeShared<RealtimeMesh::FRealtimeMeshSharedResourcesSimple>());
	MeshRef->InitializeLODs(RealtimeMesh::TFixedLODArray<FRealtimeMeshLODConfig>{FRealtimeMeshLODConfig()});
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, bool bShouldAutoCreateSectionsForPolyGroups)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);
		
		LOD->CreateOrUpdateSectionGroup(Commands, SectionGroupKey);
		const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey);
		SectionGroup->SetShouldAutoCreateSectionsForPolyGroups(bShouldAutoCreateSectionsForPolyGroups);
		
		return Commands.Commit();
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSectionGroup_InvalidLODKey", "CreateSectionGroup: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
		return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData, bool bShouldAutoCreateSectionsForPolyGroups)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);
		
		LOD->CreateOrUpdateSectionGroup(Commands, SectionGroupKey);
		const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey);
		SectionGroup->SetShouldAutoCreateSectionsForPolyGroups(bShouldAutoCreateSectionsForPolyGroups);
		SectionGroup->SetAllStreams(Commands, MoveTemp(MeshData));
		return Commands.Commit();
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSectionGroup_InvalidLODKey", "CreateSectionGroup: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
		return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
	}
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData, bool bShouldAutoCreateSectionsForPolyGroups)
{
	FRealtimeMeshStreamSet MeshDataCopy(MeshData);
	return CreateSectionGroup(SectionGroupKey, MoveTemp(MeshDataCopy), bShouldAutoCreateSectionsForPolyGroups);
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey))
		{
			return SectionGroup->SetAllStreams(MoveTemp(MeshData));
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("UpdateSectionGroup_InvalidSectionGroupKey", "UpdateSectionGroup: Invalid SectionGroupKey key {0}"),
							  FText::FromString(SectionGroupKey.ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("UpdateSectionGroup_InvalidLODKey", "UpdateSectionGroup: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData)
{
	FRealtimeMeshStreamSet Copy(MeshData);
	return UpdateSectionGroup(SectionGroupKey, MoveTemp(Copy));
}


// ReSharper disable once CppMemberFunctionMayBeConst
PRAGMA_DISABLE_DEPRECATION_WARNINGS
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);
		LOD->CreateOrUpdateSectionGroup(Commands, SectionGroupKey);
		const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey);
		SectionGroup->UpdateFromSimpleMesh(Commands, MeshData);
		return Commands.Commit();
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSectionGroup_InvalidLODKey", "CreateSectionGroup: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
		return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
	}
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

// ReSharper disable once CppMemberFunctionMayBeConst
PRAGMA_DISABLE_DEPRECATION_WARNINGS
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey))
		{
			return SectionGroup->UpdateFromSimpleMesh(MeshData);
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("UpdateSectionGroup_InvalidSectionGroupKey", "UpdateSectionGroup: Invalid SectionGroupKey key {0}"),
							  FText::FromString(SectionGroupKey.ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("UpdateSectionGroup_InvalidLODKey", "UpdateSectionGroup: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSection(const FRealtimeMeshSectionKey& SectionKey,
	const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionKey.SectionGroup()))
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
			FRealtimeMeshProxyCommandBatch Commands(SharedResources);

			SectionGroup->CreateOrUpdateSection(Commands, SectionKey, Config, StreamRange);
			const auto Section = SectionGroup->GetSectionAs<FRealtimeMeshSectionSimple>(SectionKey);
			Section->SetShouldCreateCollision(bShouldCreateCollision);

			return Commands.Commit();
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("CreateSection_InvalidSectionGroupKey", "CreateSection: Invalid SectionGroupKey key {0}"),
							  FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSection_InvalidLODKey", "CreateSection: Invalid LODKey key {0}"),
						  FText::FromString(SectionKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision)
{
	if (const auto LOD = GetMesh()->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSectionAs<FRealtimeMeshSectionSimple>(SectionKey))
			{
				FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
				FRealtimeMeshProxyCommandBatch Commands(SharedResources);

				Section->UpdateConfig(Commands, Config);
				Section->SetShouldCreateCollision(bShouldCreateCollision);

				return Commands.Commit();
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(
					FText::Format(LOCTEXT("UpdateSectionConfig_InvalidSectionKey", "UpdateSectionConfig: Invalid section key {0}"), FText::FromString(SectionKey.ToString())));
			}
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("UpdateSectionConfig_InvalidSectionGroupKey", "UpdateSectionConfig: Invalid section group key {0}"),
				FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("UpdateSectionConfig_InvalidLODKey", "UpdateSectionConfig: Invalid LOD key {0}"), FText::FromString(SectionKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionRange(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange)
{
	if (const auto LOD = MeshRef->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<RealtimeMesh::FRealtimeMeshSectionGroupSimple>(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSection(SectionKey))
			{
				return Section->UpdateStreamRange(StreamRange);
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(
					FText::Format(LOCTEXT("UpdateSectionInvalid", "Attempted to update invalid section {0} in Mesh:{1}"),
								  FText::FromString(SectionKey.ToString()), FText::FromName(GetFName())));
			}
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("UpdateSectionInvalidSectionGroup", "Attempted to update section {0} in invalid section group in Mesh:{1}"),
							  FText::FromString(SectionKey.ToString()), FText::FromName(GetFName())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("UpdateSectionInvalidLOD", "Attempted to update section in invalid LOD {0} in Mesh:{1}"),
						  FText::FromString(SectionKey.LOD().ToString()), FText::FromName(GetFName())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

TArray<FRealtimeMeshSectionGroupKey> URealtimeMeshSimple::GetSectionGroups(const FRealtimeMeshLODKey& LODKey) const
{
	TArray<FRealtimeMeshSectionGroupKey> SectionGroups;
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(LODKey))
	{
		SectionGroups = LOD->GetSectionGroupKeys().Array();
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("GetSectionGroups_InvalidLODKey", "GetSectionGroups: Invalid LODKey key {0}"),
						  FText::FromString(LODKey.ToString())));
	}
	return SectionGroups;
}

TSharedPtr<FRealtimeMeshSectionGroupSimple> URealtimeMeshSimple::GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		return LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey);
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("GetSectionGroup_InvalidLODKey", "GetSectionGroups: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
		return nullptr;
	}
}

TArray<FRealtimeMeshSectionKey> URealtimeMeshSimple::GetSectionsInGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
{	
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey))
		{
			return SectionGroup->GetSectionKeys().Array();
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("GetSectionsInGroup_InvalidSectionGroupKey", "GetSectionsInGroup: Invalid SectionGroupKey key {0}"),
							  FText::FromString(SectionGroupKey.ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("GetSectionsInGroup_InvalidLODKey", "GetSectionsInGroup: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
	}
	return TArray<FRealtimeMeshSectionKey>();
}


bool URealtimeMeshSimple::ProcessMesh(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const TFunctionRef<void(const FRealtimeMeshStreamSet&)>& ProcessFunc) const
{	
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey))
		{
			SectionGroup->ProcessMeshData(ProcessFunc);
			return true;
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("CreateSection_InvalidSectionGroupKey", "CreateSection: Invalid SectionGroupKey key {0}"),
							  FText::FromString(SectionGroupKey.ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSectionGroup_InvalidLODKey", "CreateSectionGroup: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
	}
	return false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::EditMeshInPlace(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshStreamSet&)>& EditFunc)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey))
		{
			return SectionGroup->EditMeshData(EditFunc);
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("CreateSection_InvalidSectionGroupKey", "CreateSection: Invalid SectionGroupKey key {0}"),
							  FText::FromString(SectionGroupKey.ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSectionGroup_InvalidLODKey", "CreateSectionGroup: Invalid LODKey key {0}"),
						  FText::FromString(SectionGroupKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
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
	TFuture<ERealtimeMeshProxyUpdateStatus> Continuation = MeshData? CreateSectionGroup(SectionGroupKey, MeshData->GetStreamSet()) : CreateSectionGroup(SectionGroupKey);	
	Continuation.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void URealtimeMeshSimple::CreateSectionGroupFromSimple(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData,
	const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	CreateSectionGroup(SectionGroupKey, MeshData).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

FRealtimeMeshSectionGroupKey URealtimeMeshSimple::CreateSectionGroupUnique(const FRealtimeMeshLODKey& LODKey, URealtimeMeshStreamSet* MeshData,
                                                                           const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	const FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(LODKey);
	CreateSectionGroup(SectionGroupKey, MeshData, CompletionCallback);
	return SectionGroupKey;
}


PRAGMA_DISABLE_DEPRECATION_WARNINGS
FRealtimeMeshSectionGroupKey URealtimeMeshSimple::CreateSectionGroupUniqueFromSimple(const FRealtimeMeshLODKey& LODKey, const FRealtimeMeshSimpleMeshData& MeshData,
	const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	const FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(LODKey);
	// ReSharper disable once CppDeprecatedEntity
	CreateSectionGroup(SectionGroupKey, MeshData).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
	return SectionGroupKey;
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void URealtimeMeshSimple::UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData,
                                             const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	if (MeshData)
	{
		UpdateSectionGroup(SectionGroupKey, MeshData->GetStreamSet()).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
		});
	}
	else
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(ERealtimeMeshProxyUpdateStatus::NoUpdate);
		}
	}
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void URealtimeMeshSimple::UpdateSectionGroupFromSimple(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData,
	const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	// ReSharper disable once CppDeprecatedEntity
	UpdateSectionGroup(SectionGroupKey, MeshData).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void URealtimeMeshSimple::CreateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange,
	bool bShouldCreateCollision, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	CreateSection(SectionKey, Config, StreamRange, bShouldCreateCollision).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}

FRealtimeMeshSectionKey URealtimeMeshSimple::CreateSectionUnique(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionConfig& Config,
	const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateUnique(SectionGroupKey);
	CreateSection(SectionKey, Config, StreamRange, bShouldCreateCollision).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
	return SectionKey;
}

void URealtimeMeshSimple::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision,
	const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	UpdateSectionConfig(SectionKey, Config, bShouldCreateCollision)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
		});
}

void URealtimeMeshSimple::RemoveSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	RemoveSection(SectionKey)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
		});
}

void URealtimeMeshSimple::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	RemoveSectionGroup(SectionGroupKey)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
		});
}

void URealtimeMeshSimple::SetShouldAutoCreateSectionsForPolyGroups(const FRealtimeMeshSectionGroupKey& SectionGroupKey, bool bNewValue)
{
	if (const auto LOD = GetMesh()->GetLOD(SectionGroupKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey))
		{
			SectionGroup->SetShouldAutoCreateSectionsForPolyGroups(bNewValue);
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("SetShouldAutoCreateSectionsForPolyGroups_InvalidSectionGroupKey", "SetShouldAutoCreateSectionsForPolyGroups: Invalid section group key {0}"), FText::FromString(SectionGroupKey.ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("SetShouldAutoCreateSectionsForPolyGroups_InvalidLODKey", "SetShouldAutoCreateSectionsForPolyGroups: Invalid LOD key {0}"), FText::FromString(SectionGroupKey.LOD().ToString())));
	}
}

bool URealtimeMeshSimple::ShouldAutoCreateSectionsForPolygonGroups(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
{
	if (const auto LOD = GetMesh()->GetLOD(SectionGroupKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey))
		{
			return SectionGroup->ShouldAutoCreateSectionsForPolygonGroups();
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("ShouldAutoCreateSectionsForPolygonGroups_InvalidSectionGroupKey", "ShouldAutoCreateSectionsForPolygonGroups: Invalid section group key {0}"), FText::FromString(SectionGroupKey.ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("ShouldAutoCreateSectionsForPolygonGroups_InvalidLODKey", "ShouldAutoCreateSectionsForPolygonGroups: Invalid LOD key {0}"), FText::FromString(SectionGroupKey.LOD().ToString())));
	}
	return true;
}

FRealtimeMeshSectionConfig URealtimeMeshSimple::GetSectionConfig(const FRealtimeMeshSectionKey& SectionKey) const
{
	if (const auto LOD = GetMesh()->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSection(SectionKey))
			{
				return Section->GetConfig();
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(
					FText::Format(LOCTEXT("GetSectionConfig_InvalidSectionKey", "GetSectionConfig: Invalid section key {0}"), FText::FromString(SectionKey.ToString())));
			}
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("GetSectionConfig_InvalidSectionGroupKey", "GetSectionConfig: Invalid section group key {0}"), FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("GetSectionConfig_InvalidLODKey", "GetSectionConfig: Invalid LOD key {0}"), FText::FromString(SectionKey.LOD().ToString())));
	}
	return FRealtimeMeshSectionConfig();
}

bool URealtimeMeshSimple::IsSectionVisible(const FRealtimeMeshSectionKey& SectionKey) const
{
	if (const auto LOD = GetMesh()->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSection(SectionKey))
			{
				return Section->IsVisible();
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(
					FText::Format(LOCTEXT("IsSectionVisible_InvalidSectionKey", "IsSectionVisible: Invalid section key {0}"), FText::FromString(SectionKey.ToString())));
			}
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("IsSectionVisible_InvalidSectionGroupKey", "IsSectionVisible: Invalid section group key {0}"), FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("IsSectionVisible_InvalidLODKey", "IsSectionVisible: Invalid LOD key {0}"), FText::FromString(SectionKey.LOD().ToString())));
	}
	return false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible)
{
	if (const auto LOD = GetMesh()->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSection(SectionKey))
			{
				return Section->SetVisibility(bIsVisible);
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(FText::Format(
					LOCTEXT("SetSectionVisibility_InvalidSectionKey", "SetSectionVisibility: Invalid section key {0}"), FText::FromString(SectionKey.ToString())));
			}
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("SetSectionVisibility_InvalidSectionGroupKey", "SetSectionVisibility: Invalid section group key {0}"),
				FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("SetSectionVisibility_InvalidLODKey", "SetSectionVisibility: Invalid LOD key {0}"), FText::FromString(SectionKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

void URealtimeMeshSimple::SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	SetSectionVisibility(SectionKey, bIsVisible)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
		});
}

bool URealtimeMeshSimple::IsSectionCastingShadow(const FRealtimeMeshSectionKey& SectionKey) const
{
	if (const auto LOD = GetMesh()->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSection(SectionKey))
			{
				return Section->IsCastingShadow();
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(FText::Format(
					LOCTEXT("IsSectionCastingShadow_InvalidSectionKey", "IsSectionCastingShadow: Invalid section key {0}"), FText::FromString(SectionKey.ToString())));
			}
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("IsSectionCastingShadow_InvalidSectionGroupKey", "IsSectionCastingShadow: Invalid section group key {0}"),
				FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("IsSectionCastingShadow_InvalidLODKey", "IsSectionCastingShadow: Invalid LOD key {0}"), FText::FromString(SectionKey.LOD().ToString())));
	}
	return false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow)
{
	if (const auto LOD = GetMesh()->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSection(SectionKey))
			{
				return Section->SetCastShadow(bCastShadow);
			}
			else
			{
				FMessageLog("RealtimeMesh").Error(FText::Format(
					LOCTEXT("SetSectionCastShadow_InvalidSectionKey", "SetSectionCastShadow: Invalid section key {0}"), FText::FromString(SectionKey.ToString())));
			}
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("SetSectionCastShadow_InvalidSectionGroupKey", "SetSectionCastShadow: Invalid section group key {0}"),
				FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("SetSectionCastShadow_InvalidLODKey", "SetSectionCastShadow: Invalid LOD key {0}"), FText::FromString(SectionKey.LOD().ToString())));
	}
    return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

void URealtimeMeshSimple::SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow, const FRealtimeMeshSimpleCompletionCallback& CompletionCallback)
{
	SetSectionCastShadow(SectionKey, bCastShadow)
		.Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
		{
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
		});
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
{
	if (const auto LOD = GetMesh()->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
		{
			return SectionGroup->RemoveSection(SectionKey);
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("RemoveSection_InvalidSectionGroupKey", "RemoveSection: Invalid section group key {0}"), FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("RemoveSection_InvalidLODKey", "RemoveSection: Invalid LOD key {0}"), FText::FromString(SectionKey.LOD().ToString())));
	}
    return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
{
	if (const auto LOD = GetMesh()->GetLOD(SectionGroupKey.LOD()))
	{
		return LOD->RemoveSectionGroup(SectionGroupKey);
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("RemoveSectionGroup_InvalidLODKey", "RemoveSectionGroup: Invalid LOD key {0}"), FText::FromString(SectionGroupKey.LOD().ToString())));
		return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
	}
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
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
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
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
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
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
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
			if (CompletionCallback.IsBound())
			{
				CompletionCallback.Execute(Status);
			}
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
