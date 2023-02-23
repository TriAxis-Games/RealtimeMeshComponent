// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Data/RealtimeMeshDataBuilder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshBuilderTests, "RealtimeMeshComponent.RealtimeMeshBuilder",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool RealtimeMeshBuilderTests::RunTest(const FString& Parameters)
{
	const auto Builder = MakeShared<RealtimeMesh::FRealtimeMeshVertexDataBuilder>();

	constexpr int32 TestDataNum = 128;
	FVector3f TestData[TestDataNum];
	const FRandomStream RandomStream(19385823);
	
	for (int32 Index = 0; Index < TestDataNum; Index++)
	{
		TestData[Index] = FVector3f(RandomStream.VRand());
	}

	

	auto TestBuffer = Builder->CreateVertexStream<FVector3f>(FName(TEXT("TestStream")));
	
	TestTrue(TEXT("Stride"), TestBuffer->GetStride() == sizeof(FVector3f));
	
	const auto ValidateBufferData = [&](FString BufferTestName)
	{
		TestTrue(BufferTestName + TEXT(" Length"), TestBuffer->Num() == TestDataNum);
		TestTrue(BufferTestName + TEXT(" ByteCount"), TestBuffer->Num() * TestBuffer->GetStride() == TestDataNum * sizeof(FVector3f));
		TestTrue(BufferTestName + TEXT(" Data"), !FMemory::Memcmp(TestBuffer->GetDataRaw(), &TestData, TestBuffer->Num() * TestBuffer->GetStride()));
	};	
	
	TestBuffer->Append(MakeArrayView(TestData));
	ValidateBufferData(TEXT("Append ArrayView"));
	

	
	return true;
}
