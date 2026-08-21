// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.
// RealtimeMeshSimpleContract_v1.h
// ============================================================================
// PORTABLE CONTRACT HEADER — RealtimeMesh Simple, v1 (soft plugin coupling).
//
// This is RealtimeMeshComponent's soft-bind surface for basic runtime mesh
// creation. A CONSUMER plugin copies THIS FILE VERBATIM into its own module and
// uses only what is in it — it never links RealtimeMeshComponent and never lists
// it as a .uplugin dependency. RMC (the PROVIDER) registers an implementation of
// the Section B interface under the versioned modular-feature name; when RMC is
// absent the consumer's TModularFeatureConsumer guard is simply falsy and the
// consumer degrades (e.g. falls back to its own mesh path).
//
// TWO SECTIONS with different change rules:
//   Section A — shared consumer utility (NOT ABI). Engine-only inline template
//               code (TriAxis::TModularFeatureConsumer). Guarded by a classic
//               include-guard macro so a consumer that copies several TriAxis
//               contract headers compiles it once.
//   Section B — THE CONTRACT (ABI, FROZEN FOREVER ONCE SHIPPED). Pure-abstract
//               interface + frozen POD structs + append-only enums + versioned
//               feature name. Never reorder/insert/remove/append virtuals after
//               ship; any change means a new _v2 header.
//
// The header includes ONLY engine headers, so it compiles standalone with no RMC
// dependency (proven by the standalone-compile TU in RealtimeMeshTests).
//
// While RMC's contract surface is unshipped it may be iterated freely (edit
// header + provider + every consumer copy together). It FREEZES at first
// marketplace ship; arm a frozen-hash CI check then.
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
#include "Features/IModularFeatures.h"
#include "HAL/CriticalSection.h"

class AActor;             // call-scoped borrow in CreateSimpleMeshComponent
class UMeshComponent;     // GC-managed result of CreateSimpleMeshComponent / call-scoped key elsewhere
class UMaterialInterface; // call-scoped borrow in SetMaterialSlot

// ---------------------------------------------------------------------------
// SECTION A — shared consumer utility (NOT ABI). TriAxis-namespaced variant of
// the modular-feature consumer guard. Improve only identically across all
// TriAxis contract headers, and bump the guard macro suffix if a change is not
// drop-in compatible.
// ---------------------------------------------------------------------------
#ifndef TRIAXIS_MODULAR_FEATURE_CONSUMER_UTIL_V1
#define TRIAXIS_MODULAR_FEATURE_CONSUMER_UTIL_V1

namespace TriAxis
{
	enum class EFeatureThreadPolicy : uint8
	{
		GameThreadOnly,
		MultiThreaded
	};

	namespace FeatureUtilPrivate
	{
		// No-op lock for the single-threaded policy: inlines to nothing in
		// shipping; asserts the game-thread invariant in debug builds.
		struct FSingleThreadLock
		{
			FORCEINLINE void ReadLock()    { checkSlow(IsInGameThread()); }
			FORCEINLINE void ReadUnlock()  {}
			FORCEINLINE void WriteLock()   { checkSlow(IsInGameThread()); }
			FORCEINLINE void WriteUnlock() {}
		};

		struct FMultiThreadLock
		{
			FORCEINLINE void ReadLock()    { Lock.ReadLock(); }
			FORCEINLINE void ReadUnlock()  { Lock.ReadUnlock(); }
			FORCEINLINE void WriteLock()   { Lock.WriteLock(); }
			FORCEINLINE void WriteUnlock() { Lock.WriteUnlock(); }
			FRWLock Lock;
		};

		template <EFeatureThreadPolicy Policy>
		using TLockFor = std::conditional_t<
			Policy == EFeatureThreadPolicy::MultiThreaded, FMultiThreadLock, FSingleThreadLock>;

		template <typename TLock>
		struct TWriteScope
		{
			explicit TWriteScope(TLock& InLock) : Lock(InLock) { Lock.WriteLock(); }
			~TWriteScope()                                     { Lock.WriteUnlock(); }
			TLock& Lock;
		};
	}

	// Cached, self-rebinding handle to a modular feature. One per (feature,
	// version) consumed. TFeatureInterface must expose static
	// GetModularFeatureName() and derive from IModularFeature (single
	// inheritance, which makes the casts below pointer-identity).
	template <typename TFeatureInterface,
	          EFeatureThreadPolicy Policy = EFeatureThreadPolicy::GameThreadOnly>
	class TModularFeatureConsumer
	{
		using FLock       = FeatureUtilPrivate::TLockFor<Policy>;
		using FWriteScope = FeatureUtilPrivate::TWriteScope<FLock>;

	public:
		// RAII call guard. Falsy => provider absent (no lock held in that case).
		// In MultiThreaded policy, the provider cannot tear down while a truthy
		// guard is alive.
		class FScopedCall
		{
		public:
			FScopedCall(FScopedCall&& Other)
				: Owner(Other.Owner), Feature(Other.Feature)
			{
				Other.Owner = nullptr; Other.Feature = nullptr;
			}
			~FScopedCall()
			{
				if (Owner) { Owner->Lock.ReadUnlock(); }
			}

			explicit operator bool() const        { return Feature != nullptr; }
			TFeatureInterface* operator->() const { check(Feature); return Feature; }
			TFeatureInterface* Get() const        { return Feature; }

			FScopedCall(const FScopedCall&) = delete;
			FScopedCall& operator=(const FScopedCall&) = delete;

		private:
			friend class TModularFeatureConsumer;
			FScopedCall(TModularFeatureConsumer* InOwner, TFeatureInterface* InFeature)
				: Owner(InOwner), Feature(InFeature) {}

			TModularFeatureConsumer* Owner;   // null => holds no lock (miss case)
			TFeatureInterface*       Feature; // null => provider absent
		};

		TModularFeatureConsumer()
			: Name(TFeatureInterface::GetModularFeatureName())
		{
			IModularFeatures& MF = IModularFeatures::Get();
			{
				FWriteScope W(Lock);
				if (MF.IsModularFeatureAvailable(Name))
				{
					Cached = &MF.GetModularFeature<TFeatureInterface>(Name);
				}
			}
			RegisteredHandle = MF.OnModularFeatureRegistered().AddRaw(
				this, &TModularFeatureConsumer::HandleRegistered);
			UnregisteredHandle = MF.OnModularFeatureUnregistered().AddRaw(
				this, &TModularFeatureConsumer::HandleUnregistered);
		}

		~TModularFeatureConsumer()
		{
			IModularFeatures& MF = IModularFeatures::Get();
			MF.OnModularFeatureRegistered().Remove(RegisteredHandle);
			MF.OnModularFeatureUnregistered().Remove(UnregisteredHandle);
			FWriteScope W(Lock);   // drain stragglers; no guard may outlive us
			Cached = nullptr;
		}

		// Bound delegates capture `this`; copying/moving would dangle.
		TModularFeatureConsumer(const TModularFeatureConsumer&) = delete;
		TModularFeatureConsumer& operator=(const TModularFeatureConsumer&) = delete;

		// Cheap: (uncontended) read-lock + pointer read; just the pointer read in
		// GameThreadOnly shipping builds.
		FScopedCall AcquireCall()
		{
			Lock.ReadLock();
			if (Cached == nullptr)
			{
				Lock.ReadUnlock();                 // miss: release before returning
				return FScopedCall(nullptr, nullptr);
			}
			return FScopedCall(this, Cached);      // read lock transfers to guard
		}

		// Unlocked peek — UI hints only; AcquireCall is the real gate.
		bool IsLikelyAvailable() const { return Cached != nullptr; }

	private:
		void HandleRegistered(const FName& Type, IModularFeature* Feature)
		{
			if (Type == Name)
			{
				// Use the DELIVERED pointer (never re-query the registry) and
				// first-write-wins; with null-on-unregister this makes hot reload
				// correct: old provider's unregister clears, new one's refills.
				FWriteScope W(Lock);
				if (Cached == nullptr)
				{
					Cached = static_cast<TFeatureInterface*>(Feature);
				}
			}
		}

		void HandleUnregistered(const FName& Type, IModularFeature* Feature)
		{
			if (Type == Name && Feature == static_cast<IModularFeature*>(Cached))
			{
				// MultiThreaded: THE DRAIN. UnregisterModularFeature broadcasts
				// synchronously, so this write-lock acquisition blocks the
				// provider inside its own unregister call until every in-flight
				// guard releases its read lock. GameThreadOnly: trivially instant.
				FWriteScope W(Lock);
				Cached = nullptr;
			}
		}

		FName              Name;
		FLock              Lock;               // no-op or FRWLock, per policy
		TFeatureInterface* Cached = nullptr;   // guarded by Lock
		FDelegateHandle    RegisteredHandle;
		FDelegateHandle    UnregisteredHandle;
	};
}

#endif // TRIAXIS_MODULAR_FEATURE_CONSUMER_UTIL_V1

// ---------------------------------------------------------------------------
// SECTION B — THE CONTRACT (ABI, FROZEN FOREVER ONCE SHIPPED).
//
// ABI rules honored: pure-abstract interfaces (no data members, no non-virtual
// functions except the static name + inline helpers); POD params, const TCHAR*,
// frozen structs with an explicit StructVersion; append-only enums with explicit
// values; no UCLASS/USTRUCT/UENUM, no STL, no exceptions, no default arguments
// in the vtable; opaque int32 group ids, never pointers into provider memory.
// Engine UObject types cross only as call-scoped borrows or GC-managed results
// (both plugins share the engine ABI); the provider validates every component
// pointer it is handed and returns failure for foreign/stale ones.
// All calls are GAME THREAD ONLY in v1. Stream data pointers passed to the
// provider are read synchronously during the call and copied out — they are
// valid only for the duration of the call.
// ---------------------------------------------------------------------------

namespace TriAxis::RealtimeMesh
{
	// Skew tripwire: the provider's GetRevision() must return exactly this value
	// or the consumer must disable the integration (log, don't call through).
	inline constexpr int32 SimpleContractRevision_V1 = 1;

	// Append-only, explicit values.
	enum class EMeshDrawType_V1 : int32
	{
		Static  = 0,   // infrequent updates, optimized for render performance
		Dynamic = 1,   // frequent updates, optimized for update performance
	};

	/**
	 * Scalar datum vocabulary for describing stream element formats — a portable
	 * mirror of the provider's data-type mapping. An ELEMENT is (DatumType x
	 * NumDatums), e.g. FVector3f = (Float x3), FPackedNormal = (Int8Float x4),
	 * FPackedRGBA16N = (Int16 x4, normalized), FVector2DHalf = (Half x2),
	 * FColor = (UInt8 x4). A stream ROW is NumElements consecutive elements.
	 * Append-only, explicit values.
	 */
	enum class EMeshDatumType_V1 : int32
	{
		Unknown   = 0,
		UInt8     = 1,
		Int8      = 2,
		UInt16    = 3,
		Int16     = 4,
		UInt32    = 5,
		Int32     = 6,
		Half      = 7,
		Float     = 8,
		Double    = 9,
		Int8Float = 10,  // normalized float packed into an int8 (FPackedNormal datums)
		RGB10A2   = 11,  // tightly packed 10:10:10:2
	};

	/**
	 * What a stream contains. The element FORMAT is caller-chosen via the datum
	 * mapping above; the provider accepts any format it can either use natively
	 * or convert (e.g. tangents as FPackedNormal or FPackedRGBA16N or plain
	 * FVector4f pairs; triangles as 16- or 32-bit indices) and rejects the rest
	 * by returning failure. Append-only, explicit values.
	 */
	enum class EMeshStream_V1 : int32
	{
		Position   = 0,  // 1 element/vertex, convertible to (Float x3). Required.
		Tangents   = 1,  // 2 elements/vertex, [Tangent, Normal] interleaved, packed-normal style
		TexCoords  = 2,  // 1..8 elements/vertex (UV channels), convertible to (Float x2)
		Color      = 3,  // 1 element/vertex, convertible to (UInt8 x4)
		Triangles  = 4,  // 1 row per TRIANGLE: 3 index datums (UInt16/Int16/UInt32/Int32). Required.
		PolyGroups = 5,  // 1 row per TRIANGLE: 1 index datum; drives auto-created
		                 //   sections; polygroup index == material slot index
	};

	// Frozen POD stream descriptor. Row byte size = DatumSize(DatumType) *
	// NumDatums * NumElements, tightly packed; the data blob is NumRows rows.
	// Extend ONLY by appending fields with a safe default in a future struct
	// version (bump StructVersion), never by reordering/removing.
	struct FMeshStreamDesc_V1
	{
		int32 StructVersion = 1;
		EMeshStream_V1 Semantic = EMeshStream_V1::Position;
		EMeshDatumType_V1 DatumType = EMeshDatumType_V1::Unknown;
		int32 NumDatums = 0;     // datums per element (FVector3f = 3, FPackedNormal = 4)
		int32 NumElements = 1;   // elements per row (Tangents = 2, TexCoords = channel count)
		int32 NumRows = 0;       // vertices, or triangles for Triangles/PolyGroups
	};

	// Frozen POD section-group creation options.
	struct FMeshSectionGroupConfig_V1
	{
		int32 StructVersion = 1;
		EMeshDrawType_V1 DrawType = EMeshDrawType_V1::Static;
		// When non-zero, sections are auto-created per polygroup (or one section
		// for the whole group when no PolyGroups stream is supplied).
		uint8 bAutoSectionsFromPolyGroups = 1;
	};

	/**
	 * CONSUMER-implemented view over its own mesh data — typically a thin wrapper
	 * around plain TArrays. The provider enumerates the streams and copies the
	 * data out during CreateSectionGroup/UpdateSectionGroup; nothing is retained
	 * after the call returns.
	 *
	 * Rules: exactly one stream per semantic; Position and Triangles are
	 * required; all per-vertex streams must have the same NumRows as Position;
	 * PolyGroups, when present, must have one row per triangle. A stream whose
	 * format the provider can neither use natively nor convert fails the whole
	 * call (no silent dropping).
	 */
	class IMeshStreamSource_V1
	{
	public:
		virtual ~IMeshStreamSource_V1() = default;
		virtual int32       GetNumStreams() const = 0;
		virtual bool        GetStreamDesc(int32 Index, FMeshStreamDesc_V1& OutDesc) const = 0;
		// Pointer to NumRows tightly-packed rows of the declared format. Valid
		// only for the duration of the provider call reading it.
		virtual const void* GetStreamData(int32 Index) const = 0;
	};

	/**
	 * PROVIDER-implemented (registered by the RealtimeMeshComponent plugin when
	 * installed). Drives a URealtimeMeshSimple through engine-only vocabulary.
	 */
	class IRealtimeMeshSimple_V1 : public IModularFeature
	{
	public:
		static FName GetModularFeatureName()
		{
			static const FName Name(TEXT("TriAxis.RealtimeMesh.Simple.1"));
			return Name;
		}

		virtual ~IRealtimeMeshSimple_V1() = default;

		// --- Frozen vtable. Order is ABI. Nothing may ever be added. ---

		// Must return SimpleContractRevision_V1; on mismatch the consumer disables
		// the integration.
		virtual int32 GetRevision() const = 0;

		// Creates and registers a RealtimeMesh component (returned as its engine
		// base class) on Owner, initialized as a Simple mesh, attached to Owner's
		// root component when one exists. The consumer owns attachment, transform,
		// visibility, and lifetime through normal UObject/engine APIs. Returns
		// nullptr on failure. DebugName may be null.
		virtual UMeshComponent* CreateSimpleMeshComponent(AActor* Owner, const TCHAR* DebugName) = 0;

		// Creates a section group on a component previously returned by
		// CreateSimpleMeshComponent, filled from Streams. Returns an opaque
		// group id (>= 0), or -1 on failure (invalid component / invalid streams).
		virtual int32 CreateSectionGroup(UMeshComponent* Mesh,
		                                 const FMeshSectionGroupConfig_V1& Config,
		                                 const IMeshStreamSource_V1& Streams) = 0;

		// Replaces the geometry of an existing group. Stream rules as above.
		virtual bool UpdateSectionGroup(UMeshComponent* Mesh, int32 GroupId,
		                                const IMeshStreamSource_V1& Streams) = 0;

		// Removes the group and its sections.
		virtual bool RemoveSectionGroup(UMeshComponent* Mesh, int32 GroupId) = 0;

		// Configures a material slot. Polygroup indices map to slot indices.
		// SlotName may be null (provider derives one); Material may be null
		// (slot reserved, engine default material renders).
		virtual bool SetMaterialSlot(UMeshComponent* Mesh, int32 SlotIndex,
		                             const TCHAR* SlotName, UMaterialInterface* Material) = 0;
	};
}
