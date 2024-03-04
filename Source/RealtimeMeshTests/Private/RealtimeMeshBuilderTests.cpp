// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Mesh/RealtimeMeshBuilder.h"
#include "Misc/AutomationTest.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "Templates/AreTypesEqual.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshBuilderTests, "RealtimeMeshComponent.RealtimeMeshBuilder",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

using namespace RealtimeMesh;

namespace RealtimeMesh
{
	struct FPackedNormalQuad
	{
		FPackedNormal A;
		FPackedNormal B;
		FPackedNormal C;
		FPackedNormal D;

		FPackedNormalQuad(FPackedNormal InA, FPackedNormal InB, FPackedNormal InC, FPackedNormal InD)
			: A(InA), B(InB), C(InC), D(InD) { }

		FORCEINLINE friend bool operator ==(const FPackedNormalQuad& Left, const FPackedNormalQuad& Right)
		{
			return Left.A == Right.A && Left.B == Right.B && Left.C == Right.C && Left.D == Right.D;
		}

		FORCEINLINE friend bool operator !=(const FPackedNormalQuad& Left, const FPackedNormalQuad& Right)
		{
			return !(Left == Right);
		}
	};

	struct FPackedRGBA16NQuad
	{
		FPackedRGBA16N A;
		FPackedRGBA16N B;
		FPackedRGBA16N C;
		FPackedRGBA16N D;

		FPackedRGBA16NQuad(FPackedRGBA16N InA, FPackedRGBA16N InB, FPackedRGBA16N InC, FPackedRGBA16N InD)
			: A(InA), B(InB), C(InC), D(InD) { }

		FORCEINLINE friend bool operator ==(const FPackedRGBA16NQuad& Left, const FPackedRGBA16NQuad& Right)
		{
			return Left.A == Right.A && Left.B == Right.B && Left.C == Right.C && Left.D == Right.D;
		}

		FORCEINLINE friend bool operator !=(const FPackedRGBA16NQuad& Left, const FPackedRGBA16NQuad& Right)
		{
			return !(Left == Right);
		}
	};

	
	template<> inline FPackedRGBA16NQuad ConvertRealtimeMeshType<FPackedNormalQuad, FPackedRGBA16NQuad>(const FPackedNormalQuad& Source)
	{
		return FPackedRGBA16NQuad(ConvertRealtimeMeshType<FPackedNormal, FPackedRGBA16N>(Source.A),
		                          ConvertRealtimeMeshType<FPackedNormal, FPackedRGBA16N>(Source.B),
		                          ConvertRealtimeMeshType<FPackedNormal, FPackedRGBA16N>(Source.C),
		                          ConvertRealtimeMeshType<FPackedNormal, FPackedRGBA16N>(Source.D));
	}	
	template<> inline FPackedNormalQuad ConvertRealtimeMeshType<FPackedRGBA16NQuad, FPackedNormalQuad>(const FPackedRGBA16NQuad& Source)
	{
		return FPackedNormalQuad(ConvertRealtimeMeshType<FPackedRGBA16N, FPackedNormal>(Source.A),
		                         ConvertRealtimeMeshType<FPackedRGBA16N, FPackedNormal>(Source.B),
		                         ConvertRealtimeMeshType<FPackedRGBA16N, FPackedNormal>(Source.C),
		                         ConvertRealtimeMeshType<FPackedRGBA16N, FPackedNormal>(Source.D));
	}

	template <>
	struct FRealtimeMeshBufferTypeTraits<FPackedNormalQuad>
	{
		using ElementType = FPackedNormal;
		static constexpr int32 NumElements = 4;
		static constexpr bool IsValid = true;
	};

	template <>
	struct FRealtimeMeshBufferTypeTraits<FPackedRGBA16NQuad>
	{
		using ElementType = FPackedRGBA16N;
		static constexpr int32 NumElements = 4;
		static constexpr bool IsValid = true;
	};
	
}

bool RealtimeMeshBuilderTests::RunTest(const FString& Parameters)
{
	static_assert(std::is_same_v<FRealtimeMeshBufferTypeTraits<FPackedNormalQuad>::ElementType, FPackedNormal>);
	static_assert(std::is_same_v<TRealtimeMeshElementAccessor<FPackedNormalQuad, FPackedNormalQuad>::AccessElementType, FPackedNormal>);


	{ // Direct access test
		FRealtimeMeshStream TestStream = FRealtimeMeshStream::Create<FPackedNormalQuad>(FRealtimeMeshStreams::Position);
		auto Context = TRealtimeMeshStreamDataAccessor<FPackedNormalQuad, FPackedNormalQuad, false>::InitializeContext(TestStream, 0);
		TestStream.AddZeroed(8);
		TRealtimeMeshElementAccessor<FPackedNormalQuad, FPackedNormalQuad> ElementAccessor(Context, 2, 3);
		TRealtimeMeshElementAccessor<FPackedNormalQuad, FPackedNormalQuad> ElementAccessor2(Context, 5, 1);
		
		TestTrue(TEXT("ElementAccessor Get(2,3)"), ElementAccessor == FPackedNormal());
		TestTrue(TEXT("ElementAccessor2 Get(5,1)"), ElementAccessor2 == FPackedNormal());
		ElementAccessor = FPackedNormal(FVector4f(1, 0, 1, 0));
		ElementAccessor2 = FPackedNormal(FVector4f(1, 0, 0, 1));
		TestTrue(TEXT("ElementAccessor Get(2,3) AfterSet"), ElementAccessor == FPackedNormal(FVector4f(1, 0, 1, 0)));
		TestFalse(TEXT("ElementAccessor Get(2,3) AfterSet"), ElementAccessor == FPackedNormal(FVector4f(1, 1, 0, 0)));
		TestTrue(TEXT("ElementAccessor2 Get(5,1) AfterSet"), ElementAccessor2 == FPackedNormal(FVector4f(1, 0, 0, 1)));
		TestFalse(TEXT("ElementAccessor2 Get(5,1) AfterSet"), ElementAccessor2 == FPackedNormal(FVector4f(1, 1, 0, 0)));		
	}

	{ // Dynamic conversion test
		FRealtimeMeshStream TestStream = FRealtimeMeshStream::Create<FPackedNormalQuad>(FRealtimeMeshStreams::Position);
		auto Context = TRealtimeMeshStreamDataAccessor<FPackedRGBA16NQuad, void, false>::InitializeContext(TestStream, 0);
		TestStream.AddZeroed(8);
		TRealtimeMeshElementAccessor<FPackedRGBA16NQuad, void> ElementAccessor(Context, 2, 3);
		TRealtimeMeshElementAccessor<FPackedRGBA16NQuad, void> ElementAccessor2(Context, 5, 1);
		
		TestTrue(TEXT("ElementAccessor Get(2,3)"), ElementAccessor == FPackedRGBA16N());
		TestTrue(TEXT("ElementAccessor2 Get(5,1)"), ElementAccessor2 == FPackedRGBA16N());
		ElementAccessor = FPackedRGBA16N(FVector4f(1, 0, 1, 0));
		ElementAccessor2 = FPackedRGBA16N(FVector4f(1, 0, 0, 1));
		TestTrue(TEXT("ElementAccessor Get(2,3) AfterSet"), ElementAccessor == FPackedRGBA16N(FVector4f(1, 0, 1, 0)));
		TestFalse(TEXT("ElementAccessor Get(2,3) AfterSet"), ElementAccessor == FPackedRGBA16N(FVector4f(1, 1, 0, 0)));
		TestTrue(TEXT("ElementAccessor2 Get(5,1) AfterSet"), ElementAccessor2 == FPackedRGBA16N(FVector4f(1, 0, 0, 1)));
		TestFalse(TEXT("ElementAccessor2 Get(5,1) AfterSet"), ElementAccessor2 == FPackedRGBA16N(FVector4f(1, 1, 0, 0)));		
	}
	
	{ // Static conversion test
		FRealtimeMeshStream TestStream = FRealtimeMeshStream::Create<FPackedNormalQuad>(FRealtimeMeshStreams::Position);
		auto Context = TRealtimeMeshStreamDataAccessor<FPackedRGBA16NQuad, FPackedNormalQuad, false>::InitializeContext(TestStream, 0);
		TestStream.AddZeroed(8);
		TRealtimeMeshIndexedBufferAccessor<FPackedRGBA16NQuad, FPackedNormalQuad> RowAccessor(Context, 2);
		TRealtimeMeshIndexedBufferAccessor<FPackedRGBA16NQuad, FPackedNormalQuad> RowAccessor2(Context, 5);

		FPackedRGBA16NQuad TestA
		{
			FPackedRGBA16N(FVector3f::ZAxisVector),
			FPackedRGBA16N(FVector3f::XAxisVector),
			FPackedRGBA16N(FVector3f::YAxisVector),
			FPackedRGBA16N(FVector3f::OneVector)
		};
		FPackedRGBA16NQuad TestB
		{
			FPackedRGBA16N(FVector3f::YAxisVector),
			FPackedRGBA16N(FVector3f::XAxisVector),
			FPackedRGBA16N(FVector3f::OneVector),
			FPackedRGBA16N(FVector3f::ZAxisVector)
		};

		FPackedRGBA16NQuad TestEmpty { FPackedRGBA16N(), FPackedRGBA16N(), FPackedRGBA16N(), FPackedRGBA16N() };

		TestTrue(TEXT("RowAccessor Get(2)"), RowAccessor == TestEmpty);
		TestTrue(TEXT("RowAccessor2 Get(5)"), RowAccessor2 == TestEmpty);
		RowAccessor = TestA;
		RowAccessor2 = TestB;
		TestTrue(TEXT("RowAccessor Get(2) AfterSet TestTrue"), RowAccessor == TestA);
		TestFalse(TEXT("RowAccessor Get(2) AfterSet TestFalse"), RowAccessor == TestB);
		TestTrue(TEXT("RowAccessor2 Get(5) AfterSet TestTrue"), RowAccessor2 == TestB);
		TestFalse(TEXT("RowAccessor2 Get(5) AfterSet TestFalse"), RowAccessor2 == TestA);
	}

	








	{
		/*const FPackedNormal Val0 = FPackedNormal(FVector4f(0, 1, 1, 0));
		const FPackedNormal Val1 = FPackedNormal(FVector4f(1, 0, 1, 0));
		const FPackedNormal Val2 = FPackedNormal(FVector4f(1, 0.5, 1, 0.5));
		const FPackedNormal Val3 = FPackedNormal(FVector4f(0, 0.5, 1, 0.5));
		const FPackedNormal Val4 = FPackedNormal(FVector4f(1, 0, 1, 0.5));

		FRealtimeMeshStream TestStream = FRealtimeMeshStream::Create<FPackedNormalQuad>(FRealtimeMeshStreams::Position);
		TRealtimeMeshStreamBuilder<FPackedNormalQuad> TestStreamBuilder(TestStream);
		TestStreamBuilder.Add(Val0, Val1, Val2, Val3);

		TRealtimeMeshIndexedBufferAccessor<FPackedNormalQuad> Row = TestStreamBuilder[0];
		TRealtimeMeshElementAccessor<FPackedNormalQuad> Cell = Row[0];
	
	
		TestTrue(TEXT("TestRow Initial Get(0)"), Cell == Val0);
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
		TestStreamBuilder.SetElement(2, 0, Val4);
		TestTrue(TEXT("TestBuilder Get(2)"), TestStreamBuilder.Get(2)[0] == Val4);

		TestStreamBuilder.Empty();
		FPackedNormalQuad TempData(Val3, Val1, Val2, Val0);

		TestStreamBuilder.Add(Val0, Val1);
		TestStreamBuilder.Add(Val1.ToFVector4f(), Val2.ToFVector4f());
		TestStreamBuilder.Add(Val2, Val2);
		TestStreamBuilder.Add(TempData);
		TestStreamBuilder.RemoveAt(2);


		auto T = TestStreamBuilder[2].Get();

		TestStreamBuilder[0][0] = FPackedNormal();
	
	

		TestTrue(TEXT("TestBuilder Get(2)"), TestStreamBuilder[2] == TempData);
		TestTrue(TEXT("TestBuilder GetElement(0, 1)"), TestStreamBuilder.GetElementValue(0, 1) == Val1);
		TempData.C = Val4;
		TestStreamBuilder.Set(2, TempData);
		TestTrue(TEXT("TestBuilder Get(2) after Set"), TestStreamBuilder.Get(2) == TempData);


		TestStreamBuilder.Add().SetElement(0, Val3).SetElement(1, Val2);
		TestTrue(TEXT("TestBuilder GetElement(3, 0)"), TestStreamBuilder.GetElementValue(3, 0) == Val3);
		TestTrue(TEXT("TestBuilder GetElement(3, 1)"), TestStreamBuilder.GetElementValue(3, 1) == Val2);
	
		TestStreamBuilder.Edit(3).SetRange(1, Val0);
		TestTrue(TEXT("TestBuilder GetElement(3, 1)"), TestStreamBuilder.GetElementValue(3, 1) == Val0);
		*/



	
	}



	/*FRealtimeMeshStream TestStreamPackedNormal = FRealtimeMeshStream::Create<FPackedNormal[4]>(FRealtimeMeshStreams::Position);
	TRealtimeMeshStreamBuilder<FVector4f, FRealtimeMeshStreamElementDirectRead<FVector4f, FPackedNormal>> TestStreamBuilderStaticConversion(TestStreamPackedNormal);
	TestStreamBuilder.Add(FVector4f(1, 0, 0, 0), FVector4f(0, 1, 0, 0), FVector4f(0, 0, 1, 0), FVector4f(0, 0, 0, 1));

	TestTrue(TEXT("TestRow Initial Get(0)"), TestStreamBuilderStaticConversion[0][0] == FVector4f(1, 0, 0, 0));
	TestTrue(TEXT("TestRow Initial Get(1)"), TestStreamBuilderStaticConversion[0][1] == FVector4f(0, 1, 0, 0));
	TestTrue(TEXT("TestRow Initial Get(2)"), TestStreamBuilderStaticConversion[0][2] == FVector4f(0, 0, 1, 0));
	TestTrue(TEXT("TestRow Initial Get(3)"), TestStreamBuilderStaticConversion[0][3] == FVector4f(0, 0, 0, 1));*/

	
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