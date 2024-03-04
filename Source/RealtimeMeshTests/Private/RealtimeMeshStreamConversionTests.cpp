#include "Mesh/RealtimeMeshBuilder.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshStreamConversionTests, "RealtimeMeshComponent.RealtimeMeshStreamConversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

using namespace RealtimeMesh;

bool RealtimeMeshStreamConversionTests::RunTest(const FString& Parameters)
{
	FRealtimeMeshStream DataStream = FRealtimeMeshStream::Create<TRealtimeMeshTangents<FPackedRGBA16N>>(RealtimeMesh::FRealtimeMeshStreams::Position);
	TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FPackedRGBA16N>> InitialBuilder(DataStream);

	const FVector3f InitialData[] =
	{
		FVector3f(0, 0, 1),
		FVector3f(0, 1, 0),
		FVector3f(1, 0, 0),
		FVector3f(0, 0, 0),
		FVector3f(0, 0, 1),
		FVector3f(0, 1, 0),
		FVector3f(1, 0, 0),
		FVector3f(0, 0, 0),
		FVector3f(0, 0, 1),
		FVector3f(0, 1, 0),
	};

	TArray<TRealtimeMeshTangents<FVector4f>> InitialCombined;
	for (int32 Index = 0; Index < 10; Index++)
	{
		auto Initial = TRealtimeMeshTangents<FVector4f>(InitialData[Index], InitialData[9 - Index]);
		InitialCombined.Add(Initial);

		InitialBuilder.Add(Initial);
		TRealtimeMeshTangents<FVector4f> A = InitialBuilder.GetValue(Index);
		TestTrue(FString::Printf(TEXT("TestRow: %d"), Index), A == Initial);
	}


	TRealtimeMeshTangents<FVector4f> First = InitialCombined[0];
	
	TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FPackedRGBA16N>, TRealtimeMeshTangents<FPackedRGBA16N>> DirectBuilder(DataStream);
	TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FVector4f>, TRealtimeMeshTangents<FPackedRGBA16N>> ConvertedBuilder(DataStream);

	for (int32 Index = 0; Index < 10; Index++)
	{ 
		
		TRealtimeMeshTangents<FVector4f> A(DirectBuilder[Index]);
		TRealtimeMeshTangents<FVector4f> B = ConvertedBuilder[Index];
		TestTrue(FString::Printf(TEXT("TestRow: %d Element 0"), Index), A == InitialCombined[Index]);
		TestTrue(FString::Printf(TEXT("TestRow: %d"), Index), ConvertedBuilder[Index].GetElement(0) == InitialData[9 - Index]);
		TestTrue(FString::Printf(TEXT("TestRow: %d Element 1"), Index), ConvertedBuilder[Index].GetElement(1) == InitialData[Index]);
	}


	const TArray<uint16> TestDataA =	
	{
		3423,
		6421,
		6477,
		24234,
		7423,
		117,
		7754,
		6787,
		9106,
		10022
	};	

	const TArray<uint32> TestDataB =	
	{
		624,
		523,
		436,
		4587,
		924,
		1955,
		8583,
		318,
		85,
		10349
	};

	const TArray<FIntPoint> TestDataC =
	{
		// Just some random points for testing...
		FIntPoint(0, 0),
		FIntPoint(1, 0),
		FIntPoint(0, 1),
		FIntPoint(1, 1),
		FIntPoint(2, 0),
		FIntPoint(0, 2),
		FIntPoint(2, 2),
		FIntPoint(3, 0),
		FIntPoint(0, 3),
		FIntPoint(3, 3),
	};
	
	const TArray<FIntVector> TestDataD =
	{
		// Just some points with large arbitrary numbers for testing...
		FIntVector(3, 6, 7),
		FIntVector(-3, 5, 6),
		FIntVector(0, 1, 2),
		FIntVector(300, 500, 1000),
		FIntVector(20, 30, 40),
		FIntVector(50, 60, 70),
		FIntVector(80, 90, 100),
		FIntVector(110, 120, 130),
		FIntVector(140, 150, 160),
		FIntVector(170, 180, 190),
		FIntVector(200, 210, 220),
		FIntVector(230, 240, 250),
		FIntVector(260, 270, 280),
		FIntVector(290, 300, 310),
		FIntVector(320, 330, 340),
		FIntVector(350, 360, 370),
		FIntVector(380, 390, 400),
		FIntVector(410, 420, 430),
		FIntVector(440, 450, 460),
		FIntVector(470, 480, 490),
		FIntVector(500, 510, 520),
		FIntVector(530, 540, 550),
		FIntVector(560, 570, 580),
		FIntVector(590, 600, 610),
		FIntVector(620, 630, 640),
		FIntVector(650, 660, 670),
		FIntVector(680, 690, 700),
		FIntVector(710, 720, 730),
		FIntVector(740, 750, 760),
		FIntVector(770, 780, 790),
		FIntVector(800, 810, 820),
		FIntVector(830, 840, 850)	
	};


	// Test A  Simple Full Copy
	// Test B  Interleaved Straight Copy
	// Test C  Simple Conversion Full Width
	// Test D  Conversion Interleaved Single Element
	// Test E  Conversion Interleaved Multi Element

	
	FRealtimeMeshStream TestTargetStream = FRealtimeMeshStream::Create<FIntVector>(FRealtimeMeshStreams::Position);
	TestTargetStream.Append(TestDataD);


	TArray<FIntVector> TestDirectCopyData;
	TestTargetStream.CopyRange(0, TestDataD.Num(), TestDirectCopyData);
	TestTrue(TEXT("Test Direct Copy"), TestDataD == TestDirectCopyData);

	
	

	FRealtimeMeshStream BulkConversionStream = FRealtimeMeshStream::Create<TRealtimeMeshTangents<FPackedRGBA16N>>(RealtimeMesh::FRealtimeMeshStreams::Position);
	BulkConversionStream.Append(InitialCombined);
	TArray<TRealtimeMeshTangents<FVector4f>> BulkConverted;

	BulkConversionStream.CopyRange(0, InitialCombined.Num(), BulkConverted);
	TestTrue(TEXT("Test Bulk Conversion Append/CopyRange"), InitialCombined == BulkConverted);






	
	// Make the test pass by returning true, or fail by returning false.
	return true;
}