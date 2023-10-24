// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Mesh/RealtimeMeshDataTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementTypeTests, "RealtimeMeshComponent.RealtimeMeshElementType",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementTypeTests::RunTest(const FString& Parameters)
{
	//const auto VoidType = RealtimeMesh::GetRealtimeMeshDataElementType<void>();
	const auto FloatType = RealtimeMesh::GetRealtimeMeshDataElementType<float>();
	const auto VectorType = RealtimeMesh::GetRealtimeMeshDataElementType<FVector2f>();
	const auto IntType = RealtimeMesh::GetRealtimeMeshDataElementType<uint32>();
		
	//TestFalse(TEXT("InvalidType Test"), VoidType.IsValid());
	TestTrue(TEXT("FloatType Valid"), FloatType.IsValid());
	TestTrue(TEXT("VectorType Valid"), FloatType.IsValid());
	TestTrue(TEXT("IntType Valid"), IntType.IsValid());

	const auto TestType = RealtimeMesh::GetRealtimeMeshDataElementType<float>();

	TestTrue(TEXT("FloatType == TestType"), FloatType == TestType);
	TestFalse(TEXT("FloatType != TestType"), FloatType != TestType);
	TestTrue(TEXT("FloatType != VectorType"), FloatType != VectorType);
	TestFalse(TEXT("FloatType == VectorType"), FloatType == VectorType);

	TestTrue(TEXT("VectorType DataType"), VectorType.GetDatumType() == RealtimeMesh::ERealtimeMeshDatumType::Float);
	TestTrue(TEXT("VectorType NumDatums"), VectorType.GetNumDatums() == 2);

	//const RealtimeMesh::FRealtimeMeshElementTypeDefinition InvalidDef = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetTypeDefinition(VoidType);
	const RealtimeMesh::FRealtimeMeshElementTypeDefinition ValidVertex = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetTypeDefinition(VectorType);
	const RealtimeMesh::FRealtimeMeshElementTypeDefinition ValidIndex = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetTypeDefinition(IntType);
	
	
	//TestFalse(TEXT("InvalidDef Valid"), InvalidDef.IsValid());
	TestTrue(TEXT("ValidVertex Valid"), ValidVertex.IsValid());
	TestTrue(TEXT("ValidIndex Valid"), ValidIndex.IsValid());

	TestTrue(TEXT("ValidVertex"), ValidVertex.IsSupportedVertexType() && ValidVertex.GetVertexType() == VET_Float2 && ValidVertex.GetStride() == sizeof(FVector2f) && ValidVertex.GetAlignment() == alignof(FVector2f));
	TestTrue(TEXT("ValidIndex"), ValidIndex.IsSupportedIndexType() && ValidIndex.GetIndexType() == RealtimeMesh::IET_UInt32 && ValidIndex.GetStride() == sizeof(uint32) && ValidIndex.GetAlignment() == alignof(uint32));

	const RealtimeMesh::FRealtimeMeshElementTypeDefinition PackedNormal = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetTypeDefinition(RealtimeMesh::GetRealtimeMeshDataElementType<FPackedNormal>());	
	TestTrue(TEXT("PackedNormal"), PackedNormal.IsSupportedVertexType() && PackedNormal.GetVertexType() == VET_PackedNormal && PackedNormal.GetStride() == sizeof(FPackedNormal) && PackedNormal.GetAlignment() == alignof(FPackedNormal));
	const RealtimeMesh::FRealtimeMeshElementTypeDefinition PackedRGBA16N = RealtimeMesh::FRealtimeMeshBufferLayoutUtilities::GetTypeDefinition(RealtimeMesh::GetRealtimeMeshDataElementType<FPackedRGBA16N>());	
	TestTrue(TEXT("PackedRGBA16N"), PackedRGBA16N.IsSupportedVertexType() && PackedRGBA16N.GetVertexType() == VET_Short4N && PackedRGBA16N.GetStride() == sizeof(FPackedRGBA16N) && PackedRGBA16N.GetAlignment() == alignof(FPackedRGBA16N));
	
	return true;
}
