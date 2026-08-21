// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCoreFwd.h"
#include "RealtimeMeshDataStream.h"
#include "Containers/ArrayView.h"

namespace RealtimeMesh
{
	/**
	 * Compile-time-typed dispatch over a runtime stream layout. Hands the
	 * lambda a TArrayView typed to whichever element type matches the stream's
	 * layout; the lambda is monomorphized per type so its inner loop is direct
	 * memcpy / native arithmetic with no per-row dispatch.
	 *
	 * Use this instead of writing a chain of `if (Stream.GetLayout() == ...)
	 * else if ...` branches across stream-consuming utilities. The compiler
	 * generates one specialization of the lambda body per type in the list.
	 *
	 * Visit returns true if a specialization fired, false if the stream's
	 * layout didn't match any of the supplied types. Callers that need a slow
	 * fallback can check the return value and route to a per-row converting
	 * path.
	 *
	 * Example:
	 *
	 *   FBox3f Box(ForceInit);
	 *   TLayoutDispatch<FVector3f, FVector3d>::Visit(*Position,
	 *       [&Box](auto View)
	 *       {
	 *           using T = typename decltype(View)::ElementType;
	 *           for (int32 i = 0; i < View.Num(); ++i)
	 *           {
	 *               Box += FVector3f(View[i]);
	 *           }
	 *       });
	 */
	template <typename... Ts>
	struct TLayoutDispatch
	{
		// Const variant — lambda receives TArrayView<const T> for one of the Ts.
		template <typename FnType>
		static bool Visit(const FRealtimeMeshStream& Stream, FnType&& Fn)
		{
			return VisitImpl<FnType, Ts...>(Stream, Forward<FnType>(Fn));
		}

		// Mutable variant — lambda receives TArrayView<T> for one of the Ts.
		template <typename FnType>
		static bool VisitMutable(FRealtimeMeshStream& Stream, FnType&& Fn)
		{
			return VisitImplMutable<FnType, Ts...>(Stream, Forward<FnType>(Fn));
		}

	private:
		template <typename FnType, typename First, typename... Rest>
		static bool VisitImpl(const FRealtimeMeshStream& Stream, FnType&& Fn)
		{
			if (Stream.GetLayout() == GetRealtimeMeshBufferLayout<First>())
			{
				const auto View = MakeArrayView(
					reinterpret_cast<const First*>(Stream.GetData()),
					Stream.Num());
				Fn(View);
				return true;
			}
			if constexpr (sizeof...(Rest) > 0)
			{
				return VisitImpl<FnType, Rest...>(Stream, Forward<FnType>(Fn));
			}
			else
			{
				return false;
			}
		}

		template <typename FnType, typename First, typename... Rest>
		static bool VisitImplMutable(FRealtimeMeshStream& Stream, FnType&& Fn)
		{
			if (Stream.GetLayout() == GetRealtimeMeshBufferLayout<First>())
			{
				const auto View = MakeArrayView(
					reinterpret_cast<First*>(Stream.GetData()),
					Stream.Num());
				Fn(View);
				return true;
			}
			if constexpr (sizeof...(Rest) > 0)
			{
				return VisitImplMutable<FnType, Rest...>(Stream, Forward<FnType>(Fn));
			}
			else
			{
				return false;
			}
		}
	};

	/**
	 * Sibling of TLayoutDispatch that matches on element type only, ignoring
	 * the number of elements per row. The lambda receives a flat TArrayView<T>
	 * over all elements in the stream (Num() rows × NumElements columns).
	 *
	 * Useful for "I have an integer-index stream and don't care whether it's
	 * stored as scalar uint16 or as TIndex3<uint16>" — the flat view exposes
	 * all the integers regardless. Compare to TLayoutDispatch<TIndex3<uint16>>
	 * which only fires for the multi-element shape.
	 */
	template <typename... Ts>
	struct TElementDispatch
	{
		template <typename FnType>
		static bool Visit(const FRealtimeMeshStream& Stream, FnType&& Fn)
		{
			return VisitImpl<FnType, Ts...>(Stream, Forward<FnType>(Fn));
		}

		template <typename FnType>
		static bool VisitMutable(FRealtimeMeshStream& Stream, FnType&& Fn)
		{
			return VisitImplMutable<FnType, Ts...>(Stream, Forward<FnType>(Fn));
		}

	private:
		template <typename FnType, typename First, typename... Rest>
		static bool VisitImpl(const FRealtimeMeshStream& Stream, FnType&& Fn)
		{
			if (Stream.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<First>())
			{
				const int32 TotalElems = Stream.Num() * Stream.GetLayout().GetNumElements();
				const auto View = MakeArrayView(reinterpret_cast<const First*>(Stream.GetData()), TotalElems);
				Fn(View);
				return true;
			}
			if constexpr (sizeof...(Rest) > 0)
			{
				return VisitImpl<FnType, Rest...>(Stream, Forward<FnType>(Fn));
			}
			else
			{
				return false;
			}
		}

		template <typename FnType, typename First, typename... Rest>
		static bool VisitImplMutable(FRealtimeMeshStream& Stream, FnType&& Fn)
		{
			if (Stream.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<First>())
			{
				const int32 TotalElems = Stream.Num() * Stream.GetLayout().GetNumElements();
				const auto View = MakeArrayView(reinterpret_cast<First*>(Stream.GetData()), TotalElems);
				Fn(View);
				return true;
			}
			if constexpr (sizeof...(Rest) > 0)
			{
				return VisitImplMutable<FnType, Rest...>(Stream, Forward<FnType>(Fn));
			}
			else
			{
				return false;
			}
		}
	};
}
