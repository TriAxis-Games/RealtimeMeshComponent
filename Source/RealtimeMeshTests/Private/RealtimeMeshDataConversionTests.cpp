// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Interface/Core/RealtimeMeshDataConversion.h"

using namespace RealtimeMesh;

// ============================================================================
// FRealtimeMeshElementConversionKey Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementConversionKeyConstructorTest,
	"RealtimeMeshComponent.DataConversion.FRealtimeMeshElementConversionKey.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementConversionKeyConstructorTest::RunTest(const FString& Parameters)
{
	// Test constructor with two element types
	{
		FRealtimeMeshElementType FromType = GetRealtimeMeshDataElementType<float>();
		FRealtimeMeshElementType ToType = GetRealtimeMeshDataElementType<FFloat16>();

		FRealtimeMeshElementConversionKey Key(FromType, ToType);

		TestEqual(TEXT("FromType matches"), Key.FromType, FromType);
		TestEqual(TEXT("ToType matches"), Key.ToType, ToType);
	}

	// Test with different types
	{
		FRealtimeMeshElementType FromType = GetRealtimeMeshDataElementType<FVector3f>();
		FRealtimeMeshElementType ToType = GetRealtimeMeshDataElementType<FPackedNormal>();

		FRealtimeMeshElementConversionKey Key(FromType, ToType);

		TestEqual(TEXT("FromType matches for vector types"), Key.FromType, FromType);
		TestEqual(TEXT("ToType matches for vector types"), Key.ToType, ToType);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementConversionKeyEqualityTest,
	"RealtimeMeshComponent.DataConversion.FRealtimeMeshElementConversionKey.Equality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementConversionKeyEqualityTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshElementType Float = GetRealtimeMeshDataElementType<float>();
	FRealtimeMeshElementType Half = GetRealtimeMeshDataElementType<FFloat16>();
	FRealtimeMeshElementType Vec3f = GetRealtimeMeshDataElementType<FVector3f>();

	// Test equality
	{
		FRealtimeMeshElementConversionKey Key1(Float, Half);
		FRealtimeMeshElementConversionKey Key2(Float, Half);

		TestTrue(TEXT("Identical keys are equal"), Key1 == Key2);
	}

	// Test inequality - different ToType
	{
		FRealtimeMeshElementConversionKey Key1(Float, Half);
		FRealtimeMeshElementConversionKey Key2(Float, Vec3f);

		TestFalse(TEXT("Keys with different ToType are not equal"), Key1 == Key2);
	}

	// Test inequality - different FromType
	{
		FRealtimeMeshElementConversionKey Key1(Float, Half);
		FRealtimeMeshElementConversionKey Key2(Vec3f, Half);

		TestFalse(TEXT("Keys with different FromType are not equal"), Key1 == Key2);
	}

	// Test inequality - both different
	{
		FRealtimeMeshElementConversionKey Key1(Float, Half);
		FRealtimeMeshElementConversionKey Key2(Vec3f, Vec3f);

		TestFalse(TEXT("Keys with different FromType and ToType are not equal"), Key1 == Key2);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementConversionKeyHashTest,
	"RealtimeMeshComponent.DataConversion.FRealtimeMeshElementConversionKey.Hash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementConversionKeyHashTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshElementType Float = GetRealtimeMeshDataElementType<float>();
	FRealtimeMeshElementType Half = GetRealtimeMeshDataElementType<FFloat16>();
	FRealtimeMeshElementType Vec3f = GetRealtimeMeshDataElementType<FVector3f>();

	// Test hash consistency
	{
		FRealtimeMeshElementConversionKey Key1(Float, Half);
		FRealtimeMeshElementConversionKey Key2(Float, Half);

		TestEqual(TEXT("Equal keys have equal hashes"), GetTypeHash(Key1), GetTypeHash(Key2));
	}

	// Test hash uniqueness (different keys should likely have different hashes)
	{
		FRealtimeMeshElementConversionKey Key1(Float, Half);
		FRealtimeMeshElementConversionKey Key2(Vec3f, Vec3f);

		// Note: This isn't guaranteed, but extremely likely
		TestNotEqual(TEXT("Different keys likely have different hashes"), GetTypeHash(Key1), GetTypeHash(Key2));
	}

	// Test order matters in hash (Float->Half != Half->Float)
	{
		FRealtimeMeshElementConversionKey Key1(Float, Half);
		FRealtimeMeshElementConversionKey Key2(Half, Float);

		TestNotEqual(TEXT("Key order affects hash"), GetTypeHash(Key1), GetTypeHash(Key2));
	}

	return true;
}

// ============================================================================
// FRealtimeMeshElementConverters Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementConvertersSingleElementTest,
	"RealtimeMeshComponent.DataConversion.FRealtimeMeshElementConverters.SingleElement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementConvertersSingleElementTest::RunTest(const FString& Parameters)
{
	// Create a simple converter that doubles a float
	FRealtimeMeshElementConverters Converters(
		[](const void* Input, void* Output) {
			const float& Source = *static_cast<const float*>(Input);
			float& Destination = *static_cast<float*>(Output);
			Destination = Source * 2.0f;
		},
		[](const void* Input, void* Output, uint32 Count) {
			const float* SourceArr = static_cast<const float*>(Input);
			float* DestinationArr = static_cast<float*>(Output);
			for (uint32 Index = 0; Index < Count; Index++)
			{
				DestinationArr[Index] = SourceArr[Index] * 2.0f;
			}
		}
	);

	// Test single element conversion
	{
		float Input = 5.0f;
		float Output = 0.0f;

		Converters.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("Single element conversion works"), Output, 10.0f, 0.001f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshElementConvertersArrayTest,
	"RealtimeMeshComponent.DataConversion.FRealtimeMeshElementConverters.Array",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshElementConvertersArrayTest::RunTest(const FString& Parameters)
{
	// Create a simple converter that doubles a float
	FRealtimeMeshElementConverters Converters(
		[](const void* Input, void* Output) {
			const float& Source = *static_cast<const float*>(Input);
			float& Destination = *static_cast<float*>(Output);
			Destination = Source * 2.0f;
		},
		[](const void* Input, void* Output, uint32 Count) {
			const float* SourceArr = static_cast<const float*>(Input);
			float* DestinationArr = static_cast<float*>(Output);
			for (uint32 Index = 0; Index < Count; Index++)
			{
				DestinationArr[Index] = SourceArr[Index] * 2.0f;
			}
		}
	);

	// Test array conversion
	{
		float Input[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
		float Output[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

		Converters.ConvertContiguousArray(Input, Output, 5);

		TestEqual(TEXT("Array element 0 converted"), Output[0], 2.0f, 0.001f);
		TestEqual(TEXT("Array element 1 converted"), Output[1], 4.0f, 0.001f);
		TestEqual(TEXT("Array element 2 converted"), Output[2], 6.0f, 0.001f);
		TestEqual(TEXT("Array element 3 converted"), Output[3], 8.0f, 0.001f);
		TestEqual(TEXT("Array element 4 converted"), Output[4], 10.0f, 0.001f);
	}

	return true;
}

// ============================================================================
// FRealtimeMeshTypeConversionUtilities Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshTypeConversionUtilitiesRegistrationTest,
	"RealtimeMeshComponent.DataConversion.FRealtimeMeshTypeConversionUtilities.Registration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshTypeConversionUtilitiesRegistrationTest::RunTest(const FString& Parameters)
{
	// Test that float->float conversion is registered (from the cpp file)
	{
		FRealtimeMeshElementType Float = GetRealtimeMeshDataElementType<float>();

		TestTrue(TEXT("float->float conversion is registered"),
			FRealtimeMeshTypeConversionUtilities::CanConvert(Float, Float));
	}

	// Test that float->FFloat16 conversion is registered
	{
		FRealtimeMeshElementType Float = GetRealtimeMeshDataElementType<float>();
		FRealtimeMeshElementType Half = GetRealtimeMeshDataElementType<FFloat16>();

		TestTrue(TEXT("float->FFloat16 conversion is registered"),
			FRealtimeMeshTypeConversionUtilities::CanConvert(Float, Half));
	}

	// Test that FFloat16->float conversion is registered
	{
		FRealtimeMeshElementType Float = GetRealtimeMeshDataElementType<float>();
		FRealtimeMeshElementType Half = GetRealtimeMeshDataElementType<FFloat16>();

		TestTrue(TEXT("FFloat16->float conversion is registered"),
			FRealtimeMeshTypeConversionUtilities::CanConvert(Half, Float));
	}

	// Test that an invalid conversion is not registered (float->FVector3f doesn't exist)
	{
		FRealtimeMeshElementType Float = GetRealtimeMeshDataElementType<float>();
		FRealtimeMeshElementType Vec3f = GetRealtimeMeshDataElementType<FVector3f>();

		TestFalse(TEXT("float->FVector3f conversion is not registered"),
			FRealtimeMeshTypeConversionUtilities::CanConvert(Float, Vec3f));
	}

	return true;
}

// ============================================================================
// Integer Conversion Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshIntegerTypeConversionTest,
	"RealtimeMeshComponent.DataConversion.IntegerTypeConversions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshIntegerTypeConversionTest::RunTest(const FString& Parameters)
{
	// Test uint32 conversions
	TestTrue(TEXT("uint32->uint32 conversion exists"),
		FRealtimeMeshTypeConversionUtilities::CanConvert(
			GetRealtimeMeshDataElementType<uint32>(),
			GetRealtimeMeshDataElementType<uint32>()));

	TestTrue(TEXT("uint32->int32 conversion exists"),
		FRealtimeMeshTypeConversionUtilities::CanConvert(
			GetRealtimeMeshDataElementType<uint32>(),
			GetRealtimeMeshDataElementType<int32>()));

	TestTrue(TEXT("int32->float conversion exists"),
		FRealtimeMeshTypeConversionUtilities::CanConvert(
			GetRealtimeMeshDataElementType<int32>(),
			GetRealtimeMeshDataElementType<float>()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshIntegerConversionValuesTest,
	"RealtimeMeshComponent.DataConversion.IntegerConversions.Values",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshIntegerConversionValuesTest::RunTest(const FString& Parameters)
{
	// Test uint32 -> int32 conversion using the conversion utilities
	{
		uint32 Input = 1000;
		int32 Output = 0;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<uint32>(),
			GetRealtimeMeshDataElementType<int32>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("uint32 -> int32 conversion preserves value"), Output, static_cast<int32>(1000));
	}

	// Test int32 -> float conversion
	{
		int32 Input = 42;
		float Output = 0.0f;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<int32>(),
			GetRealtimeMeshDataElementType<float>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("int32 -> float conversion preserves value"), Output, 42.0f, 0.001f);
	}

	// Test array conversion: int32 -> float
	{
		int32 Input[4] = { 10, 20, 30, 40 };
		float Output[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<int32>(),
			GetRealtimeMeshDataElementType<float>());

		Converter.ConvertContiguousArray(Input, Output, 4);

		TestEqual(TEXT("int32 array element 0 converted"), Output[0], 10.0f, 0.001f);
		TestEqual(TEXT("int32 array element 1 converted"), Output[1], 20.0f, 0.001f);
		TestEqual(TEXT("int32 array element 2 converted"), Output[2], 30.0f, 0.001f);
		TestEqual(TEXT("int32 array element 3 converted"), Output[3], 40.0f, 0.001f);
	}

	return true;
}

// ============================================================================
// Float Conversion Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshFloatConversionTest,
	"RealtimeMeshComponent.DataConversion.FloatConversions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshFloatConversionTest::RunTest(const FString& Parameters)
{
	// Test float -> FFloat16 conversion
	{
		float Input = 3.14159f;
		FFloat16 Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<float>(),
			GetRealtimeMeshDataElementType<FFloat16>());

		Converter.ConvertSingleElement(&Input, &Output);

		// FFloat16 has less precision, so use a larger tolerance
		TestEqual(TEXT("float -> FFloat16 conversion approximates value"), static_cast<float>(Output), 3.14159f, 0.01f);
	}

	// Test FFloat16 -> float conversion
	{
		FFloat16 Input(2.5f);
		float Output = 0.0f;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FFloat16>(),
			GetRealtimeMeshDataElementType<float>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FFloat16 -> float conversion preserves value"), Output, 2.5f, 0.001f);
	}

	// Test float -> float (identity conversion)
	{
		float Input = 1.23f;
		float Output = 0.0f;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<float>(),
			GetRealtimeMeshDataElementType<float>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("float -> float identity conversion"), Output, 1.23f, 0.001f);
	}

	return true;
}

// ============================================================================
// Vector2 Conversion Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshVector2ConversionTest,
	"RealtimeMeshComponent.DataConversion.Vector2Conversions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshVector2ConversionTest::RunTest(const FString& Parameters)
{
	// Test FVector2f -> FVector2DHalf conversion
	{
		FVector2f Input(1.5f, 2.5f);
		FVector2DHalf Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector2f>(),
			GetRealtimeMeshDataElementType<FVector2DHalf>());

		Converter.ConvertSingleElement(&Input, &Output);

		// Convert back to verify
		FVector2f Converted = static_cast<FVector2f>(Output);
		TestEqual(TEXT("FVector2f -> FVector2DHalf X component"), Converted.X, 1.5f, 0.01f);
		TestEqual(TEXT("FVector2f -> FVector2DHalf Y component"), Converted.Y, 2.5f, 0.01f);
	}

	// Test FVector2DHalf -> FVector2f conversion
	{
		FVector2DHalf Input(FVector2f(3.0f, 4.0f));
		FVector2f Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector2DHalf>(),
			GetRealtimeMeshDataElementType<FVector2f>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FVector2DHalf -> FVector2f X component"), Output.X, 3.0f, 0.01f);
		TestEqual(TEXT("FVector2DHalf -> FVector2f Y component"), Output.Y, 4.0f, 0.01f);
	}

	// Test FVector2f -> FVector2d conversion
	{
		FVector2f Input(5.5f, 6.5f);
		FVector2d Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector2f>(),
			GetRealtimeMeshDataElementType<FVector2d>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FVector2f -> FVector2d X component"), Output.X, 5.5, 0.001);
		TestEqual(TEXT("FVector2f -> FVector2d Y component"), Output.Y, 6.5, 0.001);
	}

	// Test FVector2d -> FVector2f conversion
	{
		FVector2d Input(7.25, 8.75);
		FVector2f Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector2d>(),
			GetRealtimeMeshDataElementType<FVector2f>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FVector2d -> FVector2f X component"), Output.X, 7.25f, 0.001f);
		TestEqual(TEXT("FVector2d -> FVector2f Y component"), Output.Y, 8.75f, 0.001f);
	}

	return true;
}

// ============================================================================
// Vector3 Conversion Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshVector3ConversionTest,
	"RealtimeMeshComponent.DataConversion.Vector3Conversions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshVector3ConversionTest::RunTest(const FString& Parameters)
{
	// Test FVector3f -> FVector3d conversion
	{
		FVector3f Input(1.0f, 2.0f, 3.0f);
		FVector3d Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector3f>(),
			GetRealtimeMeshDataElementType<FVector3d>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FVector3f -> FVector3d X component"), Output.X, 1.0, 0.001);
		TestEqual(TEXT("FVector3f -> FVector3d Y component"), Output.Y, 2.0, 0.001);
		TestEqual(TEXT("FVector3f -> FVector3d Z component"), Output.Z, 3.0, 0.001);
	}

	// Test FVector3d -> FVector3f conversion
	{
		FVector3d Input(4.5, 5.5, 6.5);
		FVector3f Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector3d>(),
			GetRealtimeMeshDataElementType<FVector3f>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FVector3d -> FVector3f X component"), Output.X, 4.5f, 0.001f);
		TestEqual(TEXT("FVector3d -> FVector3f Y component"), Output.Y, 5.5f, 0.001f);
		TestEqual(TEXT("FVector3d -> FVector3f Z component"), Output.Z, 6.5f, 0.001f);
	}

	// Test FVector3f -> FVector3f (identity)
	{
		FVector3f Input(7.0f, 8.0f, 9.0f);
		FVector3f Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector3f>(),
			GetRealtimeMeshDataElementType<FVector3f>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FVector3f -> FVector3f X component"), Output.X, 7.0f, 0.001f);
		TestEqual(TEXT("FVector3f -> FVector3f Y component"), Output.Y, 8.0f, 0.001f);
		TestEqual(TEXT("FVector3f -> FVector3f Z component"), Output.Z, 9.0f, 0.001f);
	}

	return true;
}

// ============================================================================
// Vector4 and Packed Normal Conversion Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshVector4ConversionTest,
	"RealtimeMeshComponent.DataConversion.Vector4Conversions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshVector4ConversionTest::RunTest(const FString& Parameters)
{
	// Test FVector4f -> FPackedNormal conversion
	{
		FVector4f Input(0.0f, 0.0f, 1.0f, 1.0f); // Up vector
		FPackedNormal Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector4f>(),
			GetRealtimeMeshDataElementType<FPackedNormal>());

		Converter.ConvertSingleElement(&Input, &Output);

		// Convert back and verify
		FVector4f Converted = Output.ToFVector4f();
		TestEqual(TEXT("FVector4f -> FPackedNormal Z component (up)"), Converted.Z, 1.0f, 0.01f);
	}

	// Test FPackedNormal -> FVector4f conversion
	{
		FPackedNormal Input(FVector3f(0.0f, 1.0f, 0.0f)); // Forward vector
		FVector4f Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FPackedNormal>(),
			GetRealtimeMeshDataElementType<FVector4f>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FPackedNormal -> FVector4f Y component"), Output.Y, 1.0f, 0.01f);
	}

	// Test FVector4f -> FPackedRGBA16N conversion
	{
		FVector4f Input(1.0f, 0.0f, 0.0f, 1.0f); // Right vector
		FPackedRGBA16N Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector4f>(),
			GetRealtimeMeshDataElementType<FPackedRGBA16N>());

		Converter.ConvertSingleElement(&Input, &Output);

		// Convert back and verify
		FVector4f Converted = Output.ToFVector4f();
		TestEqual(TEXT("FVector4f -> FPackedRGBA16N X component"), Converted.X, 1.0f, 0.01f);
	}

	// Test FPackedRGBA16N -> FVector4f conversion
	{
		FPackedRGBA16N Input(FVector4f(0.5f, 0.5f, 0.5f, 1.0f));
		FVector4f Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FPackedRGBA16N>(),
			GetRealtimeMeshDataElementType<FVector4f>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FPackedRGBA16N -> FVector4f X component"), Output.X, 0.5f, 0.01f);
		TestEqual(TEXT("FPackedRGBA16N -> FVector4f Y component"), Output.Y, 0.5f, 0.01f);
		TestEqual(TEXT("FPackedRGBA16N -> FVector4f Z component"), Output.Z, 0.5f, 0.01f);
	}

	// Test FPackedNormal -> FPackedRGBA16N conversion
	{
		FPackedNormal Input(FVector3f(0.0f, 0.0f, 1.0f));
		FPackedRGBA16N Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FPackedNormal>(),
			GetRealtimeMeshDataElementType<FPackedRGBA16N>());

		Converter.ConvertSingleElement(&Input, &Output);

		// Both packed formats should preserve the up vector
		FVector4f Converted = Output.ToFVector4f();
		TestEqual(TEXT("FPackedNormal -> FPackedRGBA16N Z component"), Converted.Z, 1.0f, 0.01f);
	}

	// Test FPackedRGBA16N -> FPackedNormal conversion
	{
		FPackedRGBA16N Input(FVector4f(1.0f, 0.0f, 0.0f, 1.0f));
		FPackedNormal Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FPackedRGBA16N>(),
			GetRealtimeMeshDataElementType<FPackedNormal>());

		Converter.ConvertSingleElement(&Input, &Output);

		FVector4f Converted = Output.ToFVector4f();
		TestEqual(TEXT("FPackedRGBA16N -> FPackedNormal X component"), Converted.X, 1.0f, 0.01f);
	}

	return true;
}

// ============================================================================
// Color Conversion Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshColorConversionTest,
	"RealtimeMeshComponent.DataConversion.ColorConversions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshColorConversionTest::RunTest(const FString& Parameters)
{
	// Test FColor -> FLinearColor conversion
	{
		FColor Input(255, 128, 64, 255); // Bright orange, full alpha
		FLinearColor Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FColor>(),
			GetRealtimeMeshDataElementType<FLinearColor>());

		Converter.ConvertSingleElement(&Input, &Output);

		// FColor -> FLinearColor uses SRGB conversion, so we just check it's not zero
		TestTrue(TEXT("FColor -> FLinearColor R component"), Output.R > 0.9f);
		TestTrue(TEXT("FColor -> FLinearColor G component"), Output.G > 0.2f && Output.G < 0.7f);
		TestTrue(TEXT("FColor -> FLinearColor B component"), Output.B > 0.05f && Output.B < 0.2f);
	}

	// Test FLinearColor -> FColor conversion
	{
		FLinearColor Input(1.0f, 0.5f, 0.25f, 1.0f);
		FColor Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FLinearColor>(),
			GetRealtimeMeshDataElementType<FColor>());

		Converter.ConvertSingleElement(&Input, &Output);

		// Convert back to verify round-trip is reasonable
		FLinearColor RoundTrip = FLinearColor::FromSRGBColor(Output);
		TestEqual(TEXT("FLinearColor -> FColor round-trip R"), RoundTrip.R, Input.R, 0.05f);
		TestEqual(TEXT("FLinearColor -> FColor round-trip G"), RoundTrip.G, Input.G, 0.05f);
		TestEqual(TEXT("FLinearColor -> FColor round-trip B"), RoundTrip.B, Input.B, 0.05f);
	}

	// Test FColor -> FColor (identity)
	{
		FColor Input(200, 100, 50, 128);
		FColor Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FColor>(),
			GetRealtimeMeshDataElementType<FColor>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FColor -> FColor R component"), Output.R, Input.R);
		TestEqual(TEXT("FColor -> FColor G component"), Output.G, Input.G);
		TestEqual(TEXT("FColor -> FColor B component"), Output.B, Input.B);
		TestEqual(TEXT("FColor -> FColor A component"), Output.A, Input.A);
	}

	// Test FLinearColor -> FLinearColor (identity)
	{
		FLinearColor Input(0.75f, 0.5f, 0.25f, 0.5f);
		FLinearColor Output;

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FLinearColor>(),
			GetRealtimeMeshDataElementType<FLinearColor>());

		Converter.ConvertSingleElement(&Input, &Output);

		TestEqual(TEXT("FLinearColor -> FLinearColor R component"), Output.R, Input.R, 0.001f);
		TestEqual(TEXT("FLinearColor -> FLinearColor G component"), Output.G, Input.G, 0.001f);
		TestEqual(TEXT("FLinearColor -> FLinearColor B component"), Output.B, Input.B, 0.001f);
		TestEqual(TEXT("FLinearColor -> FLinearColor A component"), Output.A, Input.A, 0.001f);
	}

	return true;
}

// ============================================================================
// Template Function ConvertRealtimeMeshType Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshConvertRealtimeMeshTypeBasicTest,
	"RealtimeMeshComponent.DataConversion.ConvertRealtimeMeshType.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshConvertRealtimeMeshTypeBasicTest::RunTest(const FString& Parameters)
{
	// Test generic conversion (uses constructor)
	{
		float Input = 5.5f;
		double Output = ConvertRealtimeMeshType<float, double>(Input);

		TestEqual(TEXT("float -> double generic conversion"), Output, 5.5, 0.001);
	}

	// Test FVector2f -> FVector2d conversion
	{
		FVector2f Input(1.0f, 2.0f);
		FVector2d Output = ConvertRealtimeMeshType<FVector2f, FVector2d>(Input);

		TestEqual(TEXT("FVector2f -> FVector2d X"), Output.X, 1.0, 0.001);
		TestEqual(TEXT("FVector2f -> FVector2d Y"), Output.Y, 2.0, 0.001);
	}

	// Test FVector3f -> FVector3d conversion
	{
		FVector3f Input(3.0f, 4.0f, 5.0f);
		FVector3d Output = ConvertRealtimeMeshType<FVector3f, FVector3d>(Input);

		TestEqual(TEXT("FVector3f -> FVector3d X"), Output.X, 3.0, 0.001);
		TestEqual(TEXT("FVector3f -> FVector3d Y"), Output.Y, 4.0, 0.001);
		TestEqual(TEXT("FVector3f -> FVector3d Z"), Output.Z, 5.0, 0.001);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshConvertRealtimeMeshTypePackedNormalTest,
	"RealtimeMeshComponent.DataConversion.ConvertRealtimeMeshType.PackedNormal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshConvertRealtimeMeshTypePackedNormalTest::RunTest(const FString& Parameters)
{
	// Test FPackedNormal -> FVector3f (specialized)
	{
		FPackedNormal Input(FVector3f(0.0f, 0.0f, 1.0f));
		FVector3f Output = ConvertRealtimeMeshType<FPackedNormal, FVector3f>(Input);

		TestEqual(TEXT("FPackedNormal -> FVector3f Z component"), Output.Z, 1.0f, 0.01f);
	}

	// Test FPackedNormal -> FVector4f (specialized)
	{
		FPackedNormal Input(FVector3f(1.0f, 0.0f, 0.0f));
		FVector4f Output = ConvertRealtimeMeshType<FPackedNormal, FVector4f>(Input);

		TestEqual(TEXT("FPackedNormal -> FVector4f X component"), Output.X, 1.0f, 0.01f);
	}

	// Test FPackedRGBA16N -> FVector3f (specialized)
	{
		FPackedRGBA16N Input(FVector4f(0.0f, 1.0f, 0.0f, 1.0f));
		FVector3f Output = ConvertRealtimeMeshType<FPackedRGBA16N, FVector3f>(Input);

		TestEqual(TEXT("FPackedRGBA16N -> FVector3f Y component"), Output.Y, 1.0f, 0.01f);
	}

	// Test FPackedRGBA16N -> FVector4f (specialized)
	{
		FPackedRGBA16N Input(FVector4f(0.5f, 0.5f, 0.5f, 1.0f));
		FVector4f Output = ConvertRealtimeMeshType<FPackedRGBA16N, FVector4f>(Input);

		TestEqual(TEXT("FPackedRGBA16N -> FVector4f X component"), Output.X, 0.5f, 0.01f);
		TestEqual(TEXT("FPackedRGBA16N -> FVector4f Y component"), Output.Y, 0.5f, 0.01f);
		TestEqual(TEXT("FPackedRGBA16N -> FVector4f Z component"), Output.Z, 0.5f, 0.01f);
	}

	// Test FPackedNormal -> FPackedRGBA16N (specialized)
	{
		FPackedNormal Input(FVector3f(0.0f, 0.0f, 1.0f));
		FPackedRGBA16N Output = ConvertRealtimeMeshType<FPackedNormal, FPackedRGBA16N>(Input);

		FVector4f Converted = Output.ToFVector4f();
		TestEqual(TEXT("FPackedNormal -> FPackedRGBA16N Z component"), Converted.Z, 1.0f, 0.01f);
	}

	// Test FPackedRGBA16N -> FPackedNormal (specialized)
	{
		FPackedRGBA16N Input(FVector4f(1.0f, 0.0f, 0.0f, 1.0f));
		FPackedNormal Output = ConvertRealtimeMeshType<FPackedRGBA16N, FPackedNormal>(Input);

		FVector4f Converted = Output.ToFVector4f();
		TestEqual(TEXT("FPackedRGBA16N -> FPackedNormal X component"), Converted.X, 1.0f, 0.01f);
	}

	return true;
}

// ============================================================================
// Array/Batch Conversion Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshArrayConversionTest,
	"RealtimeMeshComponent.DataConversion.ArrayConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshArrayConversionTest::RunTest(const FString& Parameters)
{
	// Test array conversion for FVector3f -> FVector3d
	{
		FVector3f Input[3] = {
			FVector3f(1.0f, 2.0f, 3.0f),
			FVector3f(4.0f, 5.0f, 6.0f),
			FVector3f(7.0f, 8.0f, 9.0f)
		};
		FVector3d Output[3];

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FVector3f>(),
			GetRealtimeMeshDataElementType<FVector3d>());

		Converter.ConvertContiguousArray(Input, Output, 3);

		TestEqual(TEXT("Array element 0 X"), Output[0].X, 1.0, 0.001);
		TestEqual(TEXT("Array element 0 Y"), Output[0].Y, 2.0, 0.001);
		TestEqual(TEXT("Array element 0 Z"), Output[0].Z, 3.0, 0.001);

		TestEqual(TEXT("Array element 1 X"), Output[1].X, 4.0, 0.001);
		TestEqual(TEXT("Array element 1 Y"), Output[1].Y, 5.0, 0.001);
		TestEqual(TEXT("Array element 1 Z"), Output[1].Z, 6.0, 0.001);

		TestEqual(TEXT("Array element 2 X"), Output[2].X, 7.0, 0.001);
		TestEqual(TEXT("Array element 2 Y"), Output[2].Y, 8.0, 0.001);
		TestEqual(TEXT("Array element 2 Z"), Output[2].Z, 9.0, 0.001);
	}

	// Test array conversion for FPackedNormal -> FVector4f
	{
		FPackedNormal Input[2] = {
			FPackedNormal(FVector3f(0.0f, 0.0f, 1.0f)),
			FPackedNormal(FVector3f(1.0f, 0.0f, 0.0f))
		};
		FVector4f Output[2];

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FPackedNormal>(),
			GetRealtimeMeshDataElementType<FVector4f>());

		Converter.ConvertContiguousArray(Input, Output, 2);

		TestEqual(TEXT("Packed normal array element 0 Z"), Output[0].Z, 1.0f, 0.01f);
		TestEqual(TEXT("Packed normal array element 1 X"), Output[1].X, 1.0f, 0.01f);
	}

	// Test array conversion for FColor -> FLinearColor
	{
		FColor Input[3] = {
			FColor(255, 0, 0, 255),    // Red
			FColor(0, 255, 0, 255),    // Green
			FColor(0, 0, 255, 255)     // Blue
		};
		FLinearColor Output[3];

		const FRealtimeMeshElementConverters& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(
			GetRealtimeMeshDataElementType<FColor>(),
			GetRealtimeMeshDataElementType<FLinearColor>());

		Converter.ConvertContiguousArray(Input, Output, 3);

		// Just verify they converted (SRGB conversion makes exact values hard to predict)
		TestTrue(TEXT("Red color converted with R > 0.9"), Output[0].R > 0.9f);
		TestTrue(TEXT("Green color converted with G > 0.9"), Output[1].G > 0.9f);
		TestTrue(TEXT("Blue color converted with B > 0.9"), Output[2].B > 0.9f);
	}

	return true;
}
