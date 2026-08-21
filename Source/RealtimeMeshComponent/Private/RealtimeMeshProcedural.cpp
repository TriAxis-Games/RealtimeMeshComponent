// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshProcedural.h"

#include "RealtimeMeshComponent.h"
#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshBuilder.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "Core/RealtimeMeshCollision.h"

using namespace RealtimeMesh;

namespace
{
	// PMC uses 4 UV channels; the procedural layout exposes the same count
	// to make migration find-and-replace work for the LinearColor variants.
	using FProceduralBuilder = TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 4, uint16>;

	static const FName GProceduralGroupSlotName(TEXT("PMC"));

	// Shared core of every Create/Update path. Writes whichever input arrays are
	// non-empty into a fresh FRealtimeMeshStreamSet shaped for the LVF layout.
	// Caller is responsible for skipping Triangles on the update path (pass an
	// empty TArrayView there).
	//
	// bEnsureTangents / bEnsureColors force those streams to exist even when no
	// data is supplied (the create path wants a complete vertex layout). The
	// update path passes false so an unsupplied attribute leaves the existing
	// stream untouched rather than stomping it with defaults. Note that normals
	// and tangents share one packed stream, so supplying either one rewrites the
	// whole basis (the missing half falls back to its default).
	void PopulateProceduralStreams(
		FRealtimeMeshStreamSet& OutStreams,
		const TArray<FVector>& Vertices,
		TArrayView<const int32> Triangles,
		const TArray<FVector>& Normals,
		const TArray<FRealtimeMeshProceduralTangent>& Tangents,
		const TArray<FColor>& Colors,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FVector2D>& UV2,
		const TArray<FVector2D>& UV3,
		bool bEnsureTangents = true,
		bool bEnsureColors = true)
	{
		FProceduralBuilder Builder(OutStreams);

		if (bEnsureTangents || Normals.Num() > 0 || Tangents.Num() > 0)
		{
			Builder.EnableTangents();
		}
		if (bEnsureColors || Colors.Num() > 0)
		{
			Builder.EnableColors();
		}

		int32 NumActiveUVChannels = 0;
		if (UV3.Num() > 0) { NumActiveUVChannels = 4; }
		else if (UV2.Num() > 0) { NumActiveUVChannels = 3; }
		else if (UV1.Num() > 0) { NumActiveUVChannels = 2; }
		else if (UV0.Num() > 0) { NumActiveUVChannels = 1; }

		// The fixed LVF layout this builder uses always carries 4 UV channels, and
		// EnableTexCoords requires the full count to be enabled at once. So when any
		// UVs are supplied we allocate all four channels and only fill the ones we
		// have data for below; the rest stay at their default (0,0).
		if (NumActiveUVChannels > 0)
		{
			Builder.EnableTexCoords();
		}

		const int32 NumVerts = Vertices.Num();
		Builder.ReserveNumVertices(NumVerts);

		for (int32 i = 0; i < NumVerts; ++i)
		{
			const int32 Row = Builder.AddVertex(FVector3f(Vertices[i])).GetIndex();

			if (i < Normals.Num())
			{
				if (i < Tangents.Num())
				{
					Builder.SetNormalAndTangent(Row, FVector3f(Normals[i]), FVector3f(Tangents[i].TangentX), Tangents[i].bFlipTangentY);
				}
				else
				{
					Builder.SetNormal(Row, FVector3f(Normals[i]));
				}
			}
			else if (i < Tangents.Num())
			{
				Builder.SetTangent(Row, FVector3f(Tangents[i].TangentX));
			}

			if (i < Colors.Num())
			{
				Builder.SetColor(Row, Colors[i]);
			}

			if (NumActiveUVChannels >= 1 && i < UV0.Num()) { Builder.SetTexCoord(Row, 0, FVector2f(UV0[i])); }
			if (NumActiveUVChannels >= 2 && i < UV1.Num()) { Builder.SetTexCoord(Row, 1, FVector2f(UV1[i])); }
			if (NumActiveUVChannels >= 3 && i < UV2.Num()) { Builder.SetTexCoord(Row, 2, FVector2f(UV2[i])); }
			if (NumActiveUVChannels >= 4 && i < UV3.Num()) { Builder.SetTexCoord(Row, 3, FVector2f(UV3[i])); }
		}

		const int32 NumTriIndices = Triangles.Num();
		const int32 NumTris = NumTriIndices / 3;
		Builder.ReserveNumTriangles(NumTris);
		for (int32 Tri = 0; Tri < NumTris; ++Tri)
		{
			const int32 Base = Tri * 3;
			Builder.AddTriangle(
				static_cast<uint32>(Triangles[Base]),
				static_cast<uint32>(Triangles[Base + 1]),
				static_cast<uint32>(Triangles[Base + 2]));
		}
	}

	TArray<FColor> ConvertLinearColors(const TArray<FLinearColor>& In, bool bSRGB)
	{
		TArray<FColor> Out;
		Out.SetNumUninitialized(In.Num());
		for (int32 i = 0; i < In.Num(); ++i)
		{
			Out[i] = In[i].ToFColor(bSRGB);
		}
		return Out;
	}

	// DUP-010: shared create-path body. CreateMeshSection and CreateMeshSection_LinearColor
	// differ only in how they source the FColor stream (passed through vs converted from
	// FLinearColor) and how many UV channels they expose; both feed the identical
	// stream-build + three-task commit below. The thin public wrappers do the color/UV
	// prep and forward the already-derived keys here.
	void BuildAndCommitProceduralCreate(
		const TSharedRef<FRealtimeMeshProcedural>& MeshData,
		const FRealtimeMeshBufferSetKey& GroupKey,
		const FRealtimeMeshSectionKey& SectionKey,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FColor>& VertexColors,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FVector2D>& UV2,
		const TArray<FVector2D>& UV3,
		const TArray<FRealtimeMeshProceduralTangent>& Tangents,
		bool bCreateCollision)
	{
		FRealtimeMeshStreamSet StreamSet;
		PopulateProceduralStreams(StreamSet, Vertices, Triangles, Normals, Tangents, VertexColors, UV0, UV1, UV2, UV3);

		FRealtimeMeshUpdateBuilder UpdateBuilder;

		UpdateBuilder.AddLODTask<FRealtimeMeshLODManaged>(GroupKey, [GroupKey](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODManaged& LOD)
		{
			// Dynamic draw type keeps this group off the reallocating, scene-proxy-recreating
			// path so per-frame UpdateMeshSection stays cheap (matches PMC's local-update model).
			LOD.CreateOrUpdateSectionGroup(UpdateContext, GroupKey, FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType::Dynamic));
		});

		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetManaged>(GroupKey,
			[Streams = MoveTemp(StreamSet), SectionKey](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetManaged& SectionGroup) mutable
		{
			SectionGroup.SetAllStreams(UpdateContext, MoveTemp(Streams));
			SectionGroup.CreateOrUpdateSection(UpdateContext, SectionKey, FRealtimeMeshSectionConfig(), SectionGroup.GetValidStreamRange(UpdateContext));
		});

		UpdateBuilder.AddSectionTask<FRealtimeMeshSectionManaged>(SectionKey,
			[bCreateCollision](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionManaged& Section)
		{
			Section.SetShouldCreateCollision(UpdateContext, bCreateCollision);
		});

		UpdateBuilder.Commit(MeshData);
	}

	// DUP-010: shared update-path body (includes the verbatim per-stream fast-path lambda).
	// UpdateMeshSection and UpdateMeshSection_LinearColor differ only in the FColor source
	// and UV-channel count; everything from the stream build through the commit is identical.
	void BuildAndCommitProceduralUpdate(
		const TSharedRef<FRealtimeMeshProcedural>& MeshData,
		const FRealtimeMeshBufferSetKey& GroupKey,
		const TArray<FVector>& Vertices,
		const TArray<FVector>& Normals,
		const TArray<FColor>& VertexColors,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FVector2D>& UV2,
		const TArray<FVector2D>& UV3,
		const TArray<FRealtimeMeshProceduralTangent>& Tangents)
	{
		const int32 NewVertexCount = Vertices.Num();

		FRealtimeMeshStreamSet StreamSet;
		const TArray<int32> EmptyTris;
		PopulateProceduralStreams(StreamSet, Vertices, EmptyTris, Normals, Tangents, VertexColors, UV0, UV1, UV2, UV3,
			/*bEnsureTangents*/ false, /*bEnsureColors*/ false);

		// Drop the (empty) Triangles stream the builder may have created so we don't
		// stomp the existing topology.
		StreamSet.Remove(FRealtimeMeshStreams::Triangles);

		FRealtimeMeshUpdateBuilder UpdateBuilder;
		UpdateBuilder.AddSectionGroupTask<FRealtimeMeshBufferSetManaged>(GroupKey,
			[Streams = MoveTemp(StreamSet), NewVertexCount](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSetManaged& SectionGroup) mutable
		{
			// Topology (Triangles) and the section range are left untouched by an update,
			// so writing a different vertex count would leave the retained indices dangling.
			// Match PMC: only apply when the vertex count is unchanged; otherwise silently no-op.
			const FRealtimeMeshStream* ExistingPosition = SectionGroup.GetStream(UpdateContext, FRealtimeMeshStreams::Position);
			if (!ExistingPosition || ExistingPosition->Num() != NewVertexCount)
			{
				return;
			}

			// Streams is owned by this (moved-in, mutable) lambda and is discarded afterwards,
			// so move each stream straight into the section group instead of copying a second time.
			// Attribute streams that already exist with an unchanged element count take the in-place
			// fast path (no GPU buffer realloc, no publish); anything else falls back to the
			// reallocating update.
			Streams.ForEach([&](FRealtimeMeshStream& Stream)
			{
				const FRealtimeMeshStream* Existing = SectionGroup.GetStream(UpdateContext, Stream.GetStreamKey());
				if (Existing && Existing->Num() == Stream.Num())
				{
					SectionGroup.FastUpdateStream(UpdateContext, MoveTemp(Stream), FInt32Range::Empty());
				}
				else
				{
					SectionGroup.CreateOrUpdateStream(UpdateContext, MoveTemp(Stream));
				}
			});
		});

		UpdateBuilder.Commit(MeshData);
	}
}


// -------- URealtimeMeshProcedural --------

URealtimeMeshProcedural::URealtimeMeshProcedural(const FObjectInitializer& ObjectInitializer)
	: URealtimeMeshManaged(ObjectInitializer)
{
	if (!IsTemplate())
	{
		const auto SR = MakeShared<RealtimeMesh::FRealtimeMeshContext>();
		Initialize(SR, MakeShared<RealtimeMesh::FRealtimeMeshProcedural>(SR));

		FRealtimeMeshUpdateContext UpdateContext(GetMesh());
		MeshRef->InitializeLODs(UpdateContext, RealtimeMesh::TFixedLODArray<FRealtimeMeshLODConfig>{FRealtimeMeshLODConfig()});
	}
}

URealtimeMeshProcedural* URealtimeMeshProcedural::InitializeRealtimeMeshProcedural(URealtimeMeshComponent* Owner)
{
	if (IsValid(Owner))
	{
		return Owner->InitializeRealtimeMesh<URealtimeMeshProcedural>();
	}
	return nullptr;
}


FRealtimeMeshBufferSetKey URealtimeMeshProcedural::AllocateOrGetGroupKey(int32 SectionIndex)
{
	if (const FRealtimeMeshBufferSetKey* Existing = SectionGroupByIndex.Find(SectionIndex))
	{
		return *Existing;
	}

	const FRealtimeMeshBufferSetKey NewKey = FRealtimeMeshBufferSetKey::Create(FRealtimeMeshLODKey(0), SectionIndex, GProceduralGroupSlotName);
	SectionGroupByIndex.Add(SectionIndex, NewKey);
	if (SectionIndex + 1 > MaxAllocatedIndex)
	{
		MaxAllocatedIndex = SectionIndex + 1;
	}
	return NewKey;
}

bool URealtimeMeshProcedural::TryGetGroupKey(int32 SectionIndex, FRealtimeMeshBufferSetKey& OutKey) const
{
	if (const FRealtimeMeshBufferSetKey* Existing = SectionGroupByIndex.Find(SectionIndex))
	{
		OutKey = *Existing;
		return true;
	}
	return false;
}

FRealtimeMeshSectionKey URealtimeMeshProcedural::MakeSectionKey(const FRealtimeMeshBufferSetKey& GroupKey) const
{
	// Section-key identity is (LOD, SlotIndex) only — BufferSetSlotIndex is NOT part
	// of equality. So every section must get a distinct SlotIndex, or sections in
	// different groups collide in section-keyed sets/maps (dirty trees, proxy command
	// routing) and clearing one corrupts another. Deriving the name from the group's
	// index gives a unique slot per section, and this overload records the correct
	// BufferSetSlotIndex (GroupKey.Index()) so the IsPartOf check still passes.
	return FRealtimeMeshSectionKey::Create(GroupKey, GroupKey.Index());
}


void URealtimeMeshProcedural::CreateMeshSection(int32 SectionIndex,
	const TArray<FVector>& Vertices,
	const TArray<int32>& Triangles,
	const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0,
	const TArray<FColor>& VertexColors,
	const TArray<FRealtimeMeshProceduralTangent>& Tangents,
	bool bCreateCollision)
{
	if (SectionIndex < 0 || Vertices.Num() == 0 || Triangles.Num() % 3 != 0)
	{
		return;
	}

	const FRealtimeMeshBufferSetKey GroupKey = AllocateOrGetGroupKey(SectionIndex);
	const FRealtimeMeshSectionKey SectionKey = MakeSectionKey(GroupKey);

	// DUP-010: single-UV FColor variant — pass UV0 plus empty UV1-3 to the shared body.
	const TArray<FVector2D> Empty2D;
	BuildAndCommitProceduralCreate(GetProceduralMeshData(), GroupKey, SectionKey,
		Vertices, Triangles, Normals, VertexColors, UV0, Empty2D, Empty2D, Empty2D, Tangents, bCreateCollision);
}


void URealtimeMeshProcedural::CreateMeshSection_LinearColor(int32 SectionIndex,
	const TArray<FVector>& Vertices,
	const TArray<int32>& Triangles,
	const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1,
	const TArray<FVector2D>& UV2,
	const TArray<FVector2D>& UV3,
	const TArray<FLinearColor>& VertexColors,
	const TArray<FRealtimeMeshProceduralTangent>& Tangents,
	bool bCreateCollision,
	bool bSRGBConversion)
{
	if (SectionIndex < 0 || Vertices.Num() == 0 || Triangles.Num() % 3 != 0)
	{
		return;
	}

	const FRealtimeMeshBufferSetKey GroupKey = AllocateOrGetGroupKey(SectionIndex);
	const FRealtimeMeshSectionKey SectionKey = MakeSectionKey(GroupKey);

	// DUP-010: only the color conversion (FLinearColor->FColor, honoring bSRGBConversion)
	// and the full four-UV set distinguish this from CreateMeshSection.
	const TArray<FColor> Colors = ConvertLinearColors(VertexColors, bSRGBConversion);
	BuildAndCommitProceduralCreate(GetProceduralMeshData(), GroupKey, SectionKey,
		Vertices, Triangles, Normals, Colors, UV0, UV1, UV2, UV3, Tangents, bCreateCollision);
}


void URealtimeMeshProcedural::UpdateMeshSection(int32 SectionIndex,
	const TArray<FVector>& Vertices,
	const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0,
	const TArray<FColor>& VertexColors,
	const TArray<FRealtimeMeshProceduralTangent>& Tangents)
{
	FRealtimeMeshBufferSetKey GroupKey;
	if (!TryGetGroupKey(SectionIndex, GroupKey))
	{
		return;
	}

	// DUP-010: single-UV FColor variant — pass UV0 plus empty UV1-3 to the shared body.
	const TArray<FVector2D> Empty2D;
	BuildAndCommitProceduralUpdate(GetProceduralMeshData(), GroupKey,
		Vertices, Normals, VertexColors, UV0, Empty2D, Empty2D, Empty2D, Tangents);
}


void URealtimeMeshProcedural::UpdateMeshSection_LinearColor(int32 SectionIndex,
	const TArray<FVector>& Vertices,
	const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1,
	const TArray<FVector2D>& UV2,
	const TArray<FVector2D>& UV3,
	const TArray<FLinearColor>& VertexColors,
	const TArray<FRealtimeMeshProceduralTangent>& Tangents,
	bool bSRGBConversion)
{
	FRealtimeMeshBufferSetKey GroupKey;
	if (!TryGetGroupKey(SectionIndex, GroupKey))
	{
		return;
	}

	// DUP-010: only the color conversion (FLinearColor->FColor, honoring bSRGBConversion)
	// and the full four-UV set distinguish this from UpdateMeshSection.
	const TArray<FColor> Colors = ConvertLinearColors(VertexColors, bSRGBConversion);
	BuildAndCommitProceduralUpdate(GetProceduralMeshData(), GroupKey,
		Vertices, Normals, Colors, UV0, UV1, UV2, UV3, Tangents);
}


void URealtimeMeshProcedural::ClearMeshSection(int32 SectionIndex)
{
	FRealtimeMeshBufferSetKey GroupKey;
	if (!TryGetGroupKey(SectionIndex, GroupKey))
	{
		return;
	}

	FRealtimeMeshUpdateBuilder UpdateBuilder;
	UpdateBuilder.AddLODTask<FRealtimeMeshLODManaged>(GroupKey,
		[GroupKey](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODManaged& LOD)
	{
		LOD.RemoveSectionGroup(UpdateContext, GroupKey);
	});

	UpdateBuilder.Commit(GetProceduralMeshData());

	SectionGroupByIndex.Remove(SectionIndex);
	// MaxAllocatedIndex intentionally not decremented — matches PMC hole semantics.
}


void URealtimeMeshProcedural::ClearAllMeshSections()
{
	TArray<int32> Indices;
	SectionGroupByIndex.GetKeys(Indices);

	// Batch every section-group removal into a single builder/commit (one publish) rather than
	// committing once per section as ClearMeshSection() does.
	FRealtimeMeshUpdateBuilder UpdateBuilder;
	for (int32 Index : Indices)
	{
		FRealtimeMeshBufferSetKey GroupKey;
		if (!TryGetGroupKey(Index, GroupKey))
		{
			continue;
		}

		UpdateBuilder.AddLODTask<FRealtimeMeshLODManaged>(GroupKey,
			[GroupKey](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODManaged& LOD)
		{
			LOD.RemoveSectionGroup(UpdateContext, GroupKey);
		});

		SectionGroupByIndex.Remove(Index);
	}

	UpdateBuilder.Commit(GetProceduralMeshData());

	MaxAllocatedIndex = 0;
}


void URealtimeMeshProcedural::SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility)
{
	FRealtimeMeshBufferSetKey GroupKey;
	if (!TryGetGroupKey(SectionIndex, GroupKey))
	{
		return;
	}
	SetSectionVisibility(MakeSectionKey(GroupKey), bNewVisibility);
}


bool URealtimeMeshProcedural::IsMeshSectionVisible(int32 SectionIndex) const
{
	FRealtimeMeshBufferSetKey GroupKey;
	if (!TryGetGroupKey(SectionIndex, GroupKey))
	{
		return false;
	}
	return IsSectionVisible(MakeSectionKey(GroupKey));
}


int32 URealtimeMeshProcedural::GetNumSections() const
{
	return MaxAllocatedIndex;
}


bool URealtimeMeshProcedural::GetMeshSection(int32 SectionIndex,
	TArray<FVector>& Vertices,
	TArray<int32>& Triangles,
	TArray<FVector>& Normals,
	TArray<FVector2D>& UV0,
	TArray<FVector2D>& UV1,
	TArray<FVector2D>& UV2,
	TArray<FVector2D>& UV3,
	TArray<FColor>& VertexColors,
	TArray<FRealtimeMeshProceduralTangent>& Tangents) const
{
	Vertices.Reset();
	Triangles.Reset();
	Normals.Reset();
	UV0.Reset();
	UV1.Reset();
	UV2.Reset();
	UV3.Reset();
	VertexColors.Reset();
	Tangents.Reset();

	FRealtimeMeshBufferSetKey GroupKey;
	if (!TryGetGroupKey(SectionIndex, GroupKey))
	{
		return false;
	}

	bool bFound = false;

	FRealtimeMeshAccessor Accessor;
	Accessor.AddSectionGroupTask<FRealtimeMeshBufferSetManaged>(GroupKey,
		[&](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetManaged& SectionGroup)
	{
		if (!SectionGroup.HasStreams(LockContext))
		{
			return;
		}

		// Copy the section group's streams into a local set so we can drive the
		// fixed-layout reader builder over them (it needs a mutable set and decodes
		// the packed tangent/half-UV streams back into float vectors for us).
		FRealtimeMeshStreamSet LocalStreams;
		for (const FRealtimeMeshStreamKey& Key : SectionGroup.GetStreamKeys(LockContext))
		{
			if (const FRealtimeMeshStream* Stream = SectionGroup.GetStream(LockContext, Key))
			{
				LocalStreams.AddStream(*Stream);
			}
		}

		FProceduralBuilder Reader(LocalStreams);

		const int32 NumVerts = Reader.NumVertices();
		const bool bHasNormalsAndTangents = Reader.HasTangents();
		const bool bHasColors = Reader.HasVertexColors();
		const int32 NumUVChannels = Reader.NumTexCoordChannels();

		Vertices.SetNumUninitialized(NumVerts);
		if (bHasNormalsAndTangents)
		{
			Normals.SetNumUninitialized(NumVerts);
			Tangents.SetNumUninitialized(NumVerts);
		}
		if (bHasColors)
		{
			VertexColors.SetNumUninitialized(NumVerts);
		}
		if (NumUVChannels >= 1) { UV0.SetNumUninitialized(NumVerts); }
		if (NumUVChannels >= 2) { UV1.SetNumUninitialized(NumVerts); }
		if (NumUVChannels >= 3) { UV2.SetNumUninitialized(NumVerts); }
		if (NumUVChannels >= 4) { UV3.SetNumUninitialized(NumVerts); }

		for (int32 i = 0; i < NumVerts; ++i)
		{
			Vertices[i] = FVector(Reader.GetPosition(i));

			if (bHasNormalsAndTangents)
			{
				Normals[i] = FVector(Reader.GetNormal(i));
				Tangents[i] = FRealtimeMeshProceduralTangent(FVector(Reader.GetTangent(i)), false);
			}
			if (bHasColors)
			{
				VertexColors[i] = Reader.GetColor(i);
			}
			if (NumUVChannels >= 1) { UV0[i] = FVector2D(Reader.GetTexCoord(i, 0)); }
			if (NumUVChannels >= 2) { UV1[i] = FVector2D(Reader.GetTexCoord(i, 1)); }
			if (NumUVChannels >= 3) { UV2[i] = FVector2D(Reader.GetTexCoord(i, 2)); }
			if (NumUVChannels >= 4) { UV3[i] = FVector2D(Reader.GetTexCoord(i, 3)); }
		}

		const int32 NumTris = Reader.NumTriangles();
		Triangles.Reserve(NumTris * 3);
		for (int32 Tri = 0; Tri < NumTris; ++Tri)
		{
			const TIndex3<uint32> Indices = Reader.GetTriangle(Tri);
			Triangles.Add(static_cast<int32>(Indices.V0));
			Triangles.Add(static_cast<int32>(Indices.V1));
			Triangles.Add(static_cast<int32>(Indices.V2));
		}

		bFound = true;
	});
	Accessor.Execute(GetProceduralMeshData());

	return bFound;
}


void URealtimeMeshProcedural::AddCollisionConvexMesh(const TArray<FVector>& ConvexVerts)
{
	if (ConvexVerts.Num() < 4)
	{
		return;
	}
	FRealtimeMeshSimpleGeometry Geo = GetSimpleGeometry();
	FRealtimeMeshCollisionConvex Convex;
	Convex.SetVertices(ConvexVerts);
	Geo.ConvexHulls.Add(Convex);
	SetSimpleGeometry(Geo);
}


void URealtimeMeshProcedural::ClearCollisionConvexMeshes()
{
	FRealtimeMeshSimpleGeometry Geo = GetSimpleGeometry();
	Geo.ConvexHulls = FSimpleShapeSet<FRealtimeMeshCollisionConvex>();
	SetSimpleGeometry(Geo);
}


bool URealtimeMeshProcedural::GetUseComplexAsSimpleCollision() const
{
	return GetCollisionConfig().bUseComplexAsSimpleCollision;
}


void URealtimeMeshProcedural::SetUseComplexAsSimpleCollision(bool bNewUseComplexAsSimpleCollision)
{
	FRealtimeMeshCollisionConfiguration Config = GetCollisionConfig();
	if (Config.bUseComplexAsSimpleCollision != bNewUseComplexAsSimpleCollision)
	{
		Config.bUseComplexAsSimpleCollision = bNewUseComplexAsSimpleCollision;
		SetCollisionConfig(Config);
	}
}


void URealtimeMeshProcedural::Reset()
{
	Super::Reset();
	SectionGroupByIndex.Reset();
	MaxAllocatedIndex = 0;
}


void URealtimeMeshProcedural::PostLoad()
{
	// Base URealtimeMeshManaged::PostLoad kicks off the committed collision
	// rebuild (API-H2) — preserve it.
	Super::PostLoad();

	// SectionGroupByIndex / MaxAllocatedIndex are Transient (the group-key USTRUCT
	// has no UPROPERTY members, so it can't be serialized meaningfully). Rebuild the
	// PMC-parity bookkeeping from the loaded section groups instead — this also
	// repairs assets saved before this fix. AllocateOrGetGroupKey creates each group
	// as Create(LODKey(0), SectionIndex, "PMC"), so GroupKey.Index() == SectionIndex.
	//
	// This rebuild is only correct because the group-key SlotIndex now round-trips
	// faithfully (FRealtimeMeshVersion::SerializeSectionGroupKeySlotIndex): every group
	// is named "PMC", so before that bump the load path re-derived SlotIndex from
	// CRC32("PMC") for every group, collapsing them to one identical key and making the
	// GroupKey.Index() == SectionIndex assumption below false. With distinct SlotIndex
	// preserved, all groups survive load and the scan reconstructs one entry per section.
	SectionGroupByIndex.Reset();
	MaxAllocatedIndex = 0;

	if (IsTemplate())
	{
		return;
	}

	const TArray<FRealtimeMeshBufferSetKey> Groups = GetBufferSets(FRealtimeMeshLODKey(0));
	for (const FRealtimeMeshBufferSetKey& GroupKey : Groups)
	{
		if (GroupKey.Name() == GProceduralGroupSlotName)
		{
			const int32 SectionIndex = GroupKey.Index();
			SectionGroupByIndex.Add(SectionIndex, GroupKey);
			if (SectionIndex + 1 > MaxAllocatedIndex)
			{
				MaxAllocatedIndex = SectionIndex + 1;
			}
		}
	}
}
