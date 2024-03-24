// Copyright TriAxis Games, L.L.C. All Rights Reserved.

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
			MarkCollisionDirtyIfNecessary();
		}
	}

	void FRealtimeMeshSectionSimple::UpdateStreamRange(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamRange& InRange)
	{
		if (!GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>()->IsStandalone())
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
			FRealtimeMeshSection::UpdateStreamRange(Commands, InRange);

			MarkBoundsDirtyIfNotOverridden();
			MarkCollisionDirtyIfNecessary();
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("UpdateStreamRange_StandaloneInvalid", "Attempted to update stream range of standalone section. You cannot update this separately from SectionGroup mesh data in Mesh:{1}"),
							  FText::FromName(SharedResources->GetMeshName())));
		}
	}

	bool FRealtimeMeshSectionSimple::GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		if (bShouldCreateMeshCollision)
		{
			if (const auto SectionGroup = GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>())
			{
				const auto PositionStream = SectionGroup->GetStream(FRealtimeMeshStreams::Position);
				const auto TriangleStream = SectionGroup->GetStream(FRealtimeMeshStreams::Triangles);

				const auto TexCoordsStream = SectionGroup->GetStream(FRealtimeMeshStreamKey(FRealtimeMeshStreams::TexCoords));

				auto& CollisionVertices = CollisionData.GetVertices();
				auto& CollisionUVs = CollisionData.GetUVs();
				auto& CollisionMaterials = CollisionData.GetMaterials();
				auto& CollisionTriangles = CollisionData.GetTriangles();

				if (PositionStream && TriangleStream)
				{
					if (PositionStream->Num() >= 3 && (TriangleStream->Num() * TriangleStream->GetNumElements()) >= 3)
					{
						const int32 StartVertexIndex = CollisionVertices.Num();

						// Copy in the vertices
						const auto PositionsView = PositionStream->GetArrayView<FVector3f>();
						CollisionVertices.Append(PositionsView.GetData(), PositionsView.Num());

						if (TexCoordsStream)
						{
							CollisionUVs.SetNum(TexCoordsStream->GetNumElements());
							for (int32 ChannelIndex = 0; ChannelIndex < TexCoordsStream->GetNumElements(); ChannelIndex++)
							{
								TRealtimeMeshStridedStreamBuilder<const FVector2d, void> UVData(*TexCoordsStream, ChannelIndex);
								const int32 NumUVsToCopy = FMath::Min(UVData.Num(), PositionStream->Num());
								auto& CollisionUVChannel = CollisionUVs[ChannelIndex];
								CollisionUVChannel.SetNumUninitialized(NumUVsToCopy);
								
								for (int32 TexCoordIdx = 0; TexCoordIdx < NumUVsToCopy; TexCoordIdx++)
								{
									CollisionUVChannel[TexCoordIdx] = UVData[TexCoordIdx];
								}

								// Make sure the uv data is the same length as the position data
								if (PositionStream->Num() > UVData.Num())
								{
									CollisionUVChannel.SetNumZeroed(PositionStream->Num());
								}
							}
						}
						else
						{
							// Zero fill a single channel since we didn't have valid UVs
							CollisionUVs.SetNum(1);
							CollisionUVs[0].SetNumZeroed(PositionStream->Num());
						}

						const int32 MaterialSlot = GetConfig().MaterialSlot;

						TRealtimeMeshStreamBuilder<const TIndex3<uint32>, void> TrianglesData(*TriangleStream);
						CollisionTriangles.Reserve(TrianglesData.Num());
						CollisionMaterials.Reserve(TrianglesData.Num());

						for (int32 TriIdx = 0; TriIdx < TrianglesData.Num(); TriIdx++)
						{
							FTriIndices& Tri = CollisionTriangles.AddDefaulted_GetRef();
							Tri.v0 = TrianglesData[TriIdx].GetElement(0).GetValue() + StartVertexIndex;
							Tri.v1 = TrianglesData[TriIdx].GetElement(1).GetValue() + StartVertexIndex;
							Tri.v2 = TrianglesData[TriIdx].GetElement(2).GetValue() + StartVertexIndex;

							CollisionMaterials.Add(MaterialSlot);
						}
						
						return true;
					}
				}
			}
		}
		return false;
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

	void FRealtimeMeshSectionSimple::MarkCollisionDirtyIfNecessary() const
	{
		auto SimpleSharedResources = StaticCastSharedRef<FRealtimeMeshSharedResourcesSimple>(SharedResources);
		auto SimpleOwner = StaticCastSharedPtr<FRealtimeMeshSimple>(SimpleSharedResources->GetOwner());
		
		if (bShouldCreateMeshCollision && !SimpleOwner->HasCustomComplexMeshGeometry())
		{
			SimpleSharedResources->BroadcastCollisionDataChanged();
		}
	}

	

	FRealtimeMeshSectionPtr FRealtimeMeshSectionGroupSimple::GetStandaloneSection() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		if (IsStandalone())
		{
			check(Sections.Num() <= 1);
			return Sections.Num() > 0 ? *Sections.CreateConstIterator() : FRealtimeMeshSectionPtr(nullptr);
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(
				LOCTEXT("GetStandaloneSection_InvalidOnNonStandaloneGroup", "Unable to get standalone section of non standalone group {0} for mesh {1}"),
				FText::FromString(Key.ToString()), FText::FromName(SharedResources->GetMeshName())));
		}

		return nullptr;
	}

	FRealtimeMeshStreamRange FRealtimeMeshSectionGroupSimple::GetStreamRange() const
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
		
		if (IsStandalone() && GetStandaloneSection())
		{
			GetStandaloneSection()->UpdateStreamRange(Commands, GetStreamRange());
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
			if (Stream.GetStreamKey() == FRealtimeMeshStreams::PolyGroups ||
				Stream.GetStreamKey() == FRealtimeMeshStreams::PolyGroupSegments ||
				Stream.GetStreamKey() == FRealtimeMeshStreams::Triangles)
			{
				UpdatePolyGroupSections(Commands, false);
			}
			else if (Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyPolyGroups ||
				Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyPolyGroupSegments ||
				Stream.GetStreamKey() == FRealtimeMeshStreams::DepthOnlyTriangles)
			{
				UpdatePolyGroupSections(Commands, true);
			}
		}
		
		FRealtimeMeshSectionGroup::CreateOrUpdateStream(Commands, MoveTemp(Stream));

		if (IsStandalone() && GetStandaloneSection())
		{
			GetStandaloneSection()->UpdateStreamRange(Commands, GetStreamRange());
		}		
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
			const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(Stream);
			UpdateData->ConfigureBuffer(EBufferUsageFlags::Static, true);

			Commands.AddSectionGroupTask(Key, [UpdateData](FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.CreateOrUpdateStream(UpdateData);
			}, ShouldRecreateProxyOnStreamChange());
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
			int32 NumStreams = Streams.Num();
			Ar << NumStreams;

			if (Ar.IsLoading())
			{
				Streams.Empty();
				for (int32 Index = 0; Index < NumStreams; Index++)
				{
					FRealtimeMeshStreamKey StreamKey;
					Ar << StreamKey;
					FRealtimeMeshStream Stream;
					Ar << Stream;
					Stream.SetStreamKey(StreamKey);

					Streams.AddStream(MoveTemp(Stream));
				}
			}
			else
			{
				Streams.ForEach([&Ar](FRealtimeMeshStream& Stream)
				{					
					FRealtimeMeshStreamKey StreamKey = Stream.GetStreamKey();
					Ar << StreamKey;
					Ar << Stream;
				});
			}
		}

		return bResult;
	}

	bool FRealtimeMeshSectionGroupSimple::GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		bool bHasSectionData = false;
		for (const auto& Section : Sections)
		{
			bHasSectionData |= StaticCastSharedRef<FRealtimeMeshSectionSimple>(Section)->GenerateCollisionMesh(CollisionData);
		}
		return bHasSectionData;
	}

	void FRealtimeMeshSectionGroupSimple::UpdatePolyGroupSections(FRealtimeMeshProxyCommandBatch& Commands, bool bUpdateDepthOnly)
	{
		const auto PolyGroupSegments = bUpdateDepthOnly? Streams.Find(FRealtimeMeshStreams::DepthOnlyPolyGroupSegments) : Streams.Find(FRealtimeMeshStreams::PolyGroupSegments);
		const auto PolyGroupIndices = bUpdateDepthOnly? Streams.Find(FRealtimeMeshStreams::DepthOnlyPolyGroupSegments) : Streams.Find(FRealtimeMeshStreams::PolyGroups);
		const auto Triangles = bUpdateDepthOnly? Streams.Find(FRealtimeMeshStreams::DepthOnlyTriangles) : Streams.Find(FRealtimeMeshStreams::Triangles);

		if (Triangles)
		{
			TMap<int32, FRealtimeMeshStreamRange> Ranges;
			bool bHadPolyGroupData = false;

			if (PolyGroupSegments)
			{
				RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupRanges(*PolyGroupSegments, *Triangles, Ranges);
				bHadPolyGroupData = true;
			}
			else if (PolyGroupIndices)
			{
				RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupIndices(*PolyGroupIndices, *Triangles, Ranges);	
				bHadPolyGroupData = true;			
			}


			if (bHadPolyGroupData)
			{
				// First update all the stream ranges
				for (const auto Range : Ranges)
				{
					const FRealtimeMeshSectionKey PolyGroupKey = FRealtimeMeshSectionKey::CreateForPolyGroup(Key, Range.Key);
				
					if (const auto Section = GetSectionAs<FRealtimeMeshSectionSimple>(PolyGroupKey))
					{
						Section->UpdateStreamRange(Commands, Range.Value);
					}
					else
					{
						const FRealtimeMeshSectionConfig SectionConfig = ConfigHandler.IsBound()? ConfigHandler.Execute(Range.Key) : FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, Range.Key);				
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


	
	bool FRealtimeMeshLODSimple::GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		bool bHasSectionData = false;
		for (const auto& SectionGroup : SectionGroups)
		{
			bHasSectionData |= StaticCastSharedRef<FRealtimeMeshSectionGroupSimple>(SectionGroup)->GenerateCollisionMesh(CollisionData);
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
		if (ComplexMeshGeometry.Num() > 0)
		{
			ComplexMeshGeometry.Empty();
			return MarkCollisionDirty();
		}

		return MakeFulfilledPromise<ERealtimeMeshCollisionUpdateResult>(ERealtimeMeshCollisionUpdateResult::Ignored).GetFuture();
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::SetCustomComplexMeshGeometry(FRealtimeMeshStreamSet&& InComplexMeshGeometry)
	{
		ComplexMeshGeometry = MoveTemp(InComplexMeshGeometry);
		return MarkCollisionDirty();		
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::SetCustomComplexMeshGeometry(const FRealtimeMeshStreamSet& InComplexMeshGeometry)
	{
		FRealtimeMeshStreamSet ComplexMeshGeometryCopy(InComplexMeshGeometry);
		return SetCustomComplexMeshGeometry(MoveTemp(ComplexMeshGeometryCopy));
	}

	void FRealtimeMeshSimple::ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshStreamSet&)> ProcessFunc) const
	{
		ProcessFunc(ComplexMeshGeometry);
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMeshSimple::EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshStreamSet&)> EditFunc)
	{
		EditFunc(ComplexMeshGeometry);
		return MarkCollisionDirty();
	}

	bool FRealtimeMeshSimple::GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData)
	{
		if (ComplexMeshGeometry.Num() > 0)
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

			const auto PositionStream = ComplexMeshGeometry.Find(FRealtimeMeshStreams::Position);
			const auto TriangleStream = ComplexMeshGeometry.Find(FRealtimeMeshStreams::Triangles);
			const auto TexCoordsStream = ComplexMeshGeometry.Find(FRealtimeMeshStreamKey(FRealtimeMeshStreams::TexCoords));

			auto& CollisionVertices = CollisionData.GetVertices();
			auto& CollisionUVs = CollisionData.GetUVs();
			auto& CollisionMaterials = CollisionData.GetMaterials();
			auto& CollisionTriangles = CollisionData.GetTriangles();

			if (PositionStream && TriangleStream)
			{
				if (PositionStream->Num() >= 3 && (TriangleStream->Num() * TriangleStream->GetNumElements()) >= 3)
				{
					const int32 StartVertexIndex = CollisionVertices.Num();

					// Copy in the vertices
					const auto PositionsView = PositionStream->GetArrayView<FVector3f>();
					CollisionVertices.Append(PositionsView.GetData(), PositionsView.Num());

					if (TexCoordsStream)
					{
						CollisionUVs.SetNum(TexCoordsStream->GetNumElements());
						for (int32 ChannelIndex = 0; ChannelIndex < TexCoordsStream->GetNumElements(); ChannelIndex++)
						{
							TRealtimeMeshStridedStreamBuilder<const FVector2d, void> UVData(*TexCoordsStream, ChannelIndex);
							const int32 NumUVsToCopy = FMath::Min(UVData.Num(), PositionStream->Num());
							auto& CollisionUVChannel = CollisionUVs[ChannelIndex];
							CollisionUVChannel.SetNumUninitialized(NumUVsToCopy);
							
							for (int32 TexCoordIdx = 0; TexCoordIdx < NumUVsToCopy; TexCoordIdx++)
							{
								CollisionUVChannel[TexCoordIdx] = UVData[TexCoordIdx];
							}

							// Make sure the uv data is the same length as the position data
							if (PositionStream->Num() > UVData.Num())
							{
								CollisionUVChannel.SetNumZeroed(PositionStream->Num());
							}
						}
					}
					else
					{
						// Zero fill a single channel since we didn't have valid UVs
						CollisionUVs.SetNum(1);
						CollisionUVs[0].SetNumZeroed(PositionStream->Num());
					}

					TRealtimeMeshStreamBuilder<const TIndex3<uint32>, void> TrianglesData(*TriangleStream);
					CollisionTriangles.Reserve(TrianglesData.Num());
					CollisionMaterials.Reserve(TrianglesData.Num());

					for (int32 TriIdx = 0; TriIdx < TrianglesData.Num(); TriIdx++)
					{
						FTriIndices& Tri = CollisionTriangles.AddDefaulted_GetRef();
						Tri.v0 = TrianglesData[TriIdx].GetElement(0).GetValue() + StartVertexIndex;
						Tri.v1 = TrianglesData[TriIdx].GetElement(1).GetValue() + StartVertexIndex;
						Tri.v2 = TrianglesData[TriIdx].GetElement(2).GetValue() + StartVertexIndex;

						CollisionMaterials.Add(0 /* Material Slot */);
					}
					
					return true;
				}
			}
			
			return false;			
		}		
		
		// TODO: Allow other LOD to be used for collision?
		if (LODs.IsValidIndex(0))
		{
			return StaticCastSharedRef<FRealtimeMeshLODSimple>(LODs[0])->GenerateCollisionMesh(CollisionData);
		}
		return false;
	}

	void FRealtimeMeshSimple::Reset(FRealtimeMeshProxyCommandBatch& Commands, bool bRemoveRenderProxy)
	{
		CollisionConfig = FRealtimeMeshCollisionConfiguration();
		SimpleGeometry = FRealtimeMeshSimpleGeometry();
		
		FRealtimeMesh::Reset(Commands, bRemoveRenderProxy);

		// Default it back to a single LOD.
		InitializeLODs(Commands, {FRealtimeMeshLODConfig()});
	}

	bool FRealtimeMeshSimple::Serialize(FArchive& Ar)
	{
		const bool bResult = FRealtimeMesh::Serialize(Ar);

		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::SimpleMeshStoresCollisionConfig)
		{
			Ar << CollisionConfig;
			Ar << SimpleGeometry;
		}

		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::SimpleMeshStoresCustomComplexCollision)
		{
			int32 NumStreams = ComplexMeshGeometry.Num();
			Ar << NumStreams;

			if (Ar.IsLoading())
			{
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
			}
			else
			{
				ComplexMeshGeometry.ForEach([&Ar](FRealtimeMeshStream& Stream)
				{					
					FRealtimeMeshStreamKey StreamKey = Stream.GetStreamKey();
					Ar << StreamKey;
					Ar << Stream;
				});
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

			auto CollisionData = MakeShared<FRealtimeMeshCollisionData>();
			CollisionData->Config = CollisionConfig;
			CollisionData->SimpleGeometry = SimpleGeometry;
			
			auto ThisWeak = TWeakPtr<FRealtimeMeshSimple>(StaticCastSharedRef<FRealtimeMeshSimple>(this->AsShared()));

			const bool bGenerateOnOtherThread = bAsyncCook && IsInGameThread();

			auto GenerationPromise = MakeShared<TPromise<TSharedPtr<FRealtimeMeshCollisionData>>>();

			auto GenerateMeshDataLambda = [ThisWeak, GenerationPromise, CollisionData]() mutable
			{
				FRealtimeMeshTriMeshData CollisionMesh;
				if (const auto ThisShared = ThisWeak.Pin())
				{
					if (ThisShared->GenerateCollisionMesh(CollisionMesh))
					{
						CollisionData->ComplexGeometry = MoveTemp(CollisionMesh);
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

			GenerationPromise->GetFuture().Next([ThisWeak, ResultPromise = PendingCollisionPromise](const TSharedPtr<FRealtimeMeshCollisionData>& CollisionData) mutable
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
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		return LOD->CreateOrUpdateSectionGroup(SectionGroupKey);
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
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionGroupKey.LOD()))
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);
		
		LOD->CreateOrUpdateSectionGroup(Commands, SectionGroupKey);
		const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey);
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

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData)
{
	FRealtimeMeshStreamSet MeshDataCopy(MeshData);
	return CreateSectionGroup(SectionGroupKey, MoveTemp(MeshDataCopy));
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

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::SetCustomComplexMeshGeometry(FRealtimeMeshStreamSet&& InComplexMeshGeometry)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetCustomComplexMeshGeometry(MoveTemp(InComplexMeshGeometry));
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::SetCustomComplexMeshGeometry(const FRealtimeMeshStreamSet& InComplexMeshGeometry)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetCustomComplexMeshGeometry(InComplexMeshGeometry);
}

void URealtimeMeshSimple::ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshStreamSet&)> ProcessFunc) const
{
	return GetMeshAs<FRealtimeMeshSimple>()->ProcessCustomComplexMeshGeometry(ProcessFunc);
}

TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshStreamSet&)> EditFunc)
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
