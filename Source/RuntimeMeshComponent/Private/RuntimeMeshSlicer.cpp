// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshSlicer.h"
#include "CoreMinimal.h"
#include "GeomTools.h"
#include "Logging/TokenizedMessage.h"
#include "Logging/MessageLog.h"
#include "PhysicsEngine/ConvexElem.h"
#include "../Public/RuntimeMeshComponent.h"
#include "../Public/Providers/RuntimeMeshProviderStatic.h"

// TODO: At some point remove this as a dependency, would be better to make a custom quickhull that gets only the edge information.
// we don't care about the normal or triangle information that this generates. Plus I'd rather have it entirely on UE4 types.

#pragma warning(push)
#pragma warning(disable : 4701)
#define QUICKHULL_IMPLEMENTATION
#include "RuntimeMeshQuickHull.h"
#pragma warning(pop)

/** Util that returns 1 if on positive side of plane, -1 if negative, or 0 if split by plane */
int32 RMCBoxPlaneCompare(FBox InBox, const FPlane& InPlane)
{
	FVector BoxCenter, BoxExtents;
	InBox.GetCenterAndExtents(BoxCenter, BoxExtents);

	// Find distance of box center from plane
	float BoxCenterDist = InPlane.PlaneDot(BoxCenter);

	// See size of box in plane normal direction
	float BoxSize = FVector::BoxPushOut(InPlane, BoxExtents);

	if (BoxCenterDist > BoxSize)
	{
		return 1;
	}
	else if (BoxCenterDist < -BoxSize)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

int32 CopyVertexToNewMeshData(const FRuntimeMeshRenderableMeshData& Source, FRuntimeMeshRenderableMeshData& Destination, int32 Index)
{
	check(Source.Positions.Num() > Index);

	int32 NewIndex = Destination.Positions.Add(Source.Positions.GetPosition(Index));

	FVector TangentX, TangentY, TangentZ;
	if (Source.Triangles.Num() > Index)
	{
		Source.Tangents.GetTangents(Index, TangentX, TangentY, TangentZ);
	}
	Destination.Tangents.Add(TangentX, TangentY, TangentZ);

	Destination.Colors.Add(Source.Colors.Num() > Index ? Source.Colors.GetColor(Index) : FColor::White);

	Destination.TexCoords.Add(Source.TexCoords.Num() > Index ? Source.TexCoords.GetTexCoord(Index) : FVector2D(0, 0));

	return NewIndex;
}

// Take two vertices, interpolate between them and add it to the destination
int32 AddInterpolatedVert(FRuntimeMeshRenderableMeshData& Source, FRuntimeMeshRenderableMeshData& Destination, int32 Vertex0, int32 Vertex1, float Alpha)
{
	check(Source.Positions.Num() > Vertex0);
	check(Source.Positions.Num() > Vertex1);

	int32 MaxVertex = FMath::Max(Vertex0, Vertex1);

	// Handle invalid alpha
	if (FMath::IsNaN(Alpha) || !FMath::IsFinite(Alpha))
	{
		return CopyVertexToNewMeshData(Source, Destination, Vertex1);
	}

	FVector LeftPosition = Source.Positions.GetPosition(Vertex0);
	FVector RightPosition = Source.Positions.GetPosition(Vertex1);
	int32 NewIndex = Destination.Positions.Add(FMath::Lerp(LeftPosition, RightPosition, Alpha));

	if (Source.Tangents.Num() > MaxVertex)
	{
		FVector LeftTangentX, LeftTangentY, LeftTangentZ;
		FVector RightTangentX, RightTangentY, RightTangentZ;
		Source.Tangents.GetTangents(Vertex0, LeftTangentX, LeftTangentY, LeftTangentZ);
		Source.Tangents.GetTangents(Vertex1, RightTangentX, RightTangentY, RightTangentZ);

		Destination.Tangents.Add(
			FMath::Lerp(LeftTangentX, RightTangentX, Alpha),
			FMath::Lerp(LeftTangentY, RightTangentY, Alpha),
			FMath::Lerp(LeftTangentZ, RightTangentZ, Alpha));
	}
	else if (Source.Tangents.Num() > 0)
	{
		FVector TangentX, TangentY, TangentZ;
		Source.Tangents.GetTangents(Vertex0, TangentX, TangentY, TangentZ);

		Destination.Tangents.Add(TangentX, TangentY, TangentZ);
	}
	else
	{
		Destination.Tangents.Add(FVector::RightVector, FVector::ForwardVector, FVector::UpVector);
	}


	if (Source.Colors.Num() > MaxVertex)
	{
		FColor LeftColor = Source.Colors.GetColor(Vertex0);
		FColor RightColor = Source.Colors.GetColor(Vertex1);

		FColor ResultColor;
		ResultColor.R = FMath::Clamp(FMath::TruncToInt(FMath::Lerp(float(LeftColor.R), float(RightColor.R), Alpha)), 0, 255);
		ResultColor.G = FMath::Clamp(FMath::TruncToInt(FMath::Lerp(float(LeftColor.G), float(RightColor.G), Alpha)), 0, 255);
		ResultColor.B = FMath::Clamp(FMath::TruncToInt(FMath::Lerp(float(LeftColor.B), float(RightColor.B), Alpha)), 0, 255);
		ResultColor.A = FMath::Clamp(FMath::TruncToInt(FMath::Lerp(float(LeftColor.A), float(RightColor.A), Alpha)), 0, 255);

		Destination.Colors.Add(ResultColor);
	}
	else
	{
		Destination.Colors.Add(Source.Colors.Num() > 0 ? Source.Colors.GetColor(0) : FColor::White);
	}


	if (Source.TexCoords.Num() > MaxVertex)
	{
		FVector2D LeftTexCoord = Source.TexCoords.GetTexCoord(Vertex0);
		FVector2D RightTexCoord = Source.TexCoords.GetTexCoord(Vertex1);
		Destination.TexCoords.Add(FMath::Lerp(LeftTexCoord, RightTexCoord, Alpha));
	}
	else
	{
		Destination.TexCoords.Add(Source.TexCoords.Num() > 0 ? Source.TexCoords.GetTexCoord(0) : FVector2D::ZeroVector);
	}

	return NewIndex;
}

/** Transform triangle from 2D to 3D static-mesh triangle. */
void Transform2DPolygonTo3D(const FUtilPoly2D& InPoly, const FMatrix& InMatrix, FRuntimeMeshRenderableMeshData& OutMeshData)
{
	FVector TangentX = -InMatrix.GetUnitAxis(EAxis::X);
	FVector TangentY = -InMatrix.GetUnitAxis(EAxis::Y);
	FVector TangentZ = -InMatrix.GetUnitAxis(EAxis::Z);

	for (int32 VertexIndex = 0; VertexIndex < InPoly.Verts.Num(); VertexIndex++)
	{
		const FUtilVertex2D& InVertex = InPoly.Verts[VertexIndex];

		OutMeshData.Positions.Add(InMatrix.TransformPosition(FVector(InVertex.Pos.X, InVertex.Pos.Y, 0.f)));
		OutMeshData.Tangents.Add(TangentX, TangentY, TangentZ);
		OutMeshData.Colors.Add(InVertex.Color);
		OutMeshData.TexCoords.Add(InVertex.UV);
	}
}

/** Given a polygon, decompose into triangles. */
bool TriangulatePoly(FRuntimeMeshRenderableMeshData& MeshData, int32 VertBase, const FVector& PolyNormal)
{
	// Can't work if not enough verts for 1 triangle
	int32 NumVerts = MeshData.Positions.Num() - VertBase;
	if (NumVerts < 3)
	{
		MeshData.Triangles.AddTriangle(VertBase + 0, VertBase + 2, VertBase + 1);

		// Return true because poly is already a tri
		return true;
	}

	// Remember initial size of OutTris, in case we need to give up and return to this size
	const int32 TriBase = MeshData.Triangles.Num();

	// Init array of vert indices, in order. We'll modify this
	TArray<int32> VertIndices;
	VertIndices.AddUninitialized(NumVerts);
	for (int VertIndex = 0; VertIndex < NumVerts; VertIndex++)
	{
		VertIndices[VertIndex] = VertBase + VertIndex;
	}

	// Keep iterating while there are still vertices
	while (VertIndices.Num() >= 3)
	{
		// Look for an 'ear' triangle
		bool bFoundEar = false;
		for (int32 EarVertexIndex = 0; EarVertexIndex < VertIndices.Num(); EarVertexIndex++)
		{
			// Triangle is 'this' vert plus the one before and after it
			const int32 AIndex = (EarVertexIndex == 0) ? VertIndices.Num() - 1 : EarVertexIndex - 1;
			const int32 BIndex = EarVertexIndex;
			const int32 CIndex = (EarVertexIndex + 1) % VertIndices.Num();

			const FVector AVertPos = MeshData.Positions.GetPosition(VertIndices[AIndex]);
			const FVector BVertPos = MeshData.Positions.GetPosition(VertIndices[BIndex]);
			const FVector CVertPos = MeshData.Positions.GetPosition(VertIndices[CIndex]);

			// Check that this vertex is convex (cross product must be positive)
			const FVector ABEdge = BVertPos - AVertPos;
			const FVector ACEdge = CVertPos - AVertPos;
			const float TriangleDeterminant = (ABEdge ^ ACEdge) | PolyNormal;
			if (TriangleDeterminant > 0.f)
			{
				continue;
			}

			bool bFoundVertInside = false;
			// Look through all verts before this in array to see if any are inside triangle
			for (int32 VertexIndex = 0; VertexIndex < VertIndices.Num(); VertexIndex++)
			{
				const FVector TestVertPos = MeshData.Positions.GetPosition(VertIndices[VertexIndex]);

				if (VertexIndex != AIndex &&
					VertexIndex != BIndex &&
					VertexIndex != CIndex &&
					FGeomTools::PointInTriangle(AVertPos, BVertPos, CVertPos, TestVertPos))
				{
					bFoundVertInside = true;
					break;
				}
			}

			// Triangle with no verts inside - its an 'ear'! 
			if (!bFoundVertInside)
			{
				MeshData.Triangles.AddTriangle(VertIndices[AIndex], VertIndices[CIndex], VertIndices[BIndex]);

				// And remove vertex from polygon
				VertIndices.RemoveAt(EarVertexIndex);

				bFoundEar = true;
				break;
			}
		}

		// If we couldn't find an 'ear' it indicates something is bad with this polygon - discard triangles and return.
		if (!bFoundEar)
		{
			MeshData.Triangles.SetNum(TriBase, true);
			return false;
		}
	}

	return true;
}

void SliceConvexShape(const FRuntimeMeshCollisionConvexMesh& InConvex, const FPlane& SlicePlane, FRuntimeMeshCollisionConvexMesh& OutConvex, FRuntimeMeshCollisionConvexMesh& OutOtherConvex)
{
	OutConvex.VertexBuffer.Empty();
	OutOtherConvex.VertexBuffer.Empty();

	// We need to compute the convex hull so we can get the boundary edges
	qh_mesh_t Mesh = qh_quickhull3d(reinterpret_cast<const qh_vertex_t*>(InConvex.VertexBuffer.GetData()), InConvex.VertexBuffer.Num());


	// Find duplicate vertices
	TArray<FVector> CleanVertices;
	TArray<int32> CleanVertexRemap;
	TMap<FVector, int32> CleanVertexMap;

	CleanVertices.SetNum(OutConvex.VertexBuffer.Num());
	CleanVertexRemap.SetNum(Mesh.nvertices);

	// We need to purge the duplicate vertices here
	for (uint32 Index = 0; Index < Mesh.nvertices; Index++)
	{
		FVector Position = reinterpret_cast<FVector&>(Mesh.vertices[Index]);

		if (CleanVertexMap.Contains(Position))
		{
			CleanVertexRemap[Index] = CleanVertexMap[Position];
		}
		else
		{
			int32 NewIndex = CleanVertices.Add(Position);
			CleanVertexRemap[Index] = NewIndex;
			CleanVertexMap.Add(Position, NewIndex);
		}		
	}





	int32 NumBaseVerts = CleanVertices.Num();

	TArray<uint8> OutputConvex;
	OutputConvex.SetNum(NumBaseVerts);

	TArray<float> VertDistance;
	VertDistance.AddUninitialized(NumBaseVerts);


	// Now we need to split the existing points between the two hulls
	for (int32 BaseVertIndex = 0; BaseVertIndex < NumBaseVerts; BaseVertIndex++)
	{
		FVector Position = CleanVertices[BaseVertIndex];

		// Calc distance from plane
		VertDistance[BaseVertIndex] = SlicePlane.PlaneDot(Position);

		// See if vert is being kept in this section
		if (VertDistance[BaseVertIndex] > 0.f)
		{
			OutConvex.VertexBuffer.Add(Position);

			// Add to output map
			OutputConvex[BaseVertIndex] = 0;
		}
		// Or add to other half if desired
		else
		{
			OutOtherConvex.VertexBuffer.Add(Position);

			// Add to output map
			OutputConvex[BaseVertIndex] = 1;
		}
	}

	// If all the points ended up in one or the other, we can just return as we didn't split the convex
	if (OutConvex.VertexBuffer.Num() > 0 && OutOtherConvex.VertexBuffer.Num() > 0)
	{
		TSet<uint64> Edges;

		const auto AddEdge = [&](int32 Left, int32 Right)
		{
			int32 A = Left < Right ? Left : Right;
			int32 B = Left < Right ? Right : Left;

			uint64 Edge = ((uint64)A << 32) | B;

			Edges.Add(Edge);
		};

		// First build a unique set of edges. We do this by ordering the pair indices for an 
		// edge min to max, combining them into a single uint64 and adding them to the set
		int32 NumTriangles = Mesh.nindices / 3;
		for (int32 Index = 0; Index < NumTriangles; Index++)
		{
			int32 CleanIndices[] = { CleanVertexRemap[Index * 3 + 0], CleanVertexRemap[Index * 3 + 1], CleanVertexRemap[Index * 3 + 2] };

			AddEdge(CleanIndices[0], CleanIndices[1]);
			AddEdge(CleanIndices[1], CleanIndices[2]);
			AddEdge(CleanIndices[2], CleanIndices[0]);
		}



		for (uint64 Edge : Edges)
		{
			uint32 A = (Edge >> 32) & 0xFFFFFFFF;
			uint32 B = Edge & 0xFFFFFFFF;

			// Does this edge cross the boundary?

			if (OutputConvex[A] != OutputConvex[B])
			{

				// Calculate alpha
				float Alpha = -VertDistance[A] / (VertDistance[B] - VertDistance[A]);
				Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

				// Calculate and add position
				FVector LeftPosition = CleanVertices[A];
				FVector RightPosition = CleanVertices[B];
				FVector NewPosition = FMath::Lerp(LeftPosition, RightPosition, Alpha);

				OutConvex.VertexBuffer.Add(NewPosition);
				OutOtherConvex.VertexBuffer.Add(NewPosition);
			}
		}
	}

	qh_free_mesh(Mesh);
}







void URuntimeMeshSlicer::SliceRuntimeMesh(URuntimeMeshComponent* InRuntimeMesh, FVector PlanePosition, FVector PlaneNormal, bool bCreateOtherHalf,
	URuntimeMeshComponent*& OutOtherHalfRuntimeMesh, ERuntimeMeshSliceCapOption CapOption, UMaterialInterface* CapMaterial)
{
	if (InRuntimeMesh == nullptr)
	{
		return;
	}

	URuntimeMeshProviderStatic* SourceProvider = Cast<URuntimeMeshProviderStatic>(InRuntimeMesh->GetProvider());
	if (SourceProvider == nullptr)
	{
		return;
	}

	// Transform plane from world to local space
	FTransform ComponentToWorld = InRuntimeMesh->GetComponentToWorld();
	FVector LocalPlanePos = ComponentToWorld.InverseTransformPosition(PlanePosition);
	FVector LocalPlaneNormal = ComponentToWorld.InverseTransformVectorNoScale(PlaneNormal);
	LocalPlaneNormal = LocalPlaneNormal.GetSafeNormal(); // Ensure normalized

	FPlane SlicePlane(LocalPlanePos, LocalPlaneNormal);

	bool bSlicedAny = false;


	URuntimeMeshProviderStatic* DestinationProvider = nullptr;

	const auto CreateOtherIfNull = [&]()
	{
		if (DestinationProvider == nullptr)
		{
			// Create new component with the same outer as the proc mesh passed in
			OutOtherHalfRuntimeMesh = NewObject<URuntimeMeshComponent>(InRuntimeMesh->GetOuter());
			DestinationProvider = NewObject<URuntimeMeshProviderStatic>(OutOtherHalfRuntimeMesh);
			OutOtherHalfRuntimeMesh->Initialize(DestinationProvider);

			// Set transform to match source component
			OutOtherHalfRuntimeMesh->SetWorldTransform(InRuntimeMesh->GetComponentTransform());

			// Copy collision settings from input mesh
			OutOtherHalfRuntimeMesh->SetCollisionProfileName(InRuntimeMesh->GetCollisionProfileName());
			OutOtherHalfRuntimeMesh->SetCollisionEnabled(InRuntimeMesh->GetCollisionEnabled());

			// Copy materials
			const TArray<FRuntimeMeshMaterialSlot>& Materials = InRuntimeMesh->GetMaterialSlots();
			for (int32 Index = 0; Index < Materials.Num(); Index++)
			{
				const FRuntimeMeshMaterialSlot& Slot = Materials[Index];
				DestinationProvider->SetupMaterialSlot(Index, Slot.SlotName, Slot.Material);
			}
			OutOtherHalfRuntimeMesh->OverrideMaterials = InRuntimeMesh->OverrideMaterials;



			// Finally register
			OutOtherHalfRuntimeMesh->RegisterComponent();
		}
	};


	static const int32 LODIndex = 0;

	TArray<int32> SectionIds = SourceProvider->GetSectionIds(LODIndex);
	TSet<int32> SectionIdsForMeshCollision = (LODIndex == SourceProvider->GetLODForMeshCollision())? SourceProvider->GetSectionsForMeshCollision() : TSet<int32>();


	TArray<FUtilEdge3D> ClipEdges;
	//Per-section slicing of the rendereable mesh data
	for (int32 SectionId : SectionIds)
	{
		// Check we have valid mesh data, or just skip this section
// 		if (!SourceProvider->DoesSectionHaveValidMeshData(LODIndex, SectionId))
// 		{
// 			continue;
// 		}

		FBoxSphereBounds Bounds = SourceProvider->GetSectionBounds(LODIndex, SectionId);

		int32 BoxCompare = RMCBoxPlaneCompare(Bounds.GetBox(), SlicePlane);

		// Box not affected, leave alone (Everything is on the wanted side of the plane, do nothing)
		if (BoxCompare == 1)
		{
			continue;
		}
		// Box clipped, move section to other component (Everything is on the other side of the plane, the one we don't want to keep in the main mesh)
		else if (BoxCompare == -1)
		{
			bSlicedAny = true;

			if (bCreateOtherHalf)
			{
				CreateOtherIfNull();

				DestinationProvider->CreateSection(
					LODIndex,
					SectionId,
					SourceProvider->GetSectionProperties(LODIndex, SectionId),
					SourceProvider->GetSectionRenderDataAndClear(LODIndex, SectionId),
					Bounds,
					SectionIdsForMeshCollision.Contains(SectionId));
			}
			else
			{
				SourceProvider->ClearSection(LODIndex, SectionId);
			}
		}
		// Box split, slice mesh (There are things on either side of the plane, according to the bounds)
		else
		{
			bSlicedAny = true;

			FRuntimeMeshRenderableMeshData SourceSection = SourceProvider->GetSectionRenderData(LODIndex, SectionId);
			FRuntimeMeshRenderableMeshData NewSourceSection = SourceSection.CopyStructure();
			FRuntimeMeshRenderableMeshData DestinationSection = SourceSection.CopyStructure();


			// Map of base vert index to sliced vert index
			TMap<int32, int32> BaseToSlicedVertIndex;
			TMap<int32, int32> BaseToOtherSlicedVertIndex;

			const int32 NumBaseVerts = SourceSection.Positions.Num();

			// Distance of each base vert from slice plane
			TArray<float> VertDistance;
			VertDistance.AddUninitialized(NumBaseVerts);

			// Build vertex buffer 
			for (int32 BaseVertIndex = 0; BaseVertIndex < NumBaseVerts; BaseVertIndex++)
			{
				FVector Position = SourceSection.Positions.GetPosition(BaseVertIndex);

				// Calc distance from plane
				VertDistance[BaseVertIndex] = SlicePlane.PlaneDot(Position);

				// See if vert is being kept in this section
				if (VertDistance[BaseVertIndex] > 0.f)
				{
					// Copy to sliced vertex buffer
					int32 SlicedVertIndex = CopyVertexToNewMeshData(SourceSection, NewSourceSection, BaseVertIndex);

					// Add to map
					BaseToSlicedVertIndex.Add(BaseVertIndex, SlicedVertIndex);
				}
				// Or add to other half if desired
				else
				{
					bSlicedAny = true;

					if (bCreateOtherHalf)
					{
						// Copy to other sliced vertex buffer
						int32 SlicedVertIndex = CopyVertexToNewMeshData(SourceSection, DestinationSection, BaseVertIndex);

						// Add to map
						BaseToOtherSlicedVertIndex.Add(BaseVertIndex, SlicedVertIndex);
					}
				}
			}

			// TODO: Skip this next part if we didn't move anything, or make it simpler if we moved all of it


			// Iterate over base triangles (ie 3 indices at a time)
			int32 NumIndices = SourceSection.Triangles.Num();
			for (int32 BaseIndex = 0; BaseIndex < NumIndices; BaseIndex += 3)
			{
				int32 BaseV[3]; // Triangle vert indices in original mesh
				int32* SlicedV[3]; // Pointers to vert indices in new v buffer
				int32* SlicedOtherV[3]; // Pointers to vert indices in new 'other half' v buffer

										// For each vertex...
				for (int32 i = 0; i < 3; i++)
				{
					// Get triangle vert index
					BaseV[i] = SourceSection.Triangles.GetVertexIndex(BaseIndex + i);
					// Look up in sliced v buffer
					SlicedV[i] = BaseToSlicedVertIndex.Find(BaseV[i]);
					// Look up in 'other half' v buffer (if desired)
					if (bCreateOtherHalf)
					{
						SlicedOtherV[i] = BaseToOtherSlicedVertIndex.Find(BaseV[i]);
						// Each base vert _must_ exist in either BaseToSlicedVertIndex or BaseToOtherSlicedVertIndex 
						check((SlicedV[i] != nullptr) != (SlicedOtherV[i] != nullptr));
					}
				}

				// If all verts survived plane cull, keep the triangle
				if (SlicedV[0] != nullptr && SlicedV[1] != nullptr && SlicedV[2] != nullptr)
				{
					NewSourceSection.Triangles.Add(*SlicedV[0]);
					NewSourceSection.Triangles.Add(*SlicedV[1]);
					NewSourceSection.Triangles.Add(*SlicedV[2]);
				}
				// If all verts were removed by plane cull
				else if (SlicedV[0] == nullptr && SlicedV[1] == nullptr && SlicedV[2] == nullptr)
				{
					// If creating other half, add all verts to that
					if (bCreateOtherHalf)
					{
						DestinationSection.Triangles.Add(*SlicedOtherV[0]);
						DestinationSection.Triangles.Add(*SlicedOtherV[1]);
						DestinationSection.Triangles.Add(*SlicedOtherV[2]);
					}
				}
				// If partially culled, clip to create 1 or 2 new triangles
				else
				{
					int32 FinalVerts[4];
					int32 NumFinalVerts = 0;

					int32 OtherFinalVerts[4];
					int32 NumOtherFinalVerts = 0;

					FUtilEdge3D NewClipEdge;
					int32 ClippedEdges = 0;

					float PlaneDist[3];
					PlaneDist[0] = VertDistance[BaseV[0]];
					PlaneDist[1] = VertDistance[BaseV[1]];
					PlaneDist[2] = VertDistance[BaseV[2]];

					for (int32 EdgeIdx = 0; EdgeIdx < 3; EdgeIdx++)
					{
						int32 ThisVert = EdgeIdx;

						// If start vert is inside, add it.
						if (SlicedV[ThisVert] != nullptr)
						{
							check(NumFinalVerts < 4);
							FinalVerts[NumFinalVerts++] = *SlicedV[ThisVert];
						}
						// If not, add to other side
						else if (bCreateOtherHalf)
						{
							check(NumOtherFinalVerts < 4);
							OtherFinalVerts[NumOtherFinalVerts++] = *SlicedOtherV[ThisVert];
						}

						// If start and next vert are on opposite sides, add intersection
						int32 NextVert = (EdgeIdx + 1) % 3;

						if ((SlicedV[EdgeIdx] == nullptr) != (SlicedV[NextVert] == nullptr))
						{
							// Find distance along edge that plane is
							float Alpha = -PlaneDist[ThisVert] / (PlaneDist[NextVert] - PlaneDist[ThisVert]);

							// Add the interpolated vertex
							int32 InterpVertIndex = AddInterpolatedVert(SourceSection, NewSourceSection, BaseV[ThisVert], BaseV[NextVert], FMath::Clamp(Alpha, 0.0f, 1.0f));

							// Save vert index for this poly
							check(NumFinalVerts < 4);
							FinalVerts[NumFinalVerts++] = InterpVertIndex;

							// If desired, add to the poly for the other half as well
							if (bCreateOtherHalf)
							{
								int32 OtherInterpVertIndex = CopyVertexToNewMeshData(NewSourceSection, DestinationSection, InterpVertIndex);
								check(NumOtherFinalVerts < 4);
								OtherFinalVerts[NumOtherFinalVerts++] = OtherInterpVertIndex;
							}

							// When we make a new edge on the surface of the clip plane, save it off.
							check(ClippedEdges < 2);
							if (ClippedEdges == 0)
							{
								NewClipEdge.V0 = NewSourceSection.Positions.GetPosition(InterpVertIndex);
							}
							else
							{
								NewClipEdge.V1 = NewSourceSection.Positions.GetPosition(InterpVertIndex);
							}

							ClippedEdges++;
						}
					}

					// Triangulate the clipped polygon.
					for (int32 VertexIndex = 2; VertexIndex < NumFinalVerts; VertexIndex++)
					{
						NewSourceSection.Triangles.Add(FinalVerts[0]);
						NewSourceSection.Triangles.Add(FinalVerts[VertexIndex - 1]);
						NewSourceSection.Triangles.Add(FinalVerts[VertexIndex]);
					}

					// If we are making the other half, triangulate that as well
					if (bCreateOtherHalf)
					{
						for (int32 VertexIndex = 2; VertexIndex < NumOtherFinalVerts; VertexIndex++)
						{
							DestinationSection.Triangles.Add(OtherFinalVerts[0]);
							DestinationSection.Triangles.Add(OtherFinalVerts[VertexIndex - 1]);
							DestinationSection.Triangles.Add(OtherFinalVerts[VertexIndex]);
						}
					}

					check(ClippedEdges != 1); // Should never clip just one edge of the triangle

											  // If we created a new edge, save that off here as well
					if (ClippedEdges == 2)
					{
						ClipEdges.Add(NewClipEdge);
					}
				}
			}

			if (NewSourceSection.HasValidMeshData())
			{
				SourceProvider->UpdateSection(LODIndex, SectionId, NewSourceSection);
			}
			else
			{
				SourceProvider->ClearSection(LODIndex, SectionId);
			}

			if (bCreateOtherHalf && DestinationSection.HasValidMeshData())
			{
				CreateOtherIfNull();
				DestinationProvider->CreateSection(
					LODIndex,
					SectionId,
					SourceProvider->GetSectionProperties(LODIndex, SectionId),
					DestinationSection,
					SectionIdsForMeshCollision.Contains(SectionId));
			}
		
		}
	}



	// Create cap geometry (if some edges to create it from)
	if (CapOption != ERuntimeMeshSliceCapOption::NoCap && ClipEdges.Num() > 0)
	{
		int32 SourceSectionId = SourceProvider->GetLastSectionId(LODIndex);
		int32 DestinationSectionId = DestinationProvider->GetLastSectionId(LODIndex);
		FRuntimeMeshRenderableMeshData NewSourceCap;
		FRuntimeMeshRenderableMeshData NewDestiantionCap;

		if (CapOption == ERuntimeMeshSliceCapOption::UseLastSectionForCap)
		{
			NewSourceCap = SourceProvider->GetSectionRenderData(LODIndex, SourceSectionId);

			if (bCreateOtherHalf && DestinationProvider)
			{
				NewDestiantionCap = DestinationProvider->GetSectionRenderData(LODIndex, DestinationSectionId);
			}
		}


		// Project 3D edges onto slice plane to form 2D edges
		TArray<FUtilEdge2D> Edges2D;
		FUtilPoly2DSet PolySet;
		FGeomTools::ProjectEdges(Edges2D, PolySet.PolyToWorld, ClipEdges, SlicePlane);

		// Find 2D closed polygons from this edge soup
		FGeomTools::Buid2DPolysFromEdges(PolySet.Polys, Edges2D, FColor(255, 255, 255, 255));

		// Remember start point for vert and index buffer before adding and cap geom
		int32 CapVertBase = NewSourceCap.Positions.Num();
		int32 CapIndexBase = NewSourceCap.Triangles.Num();

		// Triangulate each poly
		for (int32 PolyIdx = 0; PolyIdx < PolySet.Polys.Num(); PolyIdx++)
		{
			// Generate UVs for the 2D polygon.
			FGeomTools::GeneratePlanarTilingPolyUVs(PolySet.Polys[PolyIdx], 64.f);

			// Remember start of vert buffer before adding triangles for this poly
			int32 PolyVertBase = NewSourceCap.Positions.Num();

			// Transform from 2D poly verts to 3D
			Transform2DPolygonTo3D(PolySet.Polys[PolyIdx], PolySet.PolyToWorld, NewSourceCap);

			// Triangulate this polygon
			TriangulatePoly(NewSourceCap, PolyVertBase, LocalPlaneNormal);
		}

		// If creating the other half, copy cap geom into other half sections
		if (bCreateOtherHalf)
		{
			// Remember current base index for verts in 'other cap section'
			int32 OtherCapVertBase = NewDestiantionCap.Positions.Num();

			// Copy verts from cap section into other cap section
			for (int32 VertIdx = CapVertBase; VertIdx < NewSourceCap.Positions.Num(); VertIdx++)
			{
				NewDestiantionCap.Positions.Add(NewSourceCap.Positions.GetPosition(VertIdx));

				FVector TangentX, TangentY, TangentZ;
				NewSourceCap.Tangents.GetTangents(VertIdx, TangentX, TangentY, TangentZ);
				TangentX *= -1.0f;
				TangentY *= -1.0f;
				TangentZ *= -1.0f;
				NewDestiantionCap.Tangents.Add(TangentX, TangentY, TangentZ);

				NewDestiantionCap.Colors.Add(NewSourceCap.Colors.GetColor(VertIdx));

				NewDestiantionCap.TexCoords.Add(NewSourceCap.TexCoords.GetTexCoord(VertIdx));
			}

			// Find offset between main cap verts and other cap verts
			int32 VertOffset = OtherCapVertBase - CapVertBase;

			// Copy indices over as well
			for (int32 IndexIdx = CapIndexBase; IndexIdx < NewSourceCap.Triangles.Num(); IndexIdx += 3)
			{
				// Need to offset and change winding
				NewDestiantionCap.Triangles.Add(NewSourceCap.Triangles.GetVertexIndex(IndexIdx + 0) + VertOffset);
				NewDestiantionCap.Triangles.Add(NewSourceCap.Triangles.GetVertexIndex(IndexIdx + 2) + VertOffset);
				NewDestiantionCap.Triangles.Add(NewSourceCap.Triangles.GetVertexIndex(IndexIdx + 1) + VertOffset);
			}
		}


		if (CapOption == ERuntimeMeshSliceCapOption::UseLastSectionForCap)
		{
			SourceProvider->UpdateSection(LODIndex, SourceSectionId, NewSourceCap);

			if (bCreateOtherHalf && DestinationProvider)
			{
				DestinationProvider->UpdateSection(LODIndex, DestinationSectionId, NewDestiantionCap);
			}
		}
		else
		{
			int32 LastMaterialId = SourceProvider->GetNumMaterials();
			SourceProvider->SetupMaterialSlot(LastMaterialId, "SliceCap", CapMaterial);

			FRuntimeMeshSectionProperties SectionProps;
			SectionProps.MaterialSlot = LastMaterialId;

			SourceProvider->CreateSection(LODIndex, SourceSectionId + 1, SectionProps, NewSourceCap);

			if (bCreateOtherHalf && DestinationProvider)
			{
				LastMaterialId = DestinationProvider->GetNumMaterials();
				DestinationProvider->SetupMaterialSlot(LastMaterialId, "SliceCap", CapMaterial);

				SectionProps.MaterialSlot = LastMaterialId;

				DestinationProvider->CreateSection(LODIndex, DestinationSectionId + 1, SectionProps, NewDestiantionCap);
			}
		}
	}


	if (bSlicedAny)
	{
		FRuntimeMeshCollisionSettings SourceCollision = SourceProvider->GetCollisionSettings();

		TArray<FRuntimeMeshCollisionConvexMesh> NewSourceConvexes;
		TArray<FRuntimeMeshCollisionConvexMesh> NewOtherConvexes;

		for (FRuntimeMeshCollisionConvexMesh& Source : SourceCollision.ConvexElements)
		{
			FRuntimeMeshCollisionConvexMesh NewSource;
			FRuntimeMeshCollisionConvexMesh NewOther;

			SliceConvexShape(Source, SlicePlane, NewSource, NewOther);

			if (NewSource.VertexBuffer.Num() > 0)
			{
				NewSourceConvexes.Add(MoveTemp(NewSource));
			}

			if (NewOther.VertexBuffer.Num() > 0)
			{
				NewOtherConvexes.Add(MoveTemp(NewOther));
			}
		}

		
		SourceCollision.ConvexElements = MoveTemp(NewSourceConvexes);
		SourceProvider->SetCollisionSettings(SourceCollision);

		if (bCreateOtherHalf && DestinationProvider)
		{
			SourceCollision.ConvexElements = MoveTemp(NewOtherConvexes);
			DestinationProvider->SetCollisionSettings(SourceCollision);
		}
	}
}




bool URuntimeMeshSlicer::SliceRuntimeMeshData(FRuntimeMeshRenderableMeshData& SourceSection, const FPlane& SlicePlane, ERuntimeMeshSliceCapOption CapOption, FRuntimeMeshRenderableMeshData& NewSourceSection,
	FRuntimeMeshRenderableMeshData& NewSourceSectionCap, bool bCreateDestination, FRuntimeMeshRenderableMeshData& DestinationSection, FRuntimeMeshRenderableMeshData& NewDestinationSectionCap)
{
	









	return true;
}
