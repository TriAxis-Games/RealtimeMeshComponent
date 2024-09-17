// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshCollision.h"
#include "Core/RealtimeMeshDataStream.h"
#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshConfig.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "Core/RealtimeMeshSectionConfig.h"
#include "Core/RealtimeMeshSectionGroupConfig.h"
#include "Interfaces/Interface_CollisionDataProvider.h"

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

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionGroupKey& Key)
{
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DataRestructure)
	{
		uint8 OldLODIndex;
		Ar << OldLODIndex;
		Key.LODIndex = OldLODIndex;
		uint8 OldGroupIndex;
		Ar << OldGroupIndex;
		Key.GroupName = FName("RM-Legacy-Group", OldGroupIndex);
	}
	else
	{
		Ar << Key.LODIndex;
		Ar << Key.GroupName;
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionKey& Key)
{
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DataRestructure)
	{
		uint8 OldLODIndex;
		Ar << OldLODIndex;
		Key.LODIndex = OldLODIndex;
		uint8 OldGroupIndex;
		Ar << OldGroupIndex;
		Key.GroupName = FName("RM-Legacy-Group", OldGroupIndex);
		uint16 OldSectionIndex;
		Ar << OldSectionIndex;
		Key.SectionName = FName("RM-Legacy-Section", OldSectionIndex);
	}
	else
	{
		Ar << Key.LODIndex;
		Ar << Key.GroupName;
		Ar << Key.SectionName;
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
	
FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionGroupConfig& Config)
{
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= RealtimeMesh::FRealtimeMeshVersion::DrawTypeMovedToSectionGroup)
	{
		Ar << Config.DrawType;
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

template<typename ShapeType>
FArchive& operator<<(FArchive& Ar, FSimpleShapeSet<ShapeType>& ShapeSet)
{
	Ar << ShapeSet.Shapes;

	if (Ar.IsLoading())
	{
		ShapeSet.RebuildNameMap();
	}
	
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
#if RMC_ENGINE_BELOW_5_4 || !WITH_EDITORONLY_DATA
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
	Ar << ComplexGeometry.Meshes;

	// Rebuild name maps on load
	if (Ar.IsLoading())
	{
		ComplexGeometry.NameMap.Empty();
		for (TSparseArray<FRealtimeMeshCollisionMesh>::TConstIterator It(ComplexGeometry.Meshes); It; ++It)
		{
			ComplexGeometry.AddToNameMap(It->Name, It.GetIndex());
		}
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
				Stream.StreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Unknown, Stream.StreamKey.GetName());
			}
		}

		Stream.CountBytes(Ar);

		FRealtimeMeshStream::SizeType SerializedNum = Ar.IsLoading() ? 0 : Stream.ArrayNum;;
		Ar << SerializedNum;

		if (SerializedNum > 0)
		{
			Stream.ArrayNum = 0;

			// Serialize simple bytes which require no construction or destruction.
			if (SerializedNum && Ar.IsLoading())
			{
				Stream.ResizeAllocation(SerializedNum);
			}

			// TODO: This will not handle endianness of the vertex data for say a network archive.
			Ar.Serialize(Stream.GetData(), SerializedNum * Stream.GetStride());
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