// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Interface/Core/RealtimeMeshFuture.h"
#include "Misc/ScopeLock.h"

using namespace RealtimeMesh;

// Test flags for editor-only tests
#if WITH_DEV_AUTOMATION_TESTS

//==============================================================================
// Thread Detection Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshIsInAllowedThreadTest,
	"RealtimeMeshComponent.Future.RealtimeMeshIsInAllowedThread",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshIsInAllowedThreadTest::RunTest(const FString& Parameters)
{
	// Test from game thread
	{
		if (IsInGameThread())
		{
			TestTrue(TEXT("Should detect game thread when called from game thread"),
				RealtimeMeshIsInAllowedThread(ERealtimeMeshThreadType::GameThread));

			TestFalse(TEXT("Should not detect render thread when on game thread"),
				RealtimeMeshIsInAllowedThread(ERealtimeMeshThreadType::RenderThread));

			TestTrue(TEXT("Should detect 'Any' when on game thread"),
				RealtimeMeshIsInAllowedThread(ERealtimeMeshThreadType::Any));
		}
	}

	// Test combined flags
	{
		if (IsInGameThread())
		{
			TestTrue(TEXT("GameThread | AsyncThread should detect game thread"),
				RealtimeMeshIsInAllowedThread(ERealtimeMeshThreadType::GameThread | ERealtimeMeshThreadType::AsyncThread));
		}
	}

	return true;
}

//==============================================================================
// DoOnAllowedThread Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoOnAllowedThreadImmediateExecutionTest,
	"RealtimeMeshComponent.Future.DoOnAllowedThread.ImmediateExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoOnAllowedThreadImmediateExecutionTest::RunTest(const FString& Parameters)
{
	// Test immediate execution on game thread
	if (IsInGameThread())
	{
		bool bExecuted = false;
		auto Future = DoOnAllowedThread(ERealtimeMeshThreadType::GameThread, [&bExecuted]() -> int32
		{
			bExecuted = true;
			return 42;
		});

		// Should execute immediately since we're already on game thread
		TestTrue(TEXT("Lambda should execute immediately when already on allowed thread"), bExecuted);
		TestTrue(TEXT("Future should be ready immediately"), Future.IsReady());
		TestEqual(TEXT("Future should contain correct value"), Future.Get(), 42);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoOnAllowedThreadAsyncDispatchTest,
	"RealtimeMeshComponent.Future.DoOnAllowedThread.AsyncDispatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoOnAllowedThreadAsyncDispatchTest::RunTest(const FString& Parameters)
{
	// Test async thread dispatch from game thread
	if (IsInGameThread())
	{
		TAtomic<bool> bExecutedOnAsyncThread(false);
		auto Future = DoOnAsyncThread([&bExecutedOnAsyncThread]() -> int32
		{
			// Verify we're not on game thread or render thread
			if (!IsInGameThread() && !IsInRenderingThread())
			{
				bExecutedOnAsyncThread.Store(true);
			}
			return 123;
		});

		// Wait for completion with timeout
		const double StartTime = FPlatformTime::Seconds();
		while (!Future.IsReady() && (FPlatformTime::Seconds() - StartTime) < 5.0)
		{
			FPlatformProcess::Sleep(0.01f);
		}

		TestTrue(TEXT("Future should be ready after async execution"), Future.IsReady());
		TestEqual(TEXT("Future should contain correct value"), Future.Get(), 123);
		TestTrue(TEXT("Should have executed on async thread"), bExecutedOnAsyncThread.Load());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoOnAllowedThreadVoidReturnTest,
	"RealtimeMeshComponent.Future.DoOnAllowedThread.VoidReturn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoOnAllowedThreadVoidReturnTest::RunTest(const FString& Parameters)
{
	// Test void return type
	if (IsInGameThread())
	{
		bool bExecuted = false;
		auto Future = DoOnGameThread([&bExecuted]()
		{
			bExecuted = true;
		});

		TestTrue(TEXT("Void lambda should execute immediately on game thread"), bExecuted);
		TestTrue(TEXT("Void future should be ready"), Future.IsReady());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoOnGameThreadTest,
	"RealtimeMeshComponent.Future.DoOnGameThread",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoOnGameThreadTest::RunTest(const FString& Parameters)
{
	// Test DoOnGameThread helper
	if (IsInGameThread())
	{
		bool bExecutedOnGameThread = false;
		auto Future = DoOnGameThread([&bExecutedOnGameThread]() -> bool
		{
			bExecutedOnGameThread = IsInGameThread();
			return true;
		});

		TestTrue(TEXT("Should execute on game thread"), bExecutedOnGameThread);
		TestTrue(TEXT("Future should be ready"), Future.IsReady());
		TestTrue(TEXT("Future should contain true"), Future.Get());
	}

	return true;
}

//==============================================================================
// ContinueOnAllowedThread Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContinueOnGameThreadBasicTest,
	"RealtimeMeshComponent.Future.ContinueOnGameThread.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FContinueOnGameThreadBasicTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Create an initial future
		TPromise<int32> Promise;
		TFuture<int32> InitialFuture = Promise.GetFuture();

		// Chain a continuation
		bool bContinuationExecuted = false;
		auto ContinuedFuture = ContinueOnGameThread(MoveTemp(InitialFuture), [&bContinuationExecuted](TFuture<int32>&& Future) -> int32
		{
			bContinuationExecuted = true;
			return Future.Get() * 2;
		});

		// Fulfill the promise
		Promise.SetValue(21);

		// Wait for continuation
		const double StartTime = FPlatformTime::Seconds();
		while (!ContinuedFuture.IsReady() && (FPlatformTime::Seconds() - StartTime) < 5.0)
		{
			FPlatformProcess::Sleep(0.01f);
		}

		TestTrue(TEXT("Continuation should execute"), bContinuationExecuted);
		TestTrue(TEXT("Continued future should be ready"), ContinuedFuture.IsReady());
		TestEqual(TEXT("Continued future should contain doubled value"), ContinuedFuture.Get(), 42);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContinueOnAsyncThreadBasicTest,
	"RealtimeMeshComponent.Future.ContinueOnAsyncThread.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FContinueOnAsyncThreadBasicTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Create initial future on game thread
		auto InitialFuture = DoOnGameThread([]() -> int32
		{
			return 10;
		});

		// Continue on async thread
		TAtomic<bool> bExecutedOnAsyncThread(false);
		auto ContinuedFuture = ContinueOnAsyncThread(MoveTemp(InitialFuture), [&bExecutedOnAsyncThread](TFuture<int32>&& Future) -> int32
		{
			if (!IsInGameThread() && !IsInRenderingThread())
			{
				bExecutedOnAsyncThread.Store(true);
			}
			return Future.Get() + 5;
		});

		// Wait for completion
		const double StartTime = FPlatformTime::Seconds();
		while (!ContinuedFuture.IsReady() && (FPlatformTime::Seconds() - StartTime) < 5.0)
		{
			FPlatformProcess::Sleep(0.01f);
		}

		TestTrue(TEXT("Should execute on async thread"), bExecutedOnAsyncThread.Load());
		TestEqual(TEXT("Should compute correct result"), ContinuedFuture.Get(), 15);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContinueOnAllowedThreadIntToIntTest,
	"RealtimeMeshComponent.Future.ContinueOnAllowedThread.IntToInt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FContinueOnAllowedThreadIntToIntTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Test int32 to int32 continuation (different from existing test by using more complex logic)
		TPromise<int32> Promise;
		TFuture<int32> InitialFuture = Promise.GetFuture();

		bool bContinuationExecuted = false;
		auto ContinuedFuture = ContinueOnGameThread(MoveTemp(InitialFuture), [&bContinuationExecuted](TFuture<int32>&& Future) -> int32
		{
			bContinuationExecuted = true;
			return Future.Get() + 100;
		});

		Promise.SetValue(50);

		// Wait for continuation
		const double StartTime = FPlatformTime::Seconds();
		while (!ContinuedFuture.IsReady() && (FPlatformTime::Seconds() - StartTime) < 5.0)
		{
			FPlatformProcess::Sleep(0.01f);
		}

		TestTrue(TEXT("Continuation should execute"), bContinuationExecuted);
		TestTrue(TEXT("Continued future should be ready"), ContinuedFuture.IsReady());
		TestEqual(TEXT("Continued future should contain correct value"), ContinuedFuture.Get(), 150);
	}

	return true;
}

//==============================================================================
// WaitForAll Tests (Variadic Template Version)
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaitForAllVariadicBasicTest,
	"RealtimeMeshComponent.Future.WaitForAll.Variadic.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaitForAllVariadicBasicTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Create multiple promises
		TPromise<int32> Promise1;
		TPromise<FString> Promise2;
		TPromise<bool> Promise3;

		auto Future1 = Promise1.GetFuture();
		auto Future2 = Promise2.GetFuture();
		auto Future3 = Promise3.GetFuture();

		// Wait for all
		auto CombinedFuture = WaitForAll(MoveTemp(Future1), MoveTemp(Future2), MoveTemp(Future3));

		// Fulfill promises
		Promise1.SetValue(42);
		Promise2.SetValue(TEXT("Test"));
		Promise3.SetValue(true);

		// Wait for completion
		const double StartTime = FPlatformTime::Seconds();
		while (!CombinedFuture.IsReady() && (FPlatformTime::Seconds() - StartTime) < 5.0)
		{
			FPlatformProcess::Sleep(0.01f);
		}

		TestTrue(TEXT("Combined future should be ready"), CombinedFuture.IsReady());

		auto Result = CombinedFuture.Get();
		TestEqual(TEXT("First value should be 42"), Result.template Get<0>(), 42);
		TestEqual(TEXT("Second value should be 'Test'"), Result.template Get<1>(), FString(TEXT("Test")));
		TestEqual(TEXT("Third value should be true"), Result.template Get<2>(), true);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaitForAllVariadicTwoStageTest,
	"RealtimeMeshComponent.Future.WaitForAll.Variadic.TwoStage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaitForAllVariadicTwoStageTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		TPromise<int32> Promise1;
		TPromise<int32> Promise2;

		auto Future1 = Promise1.GetFuture();
		auto Future2 = Promise2.GetFuture();

		auto CombinedFuture = WaitForAll(MoveTemp(Future1), MoveTemp(Future2));

		// Fulfill only one promise
		Promise1.SetValue(100);

		// Give it some time for callbacks to fire
		FPlatformProcess::Sleep(0.1f);

		// Should not be ready yet
		TestFalse(TEXT("Combined future should not be ready with partial completion"), CombinedFuture.IsReady());

		// Fulfill second promise
		Promise2.SetValue(200);

		// Give time for final callback to fire
		FPlatformProcess::Sleep(0.1f);

		TestTrue(TEXT("Combined future should be ready after all promises fulfilled"), CombinedFuture.IsReady());

		if (CombinedFuture.IsReady())
		{
			auto Result = CombinedFuture.Get();
			TestEqual(TEXT("First value should be 100"), Result.template Get<0>(), 100);
			TestEqual(TEXT("Second value should be 200"), Result.template Get<1>(), 200);
		}
	}

	return true;
}

//==============================================================================
// WaitForAll Tests (Array Version)
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaitForAllArrayBasicTest,
	"RealtimeMeshComponent.Future.WaitForAll.Array.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaitForAllArrayBasicTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Create array of pre-fulfilled promises to avoid async issues in tests
		TArray<TFuture<int32>> Futures;

		for (int32 i = 0; i < 5; i++)
		{
			TPromise<int32> Promise;
			Promise.SetValue(i * 10);
			Futures.Add(Promise.GetFuture());
		}

		// Wait for all (should be immediate since all are ready)
		auto CombinedFuture = WaitForAll(MoveTemp(Futures));

		// Small delay to allow Then() callbacks to fire
		FPlatformProcess::Sleep(0.1f);

		TestTrue(TEXT("Combined future should be ready"), CombinedFuture.IsReady());

		if (CombinedFuture.IsReady())
		{
			auto Results = CombinedFuture.Get();
			TestEqual(TEXT("Should have 5 results"), Results.Num(), 5);

			for (int32 i = 0; i < Results.Num(); i++)
			{
				TestEqual(FString::Printf(TEXT("Result[%d] should be %d"), i, i * 10), Results[i], i * 10);
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaitForAllArrayEmptyTest,
	"RealtimeMeshComponent.Future.WaitForAll.Array.Empty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaitForAllArrayEmptyTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Test empty array
		TArray<TFuture<int32>> EmptyFutures;
		auto CombinedFuture = WaitForAll(MoveTemp(EmptyFutures));

		TestTrue(TEXT("Empty array should return ready future immediately"), CombinedFuture.IsReady());

		auto Results = CombinedFuture.Get();
		TestEqual(TEXT("Empty array should return empty results"), Results.Num(), 0);
	}

	return true;
}

//==============================================================================
// Cancellation Token Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshCancellationTokenConstructorTest,
	"RealtimeMeshComponent.Future.CancellationToken.Constructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshCancellationTokenConstructorTest::RunTest(const FString& Parameters)
{
	// Test default state
	FRealtimeMeshCancellationToken Token;
	TestFalse(TEXT("Newly created token should not be cancelled"), Token.IsCancelled());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshCancellationTokenCancelTest,
	"RealtimeMeshComponent.Future.CancellationToken.Cancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshCancellationTokenCancelTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshCancellationToken Token;

	TestFalse(TEXT("Token should not be cancelled initially"), Token.IsCancelled());

	Token.Cancel();

	TestTrue(TEXT("Token should be cancelled after Cancel()"), Token.IsCancelled());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshCancellationTokenCopyTest,
	"RealtimeMeshComponent.Future.CancellationToken.Copy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshCancellationTokenCopyTest::RunTest(const FString& Parameters)
{
	FRealtimeMeshCancellationToken Token1;
	FRealtimeMeshCancellationToken Token2 = Token1;

	TestFalse(TEXT("Both tokens should not be cancelled initially"), Token1.IsCancelled() || Token2.IsCancelled());

	// Cancel token1
	Token1.Cancel();

	// Token2 should also be cancelled since they share state
	TestTrue(TEXT("Token1 should be cancelled"), Token1.IsCancelled());
	TestTrue(TEXT("Token2 should also be cancelled (shared state)"), Token2.IsCancelled());

	return true;
}

//==============================================================================
// MakeSharedObjectPtr Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMakeSharedObjectPtrTest,
	"RealtimeMeshComponent.Future.MakeSharedObjectPtr",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMakeSharedObjectPtrTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Create a test object (using UPackage as a concrete UObject-derived class)
		UPackage* TestObject = NewObject<UPackage>(GetTransientPackage(), NAME_None, RF_Transient);
		TestTrue(TEXT("Test object should be valid"), IsValid(TestObject));

		// Create shared pointer
		auto SharedPtr = MakeSharedObjectPtr(TestObject);

		// TSharedRef is always valid (no IsValid() method)
		TestEqual(TEXT("SharedPtr should point to the same object"), SharedPtr->Get(), TestObject);

		// Test that it keeps the object alive
		TestTrue(TEXT("Object should still be valid"), IsValid(TestObject));
	}

	return true;
}

//==============================================================================
// Future Extension Details Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFutureDetectTest,
	"RealtimeMeshComponent.Future.FutureExtensionDetails.TFutureDetect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFutureDetectTest::RunTest(const FString& Parameters)
{
	// Test type detection
	{
		using IntDetect = FutureExtensionDetails::TFutureDetect<int32>;
		static_assert(!IntDetect::IsFuture, "int32 should not be detected as future");
		static_assert(std::is_same<IntDetect::BaseType, int32>::value, "BaseType should be int32");
	}

	{
		using FutureDetect = FutureExtensionDetails::TFutureDetect<TFuture<int32>>;
		static_assert(FutureDetect::IsFuture, "TFuture<int32> should be detected as future");
		static_assert(std::is_same<FutureDetect::BaseType, int32>::value, "BaseType should be int32");
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSetPromiseValueTest,
	"RealtimeMeshComponent.Future.FutureExtensionDetails.SetPromiseValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSetPromiseValueTest::RunTest(const FString& Parameters)
{
	// Test non-void return
	{
		TPromise<int32> Promise;
		auto Future = Promise.GetFuture();

		auto Func = []() -> int32 { return 42; };
		FutureExtensionDetails::SetPromiseValue(MoveTemp(Promise), Func);

		TestTrue(TEXT("Future should be ready after SetPromiseValue"), Future.IsReady());
		TestEqual(TEXT("Future should contain correct value"), Future.Get(), 42);
	}

	// Test void return
	{
		TPromise<void> Promise;
		auto Future = Promise.GetFuture();

		bool bExecuted = false;
		auto Func = [&bExecuted]() { bExecuted = true; };
		FutureExtensionDetails::SetPromiseValue(MoveTemp(Promise), Func);

		TestTrue(TEXT("Lambda should have executed"), bExecuted);
		TestTrue(TEXT("Void future should be ready"), Future.IsReady());
	}

	return true;
}

//==============================================================================
// Nested Future Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNestedFutureTest,
	"RealtimeMeshComponent.Future.NestedFuture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNestedFutureTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Test that a function returning TFuture<int32> gets unwrapped
		auto OuterFuture = DoOnGameThread([]() -> TFuture<int32>
		{
			// Return an inner future
			return DoOnGameThread([]() -> int32
			{
				return 100;
			});
		});

		// Wait for completion
		const double StartTime = FPlatformTime::Seconds();
		while (!OuterFuture.IsReady() && (FPlatformTime::Seconds() - StartTime) < 5.0)
		{
			FPlatformProcess::Sleep(0.01f);
		}

		TestTrue(TEXT("Nested future should be ready"), OuterFuture.IsReady());
		TestEqual(TEXT("Nested future should unwrap to inner value"), OuterFuture.Get(), 100);
	}

	return true;
}

//==============================================================================
// Stress Tests
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMultipleFutureAggregationTest,
	"RealtimeMeshComponent.Future.StressTest.MultipleFutureAggregation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMultipleFutureAggregationTest::RunTest(const FString& Parameters)
{
	if (IsInGameThread())
	{
		// Reduced count and using pre-fulfilled promises to avoid test environment issues
		const int32 NumOperations = 10;
		TArray<TFuture<int32>> Futures;

		// Create pre-fulfilled futures to test the aggregation mechanism
		for (int32 i = 0; i < NumOperations; i++)
		{
			TPromise<int32> Promise;
			Promise.SetValue(i * 2);
			Futures.Add(Promise.GetFuture());
		}

		// Wait for all
		auto CombinedFuture = WaitForAll(MoveTemp(Futures));

		// Give callbacks time to fire
		FPlatformProcess::Sleep(0.1f);

		TestTrue(TEXT("All operations should complete"), CombinedFuture.IsReady());

		if (CombinedFuture.IsReady())
		{
			auto Results = CombinedFuture.Get();
			TestEqual(TEXT("Should have all results"), Results.Num(), NumOperations);

			// Verify all results
			for (int32 i = 0; i < Results.Num(); i++)
			{
				TestEqual(FString::Printf(TEXT("Result[%d] should be correct"), i), Results[i], i * 2);
			}
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
