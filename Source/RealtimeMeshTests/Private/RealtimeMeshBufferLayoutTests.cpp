// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Mesh/RealtimeMeshDataTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshBufferLayoutTests, "RealtimeMeshComponent.RealtimeMeshBufferLayout",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool RealtimeMeshBufferLayoutTests::RunTest(const FString& Parameters)
{
	//const auto VoidType = RealtimeMesh::GetRealtimeMeshDataElementType<void>();
	const auto FloatType = RealtimeMesh::GetRealtimeMeshDataElementType<float>();
	const auto VectorType = RealtimeMesh::GetRealtimeMeshDataElementType<FVector2f>();
	const auto IntType = RealtimeMesh::GetRealtimeMeshDataElementType<uint32>();
	
	//const RealtimeMesh::FRealtimeMeshBufferLayout VoidBuffer(VoidType, 1);
	const RealtimeMesh::FRealtimeMeshBufferLayout ZeroLengthBuffer(FloatType, 0);
	const RealtimeMesh::FRealtimeMeshBufferLayout FloatBuffer1(FloatType, 1);
	const RealtimeMesh::FRealtimeMeshBufferLayout FloatBuffer2(FloatType, 2);
	const RealtimeMesh::FRealtimeMeshBufferLayout VectorBuffer1(VectorType, 1);
	const RealtimeMesh::FRealtimeMeshBufferLayout VectorBuffer2(VectorType, 2);
	const RealtimeMesh::FRealtimeMeshBufferLayout UInt32Buffer(IntType, 1);

	//TestFalse(TEXT("VoidBuffer InValid"), VoidType.IsValid());
	TestFalse(TEXT("ZeroLengthBuffer Invalid"), ZeroLengthBuffer.IsValid());
	TestTrue(TEXT("FloatBuffer1 Valid"), FloatBuffer1.IsValid());
	TestTrue(TEXT("FloatBuffer2 Valid"), FloatBuffer2.IsValid());
	TestTrue(TEXT("VectorBuffer1 Valid"), VectorBuffer1.IsValid());
	TestTrue(TEXT("VectorBuffer2 Valid"), VectorBuffer2.IsValid());
	TestTrue(TEXT("UInt32Buffer Valid"), UInt32Buffer.IsValid());

	const RealtimeMesh::FRealtimeMeshBufferLayout TestBuffer(VectorType, 2);

	TestTrue(TEXT("VectorBuffer2 == TestBuffer"), VectorBuffer2 == TestBuffer);
	TestFalse(TEXT("VectorBuffer2 != TestBuffer"), VectorBuffer2 != TestBuffer);

	TestTrue(TEXT("VectorBuffer2 DataType"), VectorBuffer2.GetElementType().GetDatumType() == RealtimeMesh::ERealtimeMeshDatumType::Float);
	TestTrue(TEXT("VectorBuffer2 NumDatums"), VectorBuffer2.GetElementType().GetNumDatums() == 2);
	TestTrue(TEXT("VectorBuffer2 NumElements"), VectorBuffer2.GetNumElements() == 2);


	//const RealtimeMesh::FRealtimeMeshBufferLayoutDefinition InvalidDef = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(VoidBuffer);
	const RealtimeMesh::FRealtimeMeshElementTypeDetails InvalidDefZero = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetElementTypeDetails(ZeroLengthBuffer);
	const RealtimeMesh::FRealtimeMeshElementTypeDetails ValidDefVector2 = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetElementTypeDetails(VectorBuffer2);
	const RealtimeMesh::FRealtimeMeshElementTypeDetails ValidDefUInt32 = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetElementTypeDetails(UInt32Buffer);
	
	
	//TestFalse(TEXT("InvalidDef Valid"), InvalidDef.IsValid());
	TestFalse(TEXT("InvalidDefZero Valid"), InvalidDefZero.IsValid());
	TestTrue(TEXT("ValidDefVector2 Valid"), ValidDefVector2.IsValid());
	TestTrue(TEXT("ValidDefUInt32 Valid"), ValidDefUInt32.IsValid());

	struct TestVec
	{
		FVector2f A, B;
	};
	
	TestTrue(TEXT("ValidDefVector2"),
		ValidDefVector2.IsSupportedVertexType()
		&& ValidDefVector2.GetStride() == sizeof(TestVec)
		&& ValidDefVector2.GetAlignment() == alignof(TestVec));
	TestTrue(TEXT("ValidDefUInt32"), 
		ValidDefUInt32.IsSupportedIndexType()
		&& ValidDefUInt32.GetStride() == sizeof(uint32)
		&& ValidDefUInt32.GetAlignment() == alignof(uint32));
	
	return true;
}
