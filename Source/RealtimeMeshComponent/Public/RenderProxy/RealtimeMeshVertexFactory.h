// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshGPUBuffer.h"
#include "ShaderParameters.h"
#include "Components.h"
#include "VertexFactory.h"
#include "Data/RealtimeMeshConfig.h"


struct FRealtimeMeshStreamRange;
class FMaterial;
class FSceneView;
struct FMeshBatchElement;


namespace RealtimeMesh
{
	extern REALTIMEMESHCOMPONENT_API TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters> CreateRealtimeMeshVFUniformBuffer(const class FRealtimeMeshLocalVertexFactory* VertexFactory, uint32 LODLightmapDataIndex);


	class REALTIMEMESHCOMPONENT_API FRealtimeMeshVertexFactory : public FVertexFactory
	{
	
	public:
		FRealtimeMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) 
			: FVertexFactory(InFeatureLevel)
		{
		}
	
		virtual EPrimitiveType GetPrimitiveType() const { return PT_TriangleList; }

		virtual FIndexBuffer& GetIndexBuffer(bool& bDepthOnly, bool& bMatrixInverted, TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const = 0;

		virtual FRealtimeMeshStreamRange GetValidRange() const = 0;
		virtual bool IsValidStreamRange(const FRealtimeMeshStreamRange& StreamRange) const = 0;

		virtual void Initialize(const TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>& Buffers) = 0;

		virtual FRHIUniformBuffer* GetUniformBuffer() const = 0;

		virtual bool GatherVertexBufferResources(TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const = 0;
	protected:
		static TSharedPtr<FRealtimeMeshGPUBuffer> FindBuffer(const FRealtimeMeshGPUBufferMap& Buffers, ERealtimeMeshStreamType StreamType, FName BufferName)
		{
			const FRealtimeMeshStreamKey Key(StreamType, BufferName);
			const TSharedPtr<FRealtimeMeshGPUBuffer>* FoundBuffer = Buffers.Find(Key);
			return FoundBuffer? *FoundBuffer : TSharedPtr<FRealtimeMeshGPUBuffer>();			
		}
		
		static void BindVertexBufferSRV(bool& bIsValid, FRHIShaderResourceView*& OutStreamSRV, const FRealtimeMeshGPUBufferMap& Buffers, FName BufferName, bool bIsOptional = false)
		{
			const TSharedPtr<FRealtimeMeshGPUBuffer> FoundBuffer = FindBuffer(Buffers, ERealtimeMeshStreamType::Vertex, BufferName);

			if (!FoundBuffer.IsValid())
			{
				// If the buffer isn't optional, invalidate the result
				bIsValid &= bIsOptional;
				return;
			}

			const TSharedPtr<FRealtimeMeshVertexBuffer> VertexBuffer = StaticCastSharedPtr<FRealtimeMeshVertexBuffer>(FoundBuffer);
			OutStreamSRV = VertexBuffer->ShaderResourceViewRHI;
		}
		static void BindVertexBuffer(bool& bIsValid, FInt32Range& ValidRange, TSet<TWeakPtr<FRealtimeMeshVertexBuffer>>& InUseBuffers, FVertexStreamComponent& OutStreamComponent, const FRealtimeMeshGPUBufferMap& Buffers,
			FName BufferName, EVertexStreamUsage Usage, bool bIsOptional = false, FName ElementName = NAME_None)
		{
			const TSharedPtr<FRealtimeMeshGPUBuffer> FoundBuffer = FindBuffer(Buffers, ERealtimeMeshStreamType::Vertex, BufferName);

			if (!FoundBuffer.IsValid())
			{
				// If the buffer isn't optional, invalidate the result
				bIsValid &= bIsOptional;
				if (!bIsOptional)
				{
					ValidRange = FInt32Range(0, 0);
				}
				return;
			}

			const TSharedPtr<FRealtimeMeshVertexBuffer> VertexBuffer = StaticCastSharedPtr<FRealtimeMeshVertexBuffer>(FoundBuffer);

			uint16 ElementOffset = 0;
			// Try to grab the offset to the element if needed
			if (ElementName != NAME_None && !VertexBuffer->TryGetElementOffset(ElementName, ElementOffset))
			{
				// If the buffer isn't optional, invalidate the result
				bIsValid &= bIsOptional;
				if (!bIsOptional)
				{
					ValidRange = FInt32Range(0, 0);
				}
				return;
			}

			InUseBuffers.Add(VertexBuffer);
			
			OutStreamComponent = FVertexStreamComponent(VertexBuffer.Get(), ElementOffset, VertexBuffer->GetStride(), VertexBuffer->GetVertexType(), Usage);

			// Update the valid range
			ValidRange = FInt32Range::Intersection(ValidRange, FInt32Range(0, VertexBuffer->Num()));
		}
		static void BindIndexBuffer(bool& bIsValid, FInt32Range& ValidRange, TWeakPtr<FRealtimeMeshIndexBuffer>& OutIndexBuffer, const FRealtimeMeshGPUBufferMap& Buffers, FName BufferName, bool bIsOptional = false)
		{
			const TSharedPtr<FRealtimeMeshGPUBuffer> FoundBuffer = FindBuffer(Buffers, ERealtimeMeshStreamType::Index, BufferName);

			if (!FoundBuffer.IsValid())
			{
				// If the buffer isn't optional, invalidate the result
				bIsValid &= bIsOptional;
				if (!bIsOptional)
				{
					ValidRange = FInt32Range(0, 0);
				}
				return;
			}
			
			const TSharedPtr<FRealtimeMeshIndexBuffer> IndexBuffer = StaticCastSharedPtr<FRealtimeMeshIndexBuffer>(FoundBuffer);
			OutIndexBuffer = IndexBuffer;

			// Update the valid range
			ValidRange = FInt32Range::Intersection(ValidRange, FInt32Range(0, IndexBuffer->Num()));
		}
	};



	/**
	 * A basic vertex factory which closely resembles the functionality of LocalVertexFactory to show how to make custom vertex factories for the RMC
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLocalVertexFactory : public FRealtimeMeshVertexFactory
	{
		DECLARE_VERTEX_FACTORY_TYPE(FRealtimeMeshLocalVertexFactory);
	public:
		inline static const FName PositionStreamName = FName(TEXT("Position"));
		inline static const FName TangentsStreamName = FName(TEXT("Tangents"));
		inline static const FName ColorStreamName = FName(TEXT("Color"));
		inline static const FName TexCoordsStreamName = FName(TEXT("TexCoords"));

		inline static const FName NormalElementName = FName("Normal");
		inline static const FName TangentElementName = FName("Tangent");

		inline static const FName TexCoord0ElementName = FName("TexCoord", 0);
		inline static const FName TexCoord1ElementName = FName("TexCoord", 1);
		inline static const FName TexCoord2ElementName = FName("TexCoord", 2);
		inline static const FName TexCoord3ElementName = FName("TexCoord", 3);
		inline static const FName TexCoord4ElementName = FName("TexCoord", 4);
		inline static const FName TexCoord5ElementName = FName("TexCoord", 5);
		inline static const FName TexCoord6ElementName = FName("TexCoord", 6);
		inline static const FName TexCoord7ElementName = FName("TexCoord", 7);
	
		inline static const FName TrianglesStreamName = FName(TEXT("Triangles"));
		inline static const FName DepthOnlyTrianglesStreamName = FName(TEXT("DepthOnlyTriangles"));

		inline static const FName ReversedTrianglesStreamName = FName(TEXT("ReversedTriangles"));
		inline static const FName ReversedDepthOnlyTrianglesStreamName = FName(TEXT("ReversedDepthOnlyTriangles"));

	public:

		FRealtimeMeshLocalVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
			: FRealtimeMeshVertexFactory(InFeatureLevel)
			, IndexBuffer(nullptr)
			, DepthOnlyIndexBuffer(nullptr)
			, ReversedIndexBuffer(nullptr)
			, ReversedDepthOnlyIndexBuffer(nullptr)
			, ValidRange(FRealtimeMeshStreamRange::Empty)
			, ColorStreamIndex(INDEX_NONE)
		{
#if ENGINE_MAJOR_VERSION <= 5 && ENGINE_MINOR_VERSION <= 0
			bSupportsManualVertexFetch = true;
#endif
		}

		struct FDataType : public FStaticMeshDataType
		{
			FVertexStreamComponent PreSkinPositionComponent;
			FRHIShaderResourceView* PreSkinPositionComponentSRV = nullptr;
#if WITH_EDITORONLY_DATA
			const class UStaticMesh* StaticMesh = nullptr;
			bool bIsCoarseProxy = false;
#endif
		};

		/**
		 * Should we cache the material's shadertype on this platform with this vertex factory? 
		 */
		static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

		static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

		static void ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap, TArray<FString>& OutErrors);

		/**
		* Copy the data from another vertex factory
		* @param Other - factory to copy from
		*/
		void Copy(const FRealtimeMeshLocalVertexFactory& Other);

		virtual EPrimitiveType GetPrimitiveType() const override { return PT_TriangleList; }

		virtual FIndexBuffer& GetIndexBuffer(bool& bDepthOnly, bool& bMatrixInverted, TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const override;

		virtual FRealtimeMeshStreamRange GetValidRange() const override { return ValidRange; }
		virtual bool IsValidStreamRange(const FRealtimeMeshStreamRange& StreamRange) const override;

		virtual void Initialize(const TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>& Buffers) override;

		virtual bool GatherVertexBufferResources(TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const override;

		// FRenderResource interface.
		virtual void InitRHI() override;
		virtual void ReleaseRHI() override
		{
			UniformBuffer.SafeRelease();
			FVertexFactory::ReleaseRHI();
		}
	

		FORCEINLINE_DEBUGGABLE void SetColorOverrideStream(FRHICommandList& RHICmdList, const FVertexBuffer* ColorVertexBuffer) const
		{
			checkf(ColorVertexBuffer->IsInitialized(), TEXT("Color Vertex buffer was not initialized! Name %s"), *ColorVertexBuffer->GetFriendlyName());
			checkf(IsInitialized() && EnumHasAnyFlags(EVertexStreamUsage::Overridden, Data.ColorComponent.VertexStreamUsage) && ColorStreamIndex > 0, TEXT("Per-mesh colors with bad stream setup! Name %s"), *ColorVertexBuffer->GetFriendlyName());
			RHICmdList.SetStreamSource(ColorStreamIndex, ColorVertexBuffer->VertexBufferRHI, 0);
		}

		void GetColorOverrideStream(const FVertexBuffer* ColorVertexBuffer, FVertexInputStreamArray& VertexStreams) const
		{
			checkf(ColorVertexBuffer->IsInitialized(), TEXT("Color Vertex buffer was not initialized! Name %s"), *ColorVertexBuffer->GetFriendlyName());
			checkf(IsInitialized() && EnumHasAnyFlags(EVertexStreamUsage::Overridden, Data.ColorComponent.VertexStreamUsage) && ColorStreamIndex > 0, TEXT("Per-mesh colors with bad stream setup! Name %s"), *ColorVertexBuffer->GetFriendlyName());

			VertexStreams.Add(FVertexInputStream(ColorStreamIndex, 0, ColorVertexBuffer->VertexBufferRHI));
		}

		inline FRHIShaderResourceView* GetPositionsSRV() const
		{
			return Data.PositionComponentSRV;
		}

		inline FRHIShaderResourceView* GetPreSkinPositionSRV() const
		{
			return Data.PreSkinPositionComponentSRV ? Data.PreSkinPositionComponentSRV : GNullColorVertexBuffer.VertexBufferSRV.GetReference();
		}
	
		inline FRHIShaderResourceView* GetTangentsSRV() const
		{
			return Data.TangentsSRV;
		}

		inline FRHIShaderResourceView* GetTextureCoordinatesSRV() const
		{
			return Data.TextureCoordinatesSRV;
		}

		inline FRHIShaderResourceView* GetColorComponentsSRV() const
		{
			return Data.ColorComponentsSRV;
		}

		inline const uint32 GetColorIndexMask() const
		{
			return Data.ColorIndexMask;
		}

		inline const int GetLightMapCoordinateIndex() const
		{
			return Data.LightMapCoordinateIndex;
		}
	
		inline const int GetNumTexcoords() const
		{
			return Data.NumTexCoords;
		}

		virtual FRHIUniformBuffer* GetUniformBuffer() const override
		{
			return UniformBuffer.GetReference();
		}

	protected:
		const FDataType& GetData() const { return Data; }

		FDataType Data;
		TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters> UniformBuffer;

		TSet<TWeakPtr<FRealtimeMeshVertexBuffer>> InUseVertexBuffers;

		TWeakPtr<FRealtimeMeshIndexBuffer> IndexBuffer;
		TWeakPtr<FRealtimeMeshIndexBuffer> DepthOnlyIndexBuffer;
		TWeakPtr<FRealtimeMeshIndexBuffer> ReversedIndexBuffer;
		TWeakPtr<FRealtimeMeshIndexBuffer> ReversedDepthOnlyIndexBuffer;

		FRealtimeMeshStreamRange ValidRange;
	
		int32 ColorStreamIndex;
	};



	/** Shader parameter class used by FRealtimeMeshVertexFactory only - no derived classes. */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
	{
		DECLARE_TYPE_LAYOUT(FRealtimeMeshVertexFactoryShaderParameters, NonVirtual);
	public:
		void Bind(const FShaderParameterMap& ParameterMap);

		void GetElementShaderBindings(const FSceneInterface* Scene, const FSceneView* View, const FMeshMaterialShader* Shader,
									  const EVertexInputStreamType InputStreamType, ERHIFeatureLevel::Type FeatureLevel, const FVertexFactory* VertexFactory,
									  const FMeshBatchElement& BatchElement, FMeshDrawSingleShaderBindings& ShaderBindings, FVertexInputStreamArray& VertexStreams) const;

		// SpeedTree LOD parameter
		LAYOUT_FIELD(FShaderParameter, LODParameter);

		// True if LODParameter is bound, which puts us on the slow path in GetElementShaderBindings
		LAYOUT_FIELD(bool, bAnySpeedTreeParamIsBound);
	};
}