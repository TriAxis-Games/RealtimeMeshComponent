// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_VertexColors.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_VertexColors::ARealtimeMeshVisualTest_VertexColors()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshVisualTest_VertexColors::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slot - use default material that supports vertex colors
	RealtimeMesh->SetupMaterialSlot(0, "VertexColorMaterial");

	// Create four different vertex color demonstrations
	CreateRainbowGrid(RealtimeMesh, FVector3f(-200.0f, -200.0f, 0.0f));
	CreateRGBCornerBox(RealtimeMesh, FVector3f(200.0f, -200.0f, 0.0f));
	CreateRadialGradientPlane(RealtimeMesh, FVector3f(-200.0f, 200.0f, 0.0f));
	CreateHeightGradientMesh(RealtimeMesh, FVector3f(200.0f, 200.0f, 0.0f));
}

void ARealtimeMeshVisualTest_VertexColors::CreateRainbowGrid(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset)
{
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 GridSize = 10;
	const float CellSize = 15.0f;
	const float TotalSize = GridSize * CellSize;

	// Create grid vertices with rainbow gradient
	TArray<int32> VertexIndices;
	VertexIndices.SetNum((GridSize + 1) * (GridSize + 1));

	for (int32 Y = 0; Y <= GridSize; Y++)
	{
		for (int32 X = 0; X <= GridSize; X++)
		{
			const float XPos = X * CellSize - TotalSize * 0.5f;
			const float YPos = Y * CellSize - TotalSize * 0.5f;

			// Calculate rainbow color based on position
			const float Hue = ((float)X / GridSize + (float)Y / GridSize) * 0.5f;
			FLinearColor HSVColor(Hue * 360.0f, 1.0f, 1.0f);
			FColor VertexColor = HSVColor.HSVToLinearRGB().ToFColor(false);

			const FVector3f Position = Offset + FVector3f(XPos, YPos, 0.0f);
			const FVector3f Normal = FVector3f(0.0f, 0.0f, 1.0f);
			const FVector3f Tangent = FVector3f(1.0f, 0.0f, 0.0f);
			const FVector2f TexCoord((float)X / GridSize, (float)Y / GridSize);

			int32 VertexIndex = Builder.AddVertex(Position)
				.SetNormalAndTangent(Normal, Tangent)
				.SetColor(VertexColor)
				.SetTexCoord(TexCoord);

			VertexIndices[Y * (GridSize + 1) + X] = VertexIndex;
		}
	}

	// Create triangles
	for (int32 Y = 0; Y < GridSize; Y++)
	{
		for (int32 X = 0; X < GridSize; X++)
		{
			const int32 V0 = VertexIndices[Y * (GridSize + 1) + X];
			const int32 V1 = VertexIndices[Y * (GridSize + 1) + (X + 1)];
			const int32 V2 = VertexIndices[(Y + 1) * (GridSize + 1) + X];
			const int32 V3 = VertexIndices[(Y + 1) * (GridSize + 1) + (X + 1)];

			// Two triangles per quad
			Builder.AddTriangle(V0, V2, V1);
			Builder.AddTriangle(V1, V2, V3);
		}
	}

	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("RainbowGrid"));

	RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);
}

void ARealtimeMeshVisualTest_VertexColors::CreateRGBCornerBox(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset)
{
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const float Size = 75.0f;
	const FVector3f HalfSize(Size, Size, Size);

	// Define 8 corners of the box with distinct colors
	struct FBoxVertex
	{
		FVector3f Position;
		FColor Color;
	};

	TArray<FBoxVertex> Corners = {
		{ FVector3f(-1, -1, -1), FColor::Black },    // 0
		{ FVector3f( 1, -1, -1), FColor::Red },      // 1
		{ FVector3f(-1,  1, -1), FColor::Green },    // 2
		{ FVector3f( 1,  1, -1), FColor::Yellow },   // 3
		{ FVector3f(-1, -1,  1), FColor::Blue },     // 4
		{ FVector3f( 1, -1,  1), FColor::Magenta },  // 5
		{ FVector3f(-1,  1,  1), FColor::Cyan },     // 6
		{ FVector3f( 1,  1,  1), FColor::White }     // 7
	};

	// Box faces defined as corner indices
	struct FFace
	{
		int32 Indices[4];
		FVector3f Normal;
	};

	TArray<FFace> Faces = {
		{ {0, 1, 3, 2}, FVector3f( 0,  0, -1) }, // Front
		{ {5, 4, 6, 7}, FVector3f( 0,  0,  1) }, // Back
		{ {4, 0, 2, 6}, FVector3f(-1,  0,  0) }, // Left
		{ {1, 5, 7, 3}, FVector3f( 1,  0,  0) }, // Right
		{ {4, 5, 1, 0}, FVector3f( 0, -1,  0) }, // Bottom
		{ {2, 3, 7, 6}, FVector3f( 0,  1,  0) }  // Top
	};

	// Build each face
	for (const FFace& Face : Faces)
	{
		const FVector3f Tangent = FMath::Abs(Face.Normal.Z) < 0.9f
			? FVector3f(0, 0, 1).Cross(Face.Normal).GetSafeNormal()
			: FVector3f(1, 0, 0);

		// Create 4 vertices for this face
		int32 V0 = Builder.AddVertex(Offset + Corners[Face.Indices[0]].Position * HalfSize)
			.SetNormalAndTangent(Face.Normal, Tangent)
			.SetColor(Corners[Face.Indices[0]].Color)
			.SetTexCoord(FVector2f(0, 0));

		int32 V1 = Builder.AddVertex(Offset + Corners[Face.Indices[1]].Position * HalfSize)
			.SetNormalAndTangent(Face.Normal, Tangent)
			.SetColor(Corners[Face.Indices[1]].Color)
			.SetTexCoord(FVector2f(1, 0));

		int32 V2 = Builder.AddVertex(Offset + Corners[Face.Indices[2]].Position * HalfSize)
			.SetNormalAndTangent(Face.Normal, Tangent)
			.SetColor(Corners[Face.Indices[2]].Color)
			.SetTexCoord(FVector2f(1, 1));

		int32 V3 = Builder.AddVertex(Offset + Corners[Face.Indices[3]].Position * HalfSize)
			.SetNormalAndTangent(Face.Normal, Tangent)
			.SetColor(Corners[Face.Indices[3]].Color)
			.SetTexCoord(FVector2f(0, 1));

		// Two triangles per face
		Builder.AddTriangle(V0, V1, V2);
		Builder.AddTriangle(V0, V2, V3);
	}

	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("RGBBox"));

	RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);
}

void ARealtimeMeshVisualTest_VertexColors::CreateRadialGradientPlane(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset)
{
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 Segments = 16;
	const int32 Rings = 8;
	const float OuterRadius = 75.0f;

	// Create center vertex
	const FVector3f Center = Offset + FVector3f(0, 0, 0);
	int32 CenterVertex = Builder.AddVertex(Center)
		.SetNormalAndTangent(FVector3f(0, 0, 1), FVector3f(1, 0, 0))
		.SetColor(FColor::White)
		.SetTexCoord(FVector2f(0.5f, 0.5f));

	// Create ring vertices with color gradient from white (center) to various colors (edge)
	TArray<TArray<int32>> RingVertices;
	RingVertices.SetNum(Rings);

	for (int32 Ring = 0; Ring < Rings; Ring++)
	{
		const float RingRadius = OuterRadius * ((Ring + 1) / (float)Rings);
		const float ColorBlend = (Ring + 1) / (float)Rings;

		RingVertices[Ring].SetNum(Segments);

		for (int32 Seg = 0; Seg < Segments; Seg++)
		{
			const float Angle = (Seg / (float)Segments) * 2.0f * PI;
			const float X = FMath::Cos(Angle) * RingRadius;
			const float Y = FMath::Sin(Angle) * RingRadius;

			// Create rainbow color based on angle
			const float Hue = (Seg / (float)Segments) * 360.0f;
			FLinearColor EdgeColor = FLinearColor(Hue, 1.0f, 1.0f).HSVToLinearRGB();
			FLinearColor BlendedColor = FMath::Lerp(FLinearColor::White, EdgeColor, ColorBlend);

			const FVector3f Position = Offset + FVector3f(X, Y, 0.0f);

			int32 VertexIndex = Builder.AddVertex(Position)
				.SetNormalAndTangent(FVector3f(0, 0, 1), FVector3f(1, 0, 0))
				.SetColor(BlendedColor.ToFColor(false))
				.SetTexCoord(FVector2f(0.5f + X / (OuterRadius * 2), 0.5f + Y / (OuterRadius * 2)));

			RingVertices[Ring][Seg] = VertexIndex;
		}
	}

	// Create triangles from center to first ring
	for (int32 Seg = 0; Seg < Segments; Seg++)
	{
		int32 NextSeg = (Seg + 1) % Segments;
		Builder.AddTriangle(CenterVertex, RingVertices[0][NextSeg], RingVertices[0][Seg]);
	}

	// Create triangles between rings
	for (int32 Ring = 0; Ring < Rings - 1; Ring++)
	{
		for (int32 Seg = 0; Seg < Segments; Seg++)
		{
			int32 NextSeg = (Seg + 1) % Segments;

			int32 V0 = RingVertices[Ring][Seg];
			int32 V1 = RingVertices[Ring][NextSeg];
			int32 V2 = RingVertices[Ring + 1][Seg];
			int32 V3 = RingVertices[Ring + 1][NextSeg];

			Builder.AddTriangle(V0, V1, V2);
			Builder.AddTriangle(V1, V3, V2);
		}
	}

	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("RadialGradient"));

	RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);
}

void ARealtimeMeshVisualTest_VertexColors::CreateHeightGradientMesh(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset)
{
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 Slices = 12;
	const float Radius = 50.0f;
	const float Height = 150.0f;

	// Create vertices for cylinder with vertical rainbow gradient
	TArray<TArray<int32>> SliceVertices;
	SliceVertices.SetNum(Slices + 1);

	for (int32 Z = 0; Z <= Slices; Z++)
	{
		const float ZPos = (Z / (float)Slices) * Height;
		const float ColorBlend = Z / (float)Slices;

		// Rainbow gradient from bottom (red) to top (violet)
		const float Hue = ColorBlend * 300.0f; // 0 (red) to 300 (violet)
		FLinearColor HSVColor(Hue, 1.0f, 1.0f);
		FColor SliceColor = HSVColor.HSVToLinearRGB().ToFColor(false);

		SliceVertices[Z].SetNum(Slices);

		for (int32 Seg = 0; Seg < Slices; Seg++)
		{
			const float Angle = (Seg / (float)Slices) * 2.0f * PI;
			const float X = FMath::Cos(Angle) * Radius;
			const float Y = FMath::Sin(Angle) * Radius;

			const FVector3f Position = Offset + FVector3f(X, Y, ZPos);
			const FVector3f Normal = FVector3f(X, Y, 0).GetSafeNormal();
			const FVector3f Tangent = FVector3f(-Y, X, 0).GetSafeNormal();

			int32 VertexIndex = Builder.AddVertex(Position)
				.SetNormalAndTangent(Normal, Tangent)
				.SetColor(SliceColor)
				.SetTexCoord(FVector2f(Seg / (float)Slices, Z / (float)Slices));

			SliceVertices[Z][Seg] = VertexIndex;
		}
	}

	// Create triangles between slices
	for (int32 Z = 0; Z < Slices; Z++)
	{
		for (int32 Seg = 0; Seg < Slices; Seg++)
		{
			int32 NextSeg = (Seg + 1) % Slices;

			int32 V0 = SliceVertices[Z][Seg];
			int32 V1 = SliceVertices[Z][NextSeg];
			int32 V2 = SliceVertices[Z + 1][Seg];
			int32 V3 = SliceVertices[Z + 1][NextSeg];

			Builder.AddTriangle(V2, V1, V0);
			Builder.AddTriangle(V2, V3, V1);
		}
	}

	// Add top cap
	{
		const FVector3f TopCenter = Offset + FVector3f(0, 0, Height);
		FColor TopColor = FLinearColor(300.0f, 1.0f, 1.0f).HSVToLinearRGB().ToFColor(false);

		int32 TopCenterVertex = Builder.AddVertex(TopCenter)
			.SetNormalAndTangent(FVector3f(0, 0, 1), FVector3f(1, 0, 0))
			.SetColor(TopColor)
			.SetTexCoord(FVector2f(0.5f, 0.5f));

		for (int32 Seg = 0; Seg < Slices; Seg++)
		{
			int32 NextSeg = (Seg + 1) % Slices;
			Builder.AddTriangle(SliceVertices[Slices][NextSeg], SliceVertices[Slices][Seg], TopCenterVertex);
		}
	}

	// Add bottom cap
	{
		const FVector3f BottomCenter = Offset + FVector3f(0, 0, 0);
		FColor BottomColor = FLinearColor(0.0f, 1.0f, 1.0f).HSVToLinearRGB().ToFColor(false);

		int32 BottomCenterVertex = Builder.AddVertex(BottomCenter)
			.SetNormalAndTangent(FVector3f(0, 0, -1), FVector3f(1, 0, 0))
			.SetColor(BottomColor)
			.SetTexCoord(FVector2f(0.5f, 0.5f));

		for (int32 Seg = 0; Seg < Slices; Seg++)
		{
			int32 NextSeg = (Seg + 1) % Slices;
			Builder.AddTriangle(SliceVertices[0][Seg], SliceVertices[0][NextSeg], BottomCenterVertex);
		}
	}

	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("HeightGradient"));

	RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);
}
