#include "Mesh/RealtimeMeshBuilder.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshStreamConversionTests, "RealtimeMeshComponent.RealtimeMeshStreamConversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

using namespace RealtimeMesh;

PRAGMA_DISABLE_OPTIMIZATION
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
	
	// Make the test pass by returning true, or fail by returning false.
	return true;
}
PRAGMA_ENABLE_OPTIMIZATION