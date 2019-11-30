// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Components/MeshComponent.h"
#include "RealtimeMeshRendering.h"
#include "RealtimeMeshRenderable.h"

class FRealtimeMeshSectionProxy;


struct FRealtimeMeshSectionNullBufferElement
{
	FPackedNormal Normal;
	FPackedNormal Tangent;
	FColor Color;
	FVector2DHalf UV0;

	FRealtimeMeshSectionNullBufferElement()
		: Normal(FVector(0.0f, 0.0f, 1.0f))
		, Tangent(FVector(1.0f, 0.0f, 0.0f))
		, Color(FColor::Transparent)
		, UV0(FVector2D::ZeroVector)
	{ }
};


class FRealtimeMeshSectionProxy
{
	/** This stores all this sections config*/
	FRealtimeMeshSectionProperties Properties;

	/** Feature level in use, needed for recreating LODs vertex factories */
	ERHIFeatureLevel::Type FeatureLevel;

	/** Vertex factory for this section */
	FRealtimeMeshVertexFactory VertexFactory;

	/** Vertex buffer containing the positions for this section */
	FRealtimeMeshPositionVertexBuffer PositionBuffer;

	/** Vertex buffer containing the tangents for this section */
	FRealtimeMeshTangentsVertexBuffer TangentsBuffer;

	/** Vertex buffer containing the UVs for this section */
	FRealtimeMeshUVsVertexBuffer UVsBuffer;

	/** Vertex buffer containing the colors for this section */
	FRealtimeMeshColorVertexBuffer ColorBuffer;

	/** Index buffer for this section */
	FRealtimeMeshIndexBuffer IndexBuffer;

	/** Index buffer for this section */
	FRealtimeMeshIndexBuffer AdjacencyIndexBuffer;

public:

	FRealtimeMeshSectionProxy(ERHIFeatureLevel::Type InFeatureLevel, const FRealtimeMeshSectionProperties& SectionProperties)
		: Properties(SectionProperties)
		, FeatureLevel(InFeatureLevel)
		, VertexFactory(InFeatureLevel, this)
		, PositionBuffer(SectionProperties.UpdateFrequency)
		, TangentsBuffer(SectionProperties.UpdateFrequency, SectionProperties.bUseHighPrecisionTangents)
		, UVsBuffer(SectionProperties.UpdateFrequency, SectionProperties.bUseHighPrecisionTexCoords, SectionProperties.NumTexCoords)
		, ColorBuffer(SectionProperties.UpdateFrequency)
		, IndexBuffer(SectionProperties.UpdateFrequency, SectionProperties.bWants32BitIndices)
		, AdjacencyIndexBuffer(SectionProperties.UpdateFrequency, SectionProperties.bWants32BitIndices)
	{

	}

	~FRealtimeMeshSectionProxy()
	{
		check(IsInRenderingThread());

		Reset();
	}
	
	void Reset();

	bool CanRender() const;
	bool ShouldRender() const { return Properties.bIsVisible && CanRender(); }
	bool WantsToRenderInStaticPath() const { return Properties.UpdateFrequency == ERealtimeMeshUpdateFrequency::Infrequent; }
	bool CastsShadow() const { return Properties.bCastsShadow; }

	FRealtimeMeshVertexFactory* GetVertexFactory() { return &VertexFactory; }
	void BuildVertexDataType(FLocalVertexFactory::FDataType& DataType);

	void CreateMeshBatch(FMeshBatch& MeshBatch, bool bCastsShadow, bool bWantsAdjacencyInfo) const;

	void UpdateProperties_RenderThread(const FRealtimeMeshSectionProperties& SectionProperties);
	void UpdateSection_RenderThread(const FRealtimeMeshRenderableMeshData& MeshData);
	void ClearSection_RenderThread();
};




class FRealtimeMeshLODProxy : public TSharedFromThis<FRealtimeMeshLODProxy>
{
	/** Sections for this LOD, these do not have to be configured the same as sections in other LODs */
	TMap<int32, FRealtimeMeshSectionProxyPtr> Sections;

	FRealtimeMeshLODProperties Properties;

	ERHIFeatureLevel::Type FeatureLevel;

public:
	FRealtimeMeshLODProxy(ERHIFeatureLevel::Type InFeatureLevel);
	~FRealtimeMeshLODProxy();

	TMap<int32, FRealtimeMeshSectionProxyPtr>& GetSections() { return Sections; }

	bool CanRender() const;
	bool HasAnyStaticPath() const;
	bool HasAnyDynamicPath() const;
	bool HasAnyShadowCasters() const;


	void UpdateProperties_RenderThread(const FRealtimeMeshLODProperties& InProperties);
	void Clear_RenderThread();

	void CreateSection_RenderThread(int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties);
	void RemoveSection_RenderThread(int32 SectionId);

	void UpdateSectionProperties_RenderThread(int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties);
	void UpdateSectionMesh_RenderThread(int32 SectionId, const FRealtimeMeshRenderableMeshData& MeshData);
	void ClearSectionMesh_RenderThread(int32 SectionId);
};


