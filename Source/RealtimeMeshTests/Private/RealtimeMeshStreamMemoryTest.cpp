#include "Mesh/RealtimeMeshAlgo.h"
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
	
	// Make the test pass by returning true, or fail by returning false.
	return true;
}
