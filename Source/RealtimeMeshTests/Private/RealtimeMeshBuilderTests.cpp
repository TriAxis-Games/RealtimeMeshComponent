// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Interface/Core/RealtimeMeshBuilder.h"
#include "Interface/Core/RealtimeMeshDataStream.h"
#include "Interface/Core/RealtimeMeshDataTypes.h"

using namespace RealtimeMesh;

// =====================================================================================================================
// Stream Data Accessor Tests - Static Type Conversion
// =====================================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamDataAccessorStaticConversionTest,
	"RealtimeMeshComponent.Builder.StreamDataAccessor.StaticConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamDataAccessorStaticConversionTest::RunTest(const FString& Parameters)
{
	// Test static conversion between FVector3f and FVector3f (AccessType and BufferType same)
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());
	Stream.SetNumUninitialized(3);

	// Create accessor for FVector3f -> FVector3f
	using AccessorType = TRealtimeMeshStreamDataAccessor<FVector3f, FVector3f, false>;
	auto Context = AccessorType::InitializeContext(Stream, 0);

	// Test SetBufferValue and GetBufferValue
	FVector3f TestValue(1.0f, 2.0f, 3.0f);
	AccessorType::SetBufferValue(Context, 0, TestValue);
	FVector3f ReadValue = AccessorType::GetBufferValue(Context, 0);

	TestEqual(TEXT("Static conversion should preserve value"), ReadValue, TestValue);

	// Test another value
	FVector3f TestValue2(10.0f, 20.0f, 30.0f);
	AccessorType::SetBufferValue(Context, 1, TestValue2);
	FVector3f ReadValue2 = AccessorType::GetBufferValue(Context, 1);

	TestEqual(TEXT("Second value should be preserved"), ReadValue2, TestValue2);

	return true;
}

// =====================================================================================================================
// Stream Data Accessor Tests - Same Type Specialization
// =====================================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamDataAccessorSameTypeTest,
	"RealtimeMeshComponent.Builder.StreamDataAccessor.SameType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamDataAccessorSameTypeTest::RunTest(const FString& Parameters)
{
	// Test the optimized same-type accessor
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector2f>());
	Stream.SetNumUninitialized(2);

	using AccessorType = TRealtimeMeshStreamDataAccessor<FVector2f, FVector2f, false>;
	auto Context = AccessorType::InitializeContext(Stream, 0);

	// Test direct access without conversion
	FVector2f TestUV(0.5f, 0.75f);
	AccessorType::SetBufferValue(Context, 0, TestUV);
	FVector2f ReadUV = AccessorType::GetBufferValue(Context, 0);

	TestEqual(TEXT("Same-type accessor should preserve UV value"), ReadUV, TestUV);

	// Test another UV value
	FVector2f TestUV2(0.25f, 0.9f);
	AccessorType::SetBufferValue(Context, 1, TestUV2);
	FVector2f ReadUV2 = AccessorType::GetBufferValue(Context, 1);

	TestEqual(TEXT("Second UV value should be preserved"), ReadUV2, TestUV2);

	return true;
}

// =====================================================================================================================
// Element Accessor Tests
// =====================================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementAccessorBasicTest,
	"RealtimeMeshComponent.Builder.ElementAccessor.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FElementAccessorBasicTest::RunTest(const FString& Parameters)
{
	// Create an interleaved stream with multiple FVector2f elements per row (like UVs with 2 channels)
	// Buffer layout: 2 FVector2DHalf per row (4 half-floats total per row)
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")),
		GetRealtimeMeshBufferLayout<FVector2DHalf>(2));  // 2 UV channels
	Stream.SetNumUninitialized(1);

	// Access the first UV channel (element 0) as FVector2f with conversion from FVector2DHalf
	using StreamAccessor = TRealtimeMeshStreamDataAccessor<FVector2f, FVector2DHalf, true>;
	auto Context = StreamAccessor::InitializeContext(Stream, 0);  // ElementOffset 0 = first UV channel

	using ElementAccessor = TRealtimeMeshElementAccessor<FVector2f, FVector2DHalf, true>;

	// Access row 0, element index 0 (which accesses the first UV channel as a whole FVector2f)
	ElementAccessor UV0Accessor(Context, 0, 0);
	FVector2f TestUV(0.5f, 0.75f);
	UV0Accessor.SetValue(TestUV);
	TestTrue(TEXT("Element accessor should set UV value"), UV0Accessor.GetValue().Equals(TestUV, 0.01f));

	// Test implicit conversion
	FVector2f ReadUV = UV0Accessor;
	TestTrue(TEXT("Element accessor should implicitly convert to FVector2f"), ReadUV.Equals(TestUV, 0.01f));

	// Test assignment operator
	FVector2f NewUV(0.25f, 0.9f);
	UV0Accessor = NewUV;
	TestTrue(TEXT("Assignment operator should work"), UV0Accessor.GetValue().Equals(NewUV, 0.01f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElementAccessorOperatorsTest,
	"RealtimeMeshComponent.Builder.ElementAccessor.Operators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FElementAccessorOperatorsTest::RunTest(const FString& Parameters)
{
	// Test with float elements to test arithmetic operators
	// Create stream with 2 floats per row
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")),
		GetRealtimeMeshBufferLayout<float>(2));
	Stream.SetNumUninitialized(1);

	// Access first float element
	using StreamAccessor = TRealtimeMeshStreamDataAccessor<float, float, true>;
	auto Context = StreamAccessor::InitializeContext(Stream, 0);

	using ElementAccessor = TRealtimeMeshElementAccessor<float, float, true>;
	ElementAccessor Elem(Context, 0, 0);
	Elem.SetValue(10.0f);

	// Test arithmetic operators
	Elem += 5.0f;
	TestEqual(TEXT("Operator += should work"), Elem.GetValue(), 15.0f);

	Elem -= 3.0f;
	TestEqual(TEXT("Operator -= should work"), Elem.GetValue(), 12.0f);

	Elem *= 2.0f;
	TestEqual(TEXT("Operator *= should work"), Elem.GetValue(), 24.0f);

	Elem /= 4.0f;
	TestEqual(TEXT("Operator /= should work"), Elem.GetValue(), 6.0f);

	// Test comparison operators
	TestTrue(TEXT("Operator == should work"), Elem == 6.0f);
	TestTrue(TEXT("Operator != should work"), Elem != 5.0f);
	TestTrue(TEXT("Operator > should work"), Elem > 5.0f);
	TestTrue(TEXT("Operator < should work"), Elem < 7.0f);

	return true;
}

// =====================================================================================================================
// Indexed Buffer Accessor Tests
// =====================================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndexedBufferAccessorBasicTest,
	"RealtimeMeshComponent.Builder.IndexedBufferAccessor.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIndexedBufferAccessorBasicTest::RunTest(const FString& Parameters)
{
	// Create stream with FVector3f
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());
	Stream.SetNumUninitialized(2);

	using StreamAccessor = TRealtimeMeshStreamDataAccessor<FVector3f, FVector3f, false>;
	auto Context = StreamAccessor::InitializeContext(Stream, 0);

	using RowAccessor = TRealtimeMeshIndexedBufferAccessor<FVector3f, FVector3f, false>;

	// Test Get and Set
	RowAccessor Row0(Context, 0);
	FVector3f TestVec(1.0f, 2.0f, 3.0f);
	Row0.Set(TestVec);

	TestEqual(TEXT("Row accessor Get should return set value"), Row0.Get(), TestVec);

	// Test implicit conversion to AccessType
	FVector3f ImplicitVec = Row0;
	TestEqual(TEXT("Row accessor should implicitly convert"), ImplicitVec, TestVec);

	// Test assignment operator
	RowAccessor Row1(Context, 1);
	Row1 = FVector3f(4.0f, 5.0f, 6.0f);
	TestEqual(TEXT("Assignment operator should work"), Row1.Get(), FVector3f(4.0f, 5.0f, 6.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndexedBufferAccessorElementAccessTest,
	"RealtimeMeshComponent.Builder.IndexedBufferAccessor.ElementAccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIndexedBufferAccessorElementAccessTest::RunTest(const FString& Parameters)
{
	// Create an interleaved stream with 3 FVector3f per row (e.g., position, normal, tangent)
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")),
		GetRealtimeMeshBufferLayout<FVector3f>(3));  // 3 elements per row
	Stream.SetNumUninitialized(1);

	// Access the first element (element 0) in the interleaved stream
	using StreamAccessor = TRealtimeMeshStreamDataAccessor<FVector3f, FVector3f, true>;
	auto Context = StreamAccessor::InitializeContext(Stream, 0);  // ElementOffset 0

	using RowAccessor = TRealtimeMeshIndexedBufferAccessor<FVector3f, FVector3f, true>;
	RowAccessor Row(Context, 0);

	// Test GetElement to access different FVector3f elements in the interleaved row
	FVector3f Position(10.0f, 20.0f, 30.0f);
	FVector3f Normal(0.0f, 0.0f, 1.0f);
	FVector3f Tangent(1.0f, 0.0f, 0.0f);

	Row.GetElement(0).SetValue(Position);  // Element 0 = position
	Row.GetElement(1).SetValue(Normal);    // Element 1 = normal
	Row.GetElement(2).SetValue(Tangent);   // Element 2 = tangent

	TestEqual(TEXT("GetElement(0) should be position"), Row.GetElement(0).GetValue(), Position);
	TestEqual(TEXT("GetElement(1) should be normal"), Row.GetElement(1).GetValue(), Normal);
	TestEqual(TEXT("GetElement(2) should be tangent"), Row.GetElement(2).GetValue(), Tangent);

	// Test operator[] (which is same as GetElement)
	FVector3f NewPosition(100.0f, 200.0f, 300.0f);
	Row[0] = NewPosition;  // Set element 0

	TestEqual(TEXT("operator[0] should return element 0"), Row[0].GetValue(), NewPosition);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndexedBufferAccessorSetRangeTest,
	"RealtimeMeshComponent.Builder.IndexedBufferAccessor.SetRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIndexedBufferAccessorSetRangeTest::RunTest(const FString& Parameters)
{
	// Create a stream with a buffer type that holds 3 FVector2f per row as a single unit
	// This uses TRealtimeMeshTexCoords<FVector2f, 3> to treat all 3 UV channels as one AccessType
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")),
		GetRealtimeMeshBufferLayout<FVector2f>(3));  // 3 FVector2f elements per row
	Stream.SetNumUninitialized(1);

	// When accessing with substream access enabled and matching buffer/access types,
	// SetRange/SetAll work on individual scalar elements (FVector2f) within the row
	using StreamAccessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 3>, TRealtimeMeshTexCoords<FVector2f, 3>, false>;
	auto Context = StreamAccessor::InitializeContext(Stream, 0);

	using RowAccessor = TRealtimeMeshIndexedBufferAccessor<TRealtimeMeshTexCoords<FVector2f, 3>, TRealtimeMeshTexCoords<FVector2f, 3>, false>;
	RowAccessor Row(Context, 0);

	// Test SetRange by setting all 3 UV channels at once using TRealtimeMeshTexCoords
	TRealtimeMeshTexCoords<FVector2f, 3> UVSet;
	UVSet[0] = FVector2f(0.0f, 0.0f);
	UVSet[1] = FVector2f(0.5f, 0.5f);
	UVSet[2] = FVector2f(1.0f, 1.0f);
	Row.Set(UVSet);

	// Verify the values were set correctly
	TRealtimeMeshTexCoords<FVector2f, 3> ReadUVs = Row.Get();
	TestTrue(TEXT("UV channel 0"), ReadUVs[0].Equals(FVector2f(0.0f, 0.0f), 0.01f));
	TestTrue(TEXT("UV channel 1"), ReadUVs[1].Equals(FVector2f(0.5f, 0.5f), 0.01f));
	TestTrue(TEXT("UV channel 2"), ReadUVs[2].Equals(FVector2f(1.0f, 1.0f), 0.01f));

	// Test accessing individual elements within the row
	TestTrue(TEXT("GetElement(0)"), Row.GetElement(0).GetValue().Equals(FVector2f(0.0f, 0.0f), 0.01f));
	TestTrue(TEXT("GetElement(1)"), Row.GetElement(1).GetValue().Equals(FVector2f(0.5f, 0.5f), 0.01f));
	TestTrue(TEXT("GetElement(2)"), Row.GetElement(2).GetValue().Equals(FVector2f(1.0f, 1.0f), 0.01f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndexedBufferAccessorSetRangeBoundsTest,
	"RealtimeMeshComponent.Builder.IndexedBufferAccessor.SetRangeBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIndexedBufferAccessorSetRangeBoundsTest::RunTest(const FString& Parameters)
{
	// Create a stream with 3 FVector2f elements per row
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")),
		GetRealtimeMeshBufferLayout<FVector2f>(3));
	Stream.SetNumUninitialized(1);

	// Access using a compound type with 3 elements
	using StreamAccessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 3>, TRealtimeMeshTexCoords<FVector2f, 3>, false>;
	auto Context = StreamAccessor::InitializeContext(Stream, 0);

	using RowAccessor = TRealtimeMeshIndexedBufferAccessor<TRealtimeMeshTexCoords<FVector2f, 3>, TRealtimeMeshTexCoords<FVector2f, 3>, false>;
	RowAccessor Row(Context, 0);

	// Test 1: SetRange with too many elements should clamp to available space
	// The row has 3 elements (NumElements = 3), so trying to set 5 elements starting at index 0
	// should only set the first 3 elements without overwriting past the end
	TArray<FVector2f> TooManyElements = {
		FVector2f(1.0f, 1.0f),
		FVector2f(2.0f, 2.0f),
		FVector2f(3.0f, 3.0f),
		FVector2f(4.0f, 4.0f),  // This should be ignored
		FVector2f(5.0f, 5.0f)   // This should be ignored
	};

	Row.SetRange(0, MakeArrayView(TooManyElements));

	// Verify only the first 3 were set
	TRealtimeMeshTexCoords<FVector2f, 3> ReadUVs = Row.Get();
	TestTrue(TEXT("Element 0 set correctly"), ReadUVs[0].Equals(FVector2f(1.0f, 1.0f), 0.01f));
	TestTrue(TEXT("Element 1 set correctly"), ReadUVs[1].Equals(FVector2f(2.0f, 2.0f), 0.01f));
	TestTrue(TEXT("Element 2 set correctly"), ReadUVs[2].Equals(FVector2f(3.0f, 3.0f), 0.01f));

	// Test 2: SetRange with start offset + count exceeding NumElements should clamp
	// Starting at element 2, we only have room for 1 element (NumElements=3, so indices 0,1,2)
	TArray<FVector2f> OffsetElements = {
		FVector2f(10.0f, 10.0f),
		FVector2f(20.0f, 20.0f),  // This should be ignored
		FVector2f(30.0f, 30.0f)   // This should be ignored
	};

	Row.SetRange(2, MakeArrayView(OffsetElements));

	// Verify only element 2 was updated, elements 0 and 1 remain unchanged
	ReadUVs = Row.Get();
	TestTrue(TEXT("Element 0 unchanged after offset SetRange"), ReadUVs[0].Equals(FVector2f(1.0f, 1.0f), 0.01f));
	TestTrue(TEXT("Element 1 unchanged after offset SetRange"), ReadUVs[1].Equals(FVector2f(2.0f, 2.0f), 0.01f));
	TestTrue(TEXT("Element 2 updated correctly"), ReadUVs[2].Equals(FVector2f(10.0f, 10.0f), 0.01f));

	// Test 3: SetAll with too many elements should clamp
	Row.SetAll({
		FVector2f(100.0f, 100.0f),
		FVector2f(200.0f, 200.0f),
		FVector2f(300.0f, 300.0f),
		FVector2f(400.0f, 400.0f),  // Should be ignored
	});

	ReadUVs = Row.Get();
	TestTrue(TEXT("SetAll element 0"), ReadUVs[0].Equals(FVector2f(100.0f, 100.0f), 0.01f));
	TestTrue(TEXT("SetAll element 1"), ReadUVs[1].Equals(FVector2f(200.0f, 200.0f), 0.01f));
	TestTrue(TEXT("SetAll element 2"), ReadUVs[2].Equals(FVector2f(300.0f, 300.0f), 0.01f));

	return true;
}

// =====================================================================================================================
// Stream Builder Base Tests
// =====================================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamBuilderBasicOperationsTest,
	"RealtimeMeshComponent.Builder.StreamBuilder.BasicOperations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBuilderBasicOperationsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());

	using StreamBuilder = TRealtimeMeshStreamBuilder<FVector3f, FVector3f>;
	StreamBuilder Builder(Stream);

	// Test initial state
	TestEqual(TEXT("Initial Num should be 0"), Builder.Num(), 0);
	TestTrue(TEXT("Initial stream should be empty"), Builder.IsEmpty());

	// Test Reserve
	Builder.Reserve(10);
	TestTrue(TEXT("After reserve, GetSlack should be >= 10"), Builder.GetSlack() >= 10);

	// Test AddUninitialized
	auto Index = Builder.AddUninitialized(1);
	TestEqual(TEXT("AddUninitialized should return 0"), Index, 0);
	TestEqual(TEXT("After AddUninitialized, Num should be 1"), Builder.Num(), 1);

	// Test AddZeroed
	Builder.AddZeroed(2);
	TestEqual(TEXT("After AddZeroed(2), Num should be 3"), Builder.Num(), 3);

	// Test SetNumUninitialized
	Builder.SetNumUninitialized(5);
	TestEqual(TEXT("After SetNumUninitialized(5), Num should be 5"), Builder.Num(), 5);

	// Test Empty
	Builder.Empty();
	TestEqual(TEXT("After Empty, Num should be 0"), Builder.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamBuilderAddGetTest,
	"RealtimeMeshComponent.Builder.StreamBuilder.AddGet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBuilderAddGetTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());

	using StreamBuilder = TRealtimeMeshStreamBuilder<FVector3f, FVector3f>;
	StreamBuilder Builder(Stream);

	// Test Add with value
	FVector3f Vec1(1.0f, 2.0f, 3.0f);
	int32 Index1 = Builder.Add(Vec1);
	TestEqual(TEXT("First Add should return index 0"), Index1, 0);

	FVector3f Vec2(4.0f, 5.0f, 6.0f);
	int32 Index2 = Builder.Add(Vec2);
	TestEqual(TEXT("Second Add should return index 1"), Index2, 1);

	// Test Get
	FVector3f Read1 = Builder.Get(0).Get();
	FVector3f Read2 = Builder.Get(1).Get();

	TestEqual(TEXT("Get(0) should return Vec1"), Read1, Vec1);
	TestEqual(TEXT("Get(1) should return Vec2"), Read2, Vec2);

	// Test GetValue
	FVector3f DirectRead = Builder.GetValue(0);
	TestEqual(TEXT("GetValue should return Vec1"), DirectRead, Vec1);

	// Test operator[]
	FVector3f BracketRead = Builder[1];
	TestEqual(TEXT("operator[] should return Vec2"), BracketRead, Vec2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamBuilderSetOperationsTest,
	"RealtimeMeshComponent.Builder.StreamBuilder.SetOperations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBuilderSetOperationsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());

	using StreamBuilder = TRealtimeMeshStreamBuilder<FVector3f, FVector3f>;
	StreamBuilder Builder(Stream);

	Builder.SetNumUninitialized(5);

	// Test Set with whole FVector3f values
	Builder.Set(0, FVector3f(1.0f, 2.0f, 3.0f));
	TestEqual(TEXT("Set should update value"), Builder.GetValue(0), FVector3f(1.0f, 2.0f, 3.0f));

	Builder.Set(1, FVector3f(10.0f, 20.0f, 30.0f));
	TestEqual(TEXT("Set should update second value"), Builder.GetValue(1), FVector3f(10.0f, 20.0f, 30.0f));

	Builder.Set(2, FVector3f(100.0f, 200.0f, 300.0f));
	TestEqual(TEXT("Set should update third value"), Builder.GetValue(2), FVector3f(100.0f, 200.0f, 300.0f));

	// Test SetRange with multiple FVector3f values
	TArray<FVector3f> RangeVecs = {
		FVector3f(1.0f, 1.0f, 1.0f),
		FVector3f(2.0f, 2.0f, 2.0f)
	};
	Builder.SetRange(3, RangeVecs);
	TestEqual(TEXT("SetRange index 3"), Builder.GetValue(3), RangeVecs[0]);
	TestEqual(TEXT("SetRange index 4"), Builder.GetValue(4), RangeVecs[1]);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamBuilderAppendTest,
	"RealtimeMeshComponent.Builder.StreamBuilder.Append",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBuilderAppendTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());

	using StreamBuilder = TRealtimeMeshStreamBuilder<FVector3f, FVector3f>;
	StreamBuilder Builder(Stream);

	// Test Append with TArray
	TArray<FVector3f> VecsToAppend = {
		FVector3f(1.0f, 2.0f, 3.0f),
		FVector3f(4.0f, 5.0f, 6.0f),
		FVector3f(7.0f, 8.0f, 9.0f)
	};

	Builder.Append(VecsToAppend);
	TestEqual(TEXT("After Append, Num should be 3"), Builder.Num(), 3);
	TestEqual(TEXT("Appended value 0"), Builder.GetValue(0), VecsToAppend[0]);
	TestEqual(TEXT("Appended value 1"), Builder.GetValue(1), VecsToAppend[1]);
	TestEqual(TEXT("Appended value 2"), Builder.GetValue(2), VecsToAppend[2]);

	// Test Append with initializer list
	Builder.Append({FVector3f(10.0f, 11.0f, 12.0f), FVector3f(13.0f, 14.0f, 15.0f)});
	TestEqual(TEXT("After second Append, Num should be 5"), Builder.Num(), 5);
	TestEqual(TEXT("Appended value 3"), Builder.GetValue(3), FVector3f(10.0f, 11.0f, 12.0f));
	TestEqual(TEXT("Appended value 4"), Builder.GetValue(4), FVector3f(13.0f, 14.0f, 15.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamBuilderGeneratorTest,
	"RealtimeMeshComponent.Builder.StreamBuilder.Generator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBuilderGeneratorTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());

	using StreamBuilder = TRealtimeMeshStreamBuilder<FVector3f, FVector3f>;
	StreamBuilder Builder(Stream);

	Builder.SetNumUninitialized(3);

	// Test SetGenerator
	Builder.SetGenerator(0, 3, [](int32 RelativeIndex, int32 AbsoluteIndex) -> FVector3f
	{
		return FVector3f(AbsoluteIndex * 10.0f, AbsoluteIndex * 20.0f, AbsoluteIndex * 30.0f);
	});

	TestEqual(TEXT("Generated value 0"), Builder.GetValue(0), FVector3f(0.0f, 0.0f, 0.0f));
	TestEqual(TEXT("Generated value 1"), Builder.GetValue(1), FVector3f(10.0f, 20.0f, 30.0f));
	TestEqual(TEXT("Generated value 2"), Builder.GetValue(2), FVector3f(20.0f, 40.0f, 60.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamBuilderAppendGeneratorTest,
	"RealtimeMeshComponent.Builder.StreamBuilder.AppendGenerator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBuilderAppendGeneratorTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());

	using StreamBuilder = TRealtimeMeshStreamBuilder<FVector3f, FVector3f>;
	StreamBuilder Builder(Stream);

	// Test AppendGenerator
	Builder.AppendGenerator(3, [](int32 RelativeIndex, int32 AbsoluteIndex) -> FVector3f
	{
		return FVector3f(RelativeIndex * 1.0f, RelativeIndex * 2.0f, RelativeIndex * 3.0f);
	});

	TestEqual(TEXT("After AppendGenerator, Num should be 3"), Builder.Num(), 3);
	TestEqual(TEXT("Generated append value 0"), Builder.GetValue(0), FVector3f(0.0f, 0.0f, 0.0f));
	TestEqual(TEXT("Generated append value 1"), Builder.GetValue(1), FVector3f(1.0f, 2.0f, 3.0f));
	TestEqual(TEXT("Generated append value 2"), Builder.GetValue(2), FVector3f(2.0f, 4.0f, 6.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamBuilderRemoveTest,
	"RealtimeMeshComponent.Builder.StreamBuilder.Remove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStreamBuilderRemoveTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStream Stream(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TEXT("TestStream")), GetRealtimeMeshBufferLayout<FVector3f>());

	using StreamBuilder = TRealtimeMeshStreamBuilder<FVector3f, FVector3f>;
	StreamBuilder Builder(Stream);

	// Add 5 elements
	Builder.Append({
		FVector3f(0.0f, 0.0f, 0.0f),
		FVector3f(1.0f, 1.0f, 1.0f),
		FVector3f(2.0f, 2.0f, 2.0f),
		FVector3f(3.0f, 3.0f, 3.0f),
		FVector3f(4.0f, 4.0f, 4.0f)
	});

	TestEqual(TEXT("Before remove, Num should be 5"), Builder.Num(), 5);

	// Remove index 2
	Builder.RemoveAt(2, 1);

	TestEqual(TEXT("After RemoveAt, Num should be 4"), Builder.Num(), 4);
	TestEqual(TEXT("After remove, index 0 unchanged"), Builder.GetValue(0), FVector3f(0.0f, 0.0f, 0.0f));
	TestEqual(TEXT("After remove, index 1 unchanged"), Builder.GetValue(1), FVector3f(1.0f, 1.0f, 1.0f));
	TestEqual(TEXT("After remove, index 2 is now what was 3"), Builder.GetValue(2), FVector3f(3.0f, 3.0f, 3.0f));
	TestEqual(TEXT("After remove, index 3 is now what was 4"), Builder.GetValue(3), FVector3f(4.0f, 4.0f, 4.0f));

	return true;
}

// =====================================================================================================================
// Mesh Builder Local Tests
// =====================================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshBuilderBasicConstructionTest,
	"RealtimeMeshComponent.Builder.MeshBuilder.BasicConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeshBuilderBasicConstructionTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	// Create basic builder with defaults (uint32 indices, FPackedNormal tangents, FVector2DHalf UVs, 1 UV channel, uint16 polygroups)
	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	// Test initial state
	TestEqual(TEXT("Initial NumVertices should be 0"), Builder.NumVertices(), 0);
	TestEqual(TEXT("Initial NumTriangles should be 0"), Builder.NumTriangles(), 0);
	TestFalse(TEXT("Tangents should not be enabled initially"), Builder.HasTangents());
	TestFalse(TEXT("TexCoords should not be enabled initially"), Builder.HasTexCoords());
	TestFalse(TEXT("Colors should not be enabled initially"), Builder.HasVertexColors());
	TestFalse(TEXT("PolyGroups should not be enabled initially"), Builder.HasPolyGroups());

	// Verify position and triangle streams exist
	TestTrue(TEXT("Position stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::Position));
	TestTrue(TEXT("Triangle stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::Triangles));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshBuilderEnableStreamsTest,
	"RealtimeMeshComponent.Builder.MeshBuilder.EnableStreams",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeshBuilderEnableStreamsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	// Test EnableTangents
	Builder.EnableTangents();
	TestTrue(TEXT("After EnableTangents, HasTangents should be true"), Builder.HasTangents());
	TestTrue(TEXT("Tangents stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::Tangents));

	// Test EnableTexCoords
	Builder.EnableTexCoords(1);
	TestTrue(TEXT("After EnableTexCoords, HasTexCoords should be true"), Builder.HasTexCoords());
	TestTrue(TEXT("TexCoords stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::TexCoords));
	TestEqual(TEXT("NumTexCoordChannels should be 1"), Builder.NumTexCoordChannels(), 1);

	// Test EnableColors
	Builder.EnableColors();
	TestTrue(TEXT("After EnableColors, HasVertexColors should be true"), Builder.HasVertexColors());
	TestTrue(TEXT("Color stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::Color));

	// Test EnablePolyGroups
	Builder.EnablePolyGroups();
	TestTrue(TEXT("After EnablePolyGroups, HasPolyGroups should be true"), Builder.HasPolyGroups());
	TestTrue(TEXT("PolyGroups stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::PolyGroups));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshBuilderDisableStreamsTest,
	"RealtimeMeshComponent.Builder.MeshBuilder.DisableStreams",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeshBuilderDisableStreamsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	// Enable all streams
	Builder.EnableTangents();
	Builder.EnableTexCoords(1);
	Builder.EnableColors();
	Builder.EnablePolyGroups();

	// Disable tangents with stream removal
	Builder.DisableTangents(true);
	TestFalse(TEXT("After DisableTangents, HasTangents should be false"), Builder.HasTangents());
	TestFalse(TEXT("Tangents stream should be removed"), StreamSet.Contains(FRealtimeMeshStreams::Tangents));

	// Disable colors without stream removal
	Builder.DisableColors(false);
	TestFalse(TEXT("After DisableColors(false), HasVertexColors should be false"), Builder.HasVertexColors());
	TestTrue(TEXT("Color stream should still exist"), StreamSet.Contains(FRealtimeMeshStreams::Color));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshBuilderVertexOperationsTest,
	"RealtimeMeshComponent.Builder.MeshBuilder.VertexOperations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeshBuilderVertexOperationsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords(1);
	Builder.EnableColors();

	// Test AddVertex with position
	auto VertIdx = Builder.AddVertex(FVector3f(10.0f, 20.0f, 30.0f));
	TestEqual(TEXT("First vertex should have index 0"), int32(VertIdx), 0);
	TestEqual(TEXT("NumVertices should be 1"), Builder.NumVertices(), 1);

	// Test GetPosition
	FVector3f ReadPos = Builder.GetPosition(0);
	TestEqual(TEXT("GetPosition should return added position"), ReadPos, FVector3f(10.0f, 20.0f, 30.0f));

	// Test SetPosition
	Builder.SetPosition(0, FVector3f(100.0f, 200.0f, 300.0f));
	TestEqual(TEXT("SetPosition should update position"), Builder.GetPosition(0), FVector3f(100.0f, 200.0f, 300.0f));

	// Test SetNormal and GetNormal
	Builder.SetNormal(0, FVector3f(0.0f, 0.0f, 1.0f));
	FVector3f ReadNormal = Builder.GetNormal(0);
	TestTrue(TEXT("GetNormal should return set normal"), ReadNormal.Equals(FVector3f(0.0f, 0.0f, 1.0f), 0.01f));

	// Test SetTangent and GetTangent
	Builder.SetTangent(0, FVector3f(1.0f, 0.0f, 0.0f));
	FVector3f ReadTangent = Builder.GetTangent(0);
	TestTrue(TEXT("GetTangent should return set tangent"), ReadTangent.Equals(FVector3f(1.0f, 0.0f, 0.0f), 0.01f));

	// Test SetTexCoord and GetTexCoord
	Builder.SetTexCoord(0, 0, FVector2f(0.5f, 0.75f));
	FVector2f ReadUV = Builder.GetTexCoord(0, 0);
	TestTrue(TEXT("GetTexCoord should return set UV"), ReadUV.Equals(FVector2f(0.5f, 0.75f), 0.01f));

	// Test SetColor and GetColor
	Builder.SetColor(0, FColor::Red);
	FColor ReadColor = Builder.GetColor(0);
	TestEqual(TEXT("GetColor should return set color"), ReadColor, FColor::Red);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshBuilderTriangleOperationsTest,
	"RealtimeMeshComponent.Builder.MeshBuilder.TriangleOperations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeshBuilderTriangleOperationsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	// Add some vertices
	Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(0.0f, 1.0f, 0.0f));

	// Test AddTriangle with uint32 indices
	auto TriIdx = Builder.AddTriangle(0, 1, 2);
	TestEqual(TEXT("First triangle should have index 0"), TriIdx, 0);
	TestEqual(TEXT("NumTriangles should be 1"), Builder.NumTriangles(), 1);

	// Test GetTriangle
	TIndex3<uint32> ReadTri = Builder.GetTriangle(0);
	TestEqual(TEXT("Triangle V0"), ReadTri.V0, 0u);
	TestEqual(TEXT("Triangle V1"), ReadTri.V1, 1u);
	TestEqual(TEXT("Triangle V2"), ReadTri.V2, 2u);

	// Test SetTriangle
	Builder.SetTriangle(0, 2, 1, 0);
	TIndex3<uint32> UpdatedTri = Builder.GetTriangle(0);
	TestEqual(TEXT("Updated Triangle V0"), UpdatedTri.V0, 2u);
	TestEqual(TEXT("Updated Triangle V1"), UpdatedTri.V1, 1u);
	TestEqual(TEXT("Updated Triangle V2"), UpdatedTri.V2, 0u);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshBuilderPolyGroupsTest,
	"RealtimeMeshComponent.Builder.MeshBuilder.PolyGroups",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeshBuilderPolyGroupsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	Builder.EnablePolyGroups();

	// Add vertices
	Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(0.0f, 1.0f, 0.0f));

	// Test AddTriangle with material index
	auto TriIdx = Builder.AddTriangle(0, 1, 2, 5);
	TestEqual(TEXT("Triangle should be added"), TriIdx, 0);

	// Test GetMaterialIndex
	uint32 MatIdx = Builder.GetMaterialIndex(0);
	TestEqual(TEXT("Material index should be 5"), MatIdx, 5u);

	// Test SetTriangle with material index
	Builder.SetTriangle(0, TIndex3<uint32>(0, 1, 2), 10);
	uint32 UpdatedMatIdx = Builder.GetMaterialIndex(0);
	TestEqual(TEXT("Updated material index should be 10"), UpdatedMatIdx, 10u);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshBuilderReserveTest,
	"RealtimeMeshComponent.Builder.MeshBuilder.Reserve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeshBuilderReserveTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	// Test ReserveNumVertices
	Builder.ReserveNumVertices(100);
	// We can't directly test capacity, but we can verify it doesn't crash

	// Test ReserveAdditionalVertices
	Builder.ReserveAdditionalVertices(50);

	// Test SetNumVertices
	Builder.SetNumVertices(10);
	TestEqual(TEXT("After SetNumVertices(10), NumVertices should be 10"), Builder.NumVertices(), 10);

	// Test EmptyVertices
	Builder.EmptyVertices();
	TestEqual(TEXT("After EmptyVertices, NumVertices should be 0"), Builder.NumVertices(), 0);

	// Test ReserveNumTriangles
	Builder.ReserveNumTriangles(200);

	// Test SetNumTriangles
	Builder.SetNumTriangles(15);
	TestEqual(TEXT("After SetNumTriangles(15), NumTriangles should be 15"), Builder.NumTriangles(), 15);

	// Test EmptyTriangles
	Builder.EmptyTriangles();
	TestEqual(TEXT("After EmptyTriangles, NumTriangles should be 0"), Builder.NumTriangles(), 0);

	return true;
}

// =====================================================================================================================
// Vertex Builder Local Tests
// =====================================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVertexBuilderBasicTest,
	"RealtimeMeshComponent.Builder.VertexBuilder.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVertexBuilderBasicTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords(1);
	Builder.EnableColors();

	// Test AddVertex returns VertexBuilder
	auto VertBuilder = Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f));

	// Test GetIndex
	TestEqual(TEXT("GetIndex should return 0"), VertBuilder.GetIndex(), 0);

	// Test implicit conversion to index
	int32 Index = VertBuilder;
	TestEqual(TEXT("Implicit conversion to index should work"), Index, 0);

	// Test HasTangents, HasTexCoords, HasVertexColors
	TestTrue(TEXT("VertexBuilder.HasTangents should be true"), VertBuilder.HasTangents());
	TestTrue(TEXT("VertexBuilder.HasTexCoords should be true"), VertBuilder.HasTexCoords());
	TestTrue(TEXT("VertexBuilder.HasVertexColors should be true"), VertBuilder.HasVertexColors());
	TestEqual(TEXT("VertexBuilder.NumTexCoordChannels should be 1"), VertBuilder.NumTexCoordChannels(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVertexBuilderChainedCallsTest,
	"RealtimeMeshComponent.Builder.VertexBuilder.ChainedCalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVertexBuilderChainedCallsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords(1);
	Builder.EnableColors();

	// Test chained calls
	auto VertIdx = Builder.AddVertex()
		.SetPosition(FVector3f(10.0f, 20.0f, 30.0f))
		.SetNormal(FVector3f(0.0f, 0.0f, 1.0f))
		.SetTangent(FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(0.5f, 0.5f))
		.SetColor(FColor::Blue)
		.GetIndex();

	// Verify all values were set
	TestEqual(TEXT("Chained position"), Builder.GetPosition(VertIdx), FVector3f(10.0f, 20.0f, 30.0f));
	TestTrue(TEXT("Chained normal"), Builder.GetNormal(VertIdx).Equals(FVector3f(0.0f, 0.0f, 1.0f), 0.01f));
	TestTrue(TEXT("Chained tangent"), Builder.GetTangent(VertIdx).Equals(FVector3f(1.0f, 0.0f, 0.0f), 0.01f));
	TestTrue(TEXT("Chained UV"), Builder.GetTexCoord(VertIdx, 0).Equals(FVector2f(0.5f, 0.5f), 0.01f));
	TestEqual(TEXT("Chained color"), Builder.GetColor(VertIdx), FColor::Blue);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVertexBuilderEditTest,
	"RealtimeMeshComponent.Builder.VertexBuilder.Edit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVertexBuilderEditTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords(1);

	// Add initial vertex
	Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f));

	// Edit the vertex
	Builder.EditVertex(0)
		.SetPosition(FVector3f(100.0f, 200.0f, 300.0f))
		.SetNormal(FVector3f(0.0f, 1.0f, 0.0f))
		.SetTexCoord(FVector2f(0.25f, 0.75f));

	// Verify edits
	TestEqual(TEXT("Edited position"), Builder.GetPosition(0), FVector3f(100.0f, 200.0f, 300.0f));
	TestTrue(TEXT("Edited normal"), Builder.GetNormal(0).Equals(FVector3f(0.0f, 1.0f, 0.0f), 0.01f));
	TestTrue(TEXT("Edited UV"), Builder.GetTexCoord(0, 0).Equals(FVector2f(0.25f, 0.75f), 0.01f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVertexBuilderGettersTest,
	"RealtimeMeshComponent.Builder.VertexBuilder.Getters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVertexBuilderGettersTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords(1);
	Builder.EnableColors();

	// Set up vertex
	Builder.AddVertex(FVector3f(5.0f, 10.0f, 15.0f));
	Builder.SetNormal(0, FVector3f(0.0f, 0.0f, 1.0f));
	Builder.SetTangent(0, FVector3f(1.0f, 0.0f, 0.0f));
	Builder.SetTexCoord(0, 0, FVector2f(0.3f, 0.7f));
	Builder.SetColor(0, FColor::Green);

	// Test getters through VertexBuilder
	auto VertBuilder = Builder.EditVertex(0);

	FVector3f Pos = VertBuilder.GetPosition();
	TestEqual(TEXT("VertexBuilder GetPosition"), Pos, FVector3f(5.0f, 10.0f, 15.0f));

	FVector3f Normal = VertBuilder.GetNormal();
	TestTrue(TEXT("VertexBuilder GetNormal"), Normal.Equals(FVector3f(0.0f, 0.0f, 1.0f), 0.01f));

	FVector3f Tangent = VertBuilder.GetTangent();
	TestTrue(TEXT("VertexBuilder GetTangent"), Tangent.Equals(FVector3f(1.0f, 0.0f, 0.0f), 0.01f));

	FVector2f UV = VertBuilder.GetTexCoord(0);
	TestTrue(TEXT("VertexBuilder GetTexCoord"), UV.Equals(FVector2f(0.3f, 0.7f), 0.01f));

	FColor Color = VertBuilder.GetColor();
	TestEqual(TEXT("VertexBuilder GetColor"), Color, FColor::Green);

	FLinearColor LinearColor = VertBuilder.GetLinearColor();
	TestTrue(TEXT("VertexBuilder GetLinearColor should be greenish"), LinearColor.G > 0.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVertexBuilderSetNormalAndTangentTest,
	"RealtimeMeshComponent.Builder.VertexBuilder.SetNormalAndTangent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVertexBuilderSetNormalAndTangentTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	Builder.EnableTangents();

	// Add vertex and set normal/tangent together
	Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));

	// Verify both were set
	FVector3f Normal = Builder.GetNormal(0);
	FVector3f Tangent = Builder.GetTangent(0);

	TestTrue(TEXT("SetNormalAndTangent normal"), Normal.Equals(FVector3f(0.0f, 0.0f, 1.0f), 0.01f));
	TestTrue(TEXT("SetNormalAndTangent tangent"), Tangent.Equals(FVector3f(1.0f, 0.0f, 0.0f), 0.01f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVertexBuilderSetTangentsTest,
	"RealtimeMeshComponent.Builder.VertexBuilder.SetTangents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVertexBuilderSetTangentsTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	using MeshBuilder = TRealtimeMeshBuilderLocal<>;
	MeshBuilder Builder(StreamSet);

	Builder.EnableTangents();

	// Add vertex and set all tangents (normal, binormal, tangent)
	Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f))
		.SetTangents(
			FVector3f(0.0f, 0.0f, 1.0f),  // Normal
			FVector3f(0.0f, 1.0f, 0.0f),  // Binormal
			FVector3f(1.0f, 0.0f, 0.0f)   // Tangent
		);

	// Verify they were set
	FVector3f Normal = Builder.GetNormal(0);
	FVector3f Tangent = Builder.GetTangent(0);

	TestTrue(TEXT("SetTangents normal"), Normal.Equals(FVector3f(0.0f, 0.0f, 1.0f), 0.01f));
	TestTrue(TEXT("SetTangents tangent"), Tangent.Equals(FVector3f(1.0f, 0.0f, 0.0f), 0.01f));
	// Note: Binormal is derived from Normal and Tangent, so we don't test it directly here

	return true;
}
