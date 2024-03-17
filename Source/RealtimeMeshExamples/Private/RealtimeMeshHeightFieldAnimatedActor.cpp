// Copyright TriAxis Games, L.L.C. & xixgames All Rights Reserved.

#include "RealtimeMeshHeightFieldAnimatedActor.h"

#include "RealtimeMeshSimple.h"
#include "Async/ParallelFor.h"

ARealtimeMeshHeightFieldAnimatedActor::ARealtimeMeshHeightFieldAnimatedActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARealtimeMeshHeightFieldAnimatedActor::OnGenerateMesh_Implementation()
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

	// CUSTOM BUILD OF THE WAVE
	GenerateMesh(Builder);
	
	// Setup the two material slots
	RealtimeMesh->SetupMaterialSlot(0, "WaveMaterial", Material);
	// TODO: more mats? -> RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

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
	// TODO: RealtimeMesh->UpdateSectionConfig(PolyGroup1SectionKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1));
	
	Super::OnGenerateMesh_Implementation();
}

void ARealtimeMeshHeightFieldAnimatedActor::GeneratePoints()
{
	// Setup example height data
	// Combine variations of sine and cosine to create some variable waves
	ParallelFor(HeightValues.Num(), [this](int32 PointIndex)
	{
		const int32 X = PointIndex / (WidthSections + 1);
		const int32 Y = PointIndex % (WidthSections + 1);

		// Just some quick hardcoded offset numbers in there
		const float ValueOne = FMath::Cos((X + CurrentAnimationFrameX)*ScaleFactor) * FMath::Sin((Y + CurrentAnimationFrameY)*ScaleFactor);
		const float ValueTwo = FMath::Cos((X + CurrentAnimationFrameX*0.7f)*ScaleFactor*2.5f) * FMath::Sin((Y - CurrentAnimationFrameY*0.7f)*ScaleFactor*2.5f);
		const float AvgValue = ((ValueOne + ValueTwo) / 2) * Size.Z;
		HeightValues[PointIndex] = AvgValue;

		if (AvgValue > MaxHeightValue)
		{
			MaxHeightValue = AvgValue;
		}
	});
	
}

void ARealtimeMeshHeightFieldAnimatedActor::Tick(float DeltaSeconds)
{
	if (bAnimateMesh)
	{
		CurrentAnimationFrameX += DeltaSeconds * AnimationSpeedX;
		CurrentAnimationFrameY += DeltaSeconds * AnimationSpeedY;
		OnGenerateMesh_Implementation();
	}
}

void ARealtimeMeshHeightFieldAnimatedActor::GenerateMesh(TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1>& Builder)
{
	if (Size.X < 1 || Size.Y < 1 || LengthSections < 1 || WidthSections < 1)
	{
		return;
	}
	
	SetupMesh();
	GeneratePoints();
	GenerateGrid(FVector2D(Size.X, Size.Y), LengthSections, WidthSections, HeightValues, Builder);
}

void ARealtimeMeshHeightFieldAnimatedActor::SetupMesh()
{
	const int32 NumberOfPoints = (LengthSections + 1) * (WidthSections + 1);
	if (NumberOfPoints != HeightValues.Num())
	{
		HeightValues.Empty();
		HeightValues.AddUninitialized(NumberOfPoints);
	}
}

void ARealtimeMeshHeightFieldAnimatedActor::GenerateGrid(const FVector2D InSize,
	 const int32 InLengthSections, const int32 InWidthSections, const TArray<float>& InHeightValues,
	 TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1>& Builder)
{
	// Note the coordinates are a bit weird here since I aligned it to the transform (X is forwards or "up", which Y is to the right)
	// Should really fix this up and use standard X, Y coords then transform into object space?
	const FVector2D SectionSize = FVector2D(InSize.X / InLengthSections, InSize.Y / InWidthSections);
	int32 VertexIndex = 0;
	
	const float LengthSectionsAsFloat = static_cast<float>(InLengthSections);
	const float WidthSectionsAsFloat = static_cast<float>(InWidthSections);
	TArray<int32> BuildVertices;
    for (int32 X = 0; X < InLengthSections + 1; X++)
    {
        for (int32 Y = 0; Y < InWidthSections + 1; Y++)
        {
            // Create a new vertex
            const int32 NewVertIndex = VertexIndex++;
            const FVector NewVertex = FVector(X * SectionSize.X, Y * SectionSize.Y, InHeightValues[NewVertIndex]);

            // Note that Unreal UV origin (0,0) is top left
            const float U = static_cast<float>(X) / LengthSectionsAsFloat;
            const float V = static_cast<float>(Y) / WidthSectionsAsFloat;
            BuildVertices.Add(Builder.AddVertex(static_cast<FVector3f>(NewVertex)).SetTexCoord(FVector2f(U, V)));
            
            // Once we've created enough verts we can start adding polygons
            if (X > 0 && Y > 0)
            {
                // Each row is InWidthSections+1 number of points.
                // And we have InLength+1 rows
                // Index of current vertex in position is thus: (X * (InWidthSections + 1)) + Y;
                const int32 TopRightIndex = BuildVertices[(X * (InWidthSections + 1)) + Y];
                const int32 TopLeftIndex = BuildVertices[TopRightIndex - 1];
                const int32 BottomRightIndex = BuildVertices[((X - 1) * (InWidthSections + 1)) + Y];
                const int32 BottomLeftIndex = BuildVertices[BottomRightIndex - 1];

                // Now create two triangles from those four vertices
                // The order of these (clockwise/counter-clockwise) dictates which way the normal will face.
                // Normals
                const FVector3f NormalCurrent = FVector3f::CrossProduct(
            		Builder.GetPosition(BottomLeftIndex) - Builder.GetPosition(TopLeftIndex),
            		Builder.GetPosition(TopLeftIndex) - Builder.GetPosition(TopRightIndex)).GetSafeNormal();
            	
                Builder.EditVertex(BottomLeftIndex).SetNormal(NormalCurrent);
                Builder.EditVertex(TopRightIndex).SetNormal(NormalCurrent);
                Builder.EditVertex(TopLeftIndex).SetNormal(NormalCurrent);
                Builder.EditVertex(BottomRightIndex).SetNormal(NormalCurrent);
                
                // Add our 2 triangles, placing the vertices in counter clockwise order
                Builder.AddTriangle(BottomLeftIndex, TopRightIndex, TopLeftIndex, 0);
                Builder.AddTriangle(BottomLeftIndex, BottomRightIndex, TopRightIndex, 0);
            }
        }
    }
}
