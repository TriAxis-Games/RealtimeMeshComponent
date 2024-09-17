// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshCollisionLibrary.h"

#include "RealtimeMeshComponent.h"
#include "Core/RealtimeMeshBuilder.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"


bool URealtimeMeshCollisionTools::FindCollisionUVRealtimeMesh(const FHitResult& Hit, int32 UVChannel, FVector2D& UV)
{
	bool bSuccess = false;
	
	if (UPhysicsSettings::Get()->bSupportUVFromHitResults)
	{
		if (URealtimeMeshComponent* HitComp = Cast<URealtimeMeshComponent>(Hit.Component.Get()))
		{
			const UBodySetup* BodySetup = HitComp->GetBodySetup();
			const URealtimeMesh* RealtimeMesh = HitComp->GetRealtimeMesh();
			if (BodySetup && RealtimeMesh)
			{
				const FVector LocalHitPos = HitComp->GetComponentToWorld().InverseTransformPosition(Hit.Location);
				bSuccess = RealtimeMesh->CalcTexCoordAtLocation(LocalHitPos, Hit.ElementIndex, Hit.FaceIndex, UVChannel, UV);
			}
		}
	}

	return bSuccess;
}

void URealtimeMeshCollisionTools::CookConvexHull(FRealtimeMeshCollisionConvex& ConvexHull)
{
	if (ConvexHull.HasCookedMesh())
	{
		return;
	}
	
	using namespace Chaos;
	auto BuildConvexFromVerts = [&ConvexHull](const bool bMirrored)
	{
		const int32 NumHullVerts = ConvexHull.Vertices.Num();
		if(NumHullVerts == 0)
		{
#if RMC_ENGINE_ABOVE_5_4
			Chaos::FConvexPtr(nullptr);
#else
			return TSharedPtr<FConvex>(nullptr);
#endif
		}

		// Calculate the margin to apply to the convex - it depends on overall dimensions
		FAABB3 Bounds = FAABB3::EmptyAABB();
		for(int32 VertIndex = 0; VertIndex < NumHullVerts; ++VertIndex)
		{
			const FVector& HullVert = ConvexHull.Vertices[VertIndex];
			Bounds.GrowToInclude(HullVert);
		}

		// Create the corner vertices for the convex
		TArray<FConvex::FVec3Type> ConvexVertices;
		ConvexVertices.SetNumZeroed(NumHullVerts);

		for(int32 VertIndex = 0; VertIndex < NumHullVerts; ++VertIndex)
		{
			const FVector& HullVert = ConvexHull.Vertices[VertIndex];
			ConvexVertices[VertIndex] = FConvex::FVec3Type(bMirrored ? -HullVert.X : HullVert.X, HullVert.Y, HullVert.Z);
		}

		// Margin is always zero on convex shapes - they are intended to be instanced
		
#if RMC_ENGINE_ABOVE_5_4
		return Chaos::FConvexPtr(new FConvex(ConvexVertices, 0.0f));
#else
		return MakeShared<FConvex>(ConvexVertices, 0.0f).ToSharedPtr();
#endif
	};

	auto NonMirrored = BuildConvexFromVerts(false);
	ConvexHull.Cooked = MakeShared<FRealtimeMeshCookedConvexMeshData>(NonMirrored);
}

void URealtimeMeshCollisionTools::CookComplexMesh(FRealtimeMeshCollisionMesh& CollisionMesh)
{
	constexpr bool EnableMeshClean = false;
	
	if(CollisionMesh.Vertices.Num() == 0)
	{
		CollisionMesh.Cooked = MakeShared<FRealtimeMeshCookedTriMeshData>();
	}

	TArray<FVector3f> FinalVerts = CollisionMesh.Vertices;

	// Push indices into one flat array
	TArray<int32> FinalIndices;
	FinalIndices.Reserve(CollisionMesh.Triangles.Num() * 3);
	for(const RealtimeMesh::TIndex3<int32>& Tri : CollisionMesh.Triangles)
	{
		// NOTE: This is where the Winding order of the triangles are changed to be consistent throughout the rest of the physics engine
		// After this point we should have clockwise (CW) winding in left handed (LH) coordinates (or equivalently CCW in RH)
		// This is the opposite convention followed in most of the unreal engine
		FinalIndices.Add(CollisionMesh.bFlipNormals ? Tri.V1 : Tri.V0);
		FinalIndices.Add(CollisionMesh.bFlipNormals ? Tri.V0 : Tri.V1);
		FinalIndices.Add(Tri.V2);
	}

	/*if(EnableMeshClean)
	{
		Chaos::CleanTrimesh(FinalVerts, FinalIndices, &OutFaceRemap, &OutVertexRemap);
	}*/

	// Build particle list #BG Maybe allow TParticles to copy vectors?
	Chaos::FTriangleMeshImplicitObject::ParticlesType TriMeshParticles;
	TriMeshParticles.AddParticles(FinalVerts.Num());

	const int32 NumVerts = FinalVerts.Num();
	for(int32 VertIndex = 0; VertIndex < NumVerts; ++VertIndex)
	{
#if RMC_ENGINE_ABOVE_5_4
		TriMeshParticles.SetX(VertIndex, FinalVerts[VertIndex]);
#else
		TriMeshParticles.X(VertIndex) = FinalVerts[VertIndex];
#endif
	}

	TArray<int32> OutVertexRemap;
	TArray<int32> OutFaceRemap;

	// Build chaos triangle list. #BGTODO Just make the clean function take these types instead of double copying
	auto LambdaHelper = [&CollisionMesh, &FinalVerts, &FinalIndices, &TriMeshParticles, &OutFaceRemap, &OutVertexRemap](auto& Triangles)
	{
		const int32 NumTriangles = FinalIndices.Num() / 3;
		bool bHasMaterials = CollisionMesh.Materials.Num() > 0;
		TArray<uint16> MaterialIndices;

		if(bHasMaterials)
		{
			MaterialIndices.Reserve(NumTriangles);
		}

		// Need to rebuild face remap array, in case there are any invalid triangles
		TArray<int32> OldFaceRemap = MoveTemp(OutFaceRemap);
		OutFaceRemap.Reserve(OldFaceRemap.Num());

		Triangles.Reserve(NumTriangles);
		for(int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
		{
			// Only add this triangle if it is valid
			const int32 BaseIndex = TriangleIndex * 3;
			const bool bIsValidTriangle = Chaos::FConvexBuilder::IsValidTriangle(
				FinalVerts[FinalIndices[BaseIndex]],
				FinalVerts[FinalIndices[BaseIndex + 1]],
				FinalVerts[FinalIndices[BaseIndex + 2]]);

			// TODO: Figure out a proper way to handle this. Could these edges get sewn together? Is this important?
			//if (ensureMsgf(bIsValidTriangle, TEXT("FChaosDerivedDataCooker::BuildTriangleMeshes(): Trimesh attempted cooked with invalid triangle!")));
			if(bIsValidTriangle)
			{
				Triangles.Add(Chaos::TVector<int32, 3>(FinalIndices[BaseIndex], FinalIndices[BaseIndex + 1], FinalIndices[BaseIndex + 2]));
				OutFaceRemap.Add(OldFaceRemap.IsEmpty()? TriangleIndex : OldFaceRemap[TriangleIndex]);

				if(bHasMaterials)
				{
					/*if(EnableMeshClean)
					{
						if(!ensure(OldFaceRemap.IsValidIndex(TriangleIndex)))
						{
							MaterialIndices.Empty();
							bHasMaterials = false;
						}
						else
						{
							const int32 OriginalIndex = OldFaceRemap[TriangleIndex];

							if(ensure(Materials.IsValidIndex(OriginalIndex)))
							{
								MaterialIndices.Add(Materials[OriginalIndex]);
							}
							else
							{
								MaterialIndices.Empty();
								bHasMaterials = false;
							}
						}
					}
					else*/
					{
						if(ensure(CollisionMesh.Materials.IsValidIndex(TriangleIndex)))
						{
							MaterialIndices.Add(CollisionMesh.Materials[TriangleIndex]);
						}
						else
						{
							MaterialIndices.Empty();
							bHasMaterials = false;
						}
					}
				}
			}
		}

		TUniquePtr<TArray<int32>> OutFaceRemapPtr = MakeUnique<TArray<int32>>(OutFaceRemap);
		TUniquePtr<TArray<int32>> OutVertexRemapPtr = Chaos::TriMeshPerPolySupport ? MakeUnique<TArray<int32>>(OutVertexRemap) : nullptr;
		
		
#if RMC_ENGINE_ABOVE_5_4
		auto* RawMesh = new Chaos::FTriangleMeshImplicitObject(MoveTemp(TriMeshParticles), MoveTemp(Triangles), MoveTemp(MaterialIndices), MoveTemp(OutFaceRemapPtr), MoveTemp(OutVertexRemapPtr));
		auto CookedMesh = Chaos::FTriangleMeshImplicitObjectPtr(RawMesh);
#else
		TSharedPtr<Chaos::FTriangleMeshImplicitObject> CookedMesh = MakeShared<Chaos::FTriangleMeshImplicitObject>(MoveTemp(TriMeshParticles), MoveTemp(Triangles), MoveTemp(MaterialIndices), MoveTemp(OutFaceRemapPtr), MoveTemp(OutVertexRemapPtr));
#endif
		
		// Propagate remapped indices from the FTriangleMeshImplicitObject back to the remap array
		//const auto& TriangleMeshRef = *CookedMesh.GetReference();
		for (int32 TriangleIndex = 0; TriangleIndex < OutFaceRemap.Num(); TriangleIndex++)
		{
			OutFaceRemap[TriangleIndex] = CookedMesh->GetExternalFaceIndexFromInternal(TriangleIndex);
		}
		
		FRealtimeMeshCollisionMeshCookedUVData UVInfo;
		if (UPhysicsSettings::Get()->bSupportUVFromHitResults)
		{
			UVInfo.FillFromTriMesh(CollisionMesh);
		}
		
		CollisionMesh.Cooked = MakeShared<FRealtimeMeshCookedTriMeshData>(CookedMesh,
			MoveTemp(OutVertexRemap), MoveTemp(OutFaceRemap), MoveTemp(UVInfo));
	};

	if(FinalVerts.Num() < TNumericLimits<uint16>::Max())
	{
		TArray<Chaos::TVector<uint16, 3>> TrianglesSmallIdx;
		LambdaHelper(TrianglesSmallIdx);
	}
	else
	{
		TArray<Chaos::TVector<int32, 3>> TrianglesLargeIdx;
		LambdaHelper(TrianglesLargeIdx);
	}	
}

void URealtimeMeshCollisionTools::CopySimpleGeometryToBodySetup(const FRealtimeMeshSimpleGeometry& SimpleGeom, UBodySetup* BodySetup)
{
	for (const auto& Sphere : SimpleGeom.Spheres)
	{
		auto& BodySphere = BodySetup->AggGeom.SphereElems.AddDefaulted_GetRef();
		BodySphere.Radius = Sphere.Radius;
		BodySphere.Center = Sphere.Center;
		BodySphere.SetContributeToMass(Sphere.bContributesToMass);
		BodySphere.SetName(Sphere.Name);
	}

	for (const auto& Box : SimpleGeom.Boxes)
	{
		auto& BodyBox = BodySetup->AggGeom.BoxElems.AddDefaulted_GetRef();
		BodyBox.X = Box.Extents.X;
		BodyBox.Y = Box.Extents.Y;
		BodyBox.Z = Box.Extents.Z;
		BodyBox.Center = Box.Center;
		BodyBox.Rotation = Box.Rotation;
		BodyBox.SetContributeToMass(Box.bContributesToMass);
		BodyBox.SetName(Box.Name);
	}

	for (const auto& Capsule : SimpleGeom.Capsules)
	{
		auto& BodyCapsule = BodySetup->AggGeom.SphylElems.AddDefaulted_GetRef();
		BodyCapsule.Radius = Capsule.Radius;
		BodyCapsule.Length = Capsule.Length;
		BodyCapsule.Center = Capsule.Center;
		BodyCapsule.Rotation = Capsule.Rotation;
		BodyCapsule.SetContributeToMass(Capsule.bContributesToMass);
		BodyCapsule.SetName(Capsule.Name);
	}

	for (const auto& TaperedCapsule : SimpleGeom.TaperedCapsules)
	{
		auto& BodyTaperedCapsule = BodySetup->AggGeom.TaperedCapsuleElems.AddDefaulted_GetRef();
		BodyTaperedCapsule.Radius0 = TaperedCapsule.RadiusA;
		BodyTaperedCapsule.Radius1 = TaperedCapsule.RadiusB;
		BodyTaperedCapsule.Length = TaperedCapsule.Length;
		BodyTaperedCapsule.Center = TaperedCapsule.Center;
		BodyTaperedCapsule.Rotation = TaperedCapsule.Rotation;
		BodyTaperedCapsule.SetContributeToMass(TaperedCapsule.bContributesToMass);
		BodyTaperedCapsule.SetName(TaperedCapsule.Name);
	}

	for (const auto& Convex : SimpleGeom.ConvexHulls)
	{
		auto& BodyConvex = BodySetup->AggGeom.ConvexElems.AddDefaulted_GetRef();
		BodyConvex.VertexData = Convex.GetVertices();
		BodyConvex.SetTransform(FTransform(Convex.Rotation.Quaternion(), Convex.Center));
		BodyConvex.UpdateElemBox();
		BodyConvex.SetContributeToMass(Convex.bContributesToMass);
		BodyConvex.SetName(Convex.Name);

		if (Convex.HasCookedMesh())
		{
			const auto CookedMeshData = Convex.GetCooked();
			auto MeshData = CookedMeshData->GetNonMirrored();
#if RMC_ENGINE_ABOVE_5_4
			BodyConvex.SetConvexMeshObject(MoveTemp(MeshData));
#else
			BodyConvex.SetChaosConvexMesh(MoveTemp(MeshData));
#endif
		}
	}
}

void URealtimeMeshCollisionTools::CopyComplexGeometryToBodySetup(const FRealtimeMeshComplexGeometry& ComplexGeom, UBodySetup* BodySetup, TArray<FRealtimeMeshCollisionMeshCookedUVData>& OutUVData)
{
	for (const auto& Mesh : ComplexGeom.Meshes)
	{
		if (Mesh.HasCookedMesh())
		{
			const auto CookedMeshData = Mesh.GetCooked();
			auto MeshData = CookedMeshData->GetMesh();

#if RMC_ENGINE_ABOVE_5_4
			BodySetup->TriMeshGeometries.Add(MeshData);
#else
			BodySetup->ChaosTriMeshes.Add(MeshData);
#endif
			/*NewBodySetup->bSupportUVsAndFaceRemap;
				NewBodySetup->FaceRemap = PendingCollisionUpdate->TriMeshData.Cook();
				NewBodySetup->UVInfo;*/
			BodySetup->bCreatedPhysicsMeshes = true;

			OutUVData.Add(CookedMeshData->GetUVInfo());
		}
	}	
}

bool URealtimeMeshCollisionTools::AppendStreamsToCollisionMesh(FRealtimeMeshCollisionMesh& CollisionMesh, const RealtimeMesh::FRealtimeMeshStreamSet& Streams, int32 MaterialIndex)
{
	using namespace RealtimeMesh;

	const auto PositionStream = Streams.Find(FRealtimeMeshStreams::Position);
	const auto TriangleStream = Streams.Find(FRealtimeMeshStreams::Triangles);
	const auto TexCoordsStream = Streams.Find(FRealtimeMeshStreamKey(FRealtimeMeshStreams::TexCoords));

	if (!PositionStream || !TriangleStream)
	{
		UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Unable to append collision vertices: missing position or triangle stream."));
		return false;
	}

	if (PositionStream->Num() < 3 || (TriangleStream->Num() * TriangleStream->GetNumElements()) < 3)
	{
		UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Unable to append collision vertices: not enough elements in streams."));
		return false;
	}

	if (!PositionStream->CanConvertTo<FVector3f>())
	{
		UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Unable to append collision vertices: position stream not convertible to FVector3f"));
		return false;
	}

	CollisionMesh.ReleaseCooked();
	
	const int32 StartVertexIndex = CollisionMesh.Vertices.Num();

	// Copy in the vertices
	PositionStream->CopyTo(CollisionMesh.Vertices);

	if (UPhysicsSettings::Get()->bSupportUVFromHitResults)
	{
		if (TexCoordsStream)
		{
			// We use the max number of UV channels across the appended data.
			// We simply zero-fill the unallocated channel sections.
			const int32 NumTexCoordStreams = FMath::Max(CollisionMesh.TexCoords.Num(), TexCoordsStream->GetNumElements());		
			CollisionMesh.TexCoords.SetNum(NumTexCoordStreams);
			for (int32 Index = 0; Index < NumTexCoordStreams; Index++)
			{
				CollisionMesh.TexCoords[Index].SetNumZeroed(CollisionMesh.Vertices.Num());
			}
			
			for (int32 ChannelIndex = 0; ChannelIndex < TexCoordsStream->GetNumElements(); ChannelIndex++)
			{
				TRealtimeMeshStridedStreamBuilder<const FVector2f, void> UVData(*TexCoordsStream, ChannelIndex);
				const int32 NumUVsToCopy = FMath::Min(UVData.Num(), PositionStream->Num());
				auto& CollisionUVChannel = CollisionMesh.TexCoords[ChannelIndex];
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
			// Zero fill data to vertex data length

			if (CollisionMesh.TexCoords.Num() < 1)
			{
				CollisionMesh.TexCoords.SetNum(1);
			}
			for (int32 Index = 0; Index < CollisionMesh.TexCoords.Num(); Index++)
			{
				CollisionMesh.TexCoords[Index].SetNumZeroed(CollisionMesh.Vertices.Num());
			}
		}
	}

	TRealtimeMeshStreamBuilder<const TIndex3<uint32>, void> TrianglesData(*TriangleStream);
	CollisionMesh.Triangles.Reserve(CollisionMesh.Triangles.Num() + TrianglesData.Num());
	CollisionMesh.Materials.Reserve(CollisionMesh.Triangles.Num());

	for (int32 TriIdx = 0; TriIdx < TrianglesData.Num(); TriIdx++)
	{
		TIndex3<int32>& Tri = CollisionMesh.Triangles.AddDefaulted_GetRef();
		Tri.V0 = TrianglesData[TriIdx].GetElement(0).GetValue() + StartVertexIndex;
		Tri.V1 = TrianglesData[TriIdx].GetElement(1).GetValue() + StartVertexIndex;
		Tri.V2 = TrianglesData[TriIdx].GetElement(2).GetValue() + StartVertexIndex;

		CollisionMesh.Materials.Add(MaterialIndex);
	}
	
	return true;
}

bool URealtimeMeshCollisionTools::AppendStreamsToCollisionMesh(FRealtimeMeshCollisionMesh& CollisionMesh, const RealtimeMesh::FRealtimeMeshStreamSet& Streams, int32 MaterialIndex,
	int32 FirstTriangle, int32 TriangleCount)
{
	using namespace RealtimeMesh;

	const auto PositionStream = Streams.Find(FRealtimeMeshStreams::Position);
	const auto TriangleStream = Streams.Find(FRealtimeMeshStreams::Triangles);
	const auto TexCoordsStream = Streams.Find(FRealtimeMeshStreamKey(FRealtimeMeshStreams::TexCoords));

	if (!PositionStream || !TriangleStream)
	{
		UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Unable to append collision vertices: missing position or triangle stream."));
		return false;
	}

	if (PositionStream->Num() < 3 || (TriangleStream->Num() * TriangleStream->GetNumElements()) < 3)
	{
		UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Unable to append collision vertices: not enough elements in streams."));
		return false;
	}

	if (!PositionStream->CanConvertTo<FVector3f>())
	{
		UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Unable to append collision vertices: position stream not convertible to FVector3f"));
		return false;
	}

	CollisionMesh.ReleaseCooked();
	
	TMap<uint32, uint32> VertexRemap;
	const int32 OriginalVertexCount = CollisionMesh.Vertices.Num();
	
	TRealtimeMeshStreamBuilder<const TIndex3<uint32>, void> TrianglesData(*TriangleStream);
	CollisionMesh.Triangles.Reserve(CollisionMesh.Triangles.Num() + TriangleCount);
	CollisionMesh.Materials.Reserve(CollisionMesh.Triangles.Num() + TriangleCount);

	// Make sure materials is same length as triangles
	CollisionMesh.Materials.SetNumZeroed(CollisionMesh.Triangles.Num());


	int32 NewVertexIndex = OriginalVertexCount;
	const auto RemapVertex = [&](int32 Index)
	{
		if (const auto* Found = VertexRemap.Find(Index))
		{
			return *Found;
		}

		return VertexRemap.FindOrAdd(Index) = NewVertexIndex++;		
	};
	
	for (int32 TriIdx = 0; TriIdx < TriangleCount; TriIdx++)
	{
		TIndex3<int32>& Tri = CollisionMesh.Triangles.AddDefaulted_GetRef();
		Tri.V0 = RemapVertex(TrianglesData[TriIdx + FirstTriangle].GetElement(0).GetValue());
		Tri.V1 = RemapVertex(TrianglesData[TriIdx + FirstTriangle].GetElement(1).GetValue());
		Tri.V2 = RemapVertex(TrianglesData[TriIdx + FirstTriangle].GetElement(2).GetValue());

		CollisionMesh.Materials.Add(MaterialIndex);
	}	
	

	// Copy in the vertices
	CollisionMesh.Vertices.SetNum(OriginalVertexCount + VertexRemap.Num());
	if (PositionStream->IsOfType<FVector3f>())
	{
		TRealtimeMeshStreamBuilder<const FVector3f, FVector3f> Positions(*PositionStream);
		for (const auto& Pair : VertexRemap)
		{
			checkSlow(Pair.Value >= OriginalVertexCount && Pair.Value < CurrentVertexIndex);
			
			CollisionMesh.Vertices[Pair.Value] = Positions[Pair.Key % Positions.Num()];
		}
	}
	else
	{
		TRealtimeMeshStreamBuilder<const FVector3f, void> Positions(*PositionStream);
		for (const auto& Pair : VertexRemap)
		{
			checkSlow(Pair.Value >= OriginalVertexCount && Pair.Value < CurrentVertexIndex);
			
			CollisionMesh.Vertices[Pair.Value] = Positions[Pair.Key % Positions.Num()];
		}	
	}

	if (UPhysicsSettings::Get()->bSupportUVFromHitResults)
	{
		if (TexCoordsStream)
		{
			// We use the max number of UV channels across the appended data.
			// We simply zero-fill the unallocated channel sections.
			const int32 NumTexCoordStreams = FMath::Max(CollisionMesh.TexCoords.Num(), TexCoordsStream->GetNumElements());		
			CollisionMesh.TexCoords.SetNum(NumTexCoordStreams);
			for (int32 Index = 0; Index < NumTexCoordStreams; Index++)
			{
				CollisionMesh.TexCoords[Index].SetNumZeroed(CollisionMesh.Vertices.Num());
			}
			
			for (int32 ChannelIndex = 0; ChannelIndex < TexCoordsStream->GetNumElements(); ChannelIndex++)
			{				
				if (TexCoordsStream->GetElementType() == GetRealtimeMeshDataElementType<FVector2f>())
				{
					TRealtimeMeshStridedStreamBuilder<const FVector2f, FVector2f> UVData(*TexCoordsStream, ChannelIndex);
					auto& CollisionUVChannel = CollisionMesh.TexCoords[ChannelIndex];
					CollisionUVChannel.SetNumUninitialized(OriginalVertexCount + FMath::Min(VertexRemap.Num(), UVData.Num()));

					for (const auto& Pair : VertexRemap)
					{
						checkSlow(Pair.Value >= OriginalVertexCount && Pair.Value < CurrentVertexIndex);
			
						CollisionUVChannel[Pair.Value] = UVData[Pair.Key % UVData.Num()];
					}

					// Make sure the uv data is the same length as the position data
					if (PositionStream->Num() > UVData.Num())
					{
						CollisionUVChannel.SetNumZeroed(PositionStream->Num());
					}
				}
				else if (TexCoordsStream->GetElementType() == GetRealtimeMeshDataElementType<FVector2DHalf>())
				{
					TRealtimeMeshStridedStreamBuilder<const FVector2f, FVector2DHalf> UVData(*TexCoordsStream, ChannelIndex);
					auto& CollisionUVChannel = CollisionMesh.TexCoords[ChannelIndex];
					CollisionUVChannel.SetNumUninitialized(OriginalVertexCount + FMath::Min(VertexRemap.Num(), UVData.Num()));

					for (const auto& Pair : VertexRemap)
					{
						checkSlow(Pair.Value >= OriginalVertexCount && Pair.Value < CurrentVertexIndex);
			
						CollisionUVChannel[Pair.Value] = UVData[Pair.Key % UVData.Num()];
					}

					// Make sure the uv data is the same length as the position data
					if (PositionStream->Num() > UVData.Num())
					{
						CollisionUVChannel.SetNumZeroed(PositionStream->Num());
					}
				}
				else
				{
					TRealtimeMeshStridedStreamBuilder<const FVector2f, void> UVData(*TexCoordsStream, ChannelIndex);
					auto& CollisionUVChannel = CollisionMesh.TexCoords[ChannelIndex];
					CollisionUVChannel.SetNumUninitialized(OriginalVertexCount + FMath::Min(VertexRemap.Num(), UVData.Num()));

					for (const auto& Pair : VertexRemap)
					{
						checkSlow(Pair.Value >= OriginalVertexCount && Pair.Value < CurrentVertexIndex);
			
						CollisionUVChannel[Pair.Value] = UVData[Pair.Key % UVData.Num()];
					}

					// Make sure the uv data is the same length as the position data
					if (PositionStream->Num() > UVData.Num())
					{
						CollisionUVChannel.SetNumZeroed(PositionStream->Num());
					}	
				}
			}

			for (int32 ChannelIndex = TexCoordsStream->GetNumElements(); ChannelIndex < CollisionMesh.TexCoords.Num(); ChannelIndex++)
			{
				CollisionMesh.TexCoords[ChannelIndex].SetNumZeroed(CollisionMesh.Vertices.Num());
			}			
		}
		else
		{
			// Zero fill data to vertex data length

			if (CollisionMesh.TexCoords.Num() < 1)
			{
				CollisionMesh.TexCoords.SetNum(1);
			}
			for (int32 Index = 0; Index < CollisionMesh.TexCoords.Num(); Index++)
			{
				CollisionMesh.TexCoords[Index].SetNumZeroed(CollisionMesh.Vertices.Num());
			}
		}
	}
	
	return true;
}


// Sphere Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddSphere(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   const FRealtimeMeshCollisionSphere& InSphere, int32& OutIndex)
{
	OutIndex = SimpleGeometry.Spheres.Add(InSphere);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertSphere(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, const FRealtimeMeshCollisionSphere& InSphere, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Spheres.Insert(Index, InSphere);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetSphereByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                         FName SphereName, bool& OutSuccess, FRealtimeMeshCollisionSphere& OutSphere)
{
	OutSuccess = SimpleGeometry.Spheres.GetByName(SphereName, OutSphere);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateSphere(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, const FRealtimeMeshCollisionSphere& InSphere, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Spheres.Update(Index, InSphere);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveSphere(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Spheres.Remove(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveSphereByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                            FName SphereName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Spheres.Remove(SphereName);
	return SimpleGeometry;
}


// Box Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddBox(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                const FRealtimeMeshCollisionBox& InBox, int32& OutIndex)
{
	OutIndex = SimpleGeometry.Boxes.Add(InBox);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertBox(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   int32 Index, const FRealtimeMeshCollisionBox& InBox, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Boxes.Insert(Index, InBox);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetBoxByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      FName BoxName, bool& OutSuccess, FRealtimeMeshCollisionBox& OutBox)
{
	OutSuccess = SimpleGeometry.Boxes.GetByName(BoxName, OutBox);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateBox(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   int32 Index, const FRealtimeMeshCollisionBox& InBox, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Boxes.Update(Index, InBox);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveBox(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Boxes.Remove(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveBoxByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                         FName BoxName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Boxes.Remove(BoxName);
	return SimpleGeometry;
}


// Capsule Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                    const FRealtimeMeshCollisionCapsule& InCapsule, int32& OutIndex)
{
	OutIndex = SimpleGeometry.Capsules.Add(InCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                       int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Capsules.Insert(Index, InCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetCapsuleByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                          FName CapsuleName, bool& OutSuccess, FRealtimeMeshCollisionCapsule& OutCapsule)
{
	OutSuccess = SimpleGeometry.Capsules.GetByName(CapsuleName, OutCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                       int32 Index, const FRealtimeMeshCollisionCapsule& InCapsule, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Capsules.Update(Index, InCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                       int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Capsules.Remove(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveCapsuleByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                             FName CapsuleName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.Capsules.Remove(CapsuleName);
	return SimpleGeometry;
}


// Tapered Capsule Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddTaperedCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                           const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule, int32& OutIndex)
{
	OutIndex = SimpleGeometry.TaperedCapsules.Add(InTaperedCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertTaperedCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                              int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule,
                                                                                              bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.TaperedCapsules.Insert(Index, InTaperedCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetTaperedCapsuleByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                                 FName TaperedCapsuleName, bool& OutSuccess,
                                                                                                 FRealtimeMeshCollisionTaperedCapsule& OutTaperedCapsule)
{
	OutSuccess = SimpleGeometry.TaperedCapsules.GetByName(TaperedCapsuleName, OutTaperedCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateTaperedCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                              int32 Index, const FRealtimeMeshCollisionTaperedCapsule& InTaperedCapsule,
                                                                                              bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.TaperedCapsules.Update(Index, InTaperedCapsule);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveTaperedCapsule(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                              int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.TaperedCapsules.Remove(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveTaperedCapsuleByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                                    FName TaperedCapsuleName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.TaperedCapsules.Remove(TaperedCapsuleName);
	return SimpleGeometry;
}


// Convex Functions

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::AddConvex(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                   const FRealtimeMeshCollisionConvex& InConvex, int32& OutIndex)
{
	OutIndex = SimpleGeometry.ConvexHulls.Add(InConvex);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::InsertConvex(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, const FRealtimeMeshCollisionConvex& InConvex, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.ConvexHulls.Insert(Index, InConvex);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::GetConvexByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                         FName ConvexName, bool& OutSuccess, FRealtimeMeshCollisionConvex& OutConvex)
{
	OutSuccess = SimpleGeometry.ConvexHulls.GetByName(ConvexName, OutConvex);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::UpdateConvex(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, const FRealtimeMeshCollisionConvex& InConvex, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.ConvexHulls.Update(Index, InConvex);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveConvex(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                      int32 Index, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.ConvexHulls.Remove(Index);
	return SimpleGeometry;
}

FRealtimeMeshSimpleGeometry& URealtimeMeshSimpleGeometryFunctionLibrary::RemoveConvexByName(FRealtimeMeshSimpleGeometry& SimpleGeometry,
                                                                                            FName ConvexName, bool& OutSuccess)
{
	OutSuccess = SimpleGeometry.ConvexHulls.Remove(ConvexName);
	return SimpleGeometry;
}


