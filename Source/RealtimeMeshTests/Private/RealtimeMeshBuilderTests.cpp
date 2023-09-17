// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshBuilder.h"
#include "Misc/AutomationTest.h"
#include "..\..\RealtimeMeshComponent\Public\Data\RealtimeMeshDataStream.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshBuilderTests, "RealtimeMeshComponent.RealtimeMeshBuilder",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool RealtimeMeshBuilderTests::RunTest(const FString& Parameters)
{
	const FPackedNormal Val0 = FPackedNormal(FVector4f(0, 1, 1, 0));
	const FPackedNormal Val1 = FPackedNormal(FVector4f(1, 0, 1, 0));
	const FPackedNormal Val2 = FPackedNormal(FVector4f(1, 0.5, 1, 0.5));
	const FPackedNormal Val3 = FPackedNormal(FVector4f(0, 0.5, 1, 0.5));
	const FPackedNormal Val4 = FPackedNormal(FVector4f(1, 0, 1, 0.5));
	
	FPackedNormal TestRowData[4] = { Val0, Val1, Val2, Val3 };

	RealtimeMesh::TRealtimeMeshStreamRowReader<FPackedNormal[4]> TestRowReadonly(0, &TestRowData);

	TestTrue(TEXT("TestRowReadOnly Get(0)"), TestRowReadonly.Get(0) == TestRowData[0]);
	TestTrue(TEXT("TestRowReadOnly Get(1)"), TestRowReadonly.Get(1) == TestRowData[1]);

	FPackedNormal TestRowRead;
	TestRowReadonly.Get(1, TestRowRead);
	TestTrue(TEXT("TestRowReadOnly Get(1) After SetRange"), TestRowRead == Val1);
	


	
	RealtimeMesh::TRealtimeMeshStreamRowBuilder<FPackedNormal[4]> TestRow(0, &TestRowData);


	TestTrue(TEXT("TestRow Get(0)"), TestRow.Get(0) == TestRowData[0]);
	TestTrue(TEXT("TestRow Get(0)"), &TestRow.Get(0) == &TestRowData[0]);
	TestTrue(TEXT("TestRow Get(1)"), TestRow.Get(1) == TestRowData[1]);
	TestTrue(TEXT("TestRow Get(1)"), &TestRow.Get(1) == &TestRowData[1]);

	TestRow.Get(1) = Val2;	
	TestTrue(TEXT("TestRow Get(1) After Set through Ref"), TestRow.Get(1) == Val2);

	TestRow.Set(1, Val1);
	TestTrue(TEXT("TestRow Get(1) After Set"), TestRow.Get(1) == Val1);

	TestRow.SetRange(1, Val3, Val4);
	TestTrue(TEXT("TestRow Get(1) After SetRange"), TestRow.Get(1) == Val3);
	TestTrue(TEXT("TestRow Get(2) After SetRange"), TestRow.Get(2) == Val4);

	FPackedNormal TestVal;
	TestRow.Get(2, TestVal);
	TestTrue(TEXT("TestRow Get(2, Val))"), TestVal == Val4);

	FPackedNormal* EditVal;
	TestRow.Tie(2, EditVal);
	*EditVal = Val3;
	TestTrue(TEXT("TestRow Get(2) After SetRange"), TestRow.Get(2) == Val3);


	RealtimeMesh::TRealtimeMeshStreamBuilder<FPackedNormal> TestBuilder(
		RealtimeMesh::FRealtimeMeshStreamKey(RealtimeMesh::ERealtimeMeshStreamType::Vertex, "TestBuilder"));

	TestBuilder.Add(Val0);
	TestBuilder.Add(Val1.ToFVector4f());
	TestBuilder.Add(Val2.ToFVector4());
	TestBuilder.Add(Val3);
	TestBuilder.Add(Val4);
	TestBuilder.RemoveAt(2);

	TestTrue(TEXT("TestBuilder Get(1)"), TestBuilder.Get(1) == Val1);
	TestTrue(TEXT("TestBuilder Get(2)"), TestBuilder.Get(2) == Val3);
	TestBuilder.Set(2, Val3);
	TestTrue(TEXT("TestBuilder Get(2)"), TestBuilder.Get(2) == Val3);


	FPackedNormal TempData[2] = { Val4, Val4 };
	RealtimeMesh::TRealtimeMeshStreamBuilder<FPackedNormal[2]> TestBuilderMulti(
		RealtimeMesh::FRealtimeMeshStreamKey(RealtimeMesh::ERealtimeMeshStreamType::Vertex, "TestBuilder"));

	TestBuilderMulti.Add(Val0, Val1);
	TestBuilderMulti.Add(Val1.ToFVector4f(), Val2.ToFVector4f());
	TestBuilderMulti.Add(Val2, Val2);
	TestBuilderMulti.Add(TempData);
	TestBuilderMulti.RemoveAt(2);

	TestTrue(TEXT("TestBuilder Get(2)"), FMemory::Memcmp(TestBuilderMulti.Get(2), TempData, sizeof(FPackedNormal)*2) == 0);
	TestTrue(TEXT("TestBuilder GetElement(0, 1)"), TestBuilderMulti.GetElement(0, 1) == Val1);
	TestBuilderMulti.Set(2, TempData);
	TestTrue(TEXT("TestBuilder Get(2) after Set"), FMemory::Memcmp(TestBuilderMulti.Get(2), TempData, sizeof(FPackedNormal)*2) == 0);


	TestBuilderMulti.Add().Set(0, Val3).Set(1, Val2);
	TestTrue(TEXT("TestBuilder GetElement(3, 0)"), TestBuilderMulti.GetElement(3, 0) == Val3);
	TestTrue(TEXT("TestBuilder GetElement(3, 1)"), TestBuilderMulti.GetElement(3, 1) == Val2);
	
	TestBuilderMulti.Edit(3).SetRange(1, Val0);
	TestTrue(TEXT("TestBuilder GetElement(3, 1)"), TestBuilderMulti.GetElement(3, 1) == Val0);

	
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
