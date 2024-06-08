// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "Mesh/RealtimeMeshAlgo.h"
#include "Mesh/RealtimeMeshBuilder.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshStreamMemoryTest, "Private.RealtimeMeshStreamMemoryTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

using namespace RealtimeMesh;

struct FChunkMesh {
	FRealtimeMeshStream Vertices = FRealtimeMeshStream::Create<FVector3f>(FRealtimeMeshStreams::Position);
	FRealtimeMeshStream Tangents = FRealtimeMeshStream::Create<TRealtimeMeshTangents<FPackedNormal>>(FRealtimeMeshStreams::Tangents);
	FRealtimeMeshStream Colors = FRealtimeMeshStream::Create<FColor>(FRealtimeMeshStreams::Color);
	FRealtimeMeshStream UV0 = FRealtimeMeshStream::Create<FVector2DHalf>(FRealtimeMeshStreams::TexCoords);
};

bool RealtimeMeshStreamMemoryTest::RunTest(const FString& Parameters)
{
	FChunkMesh TestMesh;
	TestMesh.Vertices.Add(FVector3f(1.f,1.f,1.f));
	TestMesh.Tangents.Add(TRealtimeMeshTangents<FPackedNormal>(FVector3f::UpVector, FVector3f::UpVector));
	TestMesh.Colors.Add(FColor(255, 255, 255, 255));
	TestMesh.UV0.Add(FVector2DHalf(1, 1));

	FRealtimeMeshStreamSet TestSet;
	TestSet.AddStream(MoveTemp(TestMesh.Vertices));
	TestSet.AddStream(MoveTemp(TestMesh.Tangents));
	TestSet.AddStream(MoveTemp(TestMesh.Colors));
	TestSet.AddStream(MoveTemp(TestMesh.UV0));

	TestTrue(TEXT("Correct number of streams first pass"), TestSet.Num() == 4);
	
	TestMesh = FChunkMesh();	
	TestMesh.Vertices.Add(FVector3f(1.f,1.f,1.f));
	TestMesh.Tangents.Add(TRealtimeMeshTangents<FPackedNormal>(FVector3f::UpVector, FVector3f::UpVector));
	TestMesh.Colors.Add(FColor(255, 255, 255, 255));
	TestMesh.UV0.Add(FVector2DHalf(1, 1));

	FRealtimeMeshStreamSet TestSet2;
	TestSet2.AddStream(MoveTemp(TestMesh.Vertices));
	TestSet2.AddStream(MoveTemp(TestMesh.Tangents));
	TestSet2.AddStream(MoveTemp(TestMesh.Colors));
	TestSet2.AddStream(MoveTemp(TestMesh.UV0));

	TestTrue(TEXT("Correct number of streams second pass"), TestSet.Num() == 4);

	
	const FRealtimeMeshStream PositionStream = FRealtimeMeshStream::Create<FVector3f>(FRealtimeMeshStreams::Position);
	TArrayView<const FVector3f> StreamDataView = PositionStream.GetArrayView<const FVector3f>();//StreamDataPositions;

	TRealtimeMeshStreamBuilder<const FVector3f> PositionBuilder(PositionStream);
	for (int32 Index = 0; Index < PositionBuilder.Num(); ++Index)
	{
		UE_LOG(LogTemp, Warning, TEXT("X %f"), PositionBuilder.GetValue(Index).X);
	}
	
	// Make the test pass by returning true, or fail by returning false.
	return true;
}
