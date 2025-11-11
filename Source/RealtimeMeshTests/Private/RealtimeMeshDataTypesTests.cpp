// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "RealtimeMeshCore.h"
#include "Interface/Core/RealtimeMeshDataTypes.h"

using namespace RealtimeMesh;

// Test flags for editor-only tests
#if WITH_DEV_AUTOMATION_TESTS

//==============================================================================
// TRealtimeMeshTangents Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshTangentsConstructorTest,
	"RealtimeMeshComponent.DataTypes.TRealtimeMeshTangents.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshTangentsConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor compiles
	{
		[[maybe_unused]] TRealtimeMeshTangents<FPackedNormal> Tangents;
	}

	// Test constructor with normal and tangent (FVector3f)
	{
		FVector3f Normal(0.0f, 0.0f, 1.0f);
		FVector3f Tangent(1.0f, 0.0f, 0.0f);
		TRealtimeMeshTangents<FVector4f> Tangents(Normal, Tangent, false);

		TestEqual(TEXT("Normal X"), Tangents.GetNormal().X, Normal.X, 0.001f);
		TestEqual(TEXT("Normal Y"), Tangents.GetNormal().Y, Normal.Y, 0.001f);
		TestEqual(TEXT("Normal Z"), Tangents.GetNormal().Z, Normal.Z, 0.001f);
		TestEqual(TEXT("Tangent X"), Tangents.GetTangent().X, Tangent.X, 0.001f);
		TestEqual(TEXT("Tangent Y"), Tangents.GetTangent().Y, Tangent.Y, 0.001f);
		TestEqual(TEXT("Tangent Z"), Tangents.GetTangent().Z, Tangent.Z, 0.001f);
		TestFalse(TEXT("Binormal not flipped"), Tangents.IsBinormalFlipped());
	}

	// Test constructor with flipped binormal
	{
		FVector3f Normal(0.0f, 0.0f, 1.0f);
		FVector3f Tangent(1.0f, 0.0f, 0.0f);
		TRealtimeMeshTangents<FVector4f> Tangents(Normal, Tangent, true);

		TestTrue(TEXT("Binormal should be flipped"), Tangents.IsBinormalFlipped());
	}

	// Test constructor with normal, binormal, and tangent
	{
		FVector3f Normal(0.0f, 0.0f, 1.0f);
		FVector3f Tangent(1.0f, 0.0f, 0.0f);
		FVector3f Binormal(0.0f, 1.0f, 0.0f);
		TRealtimeMeshTangents<FVector4f> Tangents(Normal, Binormal, Tangent);

		TestEqual(TEXT("Normal X"), Tangents.GetNormal().X, Normal.X, 0.001f);
		TestEqual(TEXT("Normal Y"), Tangents.GetNormal().Y, Normal.Y, 0.001f);
		TestEqual(TEXT("Normal Z"), Tangents.GetNormal().Z, Normal.Z, 0.001f);
		TestEqual(TEXT("Tangent X"), Tangents.GetTangent().X, Tangent.X, 0.001f);
		TestEqual(TEXT("Tangent Y"), Tangents.GetTangent().Y, Tangent.Y, 0.001f);
		TestEqual(TEXT("Tangent Z"), Tangents.GetTangent().Z, Tangent.Z, 0.001f);
	}

	// Test copy constructor between different precision types
	{
		FVector3f Normal(0.0f, 0.0f, 1.0f);
		FVector3f Tangent(1.0f, 0.0f, 0.0f);
		TRealtimeMeshTangents<FVector4f> HighPrecision(Normal, Tangent);
		TRealtimeMeshTangents<FPackedNormal> NormalPrecision(HighPrecision);

		// Verify conversion maintains direction (with some tolerance for packing)
		FVector3f ConvertedNormal = NormalPrecision.GetNormal();
		TestEqual(TEXT("Converted Normal Z"), ConvertedNormal.Z, 1.0f, 0.01f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshTangentsGettersSettersTest,
	"RealtimeMeshComponent.DataTypes.TRealtimeMeshTangents.GettersSetters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshTangentsGettersSettersTest::RunTest(const FString& Parameters)
{
	TRealtimeMeshTangents<FVector4f> Tangents;

	// Test SetNormal
	{
		FVector3f NewNormal(0.0f, 1.0f, 0.0f);
		Tangents.SetNormal(NewNormal);
		FVector3f Result = Tangents.GetNormal();
		TestEqual(TEXT("SetNormal X"), Result.X, NewNormal.X, 0.001f);
		TestEqual(TEXT("SetNormal Y"), Result.Y, NewNormal.Y, 0.001f);
		TestEqual(TEXT("SetNormal Z"), Result.Z, NewNormal.Z, 0.001f);
	}

	// Test SetTangent
	{
		FVector3f NewTangent(0.0f, 0.0f, 1.0f);
		Tangents.SetTangent(NewTangent);
		FVector3f Result = Tangents.GetTangent();
		TestEqual(TEXT("SetTangent X"), Result.X, NewTangent.X, 0.001f);
		TestEqual(TEXT("SetTangent Y"), Result.Y, NewTangent.Y, 0.001f);
		TestEqual(TEXT("SetTangent Z"), Result.Z, NewTangent.Z, 0.001f);
	}

	// Test SetFlipBinormal
	{
		Tangents.SetFlipBinormal(true);
		TestTrue(TEXT("Binormal should be flipped after SetFlipBinormal(true)"), Tangents.IsBinormalFlipped());

		Tangents.SetFlipBinormal(false);
		TestFalse(TEXT("Binormal should not be flipped after SetFlipBinormal(false)"), Tangents.IsBinormalFlipped());
	}

	// Test GetBinormal
	{
		FVector3f Normal(0.0f, 0.0f, 1.0f);
		FVector3f Tangent(1.0f, 0.0f, 0.0f);
		Tangents.SetNormalAndTangent(Normal, Tangent, false);

		FVector3f Binormal = Tangents.GetBinormal();
		// Cross product of Z up and X right should give Y forward (or backward if flipped)
		TestEqual(TEXT("Binormal Y should be 1"), FMath::Abs(Binormal.Y), 1.0f, 0.001f);
	}

	// Test SetNormalAndTangent
	{
		FVector3f Normal(1.0f, 0.0f, 0.0f);
		FVector3f Tangent(0.0f, 1.0f, 0.0f);
		Tangents.SetNormalAndTangent(Normal, Tangent, true);

		TestEqual(TEXT("Normal X"), Tangents.GetNormal().X, Normal.X, 0.001f);
		TestEqual(TEXT("Tangent Y"), Tangents.GetTangent().Y, Tangent.Y, 0.001f);
		TestTrue(TEXT("Binormal should be flipped"), Tangents.IsBinormalFlipped());
	}

	// Test SetTangents (with all three vectors)
	{
		FVector3f Normal(0.0f, 0.0f, 1.0f);
		FVector3f Binormal(0.0f, 1.0f, 0.0f);
		FVector3f Tangent(1.0f, 0.0f, 0.0f);
		Tangents.SetTangents(Normal, Binormal, Tangent);

		TestEqual(TEXT("Normal Z"), Tangents.GetNormal().Z, Normal.Z, 0.001f);
		TestEqual(TEXT("Tangent X"), Tangents.GetTangent().X, Tangent.X, 0.001f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshTangentsEqualityTest,
	"RealtimeMeshComponent.DataTypes.TRealtimeMeshTangents.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshTangentsEqualityTest::RunTest(const FString& Parameters)
{
	FVector3f Normal(0.0f, 0.0f, 1.0f);
	FVector3f Tangent(1.0f, 0.0f, 0.0f);

	TRealtimeMeshTangents<FVector4f> Tangents1(Normal, Tangent);
	TRealtimeMeshTangents<FVector4f> Tangents2(Normal, Tangent);
	TRealtimeMeshTangents<FVector4f> Tangents3(FVector3f(0.0f, 1.0f, 0.0f), Tangent);

	// Test equality
	TestTrue(TEXT("Identical tangents should be equal"), Tangents1 == Tangents2);
	TestFalse(TEXT("Different tangents should not be equal"), Tangents1 == Tangents3);

	// Test inequality
	TestFalse(TEXT("Identical tangents should not be inequal"), Tangents1 != Tangents2);
	TestTrue(TEXT("Different tangents should be inequal"), Tangents1 != Tangents3);

	return true;
}

//==============================================================================
// TRealtimeMeshTexCoords Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshTexCoordsConstructorTest,
	"RealtimeMeshComponent.DataTypes.TRealtimeMeshTexCoords.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshTexCoordsConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor compiles
	{
		[[maybe_unused]] TRealtimeMeshTexCoords<FVector2f, 1> TexCoords;
	}

	// Test single channel constructor
	{
		FVector2f UV(0.5f, 0.75f);
		TRealtimeMeshTexCoords<FVector2f, 1> TexCoords(UV);

		TestEqual(TEXT("UV0 X"), TexCoords[0].X, UV.X, 0.001f);
		TestEqual(TEXT("UV0 Y"), TexCoords[0].Y, UV.Y, 0.001f);
	}

	// Test multi-channel constructor
	{
		FVector2f UV0(0.0f, 0.0f);
		FVector2f UV1(1.0f, 0.0f);
		FVector2f UV2(0.0f, 1.0f);
		TRealtimeMeshTexCoords<FVector2f, 3> TexCoords(UV0, UV1, UV2);

		TestEqual(TEXT("UV0 X"), TexCoords[0].X, UV0.X, 0.001f);
		TestEqual(TEXT("UV0 Y"), TexCoords[0].Y, UV0.Y, 0.001f);
		TestEqual(TEXT("UV1 X"), TexCoords[1].X, UV1.X, 0.001f);
		TestEqual(TEXT("UV1 Y"), TexCoords[1].Y, UV1.Y, 0.001f);
		TestEqual(TEXT("UV2 X"), TexCoords[2].X, UV2.X, 0.001f);
		TestEqual(TEXT("UV2 Y"), TexCoords[2].Y, UV2.Y, 0.001f);
	}

	// Test copy constructor
	{
		FVector2f UV0(0.25f, 0.5f);
		FVector2f UV1(0.75f, 0.25f);
		TRealtimeMeshTexCoords<FVector2f, 2> Original(UV0, UV1);
		TRealtimeMeshTexCoords<FVector2f, 2> Copy(Original);

		TestEqual(TEXT("Copy UV0 X"), Copy[0].X, Original[0].X, 0.001f);
		TestEqual(TEXT("Copy UV0 Y"), Copy[0].Y, Original[0].Y, 0.001f);
		TestEqual(TEXT("Copy UV1 X"), Copy[1].X, Original[1].X, 0.001f);
		TestEqual(TEXT("Copy UV1 Y"), Copy[1].Y, Original[1].Y, 0.001f);
	}

	// Test conversion between precision types
	{
		FVector2f UV(0.5f, 0.5f);
		TRealtimeMeshTexCoords<FVector2f, 1> HighPrecision(UV);
		TRealtimeMeshTexCoords<FVector2DHalf, 1> NormalPrecision(HighPrecision);

		// Verify conversion maintains values (with tolerance for half precision)
		TestEqual(TEXT("Converted UV X"), (float)NormalPrecision[0].X, UV.X, 0.01f);
		TestEqual(TEXT("Converted UV Y"), (float)NormalPrecision[0].Y, UV.Y, 0.01f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshTexCoordsAccessTest,
	"RealtimeMeshComponent.DataTypes.TRealtimeMeshTexCoords.Access",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshTexCoordsAccessTest::RunTest(const FString& Parameters)
{
	TRealtimeMeshTexCoords<FVector2f, 4> TexCoords;

	// Test operator[] for write access
	{
		TexCoords[0] = FVector2f(0.0f, 0.0f);
		TexCoords[1] = FVector2f(1.0f, 0.0f);
		TexCoords[2] = FVector2f(1.0f, 1.0f);
		TexCoords[3] = FVector2f(0.0f, 1.0f);

		TestEqual(TEXT("TexCoords[0] X"), TexCoords[0].X, 0.0f, 0.001f);
		TestEqual(TEXT("TexCoords[1] X"), TexCoords[1].X, 1.0f, 0.001f);
		TestEqual(TEXT("TexCoords[2] Y"), TexCoords[2].Y, 1.0f, 0.001f);
		TestEqual(TEXT("TexCoords[3] Y"), TexCoords[3].Y, 1.0f, 0.001f);
	}

	// Test Set method
	{
		FVector2f UV0(0.5f, 0.5f);
		FVector2f UV1(0.25f, 0.75f);
		TRealtimeMeshTexCoords<FVector2f, 2> SetTexCoords;
		SetTexCoords.Set(UV0, UV1);

		TestEqual(TEXT("Set UV0 X"), SetTexCoords[0].X, UV0.X, 0.001f);
		TestEqual(TEXT("Set UV1 Y"), SetTexCoords[1].Y, UV1.Y, 0.001f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshTexCoordsEqualityTest,
	"RealtimeMeshComponent.DataTypes.TRealtimeMeshTexCoords.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshTexCoordsEqualityTest::RunTest(const FString& Parameters)
{
	FVector2f UV0(0.0f, 0.0f);
	FVector2f UV1(1.0f, 1.0f);
	FVector2f UV2(0.5f, 0.5f);

	TRealtimeMeshTexCoords<FVector2f, 2> TexCoords1(UV0, UV1);
	TRealtimeMeshTexCoords<FVector2f, 2> TexCoords2(UV0, UV1);
	TRealtimeMeshTexCoords<FVector2f, 2> TexCoords3(UV0, UV2);

	// Test equality
	TestTrue(TEXT("Identical texcoords should be equal"), TexCoords1 == TexCoords2);
	TestFalse(TEXT("Different texcoords should not be equal"), TexCoords1 == TexCoords3);

	// Test inequality
	TestFalse(TEXT("Identical texcoords should not be inequal"), TexCoords1 != TexCoords2);
	TestTrue(TEXT("Different texcoords should be inequal"), TexCoords1 != TexCoords3);

	return true;
}

//==============================================================================
// TIndex3 Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshIndex3ConstructorTest,
	"RealtimeMeshComponent.DataTypes.TIndex3.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshIndex3ConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor
	{
		TIndex3<uint32> Index;
		TestEqual(TEXT("Default V0"), Index.V0, 0u);
		TestEqual(TEXT("Default V1"), Index.V1, 0u);
		TestEqual(TEXT("Default V2"), Index.V2, 0u);
	}

	// Test parameterized constructor
	{
		TIndex3<uint32> Index(10, 20, 30);
		TestEqual(TEXT("V0"), Index.V0, 10u);
		TestEqual(TEXT("V1"), Index.V1, 20u);
		TestEqual(TEXT("V2"), Index.V2, 30u);
	}

	// Test FIntVector constructor
	{
		FIntVector Vec(5, 10, 15);
		TIndex3<int32> Index(Vec);
		TestEqual(TEXT("From FIntVector V0"), Index.V0, 5);
		TestEqual(TEXT("From FIntVector V1"), Index.V1, 10);
		TestEqual(TEXT("From FIntVector V2"), Index.V2, 15);
	}

	// Test conversion to FIntVector
	{
		TIndex3<int32> Index(7, 14, 21);
		FIntVector Vec = Index;
		TestEqual(TEXT("To FIntVector X"), Vec.X, 7);
		TestEqual(TEXT("To FIntVector Y"), Vec.Y, 14);
		TestEqual(TEXT("To FIntVector Z"), Vec.Z, 21);
	}

	// Test Zero static method
	{
		TIndex3<uint32> Zero = TIndex3<uint32>::Zero();
		TestEqual(TEXT("Zero V0"), Zero.V0, 0u);
		TestEqual(TEXT("Zero V1"), Zero.V1, 0u);
		TestEqual(TEXT("Zero V2"), Zero.V2, 0u);
	}

	// Test Max static method
	{
		TIndex3<uint16> Max = TIndex3<uint16>::Max();
		TestEqual(TEXT("Max V0"), Max.V0, TNumericLimits<uint16>::Max());
		TestEqual(TEXT("Max V1"), Max.V1, TNumericLimits<uint16>::Max());
		TestEqual(TEXT("Max V2"), Max.V2, TNumericLimits<uint16>::Max());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshIndex3AccessTest,
	"RealtimeMeshComponent.DataTypes.TIndex3.Access",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshIndex3AccessTest::RunTest(const FString& Parameters)
{
	TIndex3<uint32> Index(100, 200, 300);

	// Test array access (const)
	{
		TestEqual(TEXT("Index[0]"), Index[0], 100u);
		TestEqual(TEXT("Index[1]"), Index[1], 200u);
		TestEqual(TEXT("Index[2]"), Index[2], 300u);
	}

	// Test array access (non-const)
	{
		Index[0] = 111;
		Index[1] = 222;
		Index[2] = 333;

		TestEqual(TEXT("Modified Index[0]"), Index[0], 111u);
		TestEqual(TEXT("Modified Index[1]"), Index[1], 222u);
		TestEqual(TEXT("Modified Index[2]"), Index[2], 333u);
	}

	// Test named member access
	{
		Index.V0 = 50;
		Index.V1 = 60;
		Index.V2 = 70;

		TestEqual(TEXT("V0"), Index.V0, 50u);
		TestEqual(TEXT("V1"), Index.V1, 60u);
		TestEqual(TEXT("V2"), Index.V2, 70u);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshIndex3UtilityTest,
	"RealtimeMeshComponent.DataTypes.TIndex3.Utility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshIndex3UtilityTest::RunTest(const FString& Parameters)
{
	TIndex3<uint32> Index(10, 20, 30);

	// Test IndexOf
	{
		TestEqual(TEXT("IndexOf V0"), Index.IndexOf(10), 0);
		TestEqual(TEXT("IndexOf V1"), Index.IndexOf(20), 1);
		TestEqual(TEXT("IndexOf V2"), Index.IndexOf(30), 2);
		TestEqual(TEXT("IndexOf not found"), Index.IndexOf(40), -1);
	}

	// Test Contains
	{
		TestTrue(TEXT("Contains V0"), Index.Contains(10));
		TestTrue(TEXT("Contains V1"), Index.Contains(20));
		TestTrue(TEXT("Contains V2"), Index.Contains(30));
		TestFalse(TEXT("Does not contain 40"), Index.Contains(40));
	}

	// Test IsDegenerate (no duplicates)
	{
		TIndex3<uint32> ValidIndex(1, 2, 3);
		TestFalse(TEXT("Valid index is not degenerate"), ValidIndex.IsDegenerate());
	}

	// Test IsDegenerate (with duplicates)
	{
		TIndex3<uint32> DegenerateIndex1(1, 1, 2);
		TestTrue(TEXT("Degenerate V0==V1"), DegenerateIndex1.IsDegenerate());

		TIndex3<uint32> DegenerateIndex2(1, 2, 1);
		TestTrue(TEXT("Degenerate V0==V2"), DegenerateIndex2.IsDegenerate());

		TIndex3<uint32> DegenerateIndex3(1, 2, 2);
		TestTrue(TEXT("Degenerate V1==V2"), DegenerateIndex3.IsDegenerate());
	}

	// Test Clamp
	{
		TIndex3<int32> ClampIndex(-5, 50, 100);
		TIndex3<int32> Clamped = ClampIndex.Clamp(0, 100);

		TestEqual(TEXT("Clamped min"), Clamped.V0, 0);
		TestEqual(TEXT("Clamped unchanged"), Clamped.V1, 50);
		TestEqual(TEXT("Clamped max"), Clamped.V2, 100);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshIndex3EqualityTest,
	"RealtimeMeshComponent.DataTypes.TIndex3.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshIndex3EqualityTest::RunTest(const FString& Parameters)
{
	TIndex3<uint32> Index1(10, 20, 30);
	TIndex3<uint32> Index2(10, 20, 30);
	TIndex3<uint32> Index3(10, 20, 40);

	// Test equality
	TestTrue(TEXT("Identical indices should be equal"), Index1 == Index2);
	TestFalse(TEXT("Different indices should not be equal"), Index1 == Index3);

	// Test inequality
	TestFalse(TEXT("Identical indices should not be inequal"), Index1 != Index2);
	TestTrue(TEXT("Different indices should be inequal"), Index1 != Index3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshIndex3HashTest,
	"RealtimeMeshComponent.DataTypes.TIndex3.Hash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshIndex3HashTest::RunTest(const FString& Parameters)
{
	TIndex3<uint32> Index1(10, 20, 30);
	TIndex3<uint32> Index2(10, 20, 30);
	TIndex3<uint32> Index3(10, 20, 40);

	uint32 Hash1 = GetTypeHash(Index1);
	uint32 Hash2 = GetTypeHash(Index2);
	uint32 Hash3 = GetTypeHash(Index3);

	// Identical indices should have identical hashes
	TestEqual(TEXT("Identical indices have same hash"), Hash1, Hash2);

	// Different indices should (very likely) have different hashes
	TestNotEqual(TEXT("Different indices have different hash"), Hash1, Hash3);

	return true;
}

//==============================================================================
// FRealtimeMeshPolygonGroupRange Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshPolygonGroupRangeConstructorTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshPolygonGroupRange.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshPolygonGroupRangeConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor
	{
		FRealtimeMeshPolygonGroupRange Range;
		TestEqual(TEXT("Default StartIndex"), Range.StartIndex, 0);
		TestEqual(TEXT("Default Count"), Range.Count, 0);
		TestEqual(TEXT("Default PolygonGroupIndex"), Range.PolygonGroupIndex, 0);
	}

	// Test parameterized constructor
	{
		FRealtimeMeshPolygonGroupRange Range(100, 50, 2);
		TestEqual(TEXT("StartIndex"), Range.StartIndex, 100);
		TestEqual(TEXT("Count"), Range.Count, 50);
		TestEqual(TEXT("PolygonGroupIndex"), Range.PolygonGroupIndex, 2);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshPolygonGroupRangeEqualityTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshPolygonGroupRange.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshPolygonGroupRangeEqualityTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshPolygonGroupRange Range1(100, 50, 2);
	FRealtimeMeshPolygonGroupRange Range2(100, 50, 2);
	FRealtimeMeshPolygonGroupRange Range3(100, 50, 3);
	FRealtimeMeshPolygonGroupRange Range4(200, 50, 2);

	// Test equality
	TestTrue(TEXT("Identical ranges should be equal"), Range1 == Range2);
	TestFalse(TEXT("Different PolygonGroupIndex"), Range1 == Range3);
	TestFalse(TEXT("Different StartIndex"), Range1 == Range4);

	return true;
}

//==============================================================================
// FRealtimeMeshElementType Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementTypeConstructorTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshElementType.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementTypeConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor
	{
		FRealtimeMeshElementType ElementType;
		TestFalse(TEXT("Default constructor creates invalid type"), ElementType.IsValid());
		TestEqual(TEXT("Default type is Unknown"), (uint8)ElementType.GetDatumType(), (uint8)ERealtimeMeshDatumType::Unknown);
		TestEqual(TEXT("Default NumDatums is 0"), ElementType.GetNumDatums(), (uint8)0);
	}

	// Test parameterized constructor
	{
		FRealtimeMeshElementType ElementType(ERealtimeMeshDatumType::Float, 3);
		TestTrue(TEXT("Valid constructor creates valid type"), ElementType.IsValid());
		TestEqual(TEXT("DatumType is Float"), (uint8)ElementType.GetDatumType(), (uint8)ERealtimeMeshDatumType::Float);
		TestEqual(TEXT("NumDatums is 3"), ElementType.GetNumDatums(), (uint8)3);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementTypeValidityTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshElementType.Validity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementTypeValidityTest::RunTest(const FString& Parameters)
{
	// Valid types
	{
		FRealtimeMeshElementType ValidType(ERealtimeMeshDatumType::Float, 1);
		TestTrue(TEXT("Float with 1 datum is valid"), ValidType.IsValid());
	}

	// Invalid type - Unknown datum
	{
		FRealtimeMeshElementType InvalidType(ERealtimeMeshDatumType::Unknown, 1);
		TestFalse(TEXT("Unknown datum type is invalid"), InvalidType.IsValid());
	}

	// Invalid type - Zero datums
	{
		FRealtimeMeshElementType InvalidType(ERealtimeMeshDatumType::Float, 0);
		TestFalse(TEXT("Zero datums is invalid"), InvalidType.IsValid());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementTypeEqualityTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshElementType.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementTypeEqualityTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshElementType Type1(ERealtimeMeshDatumType::Float, 3);
	FRealtimeMeshElementType Type2(ERealtimeMeshDatumType::Float, 3);
	FRealtimeMeshElementType Type3(ERealtimeMeshDatumType::Float, 4);
	FRealtimeMeshElementType Type4(ERealtimeMeshDatumType::Double, 3);

	// Test equality
	TestTrue(TEXT("Identical types should be equal"), Type1 == Type2);
	TestFalse(TEXT("Different NumDatums"), Type1 == Type3);
	TestFalse(TEXT("Different DatumType"), Type1 == Type4);

	// Test inequality
	TestFalse(TEXT("Identical types should not be inequal"), Type1 != Type2);
	TestTrue(TEXT("Different types should be inequal"), Type1 != Type3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementTypeTraitsTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshElementType.Traits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementTypeTraitsTest::RunTest(const FString& Parameters)
{
	// Test GetRealtimeMeshDataElementType for basic types
	{
		FRealtimeMeshElementType FloatType = GetRealtimeMeshDataElementType<float>();
		TestEqual(TEXT("float is Float datum type"), (uint8)FloatType.GetDatumType(), (uint8)ERealtimeMeshDatumType::Float);
		TestEqual(TEXT("float has 1 datum"), FloatType.GetNumDatums(), (uint8)1);
	}

	{
		FRealtimeMeshElementType Vec3Type = GetRealtimeMeshDataElementType<FVector3f>();
		TestEqual(TEXT("FVector3f is Float datum type"), (uint8)Vec3Type.GetDatumType(), (uint8)ERealtimeMeshDatumType::Float);
		TestEqual(TEXT("FVector3f has 3 datums"), Vec3Type.GetNumDatums(), (uint8)3);
	}

	{
		FRealtimeMeshElementType ColorType = GetRealtimeMeshDataElementType<FColor>();
		TestEqual(TEXT("FColor is UInt8 datum type"), (uint8)ColorType.GetDatumType(), (uint8)ERealtimeMeshDatumType::UInt8);
		TestEqual(TEXT("FColor has 4 datums"), ColorType.GetNumDatums(), (uint8)4);
	}

	{
		FRealtimeMeshElementType Int32Type = GetRealtimeMeshDataElementType<int32>();
		TestEqual(TEXT("int32 is Int32 datum type"), (uint8)Int32Type.GetDatumType(), (uint8)ERealtimeMeshDatumType::Int32);
		TestEqual(TEXT("int32 has 1 datum"), Int32Type.GetNumDatums(), (uint8)1);
	}

	return true;
}

//==============================================================================
// FRealtimeMeshBufferLayout Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshBufferLayoutConstructorTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshBufferLayout.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshBufferLayoutConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor
	{
		FRealtimeMeshBufferLayout Layout;
		TestFalse(TEXT("Default constructor creates invalid layout"), Layout.IsValid());
		TestEqual(TEXT("Default NumElements is 0"), Layout.GetNumElements(), 0);
	}

	// Test parameterized constructor
	{
		FRealtimeMeshElementType ElementType(ERealtimeMeshDatumType::Float, 3);
		FRealtimeMeshBufferLayout Layout(ElementType, 100);

		TestTrue(TEXT("Valid constructor creates valid layout"), Layout.IsValid());
		TestEqual(TEXT("ElementType matches"), Layout.GetElementType(), ElementType);
		TestEqual(TEXT("NumElements is 100"), Layout.GetNumElements(), 100);
	}

	// Test GetRealtimeMeshBufferLayout template
	{
		FRealtimeMeshBufferLayout Layout = GetRealtimeMeshBufferLayout<FVector3f>();
		TestTrue(TEXT("Template creates valid layout"), Layout.IsValid());
		TestEqual(TEXT("FVector3f has Float datum"), (uint8)Layout.GetElementType().GetDatumType(), (uint8)ERealtimeMeshDatumType::Float);
		TestEqual(TEXT("FVector3f has 1 element"), Layout.GetNumElements(), 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshBufferLayoutValidityTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshBufferLayout.Validity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshBufferLayoutValidityTest::RunTest(const FString& Parameters)
{
	// Valid layout
	{
		FRealtimeMeshElementType ElementType(ERealtimeMeshDatumType::Float, 3);
		FRealtimeMeshBufferLayout ValidLayout(ElementType, 100);
		TestTrue(TEXT("Valid element type and count creates valid layout"), ValidLayout.IsValid());
	}

	// Invalid layout - invalid element type
	{
		FRealtimeMeshElementType InvalidElementType(ERealtimeMeshDatumType::Unknown, 0);
		FRealtimeMeshBufferLayout InvalidLayout(InvalidElementType, 100);
		TestFalse(TEXT("Invalid element type creates invalid layout"), InvalidLayout.IsValid());
	}

	// Invalid layout - zero elements
	{
		FRealtimeMeshElementType ElementType(ERealtimeMeshDatumType::Float, 3);
		FRealtimeMeshBufferLayout InvalidLayout(ElementType, 0);
		TestFalse(TEXT("Zero elements creates invalid layout"), InvalidLayout.IsValid());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshBufferLayoutEqualityTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshBufferLayout.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshBufferLayoutEqualityTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshElementType ElementType1(ERealtimeMeshDatumType::Float, 3);
	FRealtimeMeshElementType ElementType2(ERealtimeMeshDatumType::Float, 4);

	FRealtimeMeshBufferLayout Layout1(ElementType1, 100);
	FRealtimeMeshBufferLayout Layout2(ElementType1, 100);
	FRealtimeMeshBufferLayout Layout3(ElementType1, 200);
	FRealtimeMeshBufferLayout Layout4(ElementType2, 100);

	// Test equality
	TestTrue(TEXT("Identical layouts should be equal"), Layout1 == Layout2);
	TestFalse(TEXT("Different NumElements"), Layout1 == Layout3);
	TestFalse(TEXT("Different ElementType"), Layout1 == Layout4);

	// Test inequality
	TestFalse(TEXT("Identical layouts should not be inequal"), Layout1 != Layout2);
	TestTrue(TEXT("Different layouts should be inequal"), Layout1 != Layout3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshBufferLayoutTraitsTest,
	"RealtimeMeshComponent.DataTypes.FRealtimeMeshBufferLayout.Traits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshBufferLayoutTraitsTest::RunTest(const FString& Parameters)
{
	// Test BufferTypeTraits for simple types
	{
		static_assert(FRealtimeMeshBufferTypeTraits<FVector3f>::IsValid, "FVector3f should be valid");
		static_assert(FRealtimeMeshBufferTypeTraits<FVector3f>::NumElements == 1, "FVector3f should have 1 element");

		FRealtimeMeshBufferLayout Layout = GetRealtimeMeshBufferLayout<FVector3f>();
		TestTrue(TEXT("FVector3f layout is valid"), Layout.IsValid());
	}

	// Test BufferTypeTraits for TRealtimeMeshTangents
	{
		static_assert(FRealtimeMeshBufferTypeTraits<FRealtimeMeshTangentsNormalPrecision>::IsValid, "Tangents should be valid");
		static_assert(FRealtimeMeshBufferTypeTraits<FRealtimeMeshTangentsNormalPrecision>::NumElements == 2, "Tangents should have 2 elements");

		FRealtimeMeshBufferLayout Layout = GetRealtimeMeshBufferLayout<FRealtimeMeshTangentsNormalPrecision>();
		TestTrue(TEXT("Tangents layout is valid"), Layout.IsValid());
		TestEqual(TEXT("Tangents has 2 elements"), Layout.GetNumElements(), 2);
	}

	// Test BufferTypeTraits for TRealtimeMeshTexCoords
	{
		static_assert(FRealtimeMeshBufferTypeTraits<FRealtimeMeshTexCoordsNormal2>::IsValid, "TexCoords should be valid");
		static_assert(FRealtimeMeshBufferTypeTraits<FRealtimeMeshTexCoordsNormal2>::NumElements == 2, "TexCoords2 should have 2 elements");

		FRealtimeMeshBufferLayout Layout = GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoordsNormal2>();
		TestTrue(TEXT("TexCoords2 layout is valid"), Layout.IsValid());
		TestEqual(TEXT("TexCoords2 has 2 elements"), Layout.GetNumElements(), 2);
	}

	// Test BufferTypeTraits for TIndex3
	{
		static_assert(FRealtimeMeshBufferTypeTraits<FIndex3UI>::IsValid, "FIndex3UI should be valid");
		static_assert(FRealtimeMeshBufferTypeTraits<FIndex3UI>::NumElements == 3, "FIndex3UI should have 3 elements");

		FRealtimeMeshBufferLayout Layout = GetRealtimeMeshBufferLayout<FIndex3UI>();
		TestTrue(TEXT("FIndex3UI layout is valid"), Layout.IsValid());
		TestEqual(TEXT("FIndex3UI has 3 elements"), Layout.GetNumElements(), 3);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
