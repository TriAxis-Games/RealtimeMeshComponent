// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

//#include "RealtimeMeshThreadingSubsystem.h"
#include "CoreMinimal.h"
#include "LatentActions.h"
#include "Engine/Engine.h"
#include "Async/Async.h"
#include "UObject/StrongObjectPtr.h"
#include "Runtime/Launch/Resources/Version.h"
#include "RenderingThread.h"

namespace RealtimeMesh
{
	enum class ERealtimeMeshThreadType : uint8
	{
		Unknown = 0x0,
		RenderThread = 0x1,
		GameThread = 0x2,
		AsyncThread = 0x4,
		Any = RenderThread | GameThread | AsyncThread
	};
	ENUM_CLASS_FLAGS(ERealtimeMeshThreadType);

	/**
	 * Checks if the current thread is one of the allowed thread types.
	 * 
	 * @param AllowedThreads Bitfield of allowed thread types to check against
	 * @return true if current thread matches any of the allowed types
	 * 
	 * @note THREAD SAFETY: This function is thread-safe and can be called from any thread.
	 * It uses UE's thread detection functions which are atomic operations.
	 */
	inline bool RealtimeMeshIsInAllowedThread(ERealtimeMeshThreadType AllowedThreads)
	{
		return
			(EnumHasAllFlags(AllowedThreads, ERealtimeMeshThreadType::RenderThread) && IsInRenderingThread()) ||
			(EnumHasAllFlags(AllowedThreads, ERealtimeMeshThreadType::GameThread) && IsInGameThread()) ||
			(EnumHasAllFlags(AllowedThreads, ERealtimeMeshThreadType::AsyncThread) && !IsInRenderingThread() && !IsInGameThread());
	}
	
	namespace FutureExtensionDetails
	{
		template<typename Type>
		struct TFutureDetect
		{
			using BaseType = Type;
			static constexpr bool IsFuture = false;
		};
	
		template<typename Type>
		struct TFutureDetect<TFuture<Type>>
		{
			using BaseType = Type;
			static constexpr bool IsFuture = true;
		};
		
		template<typename Func, typename ResultType>
		void SetPromiseValue(TPromise<ResultType>&& Promise, Func& Function)
		{
			using ContinuationResult = decltype(Function());
			using ReturnValue = typename TFutureDetect<ContinuationResult>::BaseType;

			if constexpr (TFutureDetect<ContinuationResult>::IsFuture)
			{
				auto Future = Function();
				Future.Then([Promise = MoveTemp(Promise)](TFuture<ReturnValue>&& Res) mutable
				{
					Promise.EmplaceValue(Res.Consume());
				});	
			}
			else
			{
				Promise.EmplaceValue(Function());			
			}
		}
		
		
		template<typename Func>
		void SetPromiseValue(TPromise<void>&& Promise, Func& Function)
		{
			using ContinuationResult = decltype(Function());
			using ReturnValue = typename TFutureDetect<ContinuationResult>::BaseType;

			if constexpr (TFutureDetect<ContinuationResult>::IsFuture)
			{
				auto Future = Function();
				Future.Then([Promise = MoveTemp(Promise)](TFuture<void>&&) mutable
				{
					Promise.EmplaceValue();
				});				
			}
			else
			{
				Function();
				Promise.EmplaceValue();			
			}
		}

		
		template<typename Func, typename ResultType, typename ParamType>
		void SetPromiseValue(TPromise<ResultType>&& Promise, Func& Function, TFuture<ParamType>&& Param)
		{
			using ContinuationResult = decltype(Function(MoveTemp(Param)));
			using ReturnValue = typename TFutureDetect<ContinuationResult>::BaseType;

			if constexpr (TFutureDetect<ContinuationResult>::IsFuture)
			{
				auto Future = Function(MoveTemp(Param));
				Future.Then([Promise = MoveTemp(Promise)](TFuture<ReturnValue>&& Res) mutable
				{
					Promise.EmplaceValue(Res.Consume());
				});				
			}
			else
			{
				Promise.EmplaceValue(Function(MoveTemp(Param)));			
			}
		}
		
		template<typename Func, typename ParamType>
		void SetPromiseValue(TPromise<void>&& Promise, Func& Function, TFuture<ParamType>&& Param)
		{
			using ContinuationResult = decltype(Function(MoveTemp(Param)));
			using ReturnValue = typename TFutureDetect<ContinuationResult>::BaseType;

			if constexpr (TFutureDetect<ContinuationResult>::IsFuture)
			{
				auto Future = Function(MoveTemp(Param));
				Future.Then([Promise = MoveTemp(Promise)](TFuture<ReturnValue>&&) mutable
				{
					Promise.EmplaceValue();
				});				
			}
			else
			{
				Function(MoveTemp(Param));
				Promise.EmplaceValue();			
			}
		}
		
	}

	
	/**
	 * Executes a callable on one of the specified allowed thread types.
	 * If already on an allowed thread, executes immediately. Otherwise, dispatches to an appropriate thread.
	 * 
	 * @param AllowedThreads Bitfield specifying which thread types are allowed for execution
	 * @param Callable Function/lambda to execute
	 * @return TFuture containing the result of the callable
	 * 
	 * @note THREAD SAFETY: This function is thread-safe and handles cross-thread dispatch.
	 * - If called from an allowed thread: Executes immediately on current thread
	 * - If called from disallowed thread: Safely dispatches via UE's thread system
	 * - Async dispatches use thread pool, Game/Render use UE's named thread queues
	 * - The returned TFuture is thread-safe and can be awaited from any thread
	 * 
	 * @warning The Callable must be thread-safe if executed on AsyncThread.
	 * Game and Render thread callables have single-threaded execution guarantees.
	 */
	template<typename CallableType>
	static auto DoOnAllowedThread(ERealtimeMeshThreadType AllowedThreads, CallableType Callable)
	{
		using ContinuationResult = decltype(Callable());
		using ReturnValue = typename FutureExtensionDetails::TFutureDetect<ContinuationResult>::BaseType;

		TPromise<ReturnValue> Promise;
		TFuture<ReturnValue> FutureResult = Promise.GetFuture();
		
		if (RealtimeMeshIsInAllowedThread(AllowedThreads))
		{
			// We're already in an allowed thread, so just execute in place
			auto Func = [Callable = MoveTemp(Callable), Promise = MoveTemp(Promise)]() mutable
			{
				FutureExtensionDetails::SetPromiseValue(MoveTemp(Promise), Callable);
			};
			Func();			
		}
		else if (EnumHasAllFlags(AllowedThreads, ERealtimeMeshThreadType::AsyncThread))
		{			
			// TODO: Make this work with the optional interface 
			//URealtimeMeshThreadingSubsystem* ThreadingSubsystem = URealtimeMeshThreadingSubsystem::Get();
			FQueuedThreadPool& ThreadPool = /*ThreadingSubsystem? ThreadingSubsystem->GetThreadPool() :*/ *GThreadPool;
			AsyncPool(ThreadPool, [Callable = MoveTemp(Callable), Promise = MoveTemp(Promise)]() mutable
			{
				FutureExtensionDetails::SetPromiseValue(MoveTemp(Promise), Callable);
			});
		}
		else if (EnumHasAllFlags(AllowedThreads, ERealtimeMeshThreadType::GameThread))
		{
			AsyncTask(ENamedThreads::GameThread, [Callable = MoveTemp(Callable), Promise = MoveTemp(Promise)]() mutable
			{
				FutureExtensionDetails::SetPromiseValue(MoveTemp(Promise), Callable);
			});
		}
		else
		{
			check(EnumHasAllFlags(AllowedThreads, ERealtimeMeshThreadType::RenderThread));
				
			ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_Update)([Callable = MoveTemp(Callable), Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
			{
				FutureExtensionDetails::SetPromiseValue(MoveTemp(Promise), Callable);
			});
		}

		return FutureResult;
	}
		
	template<typename CallableType>
	static auto DoOnGameThread(CallableType Callable)
	{
		return DoOnAllowedThread(ERealtimeMeshThreadType::GameThread, MoveTemp(Callable));
	}
	
	template<typename CallableType>
	static auto DoOnRenderTread(CallableType Callable)
	{
		return DoOnAllowedThread(ERealtimeMeshThreadType::RenderThread, MoveTemp(Callable));
	}
	
	template<typename CallableType>
	static auto DoOnAsyncThread(CallableType Callable)
	{
		return DoOnAllowedThread(ERealtimeMeshThreadType::AsyncThread, MoveTemp(Callable));
	}
	
	template<typename ParamType, typename Continuation>
	auto ContinueOnAllowedThread(TFuture<ParamType>&& Future, ERealtimeMeshThreadType AllowedThreads, Continuation Callback)
	{
		using ContinuationResult = decltype(Callback(MoveTemp(Future)));
		using ReturnValue = typename FutureExtensionDetails::TFutureDetect<ContinuationResult>::BaseType;

		TPromise<ReturnValue> Promise;
		TFuture<ReturnValue> FutureResult = Promise.GetFuture();
		Future.Then([Callback = MoveTemp(Callback), Promise = MoveTemp(Promise), AllowedThreads](TFuture<ParamType>&& Result) mutable
		{
			DoOnAllowedThread(AllowedThreads, [Callback = MoveTemp(Callback), Result = MoveTemp(Result), Promise = MoveTemp(Promise)]() mutable
			{
				FutureExtensionDetails::SetPromiseValue(MoveTemp(Promise), Callback, MoveTemp(Result));
			});
		});

		return FutureResult;
	}
	
	template<typename ParamType, typename Continuation>
	auto ContinueOnGameThread(TFuture<ParamType>&& Future, Continuation Callback)
	{
		return ContinueOnAllowedThread(MoveTemp(Future), ERealtimeMeshThreadType::GameThread, MoveTemp(Callback));
	}
	
	template<typename ParamType, typename Continuation>
	auto ContinueOnRenderThread(TFuture<ParamType>&& Future, Continuation Callback)
	{
		return ContinueOnAllowedThread(MoveTemp(Future), ERealtimeMeshThreadType::RenderThread, MoveTemp(Callback));
	}
	
	template<typename ParamType, typename Continuation>
	auto ContinueOnAsyncThread(TFuture<ParamType>&& Future, Continuation Callback)
	{
		return ContinueOnAllowedThread(MoveTemp(Future), ERealtimeMeshThreadType::AsyncThread, MoveTemp(Callback));
	}

	template<typename ParamType>
	inline void BindPromiseToFuture(TPromise<ParamType>&& Promise, TFuture<ParamType>&& Param)
	{
		Param.Then([Promise = MoveTemp(Promise)](TFuture<ParamType>&& Param) mutable
		{
			Promise.EmplaceValue(Param.Get());
		});
	}
	




	
	template<typename... Types>
	struct FFutureAggregationState
	{
	private:
		TTuple<Types...> Values;
		TPromise<TTuple<Types...>> Promise;
		int32 RemainingFutures;
		FCriticalSection Mutex;
	public:
		FFutureAggregationState()
			: RemainingFutures(sizeof...(Types)) { }

		template<int32 Index, typename Type>
		void EmplaceValue(Type&& Value)
		{
			Values.template Get<Index>() = MoveTemp(Value);

			FScopeLock Lock(&Mutex);
			if (--RemainingFutures == 0)
			{
				Promise.SetValue(MoveTemp(Values));
			}
		}

		TFuture<TTuple<Types...>> GetFuture()
		{
			return Promise.GetFuture();
		}
	};
	
	template <uint32 ArgCount, uint32 ArgToCompare>
	struct FFutureAggregationHelper
	{
		template <typename... Types>
		FORCEINLINE static void Bind(const TSharedRef<FFutureAggregationState<Types...>>& State, TTuple<TFuture<Types>...>& Futures)
		{
			using Type = typename TTupleElement<ArgToCompare, TTuple<Types...>>::Type;
			Futures.template Get<ArgToCompare>().Then([State](TFuture<Type>&& Result)
			{
			    State->EmplaceValue<ArgToCompare>(Result.Consume());
			});
			
			FFutureAggregationHelper<ArgCount, ArgToCompare + 1>::Bind(State, Futures);
		}
	};

	template <uint32 ArgCount>
	struct FFutureAggregationHelper<ArgCount, ArgCount>
	{
		template <typename... Types>
		FORCEINLINE static void Bind(const TSharedRef<FFutureAggregationState<Types...>>& State, TTuple<TFuture<Types>...>& Futures)
		{
		}
	};
	
	template<typename... Types>
	auto WaitForAll(TFuture<Types>&&... Futures)
	{		
		TSharedRef<FFutureAggregationState<Types...>> FutureState = MakeShared<FFutureAggregationState<Types...>>();
		
		TTuple<TFuture<Types>...> InputFutures(MoveTemp(Futures)...);
		FFutureAggregationHelper<sizeof...(Types), 0>::Bind(FutureState, InputFutures);

		return FutureState->GetFuture();
	}

	
    template<typename Type>
    TFuture<TArray<Type>> WaitForAll(TArray<TFuture<Type>>&& Futures)
    {
        if (Futures.Num() == 0)
        {
            return MakeFulfilledPromise<TArray<Type>>(TArray<Type>()).GetFuture();
        }

        struct FState
        {
            TArray<Type> Values;
            FThreadSafeCounter RemainingFutures;
            TPromise<TArray<Type>> Promise;
            bool bHasFiredPromise = false;

            ~FState()
            {
                    checkf(RemainingFutures.GetValue() == 0 && bHasFiredPromise, TEXT("Not all futures have been resolved somehow."));
            }
        };
		
		TSharedRef<FState> State = MakeShared<FState>();
		State->RemainingFutures.Set(Futures.Num());
		State->Values.SetNum(Futures.Num());

		for (int32 Index = 0; Index < Futures.Num(); Index++)
		{
			Futures[Index].Then([State, Index](TFuture<Type>&& Result) mutable
			{
				State->Values[Index] = Result.Get();
				if (State->RemainingFutures.Decrement() == 0)
				{
					State->Promise.EmplaceValue(MoveTemp(State->Values));
					State->bHasFiredPromise = true;
				}
			});
		}

		return State->Promise.GetFuture();
	}
	
	class FRealtimeMeshCancellationToken
	{
	public:
		/** Default constructor. */
		FRealtimeMeshCancellationToken()
			: Cancelled(MakeShared<TAtomic<bool>>(false))
		{ }

		/**
		* Checks whether the asynchronous result is available.
		*/
		bool IsCancelled() const
		{
			return Cancelled->Load();
		}

		/**
		 * Flags the asynchronous operation as cancelled.
		 */
		void Cancel()
		{
			return Cancelled->Store(true);
		}

	private:
		TSharedRef<TAtomic<bool>> Cancelled;
	};



	template<typename ParamType>
	class FRealtimeMeshFutureLatentAction : public FPendingLatentAction
	{
	private:	
		FName ExecutionFunction;
		int32 OutputLink;
		FWeakObjectPtr CallbackTarget;
		
		TFuture<ParamType> Future;
		TFunction<void(TFuture<ParamType>&&)> OutputFinalizer;
	public:
		FRealtimeMeshFutureLatentAction(const FLatentActionInfo& LatentInfo, TFuture<ParamType>&& InFuture, TFunction<void(TFuture<ParamType>&&)>&& InOutputFinalizer)
			: ExecutionFunction(LatentInfo.ExecutionFunction)
			, OutputLink(LatentInfo.Linkage)
			, CallbackTarget(LatentInfo.CallbackTarget)
			, Future(MoveTemp(InFuture))
			, OutputFinalizer(MoveTemp(InOutputFinalizer))
		{
		}
		virtual void UpdateOperation(FLatentResponse& Response) override
		{
			if (Future.IsReady() || !Future.IsValid())
			{
				OutputFinalizer(MoveTemp(Future));
				Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
			}
		}
	#if WITH_EDITOR
		// Returns a human readable description of the latent operation's current state
		virtual FString GetDescription() const override
		{
			return NSLOCTEXT("RealtimeMeshFutureLatentAction", "RealtimeMeshFutureLatentAction", "Waiting on future...").ToString();
		}
	#endif
	};

	template<typename ParamType>
	void SetupSimpleLatentAction(UObject* WorldContextObject, FLatentActionInfo LatentInfo, TFunctionRef<TFuture<ParamType>()> Initializer, TFunction<void(TFuture<ParamType>&&)>&& Callback)
	{	
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			FLatentActionManager& LatentManager = World->GetLatentActionManager();
			if (LatentManager.FindExistingAction<FRealtimeMeshFutureLatentAction<ParamType>>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
			{
				TFuture<ParamType> Future = Initializer();
				
				FRealtimeMeshFutureLatentAction<ParamType>* NewAction = new FRealtimeMeshFutureLatentAction<ParamType>(LatentInfo, MoveTemp(Future), MoveTemp(Callback));
				LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction);
			}
		}
	}


	/**
	 * Create a SharedPointer to a TStrongObjectPtr, so you can pass this across
	 * threads and assure a UObject stays alive at least as long as this pointer.
	 * 
	 * @param InObject UObject to keep alive across threads
	 * @return Thread-safe shared pointer that maintains object lifetime
	 * 
	 * @note THREAD SAFETY: This function provides safe cross-thread UObject access.
	 * - The returned TSharedRef can be safely copied/passed between threads
	 * - The TStrongObjectPtr prevents GC collection while any thread holds a reference
	 * - Destruction is automatically dispatched to the Game Thread (required for UObjects)
	 * - Multiple threads can safely access the shared pointer simultaneously
	 * 
	 * @warning The UObject itself may not be thread-safe. Always check UObject documentation
	 * and use appropriate synchronization when calling UObject methods from non-game threads.
	 */
	template<typename ObjectType>
	TSharedRef<TStrongObjectPtr<ObjectType>> MakeSharedObjectPtr(ObjectType* InObject)
	{
		return MakeShareable(new TStrongObjectPtr<ObjectType>(InObject), [](TStrongObjectPtr<ObjectType>* Ptr)
		{
			DoOnGameThread([Ptr]() mutable
			{
				check(IsInGameThread());
						
				delete Ptr;
			});
		});
	}





}








