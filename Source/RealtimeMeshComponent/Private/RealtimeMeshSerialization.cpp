// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshCollision.h"
#include "Core/RealtimeMeshDataStream.h"
#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshConfig.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "Core/RealtimeMeshSectionConfig.h"
#include "Core/RealtimeMeshBufferSetConfig.h"
#include "Interfaces/Interface_CollisionDataProvider.h"

#include <cstddef>

// PT-5 / GUARD-001 (ND-3): characterization asserts pinning struct layouts the explicit
// field-by-field serializers below depend on. (These once also guarded the NoExport UHT mirrors,
// since retired — the structs are now reflected directly at their native declarations.) If one of
// these fires, a field was added/moved: update the corresponding operator<< and bump the version.

// FRealtimeMeshSectionConfig: int32 MaterialSlot + 4 contiguous bools (mirror UPROPERTY order).
static_assert(sizeof(FRealtimeMeshSectionConfig) == 8, "FRealtimeMeshSectionConfig layout changed; update its serializer and bump the version");
static_assert(offsetof(FRealtimeMeshSectionConfig, MaterialSlot) == 0, "FRealtimeMeshSectionConfig::MaterialSlot moved; update its serializer");
static_assert(offsetof(FRealtimeMeshSectionConfig, bIsVisible) == 4, "FRealtimeMeshSectionConfig::bIsVisible moved; update its serializer");
static_assert(offsetof(FRealtimeMeshSectionConfig, bCastsShadow) == 5, "FRealtimeMeshSectionConfig::bCastsShadow moved; update its serializer");
static_assert(offsetof(FRealtimeMeshSectionConfig, bIsMainPassRenderable) == 6, "FRealtimeMeshSectionConfig::bIsMainPassRenderable moved; update its serializer");
static_assert(offsetof(FRealtimeMeshSectionConfig, bForceOpaque) == 7, "FRealtimeMeshSectionConfig::bForceOpaque moved; update its serializer");

// FRealtimeMeshBufferSetConfig: uint8 DrawType + bool bComputeWritable (mirror UPROPERTY order).
static_assert(sizeof(FRealtimeMeshBufferSetConfig) == 2, "FRealtimeMeshBufferSetConfig layout changed; update its serializer and bump the version");
static_assert(offsetof(FRealtimeMeshBufferSetConfig, DrawType) == 0, "FRealtimeMeshBufferSetConfig::DrawType moved; update its serializer");
static_assert(offsetof(FRealtimeMeshBufferSetConfig, bComputeWritable) == 1, "FRealtimeMeshBufferSetConfig::bComputeWritable moved; update its serializer");

// FRealtimeMeshComplexGeometry: exactly one member, FSimpleShapeSet<FRealtimeMeshCollisionMesh> Meshes
// (Meshes is private, so pin via size equality rather than offsetof — proves the single-member layout).
static_assert(sizeof(FRealtimeMeshComplexGeometry) == sizeof(FSimpleShapeSet<FRealtimeMeshCollisionMesh>),
	"FRealtimeMeshComplexGeometry is no longer exactly one FSimpleShapeSet<FRealtimeMeshCollisionMesh>; update its serializer");

FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODKey& Key)
{		
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DataRestructure)
	{
		uint8 OldLODIndex;
		Ar << OldLODIndex;
		Key.LODIndex = OldLODIndex;
	}
	else
	{
		Ar << Key.LODIndex;
	}
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshBufferSetKey& Key)
{
	// Serialization format history (mirrors the FRealtimeMeshSectionKey history):
	//  - Pre-DataRestructure: uint8 LODIndex + uint8 GroupIndex. SlotIndex is derived
	//    from the synthesized legacy group name's CRC32.
	//  - DataRestructure .. SerializeSectionGroupKeySlotIndex: LODIndex + GroupName only.
	//    SlotIndex was derived from FCrc::StrCrc32(GroupName). Group-key identity is
	//    (LODIndex, SlotIndex), so distinct groups sharing a name (e.g. every procedural
	//    group is named "PMC") collapsed to one identical key across save/load — silent
	//    data loss.
	//  - SerializeSectionGroupKeySlotIndex and later: LODIndex + GroupName + SlotIndex
	//    written directly, so the SlotIndex round-trips faithfully and distinct groups
	//    keep distinct keys.
	const bool bHasDirectSlotIndex =
		Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::SerializeSectionGroupKeySlotIndex;

	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DataRestructure)
	{
		uint8 OldLODIndex;
		Ar << OldLODIndex;
		Key.LODIndex = OldLODIndex;
		uint8 OldGroupIndex;
		Ar << OldGroupIndex;
		Key.SlotName = FName("RM-Legacy-Group", OldGroupIndex);
	}
	else if (!bHasDirectSlotIndex)
	{
		Ar << Key.LODIndex;
		Ar << Key.SlotName;
	}
	else
	{
		Ar << Key.LODIndex;
		Ar << Key.SlotName;
		Ar << Key.SlotIndex;
	}

	if (Ar.IsLoading() && !bHasDirectSlotIndex)
	{
		const FString NameString = Key.SlotName.ToString();
		Key.SlotIndex = static_cast<int32>(FCrc::StrCrc32(*NameString));
	}
	// else: SlotIndex was read directly above.

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionKey& Key)
{
	// Serialization format history:
	//  - Pre-DataRestructure: uint8 LODIndex + uint8 GroupIndex + uint16 SectionIndex.
	//    BufferSetSlotIndex is derived from the synthesized legacy group name's CRC32.
	//  - DataRestructure .. SerializeSectionKeyBufferSlotAndStreamType: LODIndex +
	//    GroupName + SectionName. Historically the GroupName was written as NAME_None
	//    on save (a bug), so BufferSetSlotIndex loaded as INDEX_NONE for these assets.
	//    We still read the (legacy) group name and derive BufferSetSlotIndex from it so
	//    any archive that happened to store a real name keeps working.
	//  - SerializeSectionKeyBufferSlotAndStreamType and later: LODIndex + SectionName +
	//    BufferSetSlotIndex written directly, so it round-trips faithfully.
	const bool bHasDirectBufferSlot =
		Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::SerializeSectionKeyBufferSlotAndStreamType;

	FName LegacyGroupName = NAME_None;
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DataRestructure)
	{
		uint8 OldLODIndex;
		Ar << OldLODIndex;
		Key.LODIndex = OldLODIndex;
		uint8 OldGroupIndex;
		Ar << OldGroupIndex;
		LegacyGroupName = FName("RM-Legacy-Group", OldGroupIndex);
		uint16 OldSectionIndex;
		Ar << OldSectionIndex;
		Key.SlotName = FName("RM-Legacy-Section", OldSectionIndex);
	}
	else if (!bHasDirectBufferSlot)
	{
		Ar << Key.LODIndex;
		Ar << LegacyGroupName;
		Ar << Key.SlotName;
	}
	else
	{
		Ar << Key.LODIndex;
		Ar << Key.SlotName;
		Ar << Key.BufferSetSlotIndex;
	}

	if (Ar.IsLoading())
	{
		const FString SlotNameString = Key.SlotName.ToString();
		Key.SlotIndex = static_cast<int32>(FCrc::StrCrc32(*SlotNameString));

		if (!bHasDirectBufferSlot)
		{
			const FString GroupNameString = LegacyGroupName.ToString();
			Key.BufferSetSlotIndex = LegacyGroupName != NAME_None
				? static_cast<int32>(FCrc::StrCrc32(*GroupNameString))
				: INDEX_NONE;
		}
		// else: BufferSetSlotIndex was read directly above.
	}

	return Ar;
}


	
FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionConfig& Config)
{		
	Ar << Config.MaterialSlot;
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DrawTypeMovedToSectionGroup)
	{
		ERealtimeMeshSectionDrawType DrawType;
		Ar << DrawType;
	}
	Ar << Config.bIsVisible;
	Ar << Config.bCastsShadow;
	Ar << Config.bIsMainPassRenderable;
	Ar << Config.bForceOpaque;
	return Ar;
}
	
FArchive& operator<<(FArchive& Ar, FRealtimeMeshBufferSetConfig& Config)
{
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::DrawTypeMovedToSectionGroup)
	{
		Ar << Config.DrawType;
	}
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::SectionGroupComputeWritable)
	{
		Ar << Config.bComputeWritable;
	}
	return Ar;
}
	
FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODConfig& Config)
{		
	Ar << Config.bIsVisible;
	Ar << Config.ScreenSize;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshConfig& Config)
{		
	Ar << Config.ForcedLOD;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConfiguration& Config)
{
	Ar << Config.bUseComplexAsSimpleCollision;
	Ar << Config.bUseAsyncCook;
	Ar << Config.bShouldFastCookMeshes;

	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::CollisionUpdateFlowRestructure)
	{
		Ar << Config.bFlipNormals;
		Ar << Config.bDeformableMesh;
	}

	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::CollisionOverhaul)
	{
		Ar << Config.bMergeAllMeshes;
	}
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionShape& Shape)
{
	Ar << Shape.Name;
	Ar << Shape.Center;
	Ar << Shape.Rotation;
	Ar << Shape.bContributesToMass;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionSphere& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.Radius;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionBox& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.Extents;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionCapsule& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.Radius;
	Ar << Shape.Length;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionTaperedCapsule& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.RadiusA;
	Ar << Shape.RadiusB;
	Ar << Shape.Length;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionConvex& Shape)
{
	Ar << static_cast<FRealtimeMeshCollisionShape&>(Shape);
	Ar << Shape.Vertices;
	Ar << Shape.BoundingBox;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSimpleGeometry& SimpleGeometry)
{
	Ar << SimpleGeometry.Spheres.Shapes;
	Ar << SimpleGeometry.Boxes.Shapes;
	Ar << SimpleGeometry.Capsules.Shapes;
	Ar << SimpleGeometry.TaperedCapsules.Shapes;
	Ar << SimpleGeometry.ConvexHulls.Shapes;

	if (Ar.IsLoading())
	{
		SimpleGeometry.Spheres.RebuildNameMap();
		SimpleGeometry.Boxes.RebuildNameMap();
		SimpleGeometry.Capsules.RebuildNameMap();
		SimpleGeometry.TaperedCapsules.RebuildNameMap();
		SimpleGeometry.ConvexHulls.RebuildNameMap();
	}

	return Ar;
}


// This is added in 5.4 but only for editor use, so we add it any other time.
#if !WITH_EDITORONLY_DATA
static FArchive& operator<<(FArchive& Ar, FTriIndices& Indices)
{
	Ar << Indices.v0 << Indices.v1 << Indices.v2;
	return Ar;
}
#endif

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionMeshCookedUVData& UVInfo)
{
	Ar << UVInfo.Triangles;
	Ar << UVInfo.Positions;
	Ar << UVInfo.TexCoords;

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionMesh& MeshData)
{
	Ar << MeshData.Vertices;
	Ar << MeshData.Triangles;
	Ar << MeshData.Materials;

	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::CollisionOverhaul)
	{
		check(Ar.IsLoading()); // This should only be for updating old data
		TArray<TArray<FVector2D>> UVData;
		Ar << UVData;

		MeshData.TexCoords.SetNum(UVData.Num());

		for (int32 ChannelId = 0; ChannelId < UVData.Num(); ++ChannelId)
		{
			MeshData.TexCoords[ChannelId].SetNumUninitialized(UVData[ChannelId].Num());
			for (int32 UVId = 0; UVId < UVData[ChannelId].Num(); ++UVId)
			{
				MeshData.TexCoords[ChannelId][UVId] = FVector2f(UVData[ChannelId][UVId]);
			}
		}
		
		MeshData.bFlipNormals = true;
	}
	else
	{
		Ar << MeshData.TexCoords;
		Ar << MeshData.bFlipNormals;
	}
	
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshComplexGeometry& ComplexGeometry)
{
	// DUP-002: ComplexGeometry now stores its meshes in an FSimpleShapeSet<FRealtimeMeshCollisionMesh>.
	// The bytes on the wire are unchanged: `Meshes.Shapes` is the same TSparseArray<FRealtimeMeshCollisionMesh>
	// that was serialized directly before the convergence. On load, RebuildNameMap() runs the identical
	// Empty()+iterate+AddToNameMap loop this function used to inline (mesh Name is not serialized, so every
	// loaded mesh is NAME_None and the map rebuilds empty).
	Ar << ComplexGeometry.Meshes.Shapes;

	// Rebuild name maps on load
	if (Ar.IsLoading())
	{
		ComplexGeometry.Meshes.RebuildNameMap();
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshCollisionInfo& CollisionInfo)
{
	Ar << CollisionInfo.Configuration;
	Ar << CollisionInfo.SimpleGeometry;
	Ar << CollisionInfo.ComplexGeometry;
	return Ar;
}


namespace RealtimeMesh
{
	FArchive& operator<<(FArchive& Ar, FRealtimeMeshStream& Stream)
	{
		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::StreamsNowHoldEntireKey)
		{
			Ar << Stream.StreamKey;
		}
		else
		{
			// ND-8: The pre-StreamsNowHoldEntireKey key format is load-only. Saves always occur at the
			// latest version, so an old-version save is impossible; the checkf documents that and keeps
			// the old write-broken save path (which serialized a default-constructed StreamName and
			// clobbered Stream.StreamKey) from ever executing.
			checkf(Ar.IsLoading(), TEXT("Cannot save FRealtimeMeshStream at a pre-StreamsNowHoldEntireKey version"));

			FName StreamName;
			Ar << StreamName;
			Stream.StreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Unknown, StreamName);
		}

		Ar << Stream.Layout;
		Stream.CacheStrides();
			
		if (Ar.IsLoading())
		{
			if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::StreamsNowHoldEntireKey)
			{
				ERealtimeMeshStreamType StreamType =
				(Stream.Layout == GetRealtimeMeshBufferLayout<uint32>() ||
					Stream.Layout == GetRealtimeMeshBufferLayout<int32>() ||
					Stream.Layout == GetRealtimeMeshBufferLayout<uint16>())
					? ERealtimeMeshStreamType::Index
					: ERealtimeMeshStreamType::Vertex;
				Stream.StreamKey = FRealtimeMeshStreamKey(StreamType, Stream.StreamKey.GetName());
			}
		}

		Stream.CountBytes(Ar);

		FRealtimeMeshStream::SizeType SerializedNum = Ar.IsLoading() ? 0 : Stream.ArrayNum;
		Ar << SerializedNum;

		if (SerializedNum > 0)
		{
			Stream.ArrayNum = 0;

			// Serialize simple bytes which require no construction or destruction.
			if (SerializedNum && Ar.IsLoading())
			{
				Stream.ResizeAllocation(SerializedNum);
			}

			// A raw byte copy is correct for a same-endian archive (the only kind any shipping
			// UE platform produces). For a byte-swapping archive (cross-endian cooked data),
			// each primitive datum must be reversed so typed values survive the round trip:
			// on load, swap after reading; on save, swap to foreign endianness, write, then
			// swap back so the live in-memory stream isn't left corrupted.
			const int64 TotalBytes = static_cast<int64>(SerializedNum) * Stream.GetStride();

			if (Ar.IsByteSwapping() && TotalBytes > 0)
			{
				const FRealtimeMeshElementType ElementType = Stream.GetLayout().GetElementType();
				const FRealtimeMeshElementTypeDetails Details = FRealtimeMeshBufferLayoutUtilities::GetElementTypeDetails(ElementType);
				const int32 NumDatums = ElementType.GetNumDatums();
				const int32 DatumSize = NumDatums > 0 ? (Details.GetStride() / NumDatums) : 0;

				auto SwapDatumsInPlace = [DatumSize, TotalBytes](uint8* Bytes)
				{
					if (DatumSize <= 1)
					{
						return; // single-byte datums (e.g. FColor channels) need no swapping
					}
					for (int64 ByteOffset = 0; ByteOffset + DatumSize <= TotalBytes; ByteOffset += DatumSize)
					{
						for (int32 Low = 0, High = DatumSize - 1; Low < High; ++Low, --High)
						{
							Swap(Bytes[ByteOffset + Low], Bytes[ByteOffset + High]);
						}
					}
				};

				uint8* DataBytes = reinterpret_cast<uint8*>(Stream.GetData());
				if (Ar.IsLoading())
				{
					Ar.Serialize(DataBytes, TotalBytes);
					SwapDatumsInPlace(DataBytes);
				}
				else
				{
					SwapDatumsInPlace(DataBytes);
					Ar.Serialize(DataBytes, TotalBytes);
					SwapDatumsInPlace(DataBytes);
				}
			}
			else
			{
				Ar.Serialize(Stream.GetData(), TotalBytes);
			}

			Stream.ArrayNum = SerializedNum;

			if (Ar.IsLoading())
			{					
				Stream.BroadcastNumChanged();
			}
		}
		else if (Ar.IsLoading())
		{
			Stream.Empty();
		}
		return Ar;
	}

	FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamSet& StreamSet)
	{
		int32 NumStreams = StreamSet.Num();
		Ar << NumStreams;

		if (Ar.IsLoading())
		{
			StreamSet.Streams.Empty();
			for (int32 Index = 0; Index < NumStreams; Index++)
			{
				FRealtimeMeshStreamKey StreamKey;
				Ar << StreamKey;
				FRealtimeMeshStream Stream;
				Ar << Stream;
				Stream.SetStreamKey(StreamKey);
					
				StreamSet.AddStream(MoveTemp(Stream));
			}
		}
		else
		{
			StreamSet.ForEach([&Ar](FRealtimeMeshStream& Stream)
			{					
				FRealtimeMeshStreamKey StreamKey = Stream.GetStreamKey();
				Ar << StreamKey;
				Ar << Stream;
			});
		}

		return Ar;
	}
	
	FArchive& operator<<(FArchive& Ar, FRealtimeMeshElementType& ElementType)
	{
		Ar << ElementType.Type;

		uint8 TempNumDatums = ElementType.NumDatums;
		Ar << TempNumDatums;
		ElementType.NumDatums = TempNumDatums;

		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::ImprovingDataTypes)
		{
			// TODO: Remove these
			bool bTempNormalized = false;
			Ar << bTempNormalized;

			bool bTempShouldConvertToFloat = false;
			Ar << bTempShouldConvertToFloat;

			if (Ar.IsLoading())
			{
				if (bTempNormalized && bTempShouldConvertToFloat && ElementType.Type == ERealtimeMeshDatumType::Int8)
				{
					ElementType.Type = ERealtimeMeshDatumType::Int8Float;
				}
			}
		}

		return Ar;
	}

	FArchive& operator<<(FArchive& Ar, FRealtimeMeshBufferLayout& Layout)
	{
		Ar << Layout.ElementType;

		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::RemovedNamedStreamElements)
		{
			TArray<FName, TInlineAllocator<REALTIME_MESH_MAX_STREAM_ELEMENTS>> Elements;
			Ar << Elements;
		}
		Ar << Layout.NumElements;
		return Ar;
	}

	
}