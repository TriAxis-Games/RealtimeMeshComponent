// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.
//
// Provider implementation of the RealtimeMesh Simple soft-linkage contract
// (Contracts/RealtimeMeshSimpleContract_v1.h). Consumers copy that header and
// talk to this implementation through IModularFeatures — no link dependency.

#include "Contracts/RealtimeMeshSimpleContract_v1.h"

#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshSimple.h"
#include "Core/RealtimeMeshDataConversion.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Core/RealtimeMeshModularFeatures.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshUpdateBuilder.h"

namespace RealtimeMesh::ContractsPrivate
{
	namespace TA = TriAxis::RealtimeMesh;

	// Deterministic group-id -> key mapping: no per-mesh registry to keep, and a
	// stale id simply names a group that no longer exists (ops return false).
	FRealtimeMeshBufferSetKey MakeGroupKey(int32 GroupId)
	{
		return FRealtimeMeshBufferSetKey::Create(0, FName(*FString::Printf(TEXT("TriAxisSimpleContract_%d"), GroupId)));
	}

	// Maps a portable datum enum to the real one. A switch (not a cast) so a
	// future reorder on either side breaks loudly here instead of corrupting data.
	ERealtimeMeshDatumType MapDatumType(TA::EMeshDatumType_V1 Datum)
	{
		switch (Datum)
		{
		case TA::EMeshDatumType_V1::UInt8:     return ERealtimeMeshDatumType::UInt8;
		case TA::EMeshDatumType_V1::Int8:      return ERealtimeMeshDatumType::Int8;
		case TA::EMeshDatumType_V1::UInt16:    return ERealtimeMeshDatumType::UInt16;
		case TA::EMeshDatumType_V1::Int16:     return ERealtimeMeshDatumType::Int16;
		case TA::EMeshDatumType_V1::UInt32:    return ERealtimeMeshDatumType::UInt32;
		case TA::EMeshDatumType_V1::Int32:     return ERealtimeMeshDatumType::Int32;
		case TA::EMeshDatumType_V1::Half:      return ERealtimeMeshDatumType::Half;
		case TA::EMeshDatumType_V1::Float:     return ERealtimeMeshDatumType::Float;
		case TA::EMeshDatumType_V1::Double:    return ERealtimeMeshDatumType::Double;
		case TA::EMeshDatumType_V1::Int8Float: return ERealtimeMeshDatumType::Int8Float;
		case TA::EMeshDatumType_V1::RGB10A2:   return ERealtimeMeshDatumType::RGB10A2;
		default:                               return ERealtimeMeshDatumType::Unknown;
		}
	}

	// A stream format is acceptable if it IS one of the semantic's native element
	// types or the conversion registry can convert it to the canonical one.
	bool IsElementTypeUsable(const FRealtimeMeshElementType& UserType, const FRealtimeMeshElementType& CanonicalType,
		std::initializer_list<FRealtimeMeshElementType> NativeTypes)
	{
		for (const FRealtimeMeshElementType& Native : NativeTypes)
		{
			if (UserType == Native)
			{
				return true;
			}
		}
		return FRealtimeMeshTypeConversionUtilities::CanConvert(UserType, CanonicalType);
	}

	bool IsIndexDatum(ERealtimeMeshDatumType Datum)
	{
		return Datum == ERealtimeMeshDatumType::UInt16 || Datum == ERealtimeMeshDatumType::Int16
			|| Datum == ERealtimeMeshDatumType::UInt32 || Datum == ERealtimeMeshDatumType::Int32;
	}

	// Reads index datum I of a tightly packed 16- or 32-bit index blob.
	uint32 ReadIndexDatum(const void* Data, bool b16Bit, int32 DatumIndex)
	{
		return b16Bit
			? static_cast<uint32>(static_cast<const uint16*>(Data)[DatumIndex])
			: static_cast<const uint32*>(Data)[DatumIndex];
	}

	// One declared stream, mapped and format-checked but not yet range-validated.
	struct FMappedStream
	{
		TA::FMeshStreamDesc_V1 Desc;
		const void* Data = nullptr;
		FRealtimeMeshBufferLayout Layout = FRealtimeMeshBufferLayout::Invalid;
	};

	// Builds the pass-through stream set: each stream keeps the CALLER's declared
	// element format (validated native-or-convertible); RMC's data-type mapping
	// and conversion machinery handle the rest downstream.
	bool BuildStreamSet(const TA::IMeshStreamSource_V1& Source, FRealtimeMeshStreamSet& OutStreamSet)
	{
		// Semantic -> (canonical, natives) acceptance sets.
		const FRealtimeMeshElementType Float3 = GetRealtimeMeshDataElementType<FVector3f>();
		const FRealtimeMeshElementType Float2 = GetRealtimeMeshDataElementType<FVector2f>();
		const FRealtimeMeshElementType Half2 = GetRealtimeMeshDataElementType<FVector2DHalf>();
		const FRealtimeMeshElementType ColorType = GetRealtimeMeshDataElementType<FColor>();
		const FRealtimeMeshElementType PackedNormalType = GetRealtimeMeshDataElementType<FPackedNormal>();
		const FRealtimeMeshElementType PackedRGBA16NType = GetRealtimeMeshDataElementType<FPackedRGBA16N>();

		TMap<TA::EMeshStream_V1, FMappedStream> Streams;

		const int32 NumStreams = Source.GetNumStreams();
		for (int32 Index = 0; Index < NumStreams; Index++)
		{
			FMappedStream Mapped;
			if (!Source.GetStreamDesc(Index, Mapped.Desc) || Mapped.Desc.StructVersion < 1)
			{
				return false;
			}
			const TA::FMeshStreamDesc_V1& Desc = Mapped.Desc;

			const FRealtimeMeshElementType ElementType(MapDatumType(Desc.DatumType), Desc.NumDatums);
			if (!ElementType.IsValid() || Desc.NumDatums < 1 || Desc.NumDatums > 4
				|| Desc.NumElements < 1 || Desc.NumElements > REALTIME_MESH_MAX_STREAM_ELEMENTS
				|| Desc.NumRows <= 0)
			{
				return false;
			}
			Mapped.Layout = FRealtimeMeshBufferLayout(ElementType, Desc.NumElements);

			Mapped.Data = Source.GetStreamData(Index);
			if (Mapped.Data == nullptr)
			{
				return false;
			}

			// Per-semantic format/shape acceptance.
			bool bUsable = false;
			switch (Desc.Semantic)
			{
			case TA::EMeshStream_V1::Position:
				bUsable = Desc.NumElements == 1 && IsElementTypeUsable(ElementType, Float3, {Float3});
				break;
			case TA::EMeshStream_V1::Tangents:
				bUsable = Desc.NumElements == 2
					&& IsElementTypeUsable(ElementType, PackedNormalType, {PackedNormalType, PackedRGBA16NType});
				break;
			case TA::EMeshStream_V1::TexCoords:
				bUsable = IsElementTypeUsable(ElementType, Float2, {Float2, Half2});
				break;
			case TA::EMeshStream_V1::Color:
				bUsable = Desc.NumElements == 1 && IsElementTypeUsable(ElementType, ColorType, {ColorType});
				break;
			case TA::EMeshStream_V1::Triangles:
				// 3 index datums per row, however the caller factors element/datum.
				// Normalized to RMC's canonical factoring — scalar element x3 (the
				// TIndex3 layout) — same bytes, but the shape the poly-group and
				// render paths dispatch on.
				bUsable = IsIndexDatum(ElementType.GetDatumType()) && Desc.NumDatums * Desc.NumElements == 3;
				if (bUsable)
				{
					Mapped.Layout = FRealtimeMeshBufferLayout(FRealtimeMeshElementType(ElementType.GetDatumType(), 1), 3);
				}
				break;
			case TA::EMeshStream_V1::PolyGroups:
				bUsable = IsIndexDatum(ElementType.GetDatumType()) && Desc.NumDatums == 1 && Desc.NumElements == 1;
				break;
			default:
				// Unknown semantic from a newer contract copy: reject rather than
				// silently drop data the consumer expected to matter.
				bUsable = false;
				break;
			}
			if (!bUsable || Streams.Contains(Desc.Semantic))
			{
				return false;
			}
			Streams.Add(Desc.Semantic, Mapped);
		}

		const FMappedStream* Position = Streams.Find(TA::EMeshStream_V1::Position);
		const FMappedStream* Triangles = Streams.Find(TA::EMeshStream_V1::Triangles);
		if (!Position || !Triangles)
		{
			return false;
		}
		const int32 NumVertices = Position->Desc.NumRows;
		const int32 NumTriangles = Triangles->Desc.NumRows;

		// Cross-stream shape checks: per-vertex streams match Position, per-triangle
		// streams match Triangles.
		for (const TPair<TA::EMeshStream_V1, FMappedStream>& Pair : Streams)
		{
			const bool bPerTriangle = Pair.Key == TA::EMeshStream_V1::Triangles || Pair.Key == TA::EMeshStream_V1::PolyGroups;
			if (Pair.Value.Desc.NumRows != (bPerTriangle ? NumTriangles : NumVertices))
			{
				return false;
			}
		}

		// Index range check (16- or 32-bit datums).
		{
			const bool b16Bit = Triangles->Layout.GetElementType().GetDatumType() == ERealtimeMeshDatumType::UInt16
				|| Triangles->Layout.GetElementType().GetDatumType() == ERealtimeMeshDatumType::Int16;
			for (int32 DatumIndex = 0; DatumIndex < NumTriangles * 3; DatumIndex++)
			{
				if (ReadIndexDatum(Triangles->Data, b16Bit, DatumIndex) >= static_cast<uint32>(NumVertices))
				{
					return false;
				}
			}
		}

		// Copy each stream through in its declared layout. The consumer's rows are
		// tightly packed; require RMC's computed stride to agree so the memcpy
		// cannot reinterpret padding differently than the caller laid it out.
		for (const TPair<TA::EMeshStream_V1, FMappedStream>& Pair : Streams)
		{
			const FMappedStream& Mapped = Pair.Value;

			FRealtimeMeshStreamKey StreamKey;
			switch (Pair.Key)
			{
			case TA::EMeshStream_V1::Position:   StreamKey = FRealtimeMeshStreams::Position; break;
			case TA::EMeshStream_V1::Tangents:   StreamKey = FRealtimeMeshStreams::Tangents; break;
			case TA::EMeshStream_V1::TexCoords:  StreamKey = FRealtimeMeshStreams::TexCoords; break;
			case TA::EMeshStream_V1::Color:      StreamKey = FRealtimeMeshStreams::Color; break;
			case TA::EMeshStream_V1::Triangles:  StreamKey = FRealtimeMeshStreams::Triangles; break;
			case TA::EMeshStream_V1::PolyGroups: StreamKey = FRealtimeMeshStreams::PolyGroups; break;
			default: return false;
			}

			FRealtimeMeshStream Stream(StreamKey, Mapped.Layout);
			const int32 Stride = Stream.GetStride();
			const int32 PackedRowSize = FRealtimeMeshBufferLayoutUtilities::GetElementStride(Mapped.Layout.GetElementType()) * Mapped.Layout.GetNumElements();
			if (Stride != PackedRowSize)
			{
				return false;
			}
			Stream.SetNumUninitialized(Mapped.Desc.NumRows);
			FMemory::Memcpy(Stream.GetData(), Mapped.Data, static_cast<SIZE_T>(Stride) * Mapped.Desc.NumRows);
			OutStreamSet.AddStream(MoveTemp(Stream));
		}
		return true;
	}

	// Silent existence probe: unlike GetSectionGroup()/task-based lookups, a miss
	// here must NOT log an error — an unknown id is a normal contract outcome.
	bool SectionGroupExists(URealtimeMeshSimple& Simple, const FRealtimeMeshBufferSetKey& Key)
	{
		bool bExists = false;
		FRealtimeMeshAccessor Accessor;
		Accessor.AddLODTask<FRealtimeMeshLOD>(Key.LOD(),
			[&bExists, &Key](const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshLOD& LOD)
			{
				bExists = LOD.GetSectionGroupKeys(LockContext).Contains(Key);
			});
		Accessor.Execute(Simple.GetMeshData());
		return bExists;
	}

	// Validates the borrowed component pointer and resolves the Simple mesh on it.
	URealtimeMeshSimple* ResolveSimpleMesh(UMeshComponent* Mesh)
	{
		URealtimeMeshComponent* Component = Cast<URealtimeMeshComponent>(Mesh);
		if (!IsValid(Component))
		{
			return nullptr;
		}
		return Cast<URealtimeMeshSimple>(Component->GetRealtimeMesh());
	}

	class FRealtimeMeshSimpleContractProvider final : public TA::IRealtimeMeshSimple_V1
	{
	public:
		// Inherited GetModularFeatureName() from IRealtimeMeshSimple_V1 is used
		// by the registration helper below.

		virtual int32 GetRevision() const override
		{
			return TA::SimpleContractRevision_V1;
		}

		virtual UMeshComponent* CreateSimpleMeshComponent(AActor* Owner, const TCHAR* DebugName) override
		{
			check(IsInGameThread());
			if (!IsValid(Owner))
			{
				return nullptr;
			}

			const FName ComponentName = MakeUniqueObjectName(Owner, URealtimeMeshComponent::StaticClass(),
				DebugName ? FName(DebugName) : FName(TEXT("TriAxisRealtimeMesh")));
			URealtimeMeshComponent* Component = NewObject<URealtimeMeshComponent>(Owner, ComponentName);
			if (!Component)
			{
				return nullptr;
			}

			if (USceneComponent* Root = Owner->GetRootComponent())
			{
				Component->SetupAttachment(Root);
			}
			Component->RegisterComponent();

			if (!URealtimeMeshSimple::InitializeRealtimeMeshSimple(Component))
			{
				Component->DestroyComponent();
				return nullptr;
			}
			return Component;
		}

		virtual int32 CreateSectionGroup(UMeshComponent* Mesh, const TA::FMeshSectionGroupConfig_V1& Config,
			const TA::IMeshStreamSource_V1& Streams) override
		{
			check(IsInGameThread());
			URealtimeMeshSimple* Simple = ResolveSimpleMesh(Mesh);
			if (!Simple || Config.StructVersion < 1)
			{
				return -1;
			}

			FRealtimeMeshStreamSet StreamSet;
			if (!BuildStreamSet(Streams, StreamSet))
			{
				return -1;
			}

			const int32 GroupId = NextGroupId.Increment();
			const ERealtimeMeshSectionDrawType DrawType = Config.DrawType == TA::EMeshDrawType_V1::Dynamic
				? ERealtimeMeshSectionDrawType::Dynamic
				: ERealtimeMeshSectionDrawType::Static;
			Simple->CreateBufferSet(MakeGroupKey(GroupId), MoveTemp(StreamSet),
				FRealtimeMeshBufferSetConfig(DrawType), Config.bAutoSectionsFromPolyGroups != 0);
			return GroupId;
		}

		virtual bool UpdateSectionGroup(UMeshComponent* Mesh, int32 GroupId, const TA::IMeshStreamSource_V1& Streams) override
		{
			check(IsInGameThread());
			URealtimeMeshSimple* Simple = ResolveSimpleMesh(Mesh);
			if (!Simple || GroupId <= 0 || !SectionGroupExists(*Simple, MakeGroupKey(GroupId)))
			{
				return false;
			}

			FRealtimeMeshStreamSet StreamSet;
			if (!BuildStreamSet(Streams, StreamSet))
			{
				return false;
			}
			Simple->UpdateBufferSet(MakeGroupKey(GroupId), MoveTemp(StreamSet));
			return true;
		}

		virtual bool RemoveSectionGroup(UMeshComponent* Mesh, int32 GroupId) override
		{
			check(IsInGameThread());
			URealtimeMeshSimple* Simple = ResolveSimpleMesh(Mesh);
			if (!Simple || GroupId <= 0 || !SectionGroupExists(*Simple, MakeGroupKey(GroupId)))
			{
				return false;
			}
			Simple->RemoveBufferSet(MakeGroupKey(GroupId));
			return true;
		}

		virtual bool SetMaterialSlot(UMeshComponent* Mesh, int32 SlotIndex, const TCHAR* SlotName,
			UMaterialInterface* Material) override
		{
			check(IsInGameThread());
			URealtimeMeshSimple* Simple = ResolveSimpleMesh(Mesh);
			if (!Simple || SlotIndex < 0)
			{
				return false;
			}
			const FName ResolvedName = SlotName
				? FName(SlotName)
				: FName(*FString::Printf(TEXT("Slot_%d"), SlotIndex));
			Simple->SetupMaterialSlot(SlotIndex, ResolvedName, Material);
			return true;
		}

	private:
		FThreadSafeCounter NextGroupId;   // ids start at 1; 0/negatives never valid
	};

	// Registers at module load, unregisters at unload.
	TRealtimeMeshModularFeatureRegistration<FRealtimeMeshSimpleContractProvider> GSimpleContractProviderRegistration;
}
