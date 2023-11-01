// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Mesh/RealtimeMeshBuilder.h"
#include "Misc/AutomationTest.h"
#include "Mesh/RealtimeMeshDataStream.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshBuilderTests, "RealtimeMeshComponent.RealtimeMeshBuilder",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

using namespace RealtimeMesh;

bool RealtimeMeshBuilderTests::RunTest(const FString& Parameters)
{
	const FPackedNormal Val0 = FPackedNormal(FVector4f(0, 1, 1, 0));
	const FPackedNormal Val1 = FPackedNormal(FVector4f(1, 0, 1, 0));
	const FPackedNormal Val2 = FPackedNormal(FVector4f(1, 0.5, 1, 0.5));
	const FPackedNormal Val3 = FPackedNormal(FVector4f(0, 0.5, 1, 0.5));
	const FPackedNormal Val4 = FPackedNormal(FVector4f(1, 0, 1, 0.5));

	FRealtimeMeshStream TestStream = FRealtimeMeshStream::Create<FPackedNormal[4]>(FRealtimeMeshStreams::Position);
	TRealtimeMeshStreamBuilder<FPackedNormal[4]> TestStreamBuilder(TestStream);
	TestStreamBuilder.Add(Val0, Val1, Val2, Val3);

	TestTrue(TEXT("TestRow Initial Get(0)"), TestStreamBuilder[0][0] == Val0);
	TestTrue(TEXT("TestRow Initial Get(1)"), TestStreamBuilder[0][1] == Val1);
	TestTrue(TEXT("TestRow Initial Get(2)"), TestStreamBuilder[0][2] == Val2);
	TestTrue(TEXT("TestRow Initial Get(3)"), TestStreamBuilder[0][3] == Val3);

	TestStreamBuilder.Edit(0).SetAll(Val1, Val2, Val3, Val4);
	
	TestTrue(TEXT("TestRow AfterSetAll Get(0)"), TestStreamBuilder[0][0] == Val1);
	TestTrue(TEXT("TestRow AfterSetAll Get(1)"), TestStreamBuilder[0][1] == Val2);
	TestTrue(TEXT("TestRow AfterSetAll Get(2)"), TestStreamBuilder[0][2] == Val3);
	TestTrue(TEXT("TestRow AfterSetAll Get(3)"), TestStreamBuilder[0][3] == Val4);

	
	
	TestStreamBuilder.Empty();

	TestStreamBuilder.Add(Val0);
	TestStreamBuilder.Add(Val1.ToFVector4f());
	TestStreamBuilder.Add(Val2.ToFVector4());
	TestStreamBuilder.Add(Val3);
	TestStreamBuilder.Add(Val4);
	TestStreamBuilder.RemoveAt(2);

	TestTrue(TEXT("TestBuilder Get(1)"), TestStreamBuilder.Get(1)[0] == Val1);
	TestTrue(TEXT("TestBuilder Get(2)"), TestStreamBuilder.Get(2)[0] == Val3);
	TestStreamBuilder.SetElement(2, 0, Val3);
	TestTrue(TEXT("TestBuilder Get(2)"), TestStreamBuilder.Get(2)[0] == Val3);

	TestStreamBuilder.Empty();
	FPackedNormal TempData[2] = { Val4, Val4 };

	TestStreamBuilder.Add(Val0, Val1);
	TestStreamBuilder.Add(Val1.ToFVector4f(), Val2.ToFVector4f());
	TestStreamBuilder.Add(Val2, Val2);
	TestStreamBuilder.Add(TempData);
	TestStreamBuilder.RemoveAt(2);

	TestTrue(TEXT("TestBuilder Get(2)"), FMemory::Memcmp(TestStreamBuilder.Get(2), TempData, sizeof(FPackedNormal)*2) == 0);
	TestTrue(TEXT("TestBuilder GetElement(0, 1)"), TestStreamBuilder.GetElementValue(0, 1) == Val1);
	TestStreamBuilder.Set(2, TempData);
	TestTrue(TEXT("TestBuilder Get(2) after Set"), FMemory::Memcmp(TestStreamBuilder.Get(2), TempData, sizeof(FPackedNormal)*2) == 0);


	TestStreamBuilder.Add().SetElement(0, Val3).SetElement(1, Val2);
	TestTrue(TEXT("TestBuilder GetElement(3, 0)"), TestStreamBuilder.GetElementValue(3, 0) == Val3);
	TestTrue(TEXT("TestBuilder GetElement(3, 1)"), TestStreamBuilder.GetElementValue(3, 1) == Val2);
	
	TestStreamBuilder.Edit(3).SetRange(1, Val0);
	TestTrue(TEXT("TestBuilder GetElement(3, 1)"), TestStreamBuilder.GetElementValue(3, 1) == Val0);

	
	/*
	//const auto Builder = MakeShared<RealtimeMesh::FRealtimeMeshVertexDataBuilder>();

	constexpr int32 TestDataNum = 128;
	FVector3f TestData[TestDataNum];
	const FRandomStream RandomStream(19385823);
	
	for (int32 Index = 0; Index < TestDataNum; Index++)
	{
		TestData[Index] = FVector3f(RandomStream.VRand());
	}


	RealtimeMesh::TRealtimeMeshStreamBuilder<FVector3f> Vertices("Vertices");
	
	Vertices.Append(TestData);
	TestTrue(TEXT("Vertices Length"), Vertices.Num() == TestDataNum);
	TestTrue(TEXT("Vertices ByteCount"), Vertices.Num() * Vertices.GetStream()->GetStride() == TestDataNum * sizeof(FVector3f));
	TestTrue(TEXT("Vertices Data"), !FMemory::Memcmp(Vertices.GetStream()->GetData(), &TestData, Vertices.Num() * Vertices.GetStream()->GetStride()));


	
	RealtimeMesh::TRealtimeMeshStreamBuilder<RealtimeMesh::FRealtimeMeshTangentsHighPrecision> TestSimpleArray2("Tangents");
	TestSimpleArray2.Emplace(FVector3f(), FVector3f());
	FVector3f Normal;
	int32 VertIdx = TestSimpleArray2.Add().Set(0, FVector3f());

	TestSimpleArray2.Edit(0).SetRange(0, FVector3f(), FVector3f());

	RealtimeMesh::TRealtimeMeshStreamBuilder<RealtimeMesh::FIndex3UI> TestTriangles("Triangles");
	TestTriangles.Emplace(0, 1, 2);
	TestTriangles.Emplace(0, 2, 3);


	TestTrue(TEXT("Triangle 0"), TestTriangles.Get(0) == RealtimeMesh::FIndex3UI(0, 1, 2));
	TestTrue(TEXT("Triangle 1"), TestTriangles.Get(1) == RealtimeMesh::FIndex3UI(0, 2, 3));


	RealtimeMesh::TRealtimeMeshBuilderLocal<> Builder;

	int32 Vert0 = Builder.AddVertex()
		.SetPosition(FVector3f())
		.SetTangents(FVector3f(), FVector3f(), FVector3f())
		.SetTexCoord(FVector2f());
	
	int32 Vert1 = Builder.AddVertex()
		.SetPosition(FVector3f())
		.SetTangents(FVector3f(), FVector3f(), FVector3f())
		.SetTexCoord(FVector2f());
	
	int32 Vert2 = Builder.AddVertex()
		.SetPosition(FVector3f())
		.SetTangents(FVector3f(), FVector3f(), FVector3f())
		.SetTexCoord(FVector2f());
		*/



	

	/*
	auto TestBuffer = Builder->CreateVertexStream<FVector3f>(FName(TEXT("TestStream")));
	
	TestTrue(TEXT("Stride"), TestBuffer->GetStride() == sizeof(FVector3f));
	
	const auto ValidateBufferData = [&](FString BufferTestName)
	{
		TestTrue(BufferTestName + TEXT(" Length"), TestBuffer->Num() == TestDataNum);
		TestTrue(BufferTestName + TEXT(" ByteCount"), TestBuffer->Num() * TestBuffer->GetStride() == TestDataNum * sizeof(FVector3f));
		TestTrue(BufferTestName + TEXT(" Data"), !FMemory::Memcmp(TestBuffer->GetData(), &TestData, TestBuffer->Num() * TestBuffer->GetStride()));
	};	
	
	TestBuffer->Append(MakeArrayView(TestData));
	ValidateBufferData(TEXT("Append ArrayView"));
	*/
	

	
	return true;
}
