// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

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
	 * Returns true if the current thread matches any of the allowed thread types.
	 * Safe to call from any thread.
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
	 * Runs Callable on one of the allowed thread types and returns a TFuture for
	 * its result. Executes inline if already on an allowed thread; otherwise
	 * dispatches (async -> thread pool, game/render -> UE's named-thread queues).
	 * Callable must be thread-safe when AsyncThread is among the allowed types.
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
			auto Func = [Callable = MoveTemp(Callable), Promise = MoveTemp(Promise)]() mutable
			{
				FutureExtensionDetails::SetPromiseValue(MoveTemp(Promise), Callable);
			};
			Func();			
		}
		else if (EnumHasAllFlags(AllowedThreads, ERealtimeMeshThreadType::AsyncThread))
		{			
			FQueuedThreadPool& ThreadPool = *GThreadPool;
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
	static auto DoOnRenderThread(CallableType Callable)
	{
		return DoOnAllowedThread(ERealtimeMeshThreadType::RenderThread, MoveTemp(Callable));
	}

	// Deprecated misspelled alias. Retained so external callers of the old name
	// keep compiling; forwards to DoOnRenderThread.
	template<typename CallableType>
	UE_DEPRECATED(5.7, "DoOnRenderTread was misspelled; use DoOnRenderThread instead.")
	static auto DoOnRenderTread(CallableType Callable)
	{
		return DoOnRenderThread(MoveTemp(Callable));
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
			    State->template EmplaceValue<ArgToCompare>(Result.Consume());
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
				// Single-consumer: this continuation fires exactly once per future,
				// so move the result out rather than copying it.
				State->Values[Index] = Result.Consume();
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
		FRealtimeMeshCancellationToken()
			: Cancelled(MakeShared<TAtomic<bool>>(false))
		{ }

		/** Returns true once Cancel() has been called. */
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
	 * Wraps a UObject in a TStrongObjectPtr behind a TSharedRef so it can be
	 * passed across threads while staying alive (the strong ptr blocks GC).
	 * Destruction is dispatched to the game thread, as UObject teardown requires.
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








