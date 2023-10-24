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
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1),
		FVector3f(0, 0, 1),
	};

	for (int32 Index = 0; Index < 10; Index++)
	{
		InitialBuilder.Add(TRealtimeMeshTangents<FPackedRGBA16N>(InitialData[Index], InitialData[9 - Index]));
	}


	TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FPackedNormal>> ConvertedBuilder(DataStream, true);

	for (int32 Index = 0; Index < 10; Index++)
	{	
		TestTrue(FString::Printf(TEXT("TestRow: %d Element 0"), Index), ConvertedBuilder[Index][0].ToFVector3f().Equals(InitialData[9 - Index]));
		TestTrue(FString::Printf(TEXT("TestRow: %d Element 1"), Index), ConvertedBuilder[Index][1].ToFVector3f().Equals(InitialData[Index]));
	}
	
	// Make the test pass by returning true, or fail by returning false.
	return true;
}
