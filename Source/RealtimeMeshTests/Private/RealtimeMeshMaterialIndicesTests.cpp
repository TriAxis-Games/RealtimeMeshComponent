#include "Algo/RandomShuffle.h"
#include "Mesh/RealtimeMeshAlgo.h"
#include "Mesh/RealtimeMeshBuilder.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshMaterialIndicesTests, "RealtimeMeshComponent.RealtimeMeshMaterialIndices", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

using namespace RealtimeMesh;

bool RealtimeMeshMaterialIndicesTests::RunTest(const FString& Parameters)
{
	const int32 StartingData[] = { 0, 0, 0, 1, 2, 3, 0, 5, 0, 9, 5, 5, 6, 6, 1, 1 };
	const uint32 ExpectedRemapTable[] = { 0, 1, 2, 6, 8, 3, 14, 15, 4, 5, 7, 10, 11, 12, 13, 9 };
	const uint32 ExpectedEndingData[] = { 0, 0, 0, 0, 0, 1, 1, 1, 2, 3, 5, 5, 5, 6, 6, 9 };

	FRealtimeMeshStream MaterialIndices = FRealtimeMeshStream::Create<uint32>(FRealtimeMeshStreams::PolyGroups);
	TRealtimeMeshStreamBuilder<uint32> MaterialIndicesBuilder(MaterialIndices);

	for (int32 Index = 0; Index < 16; Index++)
	{
		MaterialIndicesBuilder.Add(StartingData[Index]);
	}

	TestFalse(TEXT("InitialDataIsntOptimal"), RealtimeMeshAlgo::ArePolygonGroupIndicesOptimal(MaterialIndices));	

	
	uint32 RemapTable[16];	
	TestTrue(TEXT("GenerateSortedRemapTable"), RealtimeMeshAlgo::GenerateSortedRemapTable(MaterialIndices, MakeArrayView(RemapTable)));

	TestTrue(TEXT("GotExpectedRemapTable"), FMemory::Memcmp(RemapTable, ExpectedRemapTable, sizeof(uint32) * 16) == 0);

	RealtimeMeshAlgo::ApplyRemapTableToStream(MakeArrayView(RemapTable), MaterialIndices);
	
	for (int32 Index = 0; Index < 16; Index++)
	{
		TestTrue(TEXT("FinalIndicesCorrect"), ExpectedEndingData[Index] == MaterialIndicesBuilder[Index]);
	}

	TestTrue(TEXT("InitialDataIsOptimalByIndices"), RealtimeMeshAlgo::ArePolygonGroupIndicesOptimal(MaterialIndices));	

	TArray<FRealtimeMeshPolygonGroupRange> Segments;
	
	RealtimeMeshAlgo::GatherSegmentsFromPolygonGroupIndices(MaterialIndices, [&Segments](const FRealtimeMeshPolygonGroupRange& NewSegment)
	{
		Segments.Add(NewSegment);
	});

	const TArray KnownGoodSegments = {
		FRealtimeMeshPolygonGroupRange(0, 5, 0),
		FRealtimeMeshPolygonGroupRange(5, 3, 1),
		FRealtimeMeshPolygonGroupRange(8, 1, 2),
		FRealtimeMeshPolygonGroupRange(9, 1, 3),
		FRealtimeMeshPolygonGroupRange(10, 3, 5),
		FRealtimeMeshPolygonGroupRange(13, 2, 6),
		FRealtimeMeshPolygonGroupRange(15, 1, 9)
	};
	TestTrue(TEXT("AreSegmentsValid"), Segments == KnownGoodSegments);
	TestTrue(TEXT("InitialDataIsOptimal"), RealtimeMeshAlgo::ArePolygonGroupIndicesOptimal(Segments));


	TArray<int32> PolyGroupIndices = { 1, 1, 1, 1, 0, 0, 0, 0, 0, 2, 2, 2 };
	TArray<int32> Triangles = {
		0, 1, 2,
		3, 0, 1,
		2, 3, 4,
		5, 6, 7,
		
		0, 1, 2,
		3, 4, 5,
		6, 7, 8,
		9, 10, 11,
		5, 6, 7,
		
		8, 9, 10,
		11, 12, 10,
		12, 11, 10
	};

	/*
	TMap<int32, FRealtimeMeshStreamRange> StreamSegments;
	RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupIndices<int32, int32>(PolyGroupIndices, Triangles, StreamSegments);

	
	TestTrue(TEXT("StreamSegmentsLength"), StreamSegments.Num() == 3);
	TestTrue(TEXT("StreamSegmentsElement0"), StreamSegments[0] == FRealtimeMeshStreamRange(0, 8, 0, 12));
	TestTrue(TEXT("StreamSegmentsElement1"), StreamSegments[1] == FRealtimeMeshStreamRange(0, 12, 12, 27));
	TestTrue(TEXT("StreamSegmentsElement2"), StreamSegments[2] == FRealtimeMeshStreamRange(8, 13, 27, 36));
	*/
	
	
	// Make the test pass by returning true, or fail by returning false.
	return true;
}
