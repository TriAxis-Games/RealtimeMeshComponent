// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Core/RealtimeMeshDynamicBuilder.h"

namespace RealtimeMesh
{
	FRealtimeMeshDynamicBuilder::FRealtimeMeshDynamicBuilder(FRealtimeMeshStreamSet& InStreams)
		: Streams(InStreams)
	{
		// Required streams — allocate if missing with sensible defaults.
		FRealtimeMeshStream& PositionStream = Streams.FindOrAdd(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>());
		Vertices = MakeUnique<TRealtimeMeshStreamBuilder<FVector3f, void>>(PositionStream);
		VerticesLinkage.BindStream(PositionStream, FRealtimeMeshStreamDefaultRowValue());

		FRealtimeMeshStream& TriangleStream = Streams.FindOrAdd(FRealtimeMeshStreams::Triangles, GetRealtimeMeshBufferLayout<TIndex3<uint16>>());
		Triangles = MakeUnique<TRealtimeMeshStreamBuilder<TIndex3<uint32>, void>>(TriangleStream);
		TrianglesLinkage.BindStream(TriangleStream, FRealtimeMeshStreamDefaultRowValue());
	}

	void FRealtimeMeshDynamicBuilder::EnableTangents(const FRealtimeMeshElementType& ElementType)
	{
		check(ElementType.IsValid());
		checkf(FRealtimeMeshTypeConversionUtilities::CanConvert(ElementType, GetRealtimeMeshDataElementType<FVector4f>()),
			TEXT("ElementType must be convertible to FVector4f"));

		if (HasTangents())
		{
			return;
		}

		// Two-element row (Normal + Tangent), each of the supplied element type.
		FRealtimeMeshStream& Stream = Streams.FindOrAdd(FRealtimeMeshStreams::Tangents, FRealtimeMeshBufferLayout(ElementType, 2));
		Tangents = MakeUnique<TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FVector4f>, void>>(Stream);
		VerticesLinkage.BindStream(Stream,
			FRealtimeMeshStreamDefaultRowValue::Create(
				TRealtimeMeshTangents<FVector4f>(FVector3f::ZAxisVector, FVector3f::XAxisVector),
				Stream.GetLayout()));
	}

	void FRealtimeMeshDynamicBuilder::DisableTangents()
	{
		if (!HasTangents()) return;
		VerticesLinkage.RemoveStream(Tangents->GetStream());
		Tangents.Reset();
		Streams.Remove(FRealtimeMeshStreams::Tangents);
	}

	void FRealtimeMeshDynamicBuilder::EnableColors()
	{
		if (HasColors()) return;
		FRealtimeMeshStream& Stream = Streams.FindOrAdd(FRealtimeMeshStreams::Color, GetRealtimeMeshBufferLayout<FColor>());
		Colors = MakeUnique<TRealtimeMeshStreamBuilder<FColor, void>>(Stream);
		VerticesLinkage.BindStream(Stream, FRealtimeMeshStreamDefaultRowValue::Create(FColor::White));
	}

	void FRealtimeMeshDynamicBuilder::DisableColors()
	{
		if (!HasColors()) return;
		VerticesLinkage.RemoveStream(Colors->GetStream());
		Colors.Reset();
		Streams.Remove(FRealtimeMeshStreams::Color);
	}

	void FRealtimeMeshDynamicBuilder::EnableTexCoords(const FRealtimeMeshElementType& ElementType, int32 NumChannels)
	{
		check(ElementType.IsValid());
		NumChannels = FMath::Clamp(NumChannels, 1, REALTIME_MESH_MAX_TEX_COORDS);

		// If already enabled with matching layout/channel count, nothing to do.
		if (HasTexCoords() && NumTexCoordChannels() == NumChannels)
		{
			const FRealtimeMeshStream& Existing = TexCoordChannels[0]->GetStream();
			if (Existing.GetLayout().GetElementType() == ElementType
				&& Existing.GetLayout().GetNumElements() == NumChannels)
			{
				return;
			}
		}

		// Otherwise, rebuild. First unbind any previous TexCoords stream from the
		// linkage so we don't leave a dangling pointer when we remove/replace it.
		if (HasTexCoords())
		{
			VerticesLinkage.RemoveStream(TexCoordChannels[0]->GetStream());
			TexCoordChannels.Reset();
			Streams.Remove(FRealtimeMeshStreams::TexCoords);
		}

		FRealtimeMeshStream& Stream = Streams.FindOrAdd(FRealtimeMeshStreams::TexCoords, FRealtimeMeshBufferLayout(ElementType, NumChannels));
		VerticesLinkage.BindStream(Stream, FRealtimeMeshStreamDefaultRowValue());
		RebuildTexCoordChannels();
	}

	void FRealtimeMeshDynamicBuilder::DisableTexCoords()
	{
		if (!HasTexCoords()) return;
		VerticesLinkage.RemoveStream(TexCoordChannels[0]->GetStream());
		TexCoordChannels.Reset();
		Streams.Remove(FRealtimeMeshStreams::TexCoords);
	}

	void FRealtimeMeshDynamicBuilder::RebuildTexCoordChannels()
	{
		TexCoordChannels.Reset();
		FRealtimeMeshStream* Stream = Streams.Find(FRealtimeMeshStreams::TexCoords);
		if (!Stream) return;

		const int32 NumChannels = Stream->GetLayout().GetNumElements();
		TexCoordChannels.Reserve(NumChannels);
		for (int32 Channel = 0; Channel < NumChannels; ++Channel)
		{
			TexCoordChannels.Add(MakeUnique<TRealtimeMeshStridedStreamBuilder<FVector2f, void>>(*Stream, Channel));
		}
	}

	const FRealtimeMeshStream* FRealtimeMeshDynamicBuilder::GetTexCoordsStream() const
	{
		return TexCoordChannels.Num() > 0 ? &TexCoordChannels[0]->GetStream() : nullptr;
	}

	void FRealtimeMeshDynamicBuilder::SetTriangleIndexType(const FRealtimeMeshElementType& IndexType)
	{
		check(IndexType.IsValid());
		check(TrianglesLinkage.GetNumRows() == 0); // Switching format with data present is unsafe; reset triangles first.
		FRealtimeMeshStream& Stream = Streams.FindOrAdd(FRealtimeMeshStreams::Triangles, FRealtimeMeshBufferLayout(IndexType, 3));
		if (!Stream.IsOfType(FRealtimeMeshBufferLayout(IndexType, 3)))
		{
			Stream.ConvertTo(FRealtimeMeshBufferLayout(IndexType, 3));
		}
		Triangles = MakeUnique<TRealtimeMeshStreamBuilder<TIndex3<uint32>, void>>(Stream);
	}

	void FRealtimeMeshDynamicBuilder::EnableDepthOnlyTriangles(const FRealtimeMeshElementType& IndexType)
	{
		check(IndexType.IsValid());
		if (HasDepthOnlyTriangles()) return;

		FRealtimeMeshStream& Stream = Streams.FindOrAdd(FRealtimeMeshStreams::DepthOnlyTriangles, FRealtimeMeshBufferLayout(IndexType, 3));
		DepthOnlyTriangles = MakeUnique<TRealtimeMeshStreamBuilder<TIndex3<uint32>, void>>(Stream);
		DepthOnlyTrianglesLinkage.BindStream(Stream, FRealtimeMeshStreamDefaultRowValue());

		// If poly groups are already enabled, the depth-only counterpart should
		// follow them — match the existing template builder's behavior.
		if (HasPolyGroups())
		{
			const FRealtimeMeshBufferLayout PolyLayout = TrianglePolyGroups->GetBufferLayout();
			FRealtimeMeshStream& PolyStream = Streams.FindOrAdd(FRealtimeMeshStreams::DepthOnlyPolyGroups, PolyLayout);
			DepthOnlyTrianglePolyGroups = MakeUnique<TRealtimeMeshStreamBuilder<uint32, void>>(PolyStream);
			DepthOnlyTrianglesLinkage.BindStream(PolyStream, FRealtimeMeshStreamDefaultRowValue());
		}
	}

	void FRealtimeMeshDynamicBuilder::DisableDepthOnlyTriangles()
	{
		if (DepthOnlyTrianglePolyGroups.IsValid())
		{
			DepthOnlyTrianglesLinkage.RemoveStream(DepthOnlyTrianglePolyGroups->GetStream());
			DepthOnlyTrianglePolyGroups.Reset();
			Streams.Remove(FRealtimeMeshStreams::DepthOnlyPolyGroups);
		}
		if (!HasDepthOnlyTriangles()) return;
		DepthOnlyTrianglesLinkage.RemoveStream(DepthOnlyTriangles->GetStream());
		DepthOnlyTriangles.Reset();
		Streams.Remove(FRealtimeMeshStreams::DepthOnlyTriangles);
	}

	void FRealtimeMeshDynamicBuilder::EnablePolyGroups(const FRealtimeMeshElementType& IndexType)
	{
		check(IndexType.IsValid());
		if (HasPolyGroups()) return;

		FRealtimeMeshStream& Stream = Streams.FindOrAdd(FRealtimeMeshStreams::PolyGroups, FRealtimeMeshBufferLayout(IndexType, 1));
		TrianglePolyGroups = MakeUnique<TRealtimeMeshStreamBuilder<uint32, void>>(Stream);
		TrianglesLinkage.BindStream(Stream, FRealtimeMeshStreamDefaultRowValue());

		if (HasDepthOnlyTriangles())
		{
			FRealtimeMeshStream& DepthPolyStream = Streams.FindOrAdd(FRealtimeMeshStreams::DepthOnlyPolyGroups, FRealtimeMeshBufferLayout(IndexType, 1));
			DepthOnlyTrianglePolyGroups = MakeUnique<TRealtimeMeshStreamBuilder<uint32, void>>(DepthPolyStream);
			DepthOnlyTrianglesLinkage.BindStream(DepthPolyStream, FRealtimeMeshStreamDefaultRowValue());
		}
	}

	void FRealtimeMeshDynamicBuilder::DisablePolyGroups()
	{
		if (TrianglePolyGroups.IsValid())
		{
			TrianglesLinkage.RemoveStream(TrianglePolyGroups->GetStream());
			TrianglePolyGroups.Reset();
			Streams.Remove(FRealtimeMeshStreams::PolyGroups);
		}
		if (DepthOnlyTrianglePolyGroups.IsValid())
		{
			DepthOnlyTrianglesLinkage.RemoveStream(DepthOnlyTrianglePolyGroups->GetStream());
			DepthOnlyTrianglePolyGroups.Reset();
			Streams.Remove(FRealtimeMeshStreams::DepthOnlyPolyGroups);
		}
	}

	void FRealtimeMeshDynamicBuilder::EmptyVertices()
	{
		// Empty via linkage — drops all rows on every vertex-side stream uniformly.
		VerticesLinkage.SetNumRows(0);
	}

	void FRealtimeMeshDynamicBuilder::EmptyTriangles()
	{
		TrianglesLinkage.SetNumRows(0);
	}

	void FRealtimeMeshDynamicBuilder::EmptyDepthOnlyTriangles()
	{
		DepthOnlyTrianglesLinkage.SetNumRows(0);
	}

	void FRealtimeMeshDynamicBuilder::Reset()
	{
		EmptyVertices();
		EmptyTriangles();
		EmptyDepthOnlyTriangles();
	}

	void FRealtimeMeshDynamicBuilder::SetPosition(int32 Row, const FVector3f& Value)
	{
		check(Vertices);
		Vertices->Edit(Row).Set(Value);
	}

	void FRealtimeMeshDynamicBuilder::SetTangents(int32 Row, const FVector3f& Normal, const FVector3f& Tangent)
	{
		check(Tangents);
		Tangents->Edit(Row).Set(TRealtimeMeshTangents<FVector4f>(Normal, Tangent));
	}

	void FRealtimeMeshDynamicBuilder::SetTangents(int32 Row, const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent)
	{
		check(Tangents);
		Tangents->Edit(Row).Set(TRealtimeMeshTangents<FVector4f>(Normal, Binormal, Tangent));
	}

	void FRealtimeMeshDynamicBuilder::SetColor(int32 Row, const FColor& Value)
	{
		check(Colors);
		Colors->Edit(Row).Set(Value);
	}

	void FRealtimeMeshDynamicBuilder::SetColor(int32 Row, const FLinearColor& Value)
	{
		check(Colors);
		Colors->Edit(Row).Set(Value.ToFColor(true));
	}

	void FRealtimeMeshDynamicBuilder::SetTexCoord(int32 Row, int32 Channel, const FVector2f& Value)
	{
		check(TexCoordChannels.IsValidIndex(Channel));
		TexCoordChannels[Channel]->Edit(Row).Set(Value);
	}

	void FRealtimeMeshDynamicBuilder::SetTriangle(int32 Row, uint32 A, uint32 B, uint32 C)
	{
		check(Triangles);
		Triangles->Edit(Row).Set(TIndex3<uint32>(A, B, C));
	}

	void FRealtimeMeshDynamicBuilder::SetTriangle(int32 Row, uint32 A, uint32 B, uint32 C, uint32 PolyGroup)
	{
		SetTriangle(Row, A, B, C);
		if (HasPolyGroups())
		{
			SetPolyGroup(Row, PolyGroup);
		}
	}

	void FRealtimeMeshDynamicBuilder::SetDepthOnlyTriangle(int32 Row, uint32 A, uint32 B, uint32 C)
	{
		check(DepthOnlyTriangles);
		DepthOnlyTriangles->Edit(Row).Set(TIndex3<uint32>(A, B, C));
	}

	void FRealtimeMeshDynamicBuilder::SetPolyGroup(int32 Row, uint32 PolyGroupIndex)
	{
		check(TrianglePolyGroups);
		TrianglePolyGroups->Edit(Row).Set(PolyGroupIndex);
	}

	void FRealtimeMeshDynamicBuilder::SetDepthOnlyPolyGroup(int32 Row, uint32 PolyGroupIndex)
	{
		check(DepthOnlyTrianglePolyGroups);
		DepthOnlyTrianglePolyGroups->Edit(Row).Set(PolyGroupIndex);
	}

	int32 FRealtimeMeshDynamicBuilder::AddVertex(const FVector3f& Position)
	{
		const int32 Row = VerticesLinkage.AddRowsZeroed(1);
		SetPosition(Row, Position);
		return Row;
	}

	int32 FRealtimeMeshDynamicBuilder::AddTriangle(uint32 A, uint32 B, uint32 C)
	{
		const int32 Row = TrianglesLinkage.AddRowsZeroed(1);
		SetTriangle(Row, A, B, C);
		return Row;
	}

	int32 FRealtimeMeshDynamicBuilder::AddTriangle(uint32 A, uint32 B, uint32 C, uint32 PolyGroup)
	{
		const int32 Row = TrianglesLinkage.AddRowsZeroed(1);
		SetTriangle(Row, A, B, C, PolyGroup);
		return Row;
	}

	int32 FRealtimeMeshDynamicBuilder::AddDepthOnlyTriangle(uint32 A, uint32 B, uint32 C)
	{
		const int32 Row = DepthOnlyTrianglesLinkage.AddRowsZeroed(1);
		SetDepthOnlyTriangle(Row, A, B, C);
		return Row;
	}

	FVector3f FRealtimeMeshDynamicBuilder::GetPosition(int32 Row) const
	{
		check(Vertices);
		return Vertices->Get(Row).Get();
	}

	FVector3f FRealtimeMeshDynamicBuilder::GetNormal(int32 Row) const
	{
		check(Tangents);
		return Tangents->Get(Row).Get().GetNormal();
	}

	FVector3f FRealtimeMeshDynamicBuilder::GetTangent(int32 Row) const
	{
		check(Tangents);
		return Tangents->Get(Row).Get().GetTangent();
	}

	FColor FRealtimeMeshDynamicBuilder::GetColor(int32 Row) const
	{
		check(Colors);
		return Colors->Get(Row).Get();
	}

	FVector2f FRealtimeMeshDynamicBuilder::GetTexCoord(int32 Row, int32 Channel) const
	{
		check(TexCoordChannels.IsValidIndex(Channel));
		return TexCoordChannels[Channel]->Get(Row).Get();
	}

	TIndex3<uint32> FRealtimeMeshDynamicBuilder::GetTriangle(int32 Row) const
	{
		check(Triangles);
		return Triangles->Get(Row).Get();
	}

	TIndex3<uint32> FRealtimeMeshDynamicBuilder::GetDepthOnlyTriangle(int32 Row) const
	{
		check(DepthOnlyTriangles);
		return DepthOnlyTriangles->Get(Row).Get();
	}

	uint32 FRealtimeMeshDynamicBuilder::GetPolyGroup(int32 Row) const
	{
		check(TrianglePolyGroups);
		return TrianglePolyGroups->Get(Row).Get();
	}

	uint32 FRealtimeMeshDynamicBuilder::GetDepthOnlyPolyGroup(int32 Row) const
	{
		check(DepthOnlyTrianglePolyGroups);
		return DepthOnlyTrianglePolyGroups->Get(Row).Get();
	}
}
