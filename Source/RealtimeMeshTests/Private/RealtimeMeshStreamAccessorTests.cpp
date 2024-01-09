#include "Mesh/RealtimeMeshBuilder.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RealtimeMeshStreamAccessorTests,  "RealtimeMeshComponent.RealtimeMeshBuilderAccessors",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

using namespace RealtimeMesh;

bool RealtimeMeshStreamAccessorTests::RunTest(const FString& Parameters)
{
	TArray<FVector2f> TestData = { FVector2f(0, 1), FVector2f(1, 0), FVector2f(0.5, 1), FVector2f(1, 0.5) };

	//////////////////////////////////////////
	// Direct read (no conversion) accessor
	
	{
		FRealtimeMeshStream FVectorStream = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2f, 2>>(FRealtimeMeshStreams::Position);
		FVectorStream.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, TRealtimeMeshTexCoords<FVector2f, 2>, false>;
		auto Context = Accessor::InitializeContext(FVectorStream, 0);

		for (int32 Index = 0; Index < 4; Index++)
		{
			Accessor::SetElementValue(Context, Index / 2, Index % 2, TestData[Index]);		
		}
	
		for (int32 Index = 0; Index < 4; Index++)
		{
			FVector2f TestValue = Accessor::GetElementValue(Context, Index / 2, Index % 2);
			TestTrue(TEXT("StaticConversionAccessor: Get/Set Element Value"), TestValue.Equals(TestData[Index]));
		}
	}

	{
		FRealtimeMeshStream FVectorStreamWide = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2f, 4>>(FRealtimeMeshStreams::Position);
		FVectorStreamWide.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, TRealtimeMeshTexCoords<FVector2f, 2>, true>;
		auto Context = Accessor::InitializeContext(FVectorStreamWide, 1);

		for (int32 Index = 0; Index < 4; Index++)
		{
			Accessor::SetElementValue(Context, Index / 2, Index % 2, TestData[Index]);		
		}
	
		for (int32 Index = 0; Index < 4; Index++)
		{
			FVector2f TestValue = Accessor::GetElementValue(Context, Index / 2, Index % 2);
			TestTrue(TEXT("StaticConversionSubAccessor: Get/Set Element Value"), TestValue.Equals(TestData[Index]));
		}
	}

	{
		FRealtimeMeshStream FVectorStream = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2f, 2>>(FRealtimeMeshStreams::Position);
		FVectorStream.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, TRealtimeMeshTexCoords<FVector2f, 2>, false>;
		auto Context = Accessor::InitializeContext(FVectorStream, 0);

		for (int32 Index = 0; Index < 4; Index += 2)
		{
			Accessor::SetBufferValue(Context, Index / 2, TRealtimeMeshTexCoords<FVector2f, 2>(TestData[Index], TestData[Index + 1]));		
		}
	
		for (int32 Index = 0; Index < 4; Index += 2)
		{
			TRealtimeMeshTexCoords<FVector2f, 2> TestValue = Accessor::GetBufferValue(Context, Index / 2);
			TestTrue(TEXT("StaticConversionAccessor: Get/Set Buffer Value"), TestValue[0].Equals(TestData[Index]) && TestValue[1].Equals(TestData[Index + 1]));
		}
	}

	{
		FRealtimeMeshStream FVectorStreamWide = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2f, 4>>(FRealtimeMeshStreams::Position);
		FVectorStreamWide.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, TRealtimeMeshTexCoords<FVector2f, 2>, true>;
		auto Context = Accessor::InitializeContext(FVectorStreamWide, 1);

		for (int32 Index = 0; Index < 4; Index += 2)
		{
			Accessor::SetBufferValue(Context, Index / 2, TRealtimeMeshTexCoords<FVector2f, 2>(TestData[Index], TestData[Index + 1]));		
		}
	
		for (int32 Index = 0; Index < 4; Index += 2)
		{
			TRealtimeMeshTexCoords<FVector2f, 2> TestValue = Accessor::GetBufferValue(Context, Index / 2);
			TestTrue(TEXT("StaticConversionSubAccessor: Get/Set Buffer Value"), TestValue[0].Equals(TestData[Index]) && TestValue[1].Equals(TestData[Index + 1]));
		}
	}

	
	
	//////////////////////////////////////////
	// Static conversion accessor

	{
		FRealtimeMeshStream FHalfVectorStream = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 2>>(FRealtimeMeshStreams::Position);
		FHalfVectorStream.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, TRealtimeMeshTexCoords<FVector2DHalf, 2>, false>;
		auto Context = Accessor::InitializeContext(FHalfVectorStream, 0);

		for (int32 Index = 0; Index < 4; Index++)
		{
			Accessor::SetElementValue(Context, Index / 2, Index % 2, TestData[Index]);		
		}
	
		for (int32 Index = 0; Index < 4; Index++)
		{
			FVector2f TestValue = Accessor::GetElementValue(Context, Index / 2, Index % 2);
			TestTrue(TEXT("StaticConversionAccessor: Get/Set Element Value"), TestValue.Equals(TestData[Index]));
		}
	}

	{
		FRealtimeMeshStream FHalfVectorStreamWide = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 4>>(FRealtimeMeshStreams::Position);
		FHalfVectorStreamWide.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, TRealtimeMeshTexCoords<FVector2DHalf, 2>, true>;
		auto Context = Accessor::InitializeContext(FHalfVectorStreamWide, 1);

		for (int32 Index = 0; Index < 4; Index++)
		{
			Accessor::SetElementValue(Context, Index / 2, Index % 2, TestData[Index]);		
		}
	
		for (int32 Index = 0; Index < 4; Index++)
		{
			FVector2f TestValue = Accessor::GetElementValue(Context, Index / 2, Index % 2);
			TestTrue(TEXT("StaticConversionSubAccessor: Get/Set Element Value"), TestValue.Equals(TestData[Index]));
		}
	}

	{
		FRealtimeMeshStream FHalfVectorStream = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 2>>(FRealtimeMeshStreams::Position);
		FHalfVectorStream.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, TRealtimeMeshTexCoords<FVector2DHalf, 2>, false>;
		auto Context = Accessor::InitializeContext(FHalfVectorStream, 0);

		for (int32 Index = 0; Index < 4; Index += 2)
		{
			Accessor::SetBufferValue(Context, Index / 2, TRealtimeMeshTexCoords<FVector2f, 2>(TestData[Index], TestData[Index + 1]));		
		}
	
		for (int32 Index = 0; Index < 4; Index += 2)
		{
			TRealtimeMeshTexCoords<FVector2f, 2> TestValue = Accessor::GetBufferValue(Context, Index / 2);
			TestTrue(TEXT("StaticConversionAccessor: Get/Set Buffer Value"), TestValue[0].Equals(TestData[Index]) && TestValue[1].Equals(TestData[Index + 1]));
		}
	}

	{
		FRealtimeMeshStream FHalfVectorStreamWide = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 4>>(FRealtimeMeshStreams::Position);
		FHalfVectorStreamWide.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, TRealtimeMeshTexCoords<FVector2DHalf, 2>, true>;
		auto Context = Accessor::InitializeContext(FHalfVectorStreamWide, 1);

		for (int32 Index = 0; Index < 4; Index += 2)
		{
			Accessor::SetBufferValue(Context, Index / 2, TRealtimeMeshTexCoords<FVector2f, 2>(TestData[Index], TestData[Index + 1]));		
		}
	
		for (int32 Index = 0; Index < 4; Index += 2)
		{
			TRealtimeMeshTexCoords<FVector2f, 2> TestValue = Accessor::GetBufferValue(Context, Index / 2);
			TestTrue(TEXT("StaticConversionSubAccessor: Get/Set Buffer Value"), TestValue[0].Equals(TestData[Index]) && TestValue[1].Equals(TestData[Index + 1]));
		}
	}


	//////////////////////////////////////////
	// Dynamic conversion accessor

	{
		FRealtimeMeshStream FHalfVectorStream = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 2>>(FRealtimeMeshStreams::Position);
		FHalfVectorStream.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, void, false>;
		auto Context = Accessor::InitializeContext(FHalfVectorStream, 0);

		for (int32 Index = 0; Index < 4; Index++)
		{
			Accessor::SetElementValue(Context, Index / 2, Index % 2, TestData[Index]);		
		}
	
		for (int32 Index = 0; Index < 4; Index++)
		{
			FVector2f TestValue = Accessor::GetElementValue(Context, Index / 2, Index % 2);
			TestTrue(TEXT("StaticConversionAccessor: Get/Set Element Value"), TestValue.Equals(TestData[Index]));
		}
	}

	{
		FRealtimeMeshStream FHalfVectorStreamWide = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 4>>(FRealtimeMeshStreams::Position);
		FHalfVectorStreamWide.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, void, true>;
		auto Context = Accessor::InitializeContext(FHalfVectorStreamWide, 1);

		for (int32 Index = 0; Index < 4; Index++)
		{
			Accessor::SetElementValue(Context, Index / 2, Index % 2, TestData[Index]);		
		}
	
		for (int32 Index = 0; Index < 4; Index++)
		{
			FVector2f TestValue = Accessor::GetElementValue(Context, Index / 2, Index % 2);
			TestTrue(TEXT("StaticConversionSubAccessor: Get/Set Element Value"), TestValue.Equals(TestData[Index]));
		}
	}

	{
		FRealtimeMeshStream FHalfVectorStream = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 2>>(FRealtimeMeshStreams::Position);
		FHalfVectorStream.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, void, false>;
		auto Context = Accessor::InitializeContext(FHalfVectorStream, 0);

		for (int32 Index = 0; Index < 4; Index += 2)
		{
			Accessor::SetBufferValue(Context, Index / 2, TRealtimeMeshTexCoords<FVector2f, 2>(TestData[Index], TestData[Index + 1]));		
		}
	
		for (int32 Index = 0; Index < 4; Index += 2)
		{
			TRealtimeMeshTexCoords<FVector2f, 2> TestValue = Accessor::GetBufferValue(Context, Index / 2);
			TestTrue(TEXT("StaticConversionAccessor: Get/Set Buffer Value"), TestValue[0].Equals(TestData[Index]) && TestValue[1].Equals(TestData[Index + 1]));
		}
	}

	{
		FRealtimeMeshStream FHalfVectorStreamWide = FRealtimeMeshStream::Create<TRealtimeMeshTexCoords<FVector2DHalf, 4>>(FRealtimeMeshStreams::Position);
		FHalfVectorStreamWide.SetNumZeroed(2);
		using Accessor = TRealtimeMeshStreamDataAccessor<TRealtimeMeshTexCoords<FVector2f, 2>, void, true>;
		auto Context = Accessor::InitializeContext(FHalfVectorStreamWide, 1);

		for (int32 Index = 0; Index < 4; Index += 2)
		{
			Accessor::SetBufferValue(Context, Index / 2, TRealtimeMeshTexCoords<FVector2f, 2>(TestData[Index], TestData[Index + 1]));		
		}
	
		for (int32 Index = 0; Index < 4; Index += 2)
		{
			TRealtimeMeshTexCoords<FVector2f, 2> TestValue = Accessor::GetBufferValue(Context, Index / 2);
			TestTrue(TEXT("StaticConversionSubAccessor: Get/Set Buffer Value"), TestValue[0].Equals(TestData[Index]) && TestValue[1].Equals(TestData[Index + 1]));
		}
	}


	
	// Make the test pass by returning true, or fail by returning false.
	return true;
}
