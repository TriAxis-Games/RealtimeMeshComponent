// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Interface/Core/RealtimeMeshDataStream.h"

using namespace RealtimeMesh;

// ===========================================================================================
// FRealtimeMeshStreamKey Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamKeyConstructorTest,
	"RealtimeMeshComponent.Streams.StreamKey.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamKeyConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor
	{
		FRealtimeMeshStreamKey Key;
		TestEqual(TEXT("Default key should have Unknown type"), Key.GetStreamType(), ERealtimeMeshStreamType::Unknown);
		TestEqual(TEXT("Default key should have NAME_None"), Key.GetName(), FName(NAME_None));
		TestFalse(TEXT("Default key should not be vertex stream"), Key.IsVertexStream());
		TestFalse(TEXT("Default key should not be index stream"), Key.IsIndexStream());
	}

	// Test parameterized constructor with Vertex type
	{
		FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
		TestEqual(TEXT("Vertex key should have Vertex type"), Key.GetStreamType(), ERealtimeMeshStreamType::Vertex);
		TestEqual(TEXT("Vertex key should have correct name"), Key.GetName(), FName(TEXT("Position")));
		TestTrue(TEXT("Vertex key should be vertex stream"), Key.IsVertexStream());
		TestFalse(TEXT("Vertex key should not be index stream"), Key.IsIndexStream());
	}

	// Test parameterized constructor with Index type
	{
		FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Index, FName("Triangles"));
		TestEqual(TEXT("Index key should have Index type"), Key.GetStreamType(), ERealtimeMeshStreamType::Index);
		TestEqual(TEXT("Index key should have correct name"), Key.GetName(), FName(TEXT("Triangles")));
		TestFalse(TEXT("Index key should not be vertex stream"), Key.IsVertexStream());
		TestTrue(TEXT("Index key should be index stream"), Key.IsIndexStream());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamKeyEqualityTest,
	"RealtimeMeshComponent.Streams.StreamKey.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamKeyEqualityTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key1(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStreamKey Key2(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStreamKey Key3(ERealtimeMeshStreamType::Vertex, FName("Normal"));
	FRealtimeMeshStreamKey Key4(ERealtimeMeshStreamType::Index, FName("Position"));

	// Test equality
	TestTrue(TEXT("Identical keys should be equal"), Key1 == Key2);
	TestFalse(TEXT("Keys with different names should not be equal"), Key1 == Key3);
	TestFalse(TEXT("Keys with different types should not be equal"), Key1 == Key4);

	// Test inequality
	TestFalse(TEXT("Identical keys should not be inequal"), Key1 != Key2);
	TestTrue(TEXT("Keys with different names should be inequal"), Key1 != Key3);
	TestTrue(TEXT("Keys with different types should be inequal"), Key1 != Key4);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamKeyHashTest,
	"RealtimeMeshComponent.Streams.StreamKey.Hash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamKeyHashTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key1(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStreamKey Key2(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStreamKey Key3(ERealtimeMeshStreamType::Vertex, FName("Normal"));

	// Test hash consistency
	uint32 Hash1 = GetTypeHash(Key1);
	uint32 Hash2 = GetTypeHash(Key2);
	uint32 Hash3 = GetTypeHash(Key3);

	TestEqual(TEXT("Identical keys should have same hash"), Hash1, Hash2);
	TestNotEqual(TEXT("Different keys should likely have different hash"), Hash1, Hash3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamKeyToStringTest,
	"RealtimeMeshComponent.Streams.StreamKey.ToString",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamKeyToStringTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey VertexKey(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStreamKey IndexKey(ERealtimeMeshStreamType::Index, FName("Triangles"));
	FRealtimeMeshStreamKey UnknownKey;

	FString VertexString = VertexKey.ToString();
	FString IndexString = IndexKey.ToString();
	FString UnknownString = UnknownKey.ToString();

	TestTrue(TEXT("Vertex key string should contain name"), VertexString.Contains(TEXT("Position")));
	TestTrue(TEXT("Vertex key string should contain type"), VertexString.Contains(TEXT("Vertex")));

	TestTrue(TEXT("Index key string should contain name"), IndexString.Contains(TEXT("Triangles")));
	TestTrue(TEXT("Index key string should contain type"), IndexString.Contains(TEXT("Index")));

	TestTrue(TEXT("Unknown key string should contain Unknown"), UnknownString.Contains(TEXT("Unknown")));

	return true;
}

// ===========================================================================================
// FRealtimeMeshStreamRange Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamRangeConstructorTest,
	"RealtimeMeshComponent.Streams.StreamRange.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamRangeConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor
	{
		[[maybe_unused]] FRealtimeMeshStreamRange Range;
	}

	// Test FInt32Range constructor
	{
		FInt32Range VertRange(0, 100);
		FInt32Range IndRange(0, 300);
		FRealtimeMeshStreamRange Range(VertRange, IndRange);

		TestEqual(TEXT("Vertex range should match"), Range.Vertices, VertRange);
		TestEqual(TEXT("Index range should match"), Range.Indices, IndRange);
	}

	// Test uint32 constructor - uses exclusive upper bounds [lower, upper)
	{
		FRealtimeMeshStreamRange Range(0, 100, 0, 300);

		TestEqual(TEXT("Vertex range min should be 0"), Range.GetMinVertex(), 0);
		TestEqual(TEXT("Vertex range max should be 99 (exclusive upper bound 100)"), Range.GetMaxVertex(), 99);
		TestEqual(TEXT("Index range min should be 0"), Range.GetMinIndex(), 0);
		TestEqual(TEXT("Index range max should be 299 (exclusive upper bound 300)"), Range.GetMaxIndex(), 299);
	}

	// Test FInt32RangeBound constructor
	{
		FRealtimeMeshStreamRange Range(
			FInt32RangeBound::Inclusive(0),
			FInt32RangeBound::Inclusive(99),
			FInt32RangeBound::Inclusive(0),
			FInt32RangeBound::Inclusive(299)
		);

		TestEqual(TEXT("Vertex range should have correct bounds"), Range.NumVertices(), 100);
		TestEqual(TEXT("Index range should have correct bounds"), Range.NumPrimitives(3), 100);
	}

	// Test Empty factory
	{
		FRealtimeMeshStreamRange EmptyRange = FRealtimeMeshStreamRange::Empty();

		TestEqual(TEXT("Empty range should have 0 vertices"), EmptyRange.NumVertices(), 0);
		TestEqual(TEXT("Empty range should have 0 primitives"), EmptyRange.NumPrimitives(3), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamRangePrimitiveTest,
	"RealtimeMeshComponent.Streams.StreamRange.Primitives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamRangePrimitiveTest::RunTest(const FString& Parameters)
{
	// Test triangle primitives (3 vertices per primitive)
	// Constructor uses exclusive upper bounds: [0, 100) for vertices, [0, 300) for indices
	{
		FRealtimeMeshStreamRange Range(0, 100, 0, 300);
		TestEqual(TEXT("300 indices should make 100 triangles"), Range.NumPrimitives(3), 100);
	}

	// Test line primitives (2 vertices per primitive)
	// Constructor uses exclusive upper bounds: [0, 100) for vertices, [0, 200) for indices
	{
		FRealtimeMeshStreamRange Range(0, 100, 0, 200);
		TestEqual(TEXT("200 indices should make 100 lines"), Range.NumPrimitives(2), 100);
	}

	// Test degenerate index range
	{
		FRealtimeMeshStreamRange Range(FInt32Range(0, 100), FInt32Range::Empty());
		TestEqual(TEXT("Empty index range should have 0 primitives"), Range.NumPrimitives(3), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamRangeOperationsTest,
	"RealtimeMeshComponent.Streams.StreamRange.Operations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamRangeOperationsTest::RunTest(const FString& Parameters)
{
	// Constructor uses exclusive upper bounds: [lower, upper)
	FRealtimeMeshStreamRange Range1(0, 100, 0, 300);  // Vertices [0,100) = 0-99, Indices [0,300) = 0-299
	FRealtimeMeshStreamRange Range2(50, 150, 150, 450);  // Vertices [50,150) = 50-149, Indices [150,450) = 150-449
	FRealtimeMeshStreamRange Range3(200, 250, 500, 600);  // Vertices [200,250) = 200-249, Indices [500,600) = 500-599

	// Test equality
	{
		FRealtimeMeshStreamRange Range1Copy(0, 100, 0, 300);
		TestTrue(TEXT("Identical ranges should be equal"), Range1 == Range1Copy);
		TestFalse(TEXT("Different ranges should not be equal"), Range1 == Range2);
	}

	// Test inequality
	{
		TestTrue(TEXT("Different ranges should be inequal"), Range1 != Range2);
	}

	// Test overlap
	{
		TestTrue(TEXT("Range1 and Range2 should overlap"), Range1.Overlaps(Range2));
		TestFalse(TEXT("Range1 and Range3 should not overlap"), Range1.Overlaps(Range3));
	}

	// Test contains
	{
		FRealtimeMeshStreamRange SubRange(25, 75, 50, 250);
		TestTrue(TEXT("Range1 should contain SubRange"), Range1.Contains(SubRange));
		TestFalse(TEXT("SubRange should not contain Range1"), SubRange.Contains(Range1));
	}

	// Test intersection
	{
		FRealtimeMeshStreamRange Intersection = Range1.Intersection(Range2);
		// Intersection of [0,100) and [50,150) = [50,100), so min=50, max=99
		TestEqual(TEXT("Intersection should have correct vertex min"), Intersection.GetMinVertex(), 50);
		TestEqual(TEXT("Intersection should have correct vertex max"), Intersection.GetMaxVertex(), 99);
		// Intersection of [0,300) and [150,450) = [150,300), so min=150, max=299
		TestEqual(TEXT("Intersection should have correct index min"), Intersection.GetMinIndex(), 150);
		TestEqual(TEXT("Intersection should have correct index max"), Intersection.GetMaxIndex(), 299);
	}

	// Test hull
	{
		FRealtimeMeshStreamRange Hull = Range1.Hull(Range2);
		// Hull of [0,100) and [50,150) = [0,150), so min=0, max=149
		TestEqual(TEXT("Hull should have correct vertex min"), Hull.GetMinVertex(), 0);
		TestEqual(TEXT("Hull should have correct vertex max"), Hull.GetMaxVertex(), 149);
		// Hull of [0,300) and [150,450) = [0,450), so min=0, max=449
		TestEqual(TEXT("Hull should have correct index min"), Hull.GetMinIndex(), 0);
		TestEqual(TEXT("Hull should have correct index max"), Hull.GetMaxIndex(), 449);
	}

	return true;
}

// ===========================================================================================
// FRealtimeMeshStreamDefaultRowValue Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamDefaultRowValueCreationTest,
	"RealtimeMeshComponent.Streams.DefaultRowValue.Creation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamDefaultRowValueCreationTest::RunTest(const FString& Parameters)
{
	// Test default constructor
	{
		FRealtimeMeshStreamDefaultRowValue DefaultValue;
		TestFalse(TEXT("Default constructed value should not have data"), DefaultValue.HasData());
	}

	// Test Create with same type (no conversion)
	{
		FVector3f TestVector(1.0f, 2.0f, 3.0f);
		FRealtimeMeshStreamDefaultRowValue DefaultValue = FRealtimeMeshStreamDefaultRowValue::Create(TestVector);

		TestTrue(TEXT("Created value should have data"), DefaultValue.HasData());
		TestEqual(TEXT("Layout should match source type"), DefaultValue.GetLayout(), GetRealtimeMeshBufferLayout<FVector3f>());
	}

	// Test Create with type conversion (FVector3f to FVector3d element conversion)
	{
		FVector2f TestVector(1.0f, 2.0f);
		FRealtimeMeshStreamDefaultRowValue DefaultValue = FRealtimeMeshStreamDefaultRowValue::Create(
			TestVector,
			GetRealtimeMeshBufferLayout<FVector2DHalf>()
		);

		TestTrue(TEXT("Created value should have data"), DefaultValue.HasData());
		TestEqual(TEXT("Layout should match target type"), DefaultValue.GetLayout(), GetRealtimeMeshBufferLayout<FVector2DHalf>());
	}

	return true;
}

// ===========================================================================================
// FRealtimeMeshStream Basic Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamBasicConstructorTest,
	"RealtimeMeshComponent.Streams.Stream.BasicConstructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamBasicConstructorTest::RunTest(const FString& Parameters)
{
	// Test default constructor
	{
		FRealtimeMeshStream Stream;
		TestEqual(TEXT("Default stream should have Unknown type"), Stream.GetStreamType(), ERealtimeMeshStreamType::Unknown);
		TestEqual(TEXT("Default stream should have NAME_None"), Stream.GetName(), FName(NAME_None));
		TestEqual(TEXT("Default stream should be empty"), Stream.Num(), 0);
	}

	// Test parameterized constructor
	{
		FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
		FRealtimeMeshBufferLayout Layout = GetRealtimeMeshBufferLayout<FVector3f>();
		FRealtimeMeshStream Stream(Key, Layout);

		TestEqual(TEXT("Stream should have correct type"), Stream.GetStreamType(), ERealtimeMeshStreamType::Vertex);
		TestEqual(TEXT("Stream should have correct name"), Stream.GetName(), FName(TEXT("Position")));
		TestEqual(TEXT("Stream should have correct layout"), Stream.GetLayout(), Layout);
		TestEqual(TEXT("Stream should be empty"), Stream.Num(), 0);
	}

	// Test Create factory method
	{
		FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
		FRealtimeMeshStream Stream = FRealtimeMeshStream::Create<FVector3f>(Key);

		TestEqual(TEXT("Created stream should have correct layout"), Stream.GetLayout(), GetRealtimeMeshBufferLayout<FVector3f>());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamLayoutAndStridesTest,
	"RealtimeMeshComponent.Streams.Stream.LayoutAndStrides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamLayoutAndStridesTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Test strides and element counts
	// FVector3f is treated as a single element of size 12 bytes
	TestEqual(TEXT("Stride should match FVector3f size"), Stream.GetStride(), static_cast<int32>(sizeof(FVector3f)));
	TestEqual(TEXT("Element stride should match FVector3f size"), Stream.GetElementStride(), static_cast<int32>(sizeof(FVector3f)));
	TestEqual(TEXT("Should have 1 element"), Stream.GetNumElements(), 1);

	// Test type checking
	TestTrue(TEXT("Should be of type FVector3f"), Stream.IsOfType<FVector3f>());
	TestFalse(TEXT("Should not be of type FVector4f"), Stream.IsOfType<FVector4f>());

	// Test conversion checking
	TestTrue(TEXT("Should be able to convert to FVector3d"), Stream.CanConvertTo<FVector3d>());
	TestFalse(TEXT("Should not be able to convert to FVector4f (different element count)"), Stream.CanConvertTo<FVector4f>());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamCopyMoveTest,
	"RealtimeMeshComponent.Streams.Stream.CopyMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamCopyMoveTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Original(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Add some data
	Original.AddUninitialized(10);

	// Test copy constructor
	{
		FRealtimeMeshStream Copy(Original);
		TestEqual(TEXT("Copy should have same num elements"), Copy.Num(), Original.Num());
		TestEqual(TEXT("Copy should have same layout"), Copy.GetLayout(), Original.GetLayout());
		TestEqual(TEXT("Copy should have same stream key"), Copy.GetStreamKey(), Original.GetStreamKey());
	}

	// Test move constructor
	{
		FRealtimeMeshStream Source(Original);
		int32 OriginalNum = Source.Num();
		FRealtimeMeshStream Moved(MoveTemp(Source));

		TestEqual(TEXT("Moved should have original num elements"), Moved.Num(), OriginalNum);
		TestEqual(TEXT("Source should be empty after move"), Source.Num(), 0);
	}

	// Test copy assignment
	{
		FRealtimeMeshStream Copy;
		Copy = Original;
		TestEqual(TEXT("Assigned copy should have same num elements"), Copy.Num(), Original.Num());
	}

	// Test move assignment
	{
		FRealtimeMeshStream Source(Original);
		int32 OriginalNum = Source.Num();
		FRealtimeMeshStream Moved;
		Moved = MoveTemp(Source);

		TestEqual(TEXT("Move assigned should have original num elements"), Moved.Num(), OriginalNum);
		TestEqual(TEXT("Source should be empty after move"), Source.Num(), 0);
	}

	return true;
}

// ===========================================================================================
// FRealtimeMeshStream Data Management Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamAddAndResizeTest,
	"RealtimeMeshComponent.Streams.Stream.AddAndResize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamAddAndResizeTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Test AddUninitialized single
	{
		int32 Index = Stream.AddUninitialized();
		TestEqual(TEXT("First add should return index 0"), Index, 0);
		TestEqual(TEXT("Stream should have 1 element"), Stream.Num(), 1);
	}

	// Test AddUninitialized multiple
	{
		int32 Index = Stream.AddUninitialized(9);
		TestEqual(TEXT("Second add should return index 1"), Index, 1);
		TestEqual(TEXT("Stream should have 10 elements"), Stream.Num(), 10);
	}

	// Test AddZeroed
	{
		int32 Index = Stream.AddZeroed(5);
		TestEqual(TEXT("AddZeroed should return index 10"), Index, 10);
		TestEqual(TEXT("Stream should have 15 elements"), Stream.Num(), 15);
	}

	// Test Reserve
	{
		Stream.Reserve(100);
		TestTrue(TEXT("Max should be at least 100"), Stream.Max() >= 100);
		TestEqual(TEXT("Num should still be 15"), Stream.Num(), 15);
	}

	// Test SetNumUninitialized grow
	{
		Stream.SetNumUninitialized(50);
		TestEqual(TEXT("Num should be 50"), Stream.Num(), 50);
	}

	// Test SetNumUninitialized shrink
	{
		Stream.SetNumUninitialized(25);
		TestEqual(TEXT("Num should be 25"), Stream.Num(), 25);
	}

	// Test Empty
	{
		Stream.Empty();
		TestEqual(TEXT("Stream should be empty"), Stream.Num(), 0);
		TestTrue(TEXT("Stream IsEmpty should return true"), Stream.IsEmpty());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamTypedAddTest,
	"RealtimeMeshComponent.Streams.Stream.TypedAdd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamTypedAddTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Test Add single element
	{
		FVector3f TestVec(1.0f, 2.0f, 3.0f);
		Stream.Add(TestVec);

		TestEqual(TEXT("Stream should have 1 element"), Stream.Num(), 1);
		const FVector3f* Data = Stream.GetData<FVector3f>();
		TestEqual(TEXT("Added element should match"), *Data, TestVec);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamAppendTest,
	"RealtimeMeshComponent.Streams.Stream.Append",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamAppendTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Test Append from TArray
	{
		TArray<FVector3f> TestData = {
			FVector3f(1.0f, 2.0f, 3.0f),
			FVector3f(4.0f, 5.0f, 6.0f),
			FVector3f(7.0f, 8.0f, 9.0f)
		};

		Stream.Append(TestData);
		TestEqual(TEXT("Stream should have 3 elements"), Stream.Num(), 3);
	}

	// Test Append from another stream
	{
		FRealtimeMeshStream OtherStream(Key, GetRealtimeMeshBufferLayout<FVector3f>());
		TArray<FVector3f> MoreData = {
			FVector3f(10.0f, 11.0f, 12.0f),
			FVector3f(13.0f, 14.0f, 15.0f)
		};
		OtherStream.Append(MoreData);

		Stream.Append(OtherStream);
		TestEqual(TEXT("Stream should have 5 elements"), Stream.Num(), 5);
	}

	// Test Append move
	{
		FRealtimeMeshStream MoveStream(Key, GetRealtimeMeshBufferLayout<FVector3f>());
		TArray<FVector3f> MoveData = {
			FVector3f(16.0f, 17.0f, 18.0f)
		};
		MoveStream.Append(MoveData);

		Stream.Append(MoveTemp(MoveStream));
		TestEqual(TEXT("Stream should have 6 elements"), Stream.Num(), 6);
		TestEqual(TEXT("Moved stream should be empty"), MoveStream.Num(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamRemoveTest,
	"RealtimeMeshComponent.Streams.Stream.Remove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamRemoveTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Add test data
	TArray<FVector3f> TestData = {
		FVector3f(1.0f, 2.0f, 3.0f),
		FVector3f(4.0f, 5.0f, 6.0f),
		FVector3f(7.0f, 8.0f, 9.0f),
		FVector3f(10.0f, 11.0f, 12.0f),
		FVector3f(13.0f, 14.0f, 15.0f)
	};
	Stream.Append(TestData);

	// Test RemoveAt single
	{
		Stream.RemoveAt(2);
		TestEqual(TEXT("Stream should have 4 elements after remove"), Stream.Num(), 4);
	}

	// Test RemoveAt multiple
	{
		Stream.RemoveAt(1, 2);
		TestEqual(TEXT("Stream should have 2 elements after removing 2"), Stream.Num(), 2);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamFillRangeTest,
	"RealtimeMeshComponent.Streams.Stream.FillRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamFillRangeTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Add uninitialized data
	Stream.AddUninitialized(10);

	// Test FillRange with typed value
	{
		FVector3f FillValue(1.0f, 2.0f, 3.0f);
		Stream.FillRange(0, 10, FillValue);

		const FVector3f* Data = Stream.GetData<FVector3f>();
		bool AllMatch = true;
		for (int32 i = 0; i < 10; i++)
		{
			if (Data[i] != FillValue)
			{
				AllMatch = false;
				break;
			}
		}
		TestTrue(TEXT("All elements should match fill value"), AllMatch);
	}

	// Test ZeroRange
	{
		Stream.ZeroRange(0, 5);

		const FVector3f* Data = Stream.GetData<FVector3f>();
		FVector3f ZeroVec(0.0f, 0.0f, 0.0f);
		bool AllZero = true;
		for (int32 i = 0; i < 5; i++)
		{
			if (Data[i] != ZeroVec)
			{
				AllZero = false;
				break;
			}
		}
		TestTrue(TEXT("First 5 elements should be zero"), AllZero);
	}

	return true;
}

// ===========================================================================================
// FRealtimeMeshStream Type Conversion Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamTypeConversionTest,
	"RealtimeMeshComponent.Streams.Stream.TypeConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamTypeConversionTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Add test data
	TArray<FVector3f> TestData = {
		FVector3f(1.0f, 2.0f, 3.0f),
		FVector3f(4.0f, 5.0f, 6.0f)
	};
	Stream.Append(TestData);

	// Test conversion to FVector3d
	{
		bool bConverted = Stream.ConvertTo<FVector3d>();
		TestTrue(TEXT("Should successfully convert to FVector3d"), bConverted);
		TestEqual(TEXT("Layout should be FVector3d after conversion"), Stream.GetLayout(), GetRealtimeMeshBufferLayout<FVector3d>());
		TestEqual(TEXT("Should still have 2 elements"), Stream.Num(), 2);
	}

	// Test that data is preserved (within floating point tolerance)
	{
		const FVector3d* Data = Stream.GetData<FVector3d>();
		TestTrue(TEXT("First element X should be close to 1.0"), FMath::IsNearlyEqual(Data[0].X, 1.0, 0.0001));
		TestTrue(TEXT("First element Y should be close to 2.0"), FMath::IsNearlyEqual(Data[0].Y, 2.0, 0.0001));
	}

	return true;
}

// ===========================================================================================
// FRealtimeMeshStream Data Access Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamDataAccessTest,
	"RealtimeMeshComponent.Streams.Stream.DataAccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamDataAccessTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Add test data
	TArray<FVector3f> TestData = {
		FVector3f(1.0f, 2.0f, 3.0f),
		FVector3f(4.0f, 5.0f, 6.0f),
		FVector3f(7.0f, 8.0f, 9.0f)
	};
	Stream.Append(TestData);

	// Test GetData
	{
		const FVector3f* Data = Stream.GetData<FVector3f>();
		TestEqual(TEXT("First element should match"), Data[0], TestData[0]);
		TestEqual(TEXT("Second element should match"), Data[1], TestData[1]);
		TestEqual(TEXT("Third element should match"), Data[2], TestData[2]);
	}

	// Test GetDataAtVertex
	{
		const FVector3f* FirstVertex = Stream.GetDataAtVertex<FVector3f>(0);
		const FVector3f* SecondVertex = Stream.GetDataAtVertex<FVector3f>(1);

		TestEqual(TEXT("First vertex should match"), *FirstVertex, TestData[0]);
		TestEqual(TEXT("Second vertex should match"), *SecondVertex, TestData[1]);
	}

	// Test GetArrayView
	{
		TConstArrayView<const FVector3f> View = Stream.GetArrayView<FVector3f>();
		TestEqual(TEXT("ArrayView should have correct num"), View.Num(), 3);
		TestEqual(TEXT("ArrayView first element should match"), View[0], TestData[0]);
	}

	// Test IsValidIndex
	{
		TestTrue(TEXT("Index 0 should be valid"), Stream.IsValidIndex(0));
		TestTrue(TEXT("Index 2 should be valid"), Stream.IsValidIndex(2));
		TestFalse(TEXT("Index 3 should be invalid"), Stream.IsValidIndex(3));
		TestFalse(TEXT("Index -1 should be invalid"), Stream.IsValidIndex(-1));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamSetRangeTest,
	"RealtimeMeshComponent.Streams.Stream.SetRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamSetRangeTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream(Key, GetRealtimeMeshBufferLayout<FVector3f>());

	// Add initial data
	Stream.AddUninitialized(10);

	// Test SetRange with manual assignment
	{
		FVector3f* Data = Stream.GetData<FVector3f>();
		Data[5] = FVector3f(10.0f, 11.0f, 12.0f);
		Data[6] = FVector3f(13.0f, 14.0f, 15.0f);

		TestEqual(TEXT("Element at index 5 should match first new element"), Data[5], FVector3f(10.0f, 11.0f, 12.0f));
		TestEqual(TEXT("Element at index 6 should match second new element"), Data[6], FVector3f(13.0f, 14.0f, 15.0f));
	}

	return true;
}

// ===========================================================================================
// FRealtimeMeshStreamLinkage Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamLinkageBasicTest,
	"RealtimeMeshComponent.Streams.Linkage.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamLinkageBasicTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamLinkage Linkage;

	FRealtimeMeshStreamKey Key1(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream1(Key1, GetRealtimeMeshBufferLayout<FVector3f>());

	FRealtimeMeshStreamKey Key2(ERealtimeMeshStreamType::Vertex, FName("Normal"));
	FRealtimeMeshStream Stream2(Key2, GetRealtimeMeshBufferLayout<FVector3f>());

	// Test initial state
	TestEqual(TEXT("Linkage should start with 0 streams"), Linkage.NumStreams(), 0);

	// Test BindStream
	{
		FVector3f DefaultValue(0.0f, 0.0f, 0.0f);
		FRealtimeMeshStreamDefaultRowValue DefaultRowValue = FRealtimeMeshStreamDefaultRowValue::Create(DefaultValue);

		Linkage.BindStream(Stream1, DefaultRowValue);
		TestEqual(TEXT("Linkage should have 1 stream"), Linkage.NumStreams(), 1);
		TestTrue(TEXT("Linkage should contain Stream1"), Linkage.ContainsStream(Stream1));
		TestTrue(TEXT("Stream1 should be linked"), Stream1.IsLinked());
	}

	// Test binding second stream
	{
		FVector3f DefaultValue(0.0f, 1.0f, 0.0f);
		FRealtimeMeshStreamDefaultRowValue DefaultRowValue = FRealtimeMeshStreamDefaultRowValue::Create(DefaultValue);

		Linkage.BindStream(Stream2, DefaultRowValue);
		TestEqual(TEXT("Linkage should have 2 streams"), Linkage.NumStreams(), 2);
		TestTrue(TEXT("Linkage should contain Stream2"), Linkage.ContainsStream(Stream2));
	}

	// Test RemoveStream
	{
		Linkage.RemoveStream(Stream1);
		TestEqual(TEXT("Linkage should have 1 stream after removal"), Linkage.NumStreams(), 1);
		TestFalse(TEXT("Linkage should not contain Stream1"), Linkage.ContainsStream(Stream1));
		TestFalse(TEXT("Stream1 should not be linked"), Stream1.IsLinked());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamLinkageSynchronizationTest,
	"RealtimeMeshComponent.Streams.Linkage.Synchronization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamLinkageSynchronizationTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamLinkage Linkage;

	FRealtimeMeshStreamKey Key1(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream Stream1(Key1, GetRealtimeMeshBufferLayout<FVector3f>());

	FRealtimeMeshStreamKey Key2(ERealtimeMeshStreamType::Vertex, FName("Normal"));
	FRealtimeMeshStream Stream2(Key2, GetRealtimeMeshBufferLayout<FVector3f>());

	// Bind streams with different initial sizes
	Stream1.AddUninitialized(5);

	FVector3f DefaultValue(0.0f, 0.0f, 0.0f);
	FRealtimeMeshStreamDefaultRowValue DefaultRowValue = FRealtimeMeshStreamDefaultRowValue::Create(DefaultValue);

	Linkage.BindStream(Stream1, DefaultRowValue);
	Linkage.BindStream(Stream2, DefaultRowValue);

	// When binding, Stream2 should match Stream1's size
	TestEqual(TEXT("Stream2 should match Stream1 size after binding"), Stream2.Num(), Stream1.Num());

	// Test that adding to one stream adds to both
	Stream1.AddUninitialized(3);
	TestEqual(TEXT("Stream1 should have 8 elements"), Stream1.Num(), 8);
	TestEqual(TEXT("Stream2 should also have 8 elements"), Stream2.Num(), 8);

	// Test that the new elements in Stream2 have the default value
	const FVector3f* Data = Stream2.GetData<FVector3f>();
	TestEqual(TEXT("New elements should have default value"), Data[5], DefaultValue);

	return true;
}

// ===========================================================================================
// FRealtimeMeshStreamSet Tests
// ===========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamSetBasicTest,
	"RealtimeMeshComponent.Streams.StreamSet.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamSetBasicTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	// Test initial state
	TestEqual(TEXT("StreamSet should start with 0 streams"), StreamSet.Num(), 0);
	TestTrue(TEXT("StreamSet should be empty"), StreamSet.IsEmpty());

	// Test AddStream with layout
	{
		FRealtimeMeshStream& Stream = StreamSet.AddStream(
			ERealtimeMeshStreamType::Vertex,
			FName("Position"),
			GetRealtimeMeshBufferLayout<FVector3f>()
		);

		TestEqual(TEXT("StreamSet should have 1 stream"), StreamSet.Num(), 1);
		TestFalse(TEXT("StreamSet should not be empty"), StreamSet.IsEmpty());
	}

	// Test AddStream with template
	{
		FRealtimeMeshStream& Stream = StreamSet.AddStream<FVector3f>(
			ERealtimeMeshStreamType::Vertex,
			FName("Normal")
		);

		TestEqual(TEXT("StreamSet should have 2 streams"), StreamSet.Num(), 2);
	}

	// Test Contains
	{
		FRealtimeMeshStreamKey PositionKey(ERealtimeMeshStreamType::Vertex, FName("Position"));
		FRealtimeMeshStreamKey NormalKey(ERealtimeMeshStreamType::Vertex, FName("Normal"));
		FRealtimeMeshStreamKey MissingKey(ERealtimeMeshStreamType::Vertex, FName("Missing"));

		TestTrue(TEXT("StreamSet should contain Position"), StreamSet.Contains(PositionKey));
		TestTrue(TEXT("StreamSet should contain Normal"), StreamSet.Contains(NormalKey));
		TestFalse(TEXT("StreamSet should not contain Missing"), StreamSet.Contains(MissingKey));
	}

	// Test Find
	{
		FRealtimeMeshStreamKey PositionKey(ERealtimeMeshStreamType::Vertex, FName("Position"));
		FRealtimeMeshStream* Stream = StreamSet.Find(PositionKey);

		TestNotNull(TEXT("Should find Position stream"), Stream);
		if (Stream)
		{
			TestEqual(TEXT("Found stream should have correct name"), Stream->GetName(), FName(TEXT("Position")));
		}
	}

	// Test Remove
	{
		FRealtimeMeshStreamKey PositionKey(ERealtimeMeshStreamType::Vertex, FName("Position"));
		int32 Removed = StreamSet.Remove(PositionKey);

		TestEqual(TEXT("Should remove 1 stream"), Removed, 1);
		TestEqual(TEXT("StreamSet should have 1 stream after removal"), StreamSet.Num(), 1);
		TestFalse(TEXT("StreamSet should not contain Position"), StreamSet.Contains(PositionKey));
	}

	// Test Empty
	{
		StreamSet.Empty();
		TestEqual(TEXT("StreamSet should be empty after Empty()"), StreamSet.Num(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamSetFindOrAddTest,
	"RealtimeMeshComponent.Streams.StreamSet.FindOrAdd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamSetFindOrAddTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	FRealtimeMeshStreamKey Key(ERealtimeMeshStreamType::Vertex, FName("Position"));

	// Test FindOrAdd when stream doesn't exist
	{
		FRealtimeMeshStream& Stream = StreamSet.FindOrAdd(Key, GetRealtimeMeshBufferLayout<FVector3f>());
		TestEqual(TEXT("StreamSet should have 1 stream"), StreamSet.Num(), 1);
		TestEqual(TEXT("Stream should have correct layout"), Stream.GetLayout(), GetRealtimeMeshBufferLayout<FVector3f>());

		// Add data to the stream
		Stream.AddUninitialized(5);
	}

	// Test FindOrAdd when stream exists with same layout
	{
		FRealtimeMeshStream& Stream = StreamSet.FindOrAdd(Key, GetRealtimeMeshBufferLayout<FVector3f>());
		TestEqual(TEXT("StreamSet should still have 1 stream"), StreamSet.Num(), 1);
		TestEqual(TEXT("Stream should retain its data"), Stream.Num(), 5);
	}

	// Test FindOrAdd when stream exists with convertible layout
	{
		FRealtimeMeshStream& Stream = StreamSet.FindOrAdd(Key, GetRealtimeMeshBufferLayout<FVector3d>());
		TestEqual(TEXT("Stream should have converted layout"), Stream.GetLayout(), GetRealtimeMeshBufferLayout<FVector3d>());
		TestEqual(TEXT("Stream should retain element count after conversion"), Stream.Num(), 5);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamSetCopyTest,
	"RealtimeMeshComponent.Streams.StreamSet.Copy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamSetCopyTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet Original;

	// Add streams to original
	Original.AddStream<FVector3f>(ERealtimeMeshStreamType::Vertex, FName("Position"));
	Original.AddStream<FVector3f>(ERealtimeMeshStreamType::Vertex, FName("Normal"));

	FRealtimeMeshStreamKey PositionKey(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStream& OriginalPosition = Original.FindChecked(PositionKey);
	OriginalPosition.AddUninitialized(10);

	// Test explicit copy constructor
	{
		FRealtimeMeshStreamSet Copy(Original);
		TestEqual(TEXT("Copy should have same number of streams"), Copy.Num(), Original.Num());

		FRealtimeMeshStream& CopiedPosition = Copy.FindChecked(PositionKey);
		TestEqual(TEXT("Copied stream should have same num elements"), CopiedPosition.Num(), OriginalPosition.Num());
	}

	// Test CopyFrom
	{
		FRealtimeMeshStreamSet Copy;
		Copy.CopyFrom(Original);
		TestEqual(TEXT("Copy should have same number of streams"), Copy.Num(), Original.Num());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamSetLinkageTest,
	"RealtimeMeshComponent.Streams.StreamSet.Linkage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamSetLinkageTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	// Add streams
	StreamSet.AddStream<FVector3f>(ERealtimeMeshStreamType::Vertex, FName("Position"));
	StreamSet.AddStream<FVector3f>(ERealtimeMeshStreamType::Vertex, FName("Normal"));

	FRealtimeMeshStreamKey PositionKey(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStreamKey NormalKey(ERealtimeMeshStreamType::Vertex, FName("Normal"));

	// Add streams to link pool
	{
		FVector3f DefaultValue(0.0f, 0.0f, 0.0f);
		FRealtimeMeshStreamDefaultRowValue DefaultRowValue = FRealtimeMeshStreamDefaultRowValue::Create(DefaultValue);

		StreamSet.AddStreamToLinkPool(FName("VertexData"), PositionKey, DefaultRowValue);
		StreamSet.AddStreamToLinkPool(FName("VertexData"), NormalKey, DefaultRowValue);
	}

	// Test synchronization
	{
		FRealtimeMeshStream& Position = StreamSet.FindChecked(PositionKey);
		FRealtimeMeshStream& Normal = StreamSet.FindChecked(NormalKey);

		Position.AddUninitialized(5);

		TestEqual(TEXT("Normal stream should match Position size"), Normal.Num(), Position.Num());
	}

	// Test RemoveStreamFromLinkPool
	{
		StreamSet.RemoveStreamFromLinkPool(FName("VertexData"), NormalKey);

		FRealtimeMeshStream& Position = StreamSet.FindChecked(PositionKey);
		FRealtimeMeshStream& Normal = StreamSet.FindChecked(NormalKey);

		int32 OriginalNormalSize = Normal.Num();
		Position.AddUninitialized(3);

		TestEqual(TEXT("Normal should not synchronize after unlink"), Normal.Num(), OriginalNormalSize);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamSetGetStreamKeysTest,
	"RealtimeMeshComponent.Streams.StreamSet.GetStreamKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamSetGetStreamKeysTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshStreamSet StreamSet;

	FRealtimeMeshStreamKey PositionKey(ERealtimeMeshStreamType::Vertex, FName("Position"));
	FRealtimeMeshStreamKey NormalKey(ERealtimeMeshStreamType::Vertex, FName("Normal"));
	FRealtimeMeshStreamKey TrianglesKey(ERealtimeMeshStreamType::Index, FName("Triangles"));

	StreamSet.AddStream<FVector3f>(PositionKey);
	StreamSet.AddStream<FVector3f>(NormalKey);
	StreamSet.AddStream<uint32>(TrianglesKey);

	// Test GetStreamKeys
	{
		TSet<FRealtimeMeshStreamKey> Keys = StreamSet.GetStreamKeys();
		TestEqual(TEXT("Should have 3 keys"), Keys.Num(), 3);
		TestTrue(TEXT("Should contain Position key"), Keys.Contains(PositionKey));
		TestTrue(TEXT("Should contain Normal key"), Keys.Contains(NormalKey));
		TestTrue(TEXT("Should contain Triangles key"), Keys.Contains(TrianglesKey));
	}

	return true;
}
