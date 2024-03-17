// Copyright TriAxis Games, L.L.C. & xixgames All Rights Reserved.

#include "RealtimeMeshBranchingLinesActor.h"

#include "RealtimeMeshSimple.h"

ARealtimeMeshBranchingLinesActor::ARealtimeMeshBranchingLinesActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshBranchingLinesActor::OnGenerateMesh_Implementation()
{
	// Initialize to a simple mesh, this behaves the most like a ProceduralMeshComponent
	// Where you can set the mesh data and forget about it.
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// The most important part of the mesh data is the StreamSet, it contains the individual buffers,
	// like position, tangents, texcoords, triangles etc. 
	FRealtimeMeshStreamSet StreamSet;
	
	// For this example we'll use a helper class to build the mesh data
	// You can make your own helpers or skip them and use individual TRealtimeMeshStreamBuilder,
	// or skip them entirely and copy data directly into the streams
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	// here we go ahead and enable all the basic mesh data parts
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors(); // TODO
	
	// Poly groups allow us to easily create a single set of buffers with multiple sections by adding an index to the triangle data
	Builder.EnablePolyGroups();

	// CUSTOM BUILD OF THE BRANCHES
	PreCacheCrossSection();
	GenerateMesh(Builder);
	
	// Setup the two material slots
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial", Material);
	//RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	// Now create the group key. This is a unique identifier for the section group
	// A section group contains one or more sections that all share the underlying buffers
	// these sections can overlap the used vertex/index ranges depending on use case.
	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("TestTriangle"));

	// Now create the section key, this is a unique identifier for a section within a group
	// The section contains the configuration for the section, like the material slot,
	// and the draw type, as well as the range of the index/vertex buffers to use to render.
	// Here we're using the version to create the key based on the PolyGroup index
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	//const FRealtimeMeshSectionKey PolyGroup1SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);
	
	// Now we create the section group, since the stream set has polygroups, this will create the sections as well
	RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);

	// Update the configuration of both the polygroup sections.
	RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0));
	//RealtimeMesh->UpdateSectionConfig(PolyGroup1SectionKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1));
	
	Super::OnGenerateMesh_Implementation();
}

void ARealtimeMeshBranchingLinesActor::GenerateMesh(RealtimeMesh::TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1>& Builder)
{
	// -------------------------------------------------------
	// Setup the random number generator and create the branching structure
	RngStream.Initialize(RandomSeed);
	CreateSegments();

	// -------------------------------------------------------
	// Now lets loop through all the defined segments and create a cylinder for each
	for (int32 i = 0; i < Segments.Num(); i++)
	{
		GenerateCylinder(Segments[i].Start, Segments[i].End, Segments[i].Width,
						 RadialSegmentCount, Builder, bSmoothNormals);
	}
}

FVector ARealtimeMeshBranchingLinesActor::RotatePointAroundPivot(const FVector& InPoint, const FVector& InPivot, const FVector& InAngles)
{
	FVector Direction = InPoint - InPivot; // get point direction relative to pivot
	Direction = FQuat::MakeFromEuler(InAngles) * Direction; // rotate it
	return Direction + InPivot; // calculate rotated point
}

void ARealtimeMeshBranchingLinesActor::PreCacheCrossSection()
{
	if (LastCachedCrossSectionCount == RadialSegmentCount)
	{
		return;
	}

	// Generate a cross-section for use in cylinder generation
	const float AngleBetweenQuads = (2.0f / static_cast<float>(RadialSegmentCount)) * PI;
	CachedCrossSectionPoints.Empty();

	// Pre-calculate cross section points of a circle, two more than needed
	for (int32 PointIndex = 0; PointIndex < (RadialSegmentCount + 2); PointIndex++)
	{
		const float Angle = static_cast<float>(PointIndex) * AngleBetweenQuads;
		CachedCrossSectionPoints.Add(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0));
	}

	LastCachedCrossSectionCount = RadialSegmentCount;
}

void ARealtimeMeshBranchingLinesActor::CreateSegments()
{
	// We create the branching structure by constantly subdividing a line between two points by creating a new point in the middle.
	// We then take that point and offset it in a random direction, by a random amount defined within limits.
	// Next we take both of the newly created line halves, and subdivide them the same way.
	// Each new midpoint also has a chance to create a new branch
	// TODO This should really be recursive
	Segments.Empty();
	float CurrentBranchOffset = MaxBranchOffset;

	if (bMaxBranchOffsetAsPercentageOfLength)
	{
		CurrentBranchOffset = (Start - End).Size() * (FMath::Clamp(MaxBranchOffset, 0.1f, 100.0f) / 100.0f);
	}

	// Pre-calc a few floats from percentages
	const float ChangeOfFork = FMath::Clamp(ChanceOfForkPercentage, 0.0f, 100.0f) / 100.0f;
	const float BranchOffsetReductionEachGeneration = FMath::Clamp(BranchOffsetReductionEachGenerationPercentage, 0.0f, 100.0f) / 100.0f;

	// Add the first segment which is simply between the start and end points
	Segments.Add(FRealtimeMeshBranchSegment(Start, End, TrunkWidth));

	for (int32 iGen = 0; iGen < Iterations; iGen++)
	{
		TArray<FRealtimeMeshBranchSegment> NewGen;

		for (const FRealtimeMeshBranchSegment& EachSegment : Segments)
		{
			FVector Midpoint = (EachSegment.End + EachSegment.Start) / 2;

			// Offset the midpoint by a random number along the normal
			const FVector Normal = FVector::CrossProduct(EachSegment.End - EachSegment.Start, OffsetDirections[RngStream.RandRange(0, 1)]).GetSafeNormal();
			Midpoint += Normal * RngStream.RandRange(-CurrentBranchOffset, CurrentBranchOffset);

			 // Create two new segments
			NewGen.Add(FRealtimeMeshBranchSegment(EachSegment.Start, Midpoint, EachSegment.Width, EachSegment.ForkGeneration));
			NewGen.Add(FRealtimeMeshBranchSegment(Midpoint, EachSegment.End, EachSegment.Width, EachSegment.ForkGeneration));

			// Chance of fork?
			if (RngStream.FRand() > (1 - ChangeOfFork))
			{
				// TODO Normalize the direction vector and calculate a new total length and then subdiv that for X generations
				const FVector Direction = Midpoint - EachSegment.Start;
				const FVector SplitEnd = (Direction * RngStream.FRandRange(ForkLengthMin, ForkLengthMax)).RotateAngleAxis(RngStream.FRandRange(ForkRotationMin, ForkRotationMax), OffsetDirections[RngStream.RandRange(0, 1)]) + Midpoint;
				NewGen.Add(FRealtimeMeshBranchSegment(Midpoint, SplitEnd, EachSegment.Width * WidthReductionOnFork, EachSegment.ForkGeneration + 1));
			}
		}

		Segments.Empty();
		Segments = NewGen;

		// Reduce the offset slightly each generation
		CurrentBranchOffset = CurrentBranchOffset * BranchOffsetReductionEachGeneration;
	}
}

void ARealtimeMeshBranchingLinesActor::GenerateCylinder(const FVector& StartPoint, const FVector& EndPoint, const float InWidth,
    const int32 InCrossSectionCount, TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1>& Builder,
    const bool bInSmoothNormals/* = true*/)
{
    // Make a cylinder section
    const float AngleBetweenQuads = (2.0f / static_cast<float>(InCrossSectionCount)) * PI;
    const float UMapPerQuad = 1.0f / static_cast<float>(InCrossSectionCount);

    const FVector StartOffset = StartPoint - FVector(0, 0, 0);
    const FVector Offset = EndPoint - StartPoint;

    // Find angle between vectors
    const FVector LineDirection = (StartPoint - EndPoint).GetSafeNormal();
    const FVector RotationAngle = LineDirection.Rotation().Add(90.f, 0.f, 0.f).Euler();

    // Start by building up vertices that make up the cylinder sides
    for (int32 QuadIndex = 0; QuadIndex < InCrossSectionCount; QuadIndex++)
    {
        // Set up the vertices
        FVector P0 = (CachedCrossSectionPoints[QuadIndex] * InWidth) + StartOffset;
        P0 = RotatePointAroundPivot(P0, StartPoint, RotationAngle);
        FVector P1 = CachedCrossSectionPoints[QuadIndex + 1] * InWidth + StartOffset;
        P1 = RotatePointAroundPivot(P1, StartPoint, RotationAngle);
        const FVector P2 = P1 + Offset;
        const FVector P3 = P0 + Offset;

        // Normals
        const FVector NormalCurrent = FVector::CrossProduct(P0 - P3, P1 - P3).GetSafeNormal();
        FVector NormalNext, NormalPrevious, AverageNormalRight, AverageNormalLeft;
        if (bInSmoothNormals)
        {
            FVector P4 = (CachedCrossSectionPoints[QuadIndex + 2] * InWidth) + StartOffset;
            P4 = RotatePointAroundPivot(P4, StartPoint, RotationAngle);

            // p1 to p4 to p2
            NormalNext = FVector::CrossProduct(P1 - P2, P4 - P2).GetSafeNormal();
            AverageNormalRight = ((NormalCurrent + NormalNext) / 2).GetSafeNormal();

            const float PreviousAngle = static_cast<float>(QuadIndex - 1) * AngleBetweenQuads;
            FVector PMinus1 = FVector(FMath::Cos(PreviousAngle) * InWidth, FMath::Sin(PreviousAngle) * InWidth, 0.f) + StartOffset;
            PMinus1 = RotatePointAroundPivot(PMinus1, StartPoint, RotationAngle);

            // p0 to p3 to pMinus1
            NormalPrevious = FVector::CrossProduct(P0 - PMinus1, P3 - PMinus1).GetSafeNormal();
            AverageNormalLeft = ((NormalCurrent + NormalPrevious) / 2).GetSafeNormal();
        }

        // Tangents (perpendicular to the surface)
        const FVector Tangent = (P0 - P1).GetSafeNormal();

        // UVs
        const FVector2D UV0 = FVector2D(1.0f - (UMapPerQuad * QuadIndex), 1.0f);
        const FVector2D UV1 = FVector2D(1.0f - (UMapPerQuad * (QuadIndex + 1)), 1.0f);
        const FVector2D UV2 = FVector2D(1.0f - (UMapPerQuad * (QuadIndex + 1)), 0.0f);
        const FVector2D UV3 = FVector2D(1.0f - (UMapPerQuad * QuadIndex), 0.0f);

        const int32 V0 = Builder.AddVertex(static_cast<FVector3f>(P0))
            .SetNormalAndTangent(static_cast<FVector3f>(bInSmoothNormals ? AverageNormalLeft : NormalCurrent), static_cast<FVector3f>(Tangent))
            .SetTexCoord(static_cast<FVector2f>(UV0));
        const int32 V1 = Builder.AddVertex(static_cast<FVector3f>(P1))
            .SetNormalAndTangent(static_cast<FVector3f>(bInSmoothNormals ? AverageNormalRight : NormalCurrent), static_cast<FVector3f>(Tangent))
            .SetTexCoord(static_cast<FVector2f>(UV1));
        const int32 V2 = Builder.AddVertex(static_cast<FVector3f>(P2))
            .SetNormalAndTangent(static_cast<FVector3f>(bInSmoothNormals ? AverageNormalRight : NormalCurrent), static_cast<FVector3f>(Tangent))
            .SetTexCoord(static_cast<FVector2f>(UV2));
        const int32 V3 = Builder.AddVertex(static_cast<FVector3f>(P3))
            .SetNormalAndTangent(static_cast<FVector3f>(bInSmoothNormals ? AverageNormalLeft : NormalCurrent), static_cast<FVector3f>(Tangent))
            .SetTexCoord(static_cast<FVector2f>(UV3));

        // Add our 2 triangles, placing the vertices in counter clockwise order
        Builder.AddTriangle(V3, V2, V0, 0);
        Builder.AddTriangle(V2, V1, V0, 0);
    }
}
