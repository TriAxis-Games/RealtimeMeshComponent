// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshSimple.h"
#include "RealtimeMeshCore.h"
#include "Data/RealtimeMeshBuilder.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
#include "Logging/MessageLog.h"
#endif

#define LOCTEXT_NAMESPACE "RealtimeMeshSimple"


namespace RealtimeMesh
{
	namespace RealtimeMeshSimpleInternal
	{
		template <typename IndexType, typename TangentType, typename TexCoordType, int32 NumTexCoords>
		TUniquePtr<RealtimeMesh::FRealtimeMeshStreamSet> BuildMeshData(FName ComponentName,
		                                                               const FRealtimeMeshSimpleMeshData& MeshData, bool bRemoveDegenerates)
		{
			const bool bWantsColors = MeshData.Colors.Num() > 1 || MeshData.LinearColors.Num() > 1;
			TRealtimeMeshBuilderLocal<IndexType, TangentType, TexCoordType, NumTexCoords> Builder(bWantsColors);

			// Copy all the vertex data
			for (int32 Index = 0; Index < MeshData.Positions.Num(); Index++)
			{
				int32 VertIdx = Builder.AddVertex(FVector3f(MeshData.Positions[Index]));
				check(Index == VertIdx);

				const FVector Normal = MeshData.Normals.IsValidIndex(VertIdx) ? MeshData.Normals[VertIdx] : FVector::ZAxisVector;
				const FVector Tangent = MeshData.Tangents.IsValidIndex(VertIdx) ? MeshData.Tangents[VertIdx] : FVector::XAxisVector;
				const FVector Binormal = MeshData.Binormals.IsValidIndex(VertIdx) ? MeshData.Binormals[VertIdx] : FVector::CrossProduct(Normal, Tangent);
				Builder.SetTangents(VertIdx, FVector3f(Normal), FVector3f(Binormal), FVector3f(Tangent));

				const TArray<FVector2D>* UVs[4] = {&MeshData.UV0, &MeshData.UV1, &MeshData.UV2, &MeshData.UV3};
				for (int32 UVIdx = 0; UVIdx < NumTexCoords; UVIdx++)
				{
					Builder.SetTexCoord(VertIdx, UVIdx, UVs[UVIdx]->IsValidIndex(VertIdx) ? (*UVs[UVIdx])[VertIdx] : FVector2d::ZeroVector);
				}

				if (bWantsColors)
				{
					const FColor VertexColor = MeshData.LinearColors.Num() > 0
						                           ? (MeshData.LinearColors.IsValidIndex(VertIdx) ? MeshData.LinearColors[VertIdx].ToFColor(true) : FColor::White)
						                           : (MeshData.Colors.IsValidIndex(VertIdx) ? MeshData.Colors[VertIdx] : FColor::White);

					Builder.SetColor(VertIdx, VertexColor);
				}
			}

			int32 NumDegenerateTriangles = 0;

			// Copy the triangles
			for (int32 Index = 0; Index < MeshData.Triangles.Num(); Index += 3)
			{
				TIndex3<IndexType> Tri(MeshData.Triangles[Index + 0], MeshData.Triangles[Index + 1], MeshData.Triangles[Index + 2]);
				Tri = Tri.Clamp(0, MeshData.Positions.Num() - 1);

				if (!bRemoveDegenerates || !Tri.IsDegenerate())
				{
					Builder.AddTriangle(Tri);
				}
				else
				{
					NumDegenerateTriangles++;
				}
			}


			if (NumDegenerateTriangles > 0)
			{
				UE_LOG(LogTemp, Warning,
				       TEXT("Detected %d degenerate triangle%s with non-unique vertex indices for created mesh section in '%s'; degenerate triangles will be dropped."),
				       NumDegenerateTriangles, NumDegenerateTriangles > 1 ? TEXT("s") : TEXT(""), *ComponentName.ToString());
			}

			return MakeUnique<FRealtimeMeshStreamSet>(Builder.TakeStreamSet());
		}

		template <typename IndexType, typename TangentType, typename TexCoordType>
		inline TUniquePtr<RealtimeMesh::FRealtimeMeshStreamSet> BuildMeshData(FName ComponentName,
		                                                                      const FRealtimeMeshSimpleMeshData& MeshData, bool bRemoveDegenerates)
		{
			if (MeshData.UV3.Num() > 0)
			{
				return BuildMeshData<IndexType, TangentType, TexCoordType, 4>(ComponentName, MeshData, bRemoveDegenerates);
			}
			else if (MeshData.UV2.Num() > 0)
			{
				return BuildMeshData<IndexType, TangentType, TexCoordType, 3>(ComponentName, MeshData, bRemoveDegenerates);
			}
			else if (MeshData.UV1.Num() > 0)
			{
				return BuildMeshData<IndexType, TangentType, TexCoordType, 2>(ComponentName, MeshData, bRemoveDegenerates);
			}
			else
			{
				return BuildMeshData<IndexType, TangentType, TexCoordType, 1>(ComponentName, MeshData, bRemoveDegenerates);
			}
		}


		template <typename IndexType, typename TangentType>
		inline TUniquePtr<RealtimeMesh::FRealtimeMeshStreamSet> BuildMeshData(FName ComponentName, const FRealtimeMeshSimpleMeshData& MeshData,
		                                                                      bool bRemoveDegenerates)
		{
			if (MeshData.bUseHighPrecisionTexCoords)
			{
				return BuildMeshData<IndexType, TangentType, FVector2f>(ComponentName, MeshData, bRemoveDegenerates);
			}
			else
			{
				return BuildMeshData<IndexType, TangentType, FVector2DHalf>(ComponentName, MeshData, bRemoveDegenerates);
			}
		}

		template <typename IndexType>
		inline TUniquePtr<RealtimeMesh::FRealtimeMeshStreamSet> BuildMeshData(FName ComponentName,
		                                                                      const FRealtimeMeshSimpleMeshData& MeshData, bool bRemoveDegenerates)
		{
			if (MeshData.bUseHighPrecisionTangents)
			{
				return BuildMeshData<IndexType, FPackedRGBA16N>(ComponentName, MeshData, bRemoveDegenerates);
			}
			else
			{
				return BuildMeshData<IndexType, FPackedNormal>(ComponentName, MeshData, bRemoveDegenerates);
			}
		}

		inline TUniquePtr<RealtimeMesh::FRealtimeMeshStreamSet> BuildMeshData(FName ComponentName,
		                                                                      const FRealtimeMeshSimpleMeshData& MeshData, bool bRemoveDegenerates)
		{
			if (MeshData.Positions.Num() > TNumericLimits<uint16>::Max())
			{
				return BuildMeshData<uint32>(ComponentName, MeshData, bRemoveDegenerates);
			}
			else
			{
				return BuildMeshData<uint16>(ComponentName, MeshData, bRemoveDegenerates);
			}
		}
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
				const auto PositionStream = SectionGroup->GetStream(FRealtimeMeshStreamKey(
					ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::PositionStreamName));
				const auto TriangleStream = SectionGroup->GetStream(FRealtimeMeshStreamKey(
					ERealtimeMeshStreamType::Index, FRealtimeMeshStreams::TrianglesStreamName));

				const auto TexCoordsStream = SectionGroup->GetStream(FRealtimeMeshStreamKey(
					ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::TexCoordsStreamName));

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

						// TODO: We're only copying one UV set
						if (CollisionUVs.Num() < 1)
						{
							CollisionUVs.SetNum(1);
						}

						int32 NumRemainingTexCoords = PositionStream->Num();
						if (TexCoordsStream)
						{
							if (TexCoordsStream->GetLayout() == RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2DHalf>())
							{
								const auto TexCoordsView = TexCoordsStream->GetArrayView<FVector2DHalf>().Left(PositionStream->Num());
								CollisionUVs[0].SetNum(TexCoordsView.Num());
								for (int32 TexCoordIdx = 0; TexCoordIdx < TexCoordsView.Num(); TexCoordIdx++)
								{
									CollisionUVs[0][TexCoordIdx] = FVector2D(TexCoordsView[TexCoordIdx]);
								}
							}
							else
							{
								const auto TexCoordsView = TexCoordsStream->GetArrayView<FVector2f>().Left(PositionStream->Num());
								CollisionUVs[0].SetNum(TexCoordsView.Num());
								for (int32 TexCoordIdx = 0; TexCoordIdx < TexCoordsView.Num(); TexCoordIdx++)
								{
									CollisionUVs[0][TexCoordIdx] = FVector2D(TexCoordsView[TexCoordIdx]);
								}
							}
							NumRemainingTexCoords -= TexCoordsStream->Num();
						}

						if (NumRemainingTexCoords > 0)
						{
							CollisionUVs[0].AddZeroed(NumRemainingTexCoords);
						}

						const int32 MaterialSlot = GetConfig().MaterialSlot;
						if (TriangleStream->GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<uint16>())
						{
							const auto TrianglesView = TriangleStream->GetElementArrayView<uint16>();
							for (int32 TriIdx = 0; TriIdx < TrianglesView.Num(); TriIdx += 3)
							{
								FTriIndices& Tri = CollisionTriangles.AddDefaulted_GetRef();
								Tri.v0 = TrianglesView[TriIdx + 0] + StartVertexIndex;
								Tri.v1 = TrianglesView[TriIdx + 1] + StartVertexIndex;
								Tri.v2 = TrianglesView[TriIdx + 2] + StartVertexIndex;

								CollisionMaterials.Add(MaterialSlot);
							}
						}
						else
						{
							const auto TrianglesView = TriangleStream->GetElementArrayView<int32>();

							for (int32 TriIdx = 0; TriIdx < TrianglesView.Num(); TriIdx += 3)
							{
								FTriIndices& Tri = CollisionTriangles.AddDefaulted_GetRef();
								Tri.v0 = TrianglesView[TriIdx + 0] + StartVertexIndex;
								Tri.v1 = TrianglesView[TriIdx + 1] + StartVertexIndex;
								Tri.v2 = TrianglesView[TriIdx + 2] + StartVertexIndex;

								CollisionMaterials.Add(MaterialSlot);
							}
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

	void FRealtimeMeshSectionSimple::HandleStreamsChanged(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamKey& StreamKey,
	                                                      ERealtimeMeshChangeType ChangeType) const
	{
		if (!bShouldCreateMeshCollision || !Key.IsPartOf(SectionGroupKey))
		{
			return;
		}

		const auto PositionStreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::PositionStreamName);
		const auto TexCoordStreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::TexCoordsStreamName);
		const auto TriangleStreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, FRealtimeMeshStreams::TrianglesStreamName);

		if (StreamKey == PositionStreamKey || StreamKey == TexCoordStreamKey || StreamKey == TriangleStreamKey)
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
		if (bShouldCreateMeshCollision)
		{
			StaticCastSharedRef<FRealtimeMeshSharedResourcesSimple>(SharedResources)->BroadcastCollisionDataChanged();
		}
	}


	bool FRealtimeMeshSectionGroupSimple::HasStreams() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Streams.Num() > 0;
	}

	FRealtimeMeshDataStreamPtr FRealtimeMeshSectionGroupSimple::GetStream(FRealtimeMeshStreamKey StreamKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Streams.Contains(StreamKey) ? *Streams.Find(StreamKey) : FRealtimeMeshDataStreamPtr(nullptr);
	}

	void FRealtimeMeshSectionGroupSimple::CreateOrUpdateStream(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshDataStream&& Stream)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		// Replace the stored stream
		Streams.Remove(Stream.GetStreamKey());
		Streams.Add(MakeShared<FRealtimeMeshDataStream>(Stream));

		FRealtimeMeshSectionGroup::CreateOrUpdateStream(Commands, MoveTemp(Stream));

		if (IsStandalone() && GetStandaloneSection())
		{
			GetStandaloneSection()->UpdateStreamRange(Commands, GetStreamRange());
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroupSimple::UpdateFromSimpleMesh(const FRealtimeMeshSimpleMeshData& MeshData)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);
		UpdateFromSimpleMesh(Commands, MeshData);
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroupSimple::UpdateFromSimpleMesh(FRealtimeMeshProxyCommandBatch& Commands,
	                                                               const FRealtimeMeshSimpleMeshData& MeshData)
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

		const auto UpdateData = RealtimeMeshSimpleInternal::BuildMeshData(SharedResources->GetMeshName(), MeshData, false /*RemoveDegenerates*/);
		SetAllStreams(Commands, MoveTemp(*UpdateData.Get()));
	}

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

	void FRealtimeMeshSectionGroupSimple::InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		// We only send streams here, we rely on the base to send the sections
		for (const auto& Stream : Streams)
		{
			const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(Stream.Get());
			UpdateData->ConfigureBuffer(EBufferUsageFlags::Static, true);

			Commands.AddSectionGroupTask(Key, [UpdateData](FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.CreateOrUpdateStream(UpdateData);
			}, ShouldRecreateProxyOnStreamChange());
		}

		FRealtimeMeshSectionGroup::InitializeProxy(Commands);
	}

	void FRealtimeMeshSectionGroupSimple::Reset(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Streams.Reset();
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
					FRealtimeMeshDataStreamRef Stream = MakeShared<FRealtimeMeshDataStream>();
					Ar << (*Stream);

					Streams.Add(Stream);
				}
			}
			else
			{
				for (const auto& Stream : Streams)
				{
					FRealtimeMeshStreamKey StreamKey = Stream->GetStreamKey();
					Ar << StreamKey;
					Ar << *Stream;
				}
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

	FRealtimeMeshStreamRange FRealtimeMeshSectionGroupSimple::GetStreamRange() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		FRealtimeMeshStreamRange StreamRange;

		if (const FRealtimeMeshDataStreamRef* Stream = Streams.Find(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::PositionStreamName)))
		{
			StreamRange.Vertices = FInt32Range(0, (*Stream)->Num());
		}

		if (const FRealtimeMeshDataStreamRef* Stream = Streams.Find(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, FRealtimeMeshStreams::TrianglesStreamName)))
		{
			StreamRange.Indices = FInt32Range(0, (*Stream)->Num() * (*Stream)->GetNumElements());
		}

		return StreamRange;
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroupSimple::EditMeshData(TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshDataStreamRefSet&)> EditFunc)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		auto UpdatedStreams = EditFunc(Streams);

		for (const auto& UpdatedStream : UpdatedStreams)
		{
			if (auto Stream = Streams.Find(UpdatedStream))
			{
				FRealtimeMeshDataStream StreamCopy(Stream->Get());				
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


	bool FRealtimeMeshSimple::GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData)
	{
		// TODO: Allow other LOD to be used for collision?
		if (LODs.IsValidIndex(0))
		{
			return StaticCastSharedRef<FRealtimeMeshLODSimple>(LODs[0])->GenerateCollisionMesh(CollisionData);
		}
		return false;
	}

	void FRealtimeMeshSimple::Reset(FRealtimeMeshProxyCommandBatch& Commands, bool bRemoveRenderProxy)
	{
		FRealtimeMesh::Reset(Commands, bRemoveRenderProxy);

		// Default it back to a single LOD.
		InitializeLODs(Commands, {FRealtimeMeshLODConfig()});
	}

	bool FRealtimeMeshSimple::Serialize(FArchive& Ar)
	{
		const bool bResult = FRealtimeMesh::Serialize(Ar);

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

FRealtimeMeshSimpleMeshData URealtimeMeshSimpleBlueprintFunctionLibrary::MakeRealtimeMeshSimpleStream(const TArray<int32>& Triangles, const TArray<FVector>& Positions,
                                                                                                      const TArray<FVector>& Normals, const TArray<FVector>& Tangents,
                                                                                                      const TArray<FVector>& Binormals, const TArray<FLinearColor>& LinearColors,
                                                                                                      const TArray<FVector2D>& UV0,
                                                                                                      const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2,
                                                                                                      const TArray<FVector2D>& UV3, const TArray<FColor>& Colors,
                                                                                                      bool bUseHighPrecisionTangents,
                                                                                                      bool bUseHighPrecisionTexCoords)
{
	FRealtimeMeshSimpleMeshData NewMeshData;
	NewMeshData.Triangles = Triangles;
	NewMeshData.Positions = Positions;
	NewMeshData.Normals = Normals;
	NewMeshData.Tangents = Tangents;
	NewMeshData.Binormals = Binormals;
	NewMeshData.LinearColors = LinearColors;
	NewMeshData.UV0 = UV0;
	NewMeshData.UV1 = UV1;
	NewMeshData.UV2 = UV2;
	NewMeshData.UV3 = UV3;
	NewMeshData.Colors = Colors;
	NewMeshData.bUseHighPrecisionTangents = bUseHighPrecisionTangents;
	NewMeshData.bUseHighPrecisionTexCoords = bUseHighPrecisionTexCoords;
	return NewMeshData;
}

void URealtimeMeshSimple::Reset(bool bCreateNewMeshData)
{
	Super::Reset(bCreateNewMeshData);
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
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(
	const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData)
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

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData)
{
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData)
{
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateSection(const FRealtimeMeshSectionKey& SectionKey,
                                                                           const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange,
                                                                           bool bShouldCreateCollision)
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
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
                                                                                     const FRealtimeMeshSimpleMeshData& MeshData, bool bShouldCreateCollision)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionKey.LOD()))
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);

		LOD->CreateOrUpdateSectionGroup(Commands, SectionKey.SectionGroup());
		const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionKey.SectionGroup());
		SectionGroup->UpdateFromSimpleMesh(Commands, MeshData);
		SectionGroup->FlagStandalone();

		SectionGroup->CreateOrUpdateSection(Commands, SectionKey, Config, SectionGroup->GetStreamRange());
		const auto Section = SectionGroup->GetSectionAs<FRealtimeMeshSectionSimple>(SectionKey);
		Section->SetShouldCreateCollision(bShouldCreateCollision);

		return Commands.Commit();
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSection_InvalidLODKey", "CreateSection: Invalid LODKey key {0}"),
			              FText::FromString(SectionKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
	FRealtimeMeshStreamSet&& MeshData, bool bShouldCreateCollision)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionKey.LOD()))
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
		FRealtimeMeshProxyCommandBatch Commands(SharedResources);

		LOD->CreateOrUpdateSectionGroup(Commands, SectionKey.SectionGroup());
		const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionKey.SectionGroup());
		SectionGroup->SetAllStreams(Commands, MoveTemp(MeshData));
		SectionGroup->FlagStandalone();

		SectionGroup->CreateOrUpdateSection(Commands, SectionKey, Config, SectionGroup->GetStreamRange());
		const auto Section = SectionGroup->GetSectionAs<FRealtimeMeshSectionSimple>(SectionKey);
		Section->SetShouldCreateCollision(bShouldCreateCollision);

		return Commands.Commit();
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSection_InvalidLODKey", "CreateSection: Invalid LODKey key {0}"),
						  FText::FromString(SectionKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();	
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::CreateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
	const FRealtimeMeshStreamSet& MeshData, bool bShouldCreateCollision)
{
	FRealtimeMeshStreamSet Copy(MeshData);
	return CreateStandaloneSection(SectionKey, Config, MoveTemp(Copy), bShouldCreateCollision);
}

// ReSharper disable once CppMemberFunctionMayBeConst
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

/*// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSimpleMeshData& MeshData)
{
	if (const auto LOD = GetMesh()->GetLODAs<FRealtimeMeshLODSimple>(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSectionAs<FRealtimeMeshSectionSimple>(SectionKey))
			{
				FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources.ToSharedRef());
				FRealtimeMeshProxyCommandBatch Commands(SharedResources);

				SectionGroup->UpdateFromSimpleMesh(Commands, MeshData);
				Section->UpdateStreamRange(Commands, SectionGroup->GetStreamRange());

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
				FText::Format(LOCTEXT("CreateSection_InvalidSectionGroupKey", "CreateSection: Invalid SectionGroupKey key {0}"),
				              FText::FromString(SectionKey.SectionGroup().ToString())));
		}
	}
	else
	{
		FMessageLog("RealtimeMesh").Error(
			FText::Format(LOCTEXT("CreateSectionGroup_InvalidLODKey", "CreateSectionGroup: Invalid LODKey key {0}"),
			              FText::FromString(SectionKey.LOD().ToString())));
	}
	return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, FRealtimeMeshStreamSet&& MeshData)
{
}

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamSet& MeshData)
{
}*/

TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::EditMeshInPlace(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshDataStreamRefSet&)> EditFunc)
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

FRealtimeMeshSectionKey URealtimeMeshSimple::CreateMeshSection(const FRealtimeMeshLODKey& LODKey, const FRealtimeMeshSectionConfig& Config,
                                                               const FRealtimeMeshSimpleMeshData& MeshData, bool bShouldCreateCollision)
{
	const FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(LODKey);
	const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateUnique(SectionGroupKey);
	CreateStandaloneSection(SectionKey, Config, MeshData, bShouldCreateCollision);
	return SectionKey;
}

FRealtimeMeshSectionGroupKey URealtimeMeshSimple::CreateSectionGroup(const FRealtimeMeshLODKey& LODKey, const FRealtimeMeshSimpleMeshData& MeshData)
{
	const FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(LODKey);
	CreateSectionGroup(SectionGroupKey, MeshData);
	return SectionGroupKey;
}

FRealtimeMeshSectionKey URealtimeMeshSimple::CreateSection(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionConfig& Config,
                                                           const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision)
{
	const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateUnique(SectionGroupKey);
	CreateSection(SectionKey, Config, StreamRange, bShouldCreateCollision);
	return SectionKey;
}


void URealtimeMeshSimple::CreateSectionGroup_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	CreateSectionGroup(SectionGroupKey).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}

FRealtimeMeshSectionGroupKey URealtimeMeshSimple::CreateSectionGroupUnique_Blueprint(const FRealtimeMeshLODKey& LODKey, FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	const FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(LODKey);
	CreateSectionGroup(SectionGroupKey).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
	return SectionGroupKey;
}


void URealtimeMeshSimple::CreateSectionGroupWithSimpleMesh_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey,
                                                                     const FRealtimeMeshSimpleMeshData& MeshData, FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	CreateSectionGroup(SectionGroupKey, MeshData).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}

FRealtimeMeshSectionGroupKey URealtimeMeshSimple::CreateSectionGroupUniqueWithSimpleMesh_Blueprint(const FRealtimeMeshLODKey& LODKey,
                                                                                                   const FRealtimeMeshSimpleMeshData& MeshData,
                                                                                                   FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	const FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(LODKey);
	CreateSectionGroup(SectionGroupKey, MeshData).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
	return SectionGroupKey;
}

void URealtimeMeshSimple::CreateSection_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshStreamRange& StreamRange,
                                                  bool bShouldCreateCollision, FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	CreateSection(SectionKey, Config, StreamRange, bShouldCreateCollision).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}

FRealtimeMeshSectionKey URealtimeMeshSimple::CreateSectionUnique_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionConfig& Config,
                                                                           const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision,
                                                                           FRealtimeMeshSimpleCompletionCallback CompletionCallback)
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

void URealtimeMeshSimple::CreateStandaloneSectionWithSimpleMesh_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
                                                                          const FRealtimeMeshSimpleMeshData& MeshData, bool bShouldCreateCollision,
                                                                          FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	CreateStandaloneSection(SectionKey, Config, MeshData, bShouldCreateCollision).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}

FRealtimeMeshSectionKey URealtimeMeshSimple::CreateStandaloneSectionUniqueWithSimpleMesh_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey,
                                                                                                   const FRealtimeMeshSectionConfig& Config,
                                                                                                   const FRealtimeMeshSimpleMeshData& MeshData, bool bShouldCreateCollision,
                                                                                                   FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateUnique(SectionGroupKey);
	CreateStandaloneSection(SectionKey, Config, MeshData, bShouldCreateCollision).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
	return SectionKey;
}


void URealtimeMeshSimple::UpdateSectionGroupWithSimpleMesh_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData,
                                                                     FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	UpdateSectionGroup(SectionGroupKey, MeshData).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}


void URealtimeMeshSimple::UpdateStandaloneSectionWithSimpleMesh_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSimpleMeshData& MeshData,
                                                                          FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
	UpdateSectionGroup(SectionKey, MeshData).Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
	{
		if (CompletionCallback.IsBound())
		{
			CompletionCallback.Execute(Status);
		}
	});
}

void URealtimeMeshSimple::UpdateSectionSegment_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange,
	FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
    UpdateSectionSegment(SectionKey, StreamRange)
        .Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
        {
            if (CompletionCallback.IsBound())
            {
                CompletionCallback.Execute(Status);
            }
        });
}

void URealtimeMeshSimple::UpdateSectionConfig_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
	FRealtimeMeshSimpleCompletionCallback CompletionCallback)
{
    UpdateSectionConfig(SectionKey, Config)
        .Next([CompletionCallback](ERealtimeMeshProxyUpdateStatus Status)
        {
            if (CompletionCallback.IsBound())
            {
                CompletionCallback.Execute(Status);
            }
        });
}

void URealtimeMeshSimple::SetSectionVisibility_Blueprint(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible, FRealtimeMeshSimpleCompletionCallback CompletionCallback)
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

void URealtimeMeshSimple::SetSectionCastShadow_Blueprint(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow, FRealtimeMeshSimpleCompletionCallback CompletionCallback)
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

void URealtimeMeshSimple::RemoveSection_Blueprint(const FRealtimeMeshSectionKey& SectionKey, FRealtimeMeshSimpleCompletionCallback CompletionCallback)
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

void URealtimeMeshSimple::RemoveSectionGroup_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshSimpleCompletionCallback CompletionCallback)
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

void URealtimeMeshSimple::SetCollisionConfig_Blueprint(const FRealtimeMeshCollisionConfiguration& InCollisionConfig, FRealtimeMeshSimpleCollisionCompletionCallback CompletionCallback)
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

void URealtimeMeshSimple::SetSimpleGeometry_Blueprint(const FRealtimeMeshSimpleGeometry& InSimpleGeometry, FRealtimeMeshSimpleCollisionCompletionCallback CompletionCallback)
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

FRealtimeMeshSimpleGeometry URealtimeMeshSimple::GetSimpleGeometry() const
{
	return GetMeshAs<FRealtimeMeshSimple>()->GetSimpleGeometry();
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshCollisionUpdateResult> URealtimeMeshSimple::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry)
{
	return GetMeshAs<FRealtimeMeshSimple>()->SetSimpleGeometry(InSimpleGeometry);
}

void URealtimeMeshSimple::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	StaticCastSharedPtr<FRealtimeMeshSimple>(MeshRef)->MarkCollisionDirtyNoCallback();
}

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionSegment(const FRealtimeMeshSectionKey& SectionKey,
                                                                                  const FRealtimeMeshStreamRange& StreamRange)
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

// ReSharper disable once CppMemberFunctionMayBeConst
TFuture<ERealtimeMeshProxyUpdateStatus> URealtimeMeshSimple::UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config)
{
	if (const auto LOD = GetMesh()->GetLOD(SectionKey.LOD()))
	{
		if (const auto SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
		{
			if (const auto Section = SectionGroup->GetSection(SectionKey))
			{
				return Section->UpdateConfig(Config);
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


#undef LOCTEXT_NAMESPACE
