// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_VertexColors.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_VertexColors::ARealtimeMeshExample_Simple_VertexColors()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_VertexColors",
		"URealtimeMeshSimple: four ways to author per-vertex colors (rainbow grid, RGB box, radial gradient, height gradient).");
}

void ARealtimeMeshExample_Simple_VertexColors::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Use a material that reads the vertex color stream to see these.
	RealtimeMesh->SetupMaterialSlot(0, "VertexColorMaterial");

	CreateRainbowGrid(RealtimeMesh, FVector3f(-200.0f, -200.0f, 0.0f));
	CreateRGBCornerBox(RealtimeMesh, FVector3f(200.0f, -200.0f, 0.0f));
	CreateRadialGradientPlane(RealtimeMesh, FVector3f(-200.0f, 200.0f, 0.0f));
	CreateHeightGradientMesh(RealtimeMesh, FVector3f(200.0f, 200.0f, 0.0f));

	VerifyMeshBuilt();
}

void ARealtimeMeshExample_Simple_VertexColors::CreateRainbowGrid(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset)
{
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 GridSize = 10;
	const float CellSize = 15.0f;
	const float TotalSize = GridSize * CellSize;

	TArray<int32> VertexIndices;
	VertexIndices.SetNum((GridSize + 1) * (GridSize + 1));

	for (int32 Y = 0; Y <= GridSize; Y++)
	{
		for (int32 X = 0; X <= GridSize; X++)
		{
			const float XPos = X * CellSize - TotalSize * 0.5f;
			const float YPos = Y * CellSize - TotalSize * 0.5f;

			const float Hue = ((float)X / GridSize + (float)Y / GridSize) * 0.5f;
			const FLinearColor HSVColor(Hue * 360.0f, 1.0f, 1.0f);
			const FColor VertexColor = HSVColor.HSVToLinearRGB().ToFColor(false);

			const int32 VertexIndex = Builder.AddVertex(Offset + FVector3f(XPos, YPos, 0.0f))
				.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
				.SetColor(VertexColor)
				.SetTexCoord(FVector2f((float)X / GridSize, (float)Y / GridSize));

			VertexIndices[Y * (GridSize + 1) + X] = VertexIndex;
		}
	}

	for (int32 Y = 0; Y < GridSize; Y++)
	{
		for (int32 X = 0; X < GridSize; X++)
		{
			const int32 V0 = VertexIndices[Y * (GridSize + 1) + X];
			const int32 V1 = VertexIndices[Y * (GridSize + 1) + (X + 1)];
			const int32 V2 = VertexIndices[(Y + 1) * (GridSize + 1) + X];
			const int32 V3 = VertexIndices[(Y + 1) * (GridSize + 1) + (X + 1)];

			Builder.AddTriangle(V0, V2, V1);
			Builder.AddTriangle(V1, V2, V3);
		}
	}

	RealtimeMesh->CreateBufferSet(FRealtimeMeshBufferSetKey::Create(0, FName("RainbowGrid")), StreamSet);
}

void ARealtimeMeshExample_Simple_VertexColors::CreateRGBCornerBox(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset)
{
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const float Size = 75.0f;
	const FVector3f HalfSize(Size, Size, Size);

	struct FBoxVertex { FVector3f Position; FColor Color; };
	const TArray<FBoxVertex> Corners = {
		{ FVector3f(-1, -1, -1), FColor::Black },
		{ FVector3f( 1, -1, -1), FColor::Red },
		{ FVector3f(-1,  1, -1), FColor::Green },
		{ FVector3f( 1,  1, -1), FColor::Yellow },
		{ FVector3f(-1, -1,  1), FColor::Blue },
		{ FVector3f( 1, -1,  1), FColor::Magenta },
		{ FVector3f(-1,  1,  1), FColor::Cyan },
		{ FVector3f( 1,  1,  1), FColor::White }
	};

	struct FFace { int32 Indices[4]; FVector3f Normal; };
	const TArray<FFace> Faces = {
		{ {0, 1, 3, 2}, FVector3f( 0,  0, -1) },
		{ {5, 4, 6, 7}, FVector3f( 0,  0,  1) },
		{ {4, 0, 2, 6}, FVector3f(-1,  0,  0) },
		{ {1, 5, 7, 3}, FVector3f( 1,  0,  0) },
		{ {4, 5, 1, 0}, FVector3f( 0, -1,  0) },
		{ {2, 3, 7, 6}, FVector3f( 0,  1,  0) }
	};

	for (const FFace& Face : Faces)
	{
		const FVector3f Tangent = FMath::Abs(Face.Normal.Z) < 0.9f
			? FVector3f(0, 0, 1).Cross(Face.Normal).GetSafeNormal()
			: FVector3f(1, 0, 0);

		const int32 V0 = Builder.AddVertex(Offset + Corners[Face.Indices[0]].Position * HalfSize)
			.SetNormalAndTangent(Face.Normal, Tangent).SetColor(Corners[Face.Indices[0]].Color).SetTexCoord(FVector2f(0, 0));
		const int32 V1 = Builder.AddVertex(Offset + Corners[Face.Indices[1]].Position * HalfSize)
			.SetNormalAndTangent(Face.Normal, Tangent).SetColor(Corners[Face.Indices[1]].Color).SetTexCoord(FVector2f(1, 0));
		const int32 V2 = Builder.AddVertex(Offset + Corners[Face.Indices[2]].Position * HalfSize)
			.SetNormalAndTangent(Face.Normal, Tangent).SetColor(Corners[Face.Indices[2]].Color).SetTexCoord(FVector2f(1, 1));
		const int32 V3 = Builder.AddVertex(Offset + Corners[Face.Indices[3]].Position * HalfSize)
			.SetNormalAndTangent(Face.Normal, Tangent).SetColor(Corners[Face.Indices[3]].Color).SetTexCoord(FVector2f(0, 1));

		Builder.AddTriangle(V0, V1, V2);
		Builder.AddTriangle(V0, V2, V3);
	}

	RealtimeMesh->CreateBufferSet(FRealtimeMeshBufferSetKey::Create(0, FName("RGBBox")), StreamSet);
}

void ARealtimeMeshExample_Simple_VertexColors::CreateRadialGradientPlane(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset)
{
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 Segments = 16;
	const int32 Rings = 8;
	const float OuterRadius = 75.0f;

	const int32 CenterVertex = Builder.AddVertex(Offset)
		.SetNormalAndTangent(FVector3f(0, 0, 1), FVector3f(1, 0, 0))
		.SetColor(FColor::White)
		.SetTexCoord(FVector2f(0.5f, 0.5f));

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

			const float Hue = (Seg / (float)Segments) * 360.0f;
			const FLinearColor EdgeColor = FLinearColor(Hue, 1.0f, 1.0f).HSVToLinearRGB();
			const FLinearColor BlendedColor = FMath::Lerp(FLinearColor::White, EdgeColor, ColorBlend);

			RingVertices[Ring][Seg] = Builder.AddVertex(Offset + FVector3f(X, Y, 0.0f))
				.SetNormalAndTangent(FVector3f(0, 0, 1), FVector3f(1, 0, 0))
				.SetColor(BlendedColor.ToFColor(false))
				.SetTexCoord(FVector2f(0.5f + X / (OuterRadius * 2), 0.5f + Y / (OuterRadius * 2)));
		}
	}

	for (int32 Seg = 0; Seg < Segments; Seg++)
	{
		const int32 NextSeg = (Seg + 1) % Segments;
		Builder.AddTriangle(CenterVertex, RingVertices[0][NextSeg], RingVertices[0][Seg]);
	}

	for (int32 Ring = 0; Ring < Rings - 1; Ring++)
	{
		for (int32 Seg = 0; Seg < Segments; Seg++)
		{
			const int32 NextSeg = (Seg + 1) % Segments;
			const int32 V0 = RingVertices[Ring][Seg];
			const int32 V1 = RingVertices[Ring][NextSeg];
			const int32 V2 = RingVertices[Ring + 1][Seg];
			const int32 V3 = RingVertices[Ring + 1][NextSeg];

			Builder.AddTriangle(V0, V1, V2);
			Builder.AddTriangle(V1, V3, V2);
		}
	}

	RealtimeMesh->CreateBufferSet(FRealtimeMeshBufferSetKey::Create(0, FName("RadialGradient")), StreamSet);
}

void ARealtimeMeshExample_Simple_VertexColors::CreateHeightGradientMesh(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset)
{
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 Slices = 12;
	const float Radius = 50.0f;
	const float Height = 150.0f;

	TArray<TArray<int32>> SliceVertices;
	SliceVertices.SetNum(Slices + 1);

	for (int32 Z = 0; Z <= Slices; Z++)
	{
		const float ZPos = (Z / (float)Slices) * Height;
		const float ColorBlend = Z / (float)Slices;
		const float Hue = ColorBlend * 300.0f;
		const FColor SliceColor = FLinearColor(Hue, 1.0f, 1.0f).HSVToLinearRGB().ToFColor(false);

		SliceVertices[Z].SetNum(Slices);

		for (int32 Seg = 0; Seg < Slices; Seg++)
		{
			const float Angle = (Seg / (float)Slices) * 2.0f * PI;
			const float X = FMath::Cos(Angle) * Radius;
			const float Y = FMath::Sin(Angle) * Radius;

			SliceVertices[Z][Seg] = Builder.AddVertex(Offset + FVector3f(X, Y, ZPos))
				.SetNormalAndTangent(FVector3f(X, Y, 0).GetSafeNormal(), FVector3f(-Y, X, 0).GetSafeNormal())
				.SetColor(SliceColor)
				.SetTexCoord(FVector2f(Seg / (float)Slices, Z / (float)Slices));
		}
	}

	for (int32 Z = 0; Z < Slices; Z++)
	{
		for (int32 Seg = 0; Seg < Slices; Seg++)
		{
			const int32 NextSeg = (Seg + 1) % Slices;
			const int32 V0 = SliceVertices[Z][Seg];
			const int32 V1 = SliceVertices[Z][NextSeg];
			const int32 V2 = SliceVertices[Z + 1][Seg];
			const int32 V3 = SliceVertices[Z + 1][NextSeg];

			Builder.AddTriangle(V2, V1, V0);
			Builder.AddTriangle(V2, V3, V1);
		}
	}

	// Top cap
	{
		const FColor TopColor = FLinearColor(300.0f, 1.0f, 1.0f).HSVToLinearRGB().ToFColor(false);
		const int32 TopCenterVertex = Builder.AddVertex(Offset + FVector3f(0, 0, Height))
			.SetNormalAndTangent(FVector3f(0, 0, 1), FVector3f(1, 0, 0)).SetColor(TopColor).SetTexCoord(FVector2f(0.5f, 0.5f));

		for (int32 Seg = 0; Seg < Slices; Seg++)
		{
			const int32 NextSeg = (Seg + 1) % Slices;
			Builder.AddTriangle(SliceVertices[Slices][NextSeg], SliceVertices[Slices][Seg], TopCenterVertex);
		}
	}

	// Bottom cap
	{
		const FColor BottomColor = FLinearColor(0.0f, 1.0f, 1.0f).HSVToLinearRGB().ToFColor(false);
		const int32 BottomCenterVertex = Builder.AddVertex(Offset)
			.SetNormalAndTangent(FVector3f(0, 0, -1), FVector3f(1, 0, 0)).SetColor(BottomColor).SetTexCoord(FVector2f(0.5f, 0.5f));

		for (int32 Seg = 0; Seg < Slices; Seg++)
		{
			const int32 NextSeg = (Seg + 1) % Slices;
			Builder.AddTriangle(SliceVertices[0][Seg], SliceVertices[0][NextSeg], BottomCenterVertex);
		}
	}

	RealtimeMesh->CreateBufferSet(FRealtimeMeshBufferSetKey::Create(0, FName("HeightGradient")), StreamSet);
}
