// Copyright TriAxis Games, L.L.C. All Rights Reserved.

/*=============================================================================
	RealtimeMeshVertexFactory.cpp: Local vertex factory implementation
=============================================================================*/

#include "RenderProxy/RealtimeMeshVertexFactory.h"


#include "SceneView.h"
#include "MeshBatch.h"
#include "SpeedTreeWind.h"
#include "Rendering/ColorVertexBuffer.h"
#include "MeshMaterialShader.h"
#include "RealtimeMeshConfig.h"
#include "SceneInterface.h"
#include "ProfilingDebugging/LoadTimeTracker.h"

#if RMC_ENGINE_ABOVE_5_2
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#endif

namespace RealtimeMesh
{
	IMPLEMENT_TYPE_LAYOUT(FRealtimeMeshVertexFactoryShaderParameters);

	class FRealtimeMeshSpeedTreeWindNullUniformBuffer : public TUniformBuffer<FSpeedTreeUniformParameters>
	{
		typedef TUniformBuffer<FSpeedTreeUniformParameters> Super;

	public:
#if RMC_ENGINE_ABOVE_5_3
		virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
#else
		virtual void InitDynamicRHI() override;
#endif
	};

#if RMC_ENGINE_ABOVE_5_3
	void FRealtimeMeshSpeedTreeWindNullUniformBuffer::InitRHI(FRHICommandListBase& RHICmdList)
#else
	void FRealtimeMeshSpeedTreeWindNullUniformBuffer::InitDynamicRHI()
#endif
	{
		FSpeedTreeUniformParameters Parameters;
		FMemory::Memzero(Parameters);
		SetContentsNoUpdate(Parameters);

#if RMC_ENGINE_ABOVE_5_3
		Super::InitRHI(RHICmdList);
#else
		Super::InitDynamicRHI();
#endif
	}

	static TGlobalResource<FRealtimeMeshSpeedTreeWindNullUniformBuffer> GSpeedTreeWindNullUniformBuffer;

	TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters> CreateRealtimeMeshVFUniformBuffer(
		const FRealtimeMeshLocalVertexFactory* RealtimeMeshVertexFactory, uint32 LODLightmapDataIndex)
	{
		FLocalVertexFactoryUniformShaderParameters UniformParameters;

		constexpr FColorVertexBuffer* OverrideColorVertexBuffer = nullptr;
		constexpr int32 BaseVertexIndex = 0;
		constexpr int32 PreSkinBaseVertexIndex = 0;

		UniformParameters.LODLightmapDataIndex = LODLightmapDataIndex;
		int32 ColorIndexMask = 0;

		if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
		{
			UniformParameters.VertexFetch_PositionBuffer = RealtimeMeshVertexFactory->GetPositionsSRV();
			UniformParameters.VertexFetch_PreSkinPositionBuffer = RealtimeMeshVertexFactory->GetPreSkinPositionSRV();

			UniformParameters.VertexFetch_PackedTangentsBuffer = RealtimeMeshVertexFactory->GetTangentsSRV();
			UniformParameters.VertexFetch_TexCoordBuffer = RealtimeMeshVertexFactory->GetTextureCoordinatesSRV();

			if (OverrideColorVertexBuffer)
			{
				UniformParameters.VertexFetch_ColorComponentsBuffer = OverrideColorVertexBuffer->GetColorComponentsSRV();
				ColorIndexMask = OverrideColorVertexBuffer->GetNumVertices() > 1 ? ~0 : 0;
			}
			else
			{
				UniformParameters.VertexFetch_ColorComponentsBuffer = RealtimeMeshVertexFactory->GetColorComponentsSRV();
				ColorIndexMask = (int32)RealtimeMeshVertexFactory->GetColorIndexMask();
			}
		}
		else
		{
			UniformParameters.VertexFetch_PositionBuffer = GNullColorVertexBuffer.VertexBufferSRV;
			UniformParameters.VertexFetch_PreSkinPositionBuffer = GNullColorVertexBuffer.VertexBufferSRV;
			UniformParameters.VertexFetch_PackedTangentsBuffer = GNullColorVertexBuffer.VertexBufferSRV;
			UniformParameters.VertexFetch_TexCoordBuffer = GNullColorVertexBuffer.VertexBufferSRV;
		}

		if (!UniformParameters.VertexFetch_ColorComponentsBuffer)
		{
			UniformParameters.VertexFetch_ColorComponentsBuffer = GNullColorVertexBuffer.VertexBufferSRV;
		}

		const int32 NumTexCoords = RealtimeMeshVertexFactory->GetNumTexcoords();
		const int32 LightMapCoordinateIndex = RealtimeMeshVertexFactory->GetLightMapCoordinateIndex();
		const int32 EffectiveBaseVertexIndex = RHISupportsAbsoluteVertexID(GMaxRHIShaderPlatform) ? 0 : BaseVertexIndex;
		const int32 EffectivePreSkinBaseVertexIndex = RHISupportsAbsoluteVertexID(GMaxRHIShaderPlatform) ? 0 : PreSkinBaseVertexIndex;

		UniformParameters.VertexFetch_Parameters = {ColorIndexMask, NumTexCoords, LightMapCoordinateIndex, EffectiveBaseVertexIndex};
		UniformParameters.PreSkinBaseVertexIndex = EffectivePreSkinBaseVertexIndex;

		return TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters>::CreateUniformBufferImmediate(UniformParameters, UniformBuffer_MultiFrame);
	}

	FIndexBuffer& FRealtimeMeshLocalVertexFactory::GetIndexBuffer(bool& bDepthOnly, bool& bMatrixInverted,
	                                                              TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const
	{
		if (bDepthOnly)
		{
			if (bMatrixInverted)
			{
				if (const auto ReversedDepth = ReversedDepthOnlyIndexBuffer.Pin())
				{
					ResourceSubmitter(ReversedDepth.ToSharedRef());
					return *ReversedDepth.Get();
				}
			}

			if (const auto DepthOnly = DepthOnlyIndexBuffer.Pin())
			{
				ResourceSubmitter(DepthOnly.ToSharedRef());
				bMatrixInverted = false;
				return *DepthOnly.Get();
			}
		}

		if (bMatrixInverted)
		{
			if (const auto Reversed = ReversedIndexBuffer.Pin())
			{
				ResourceSubmitter(Reversed.ToSharedRef());
				bDepthOnly = false;
				return *Reversed.Get();
			}
		}

		const auto Normal = IndexBuffer.Pin();
		check(Normal.IsValid());
		ResourceSubmitter(Normal.ToSharedRef());

		bMatrixInverted = false;
		bDepthOnly = false;
		return *Normal.Get();
	}

	bool FRealtimeMeshLocalVertexFactory::IsValidStreamRange(const FRealtimeMeshStreamRange& StreamRange) const
	{
		return ValidRange.Contains(StreamRange);
	}

	void FRealtimeMeshLocalVertexFactory::Initialize(const TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>& Buffers)
	{
		FDataType DataType;

		InUseVertexBuffers.Empty();
		bool bIsValid = true;

		FInt32Range ValidVertexRange(0, TNumericLimits<int32>::Max());
		FInt32Range ValidIndexRange(0, TNumericLimits<int32>::Max());

		// Bind Position
		BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.PositionComponent, Buffers,
		                 FRealtimeMeshStreams::PositionStreamName, EVertexStreamUsage::Default);
		BindVertexBufferSRV(bIsValid, DataType.PositionComponentSRV, Buffers, FRealtimeMeshStreams::PositionStreamName);

		// Bind Tangents
		BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TangentBasisComponents[0], Buffers,
		                 FRealtimeMeshStreams::TangentsStreamName, EVertexStreamUsage::ManualFetch, false, 0);
		BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TangentBasisComponents[1], Buffers,
		                 FRealtimeMeshStreams::TangentsStreamName, EVertexStreamUsage::ManualFetch, false, 1);
		BindVertexBufferSRV(bIsValid, DataType.TangentsSRV, Buffers, FRealtimeMeshStreams::TangentsStreamName);

		// Bind Color
		BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.ColorComponent, Buffers,
		                 FRealtimeMeshStreams::ColorStreamName, EVertexStreamUsage::ManualFetch, true, 0, true);
		BindVertexBufferSRV(bIsValid, DataType.ColorComponentsSRV, Buffers, FRealtimeMeshStreams::ColorStreamName);

		// Bind TexCoords
		DataType.NumTexCoords = 0;
		DataType.TextureCoordinates.Empty();
		BindTexCoordsBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TextureCoordinates, DataType.NumTexCoords, Buffers,
		                 FRealtimeMeshStreams::TexCoordsStreamName, EVertexStreamUsage::ManualFetch);
		BindVertexBufferSRV(bIsValid, DataType.TextureCoordinatesSRV, Buffers, FRealtimeMeshStreams::TexCoordsStreamName);

		// Bind all index buffers
		BindIndexBuffer(bIsValid, ValidIndexRange, IndexBuffer, Buffers, FRealtimeMeshStreams::TrianglesStreamName);
		//BindIndexBuffer(bIsValid, ValidIndexRange, ReversedIndexBuffer, Buffers, FRealtimeMeshStreamNames::ReversedTrianglesStreamName, true);
		//BindIndexBuffer(bIsValid, ValidIndexRange, DepthOnlyIndexBuffer, Buffers, FRealtimeMeshStreamNames::DepthOnlyTrianglesStreamName, true);
		//BindIndexBuffer(bIsValid, ValidIndexRange, ReversedDepthOnlyIndexBuffer, Buffers, FRealtimeMeshStreamNames::ReversedDepthOnlyTrianglesStreamName, true);

		// Update our valid range to the sum of the valid ranges
		ValidRange = FRealtimeMeshStreamRange(ValidVertexRange, ValidIndexRange);

		ReleaseResource();

		if (bIsValid)
		{
			Data = DataType;
			
#if RMC_ENGINE_ABOVE_5_3
			InitResource(FRHICommandListImmediate::Get());
#else
			InitResource();
#endif
		}
		else
		{
			Data = FDataType();
		}
	}

	bool FRealtimeMeshLocalVertexFactory::GatherVertexBufferResources(TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const
	{
		TArray<TSharedPtr<FRealtimeMeshVertexBuffer>> TempBuffers;
		for (const auto& Buffer : InUseVertexBuffers)
		{
			auto PinnedBuffer = Buffer.Pin();
			if (!PinnedBuffer)
			{
				return false;
			}
			TempBuffers.Add(PinnedBuffer);
		}
		for (const auto& Buffer : TempBuffers)
		{
			ResourceSubmitter(Buffer.ToSharedRef());
		}
		return true;
	}


	void FRealtimeMeshVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
	{
		LODParameter.Bind(ParameterMap, TEXT("SpeedTreeLODInfo"));
		bAnySpeedTreeParamIsBound = LODParameter.IsBound() || ParameterMap.ContainsParameterAllocation(TEXT("SpeedTreeData"));
	}

	void FRealtimeMeshVertexFactoryShaderParameters::GetElementShaderBindings(
		const FSceneInterface* Scene,
		const FSceneView* View,
		const FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement,
		FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams
	) const
	{
		// Decode VertexFactoryUserData as VertexFactoryUniformBuffer
		const FRHIUniformBuffer* VertexFactoryUniformBuffer = static_cast<FRHIUniformBuffer*>(BatchElement.VertexFactoryUserData);

		const FRealtimeMeshLocalVertexFactory* LocalVertexFactory = static_cast<const FRealtimeMeshLocalVertexFactory*>(VertexFactory);

		if (LocalVertexFactory->SupportsManualVertexFetch(FeatureLevel) || UseGPUScene(GMaxRHIShaderPlatform, FeatureLevel))
		{
			if (!VertexFactoryUniformBuffer)
			{
				// No batch element override
				VertexFactoryUniformBuffer = LocalVertexFactory->GetUniformBuffer();
			}

			ShaderBindings.Add(Shader->GetUniformBufferParameter<FLocalVertexFactoryUniformShaderParameters>(), VertexFactoryUniformBuffer);
		}

		//@todo - allow FMeshBatch to supply vertex streams (instead of requiring that they come from the vertex factory), and this userdata hack will no longer be needed for override vertex color
		if (BatchElement.bUserDataIsColorVertexBuffer)
		{
			const FColorVertexBuffer* OverrideColorVertexBuffer = (FColorVertexBuffer*)BatchElement.UserData;
			check(OverrideColorVertexBuffer);

			if (!LocalVertexFactory->SupportsManualVertexFetch(FeatureLevel))
			{
				LocalVertexFactory->GetColorOverrideStream(OverrideColorVertexBuffer, VertexStreams);
			}
		}

		if (bAnySpeedTreeParamIsBound)
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_FLocalVertexFactoryShaderParameters_SetMesh_SpeedTree);
			const FRHIUniformBuffer* SpeedTreeUniformBuffer = Scene ? Scene->GetSpeedTreeUniformBuffer(VertexFactory) : nullptr;
			if (SpeedTreeUniformBuffer == nullptr)
			{
				SpeedTreeUniformBuffer = GSpeedTreeWindNullUniformBuffer.GetUniformBufferRHI();
			}
			check(SpeedTreeUniformBuffer != nullptr);

			ShaderBindings.Add(Shader->GetUniformBufferParameter<FSpeedTreeUniformParameters>(), SpeedTreeUniformBuffer);

			if (LODParameter.IsBound())
			{
				const FVector3f LODData(BatchElement.MinScreenSize, BatchElement.MaxScreenSize, BatchElement.MaxScreenSize - BatchElement.MinScreenSize);
				ShaderBindings.Add(LODParameter, LODData);
			}
		}
	}

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	bool FRealtimeMeshLocalVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
	{
		// Only compile this permutation inside the editor - it's not applicable in games, but occasionally the editor needs it.
		if (Parameters.MaterialParameters.MaterialDomain == MD_UI)
		{
			return !!WITH_EDITOR;
		}

		return true;
	}

	void FRealtimeMeshLocalVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
#if RMC_ENGINE_ABOVE_5_3
		OutEnvironment.SetDefineIfUnset(TEXT("VF_SUPPORTS_SPEEDTREE_WIND"), TEXT("1"));

		if (RHISupportsManualVertexFetch(Parameters.Platform))
		{
			OutEnvironment.SetDefineIfUnset(TEXT("MANUAL_VERTEX_FETCH"), TEXT("1"));
		}
#else
		if (!OutEnvironment.GetDefinitions().Contains("VF_SUPPORTS_SPEEDTREE_WIND"))
		{
			OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_SPEEDTREE_WIND"), TEXT("1"));
		}
		const bool ContainsManualVertexFetch = OutEnvironment.GetDefinitions().Contains("MANUAL_VERTEX_FETCH");
		if (!ContainsManualVertexFetch && RHISupportsManualVertexFetch(Parameters.Platform))
		{
			OutEnvironment.SetDefine(TEXT("MANUAL_VERTEX_FETCH"), TEXT("1"));
		}
#endif
		
		const bool bVFSupportsPrimtiveSceneData = Parameters.VertexFactoryType->SupportsPrimitiveIdStream() && UseGPUScene(
			Parameters.Platform, GetMaxSupportedFeatureLevel(Parameters.Platform));
		OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), bVFSupportsPrimtiveSceneData);
		OutEnvironment.SetDefine(TEXT("RAY_TRACING_DYNAMIC_MESH_IN_LOCAL_SPACE"), TEXT("1"));
	}

	void FRealtimeMeshLocalVertexFactory::ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap,
	                                                             TArray<FString>& OutErrors)
	{
		if (Type->SupportsPrimitiveIdStream()
			&& UseGPUScene(Platform, GetMaxSupportedFeatureLevel(Platform))
			&& !IsMobilePlatform(Platform) // On mobile VS may use PrimtiveUB while GPUScene is enabled
#if RMC_ENGINE_ABOVE_5_3		
			&& ParameterMap.ContainsParameterAllocation(FPrimitiveUniformShaderParameters::FTypeInfo::GetStructMetadata()->GetShaderVariableName()))
#else
			&& ParameterMap.ContainsParameterAllocation(FPrimitiveUniformShaderParameters::StaticStructMetadata.GetShaderVariableName()))
#endif
		{
			OutErrors.AddUnique(*FString::Printf(
				TEXT(
					"Shader attempted to bind the Primitive uniform buffer even though Vertex Factory %s computes a PrimitiveId per-instance.  This will break auto-instancing.  Shaders should use GetPrimitiveData(Parameters).Member instead of Primitive.Member."),
				Type->GetName()));
		}
	}

	/**
	* Copy the data from another vertex factory
	* @param Other - factory to copy from
	*/
	void FRealtimeMeshLocalVertexFactory::Copy(const FRealtimeMeshLocalVertexFactory& Other)
	{
		FRealtimeMeshLocalVertexFactory* VertexFactory = this;
		const FDataType* DataCopy = &Other.Data;
		ENQUEUE_RENDER_COMMAND(FRealtimeMeshVertexFactoryCopyData)(
			[VertexFactory, DataCopy](FRHICommandListImmediate& RHICmdList)
			{
				VertexFactory->Data = *DataCopy;
			});
		BeginUpdateResourceRHI(this);
	}
	
#if RMC_ENGINE_ABOVE_5_3
	void FRealtimeMeshLocalVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
#else
	void FRealtimeMeshLocalVertexFactory::InitRHI()
#endif
	{
		SCOPED_LOADTIMER(FLocalVertexFactory_InitRHI);

		// We create different streams based on feature level
		check(HasValidFeatureLevel());

		// VertexFactory needs to be able to support max possible shader platform and feature level
		// in case if we switch feature level at runtime.
		const bool bCanUseGPUScene = UseGPUScene(GMaxRHIShaderPlatform, GMaxRHIFeatureLevel);

		// If the vertex buffer containing position is not the same vertex buffer containing the rest of the data,
		// then initialize PositionStream and PositionDeclaration.
		if (Data.PositionComponent.VertexBuffer != Data.TangentBasisComponents[0].VertexBuffer)
		{
			auto AddDeclaration = [this](EVertexInputStreamType InputStreamType, bool bAddNormal)
			{
				FVertexDeclarationElementList StreamElements;
				StreamElements.Add(AccessStreamComponent(Data.PositionComponent, 0, InputStreamType));

				bAddNormal = bAddNormal && Data.TangentBasisComponents[1].VertexBuffer != NULL;
				if (bAddNormal)
				{
					StreamElements.Add(AccessStreamComponent(Data.TangentBasisComponents[1], 2, InputStreamType));
				}

				AddPrimitiveIdStreamElement(InputStreamType, StreamElements, 1, 8);

				InitDeclaration(StreamElements, InputStreamType);
			};

			AddDeclaration(EVertexInputStreamType::PositionOnly, false);
			AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly, true);
		}

		FVertexDeclarationElementList Elements;
		if (Data.PositionComponent.VertexBuffer != nullptr)
		{
			Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));
		}

		AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, Elements, 13, 8);

		// Only the tangent and normal are used by the stream; the bitangent is derived in the shader.
		uint8 TangentBasisAttributes[2] = {1, 2};
		for (int32 AxisIndex = 0; AxisIndex < 2; AxisIndex++)
		{
			if (Data.TangentBasisComponents[AxisIndex].VertexBuffer != nullptr)
			{
				Elements.Add(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex], TangentBasisAttributes[AxisIndex]));
			}
		}

		if (Data.ColorComponentsSRV == nullptr)
		{
			Data.ColorComponentsSRV = GNullColorVertexBuffer.VertexBufferSRV;
			Data.ColorIndexMask = 0;
		}

		ColorStreamIndex = -1;
		if (Data.ColorComponent.VertexBuffer)
		{
			Elements.Add(AccessStreamComponent(Data.ColorComponent, 3));
			ColorStreamIndex = Elements.Last().StreamIndex;
		}
		else
		{
			// If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
			// This wastes 4 bytes per vertex, but prevents having to compile out twice the number of vertex factories.
			FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);
			Elements.Add(AccessStreamComponent(NullColorComponent, 3));
			ColorStreamIndex = Elements.Last().StreamIndex;
		}

		if (Data.TextureCoordinates.Num())
		{
			const int32 BaseTexCoordAttribute = 4;
			for (int32 CoordinateIndex = 0; CoordinateIndex < Data.TextureCoordinates.Num(); ++CoordinateIndex)
			{
				Elements.Add(AccessStreamComponent(
					Data.TextureCoordinates[CoordinateIndex],
					BaseTexCoordAttribute + CoordinateIndex
				));
			}

			for (int32 CoordinateIndex = Data.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; ++CoordinateIndex)
			{
				Elements.Add(AccessStreamComponent(
					Data.TextureCoordinates[Data.TextureCoordinates.Num() - 1],
					BaseTexCoordAttribute + CoordinateIndex
				));
			}
		}

		// Only used with FGPUSkinPassthroughVertexFactory on platforms that do not use ManualVertexFetch
		if (Data.PreSkinPositionComponent.VertexBuffer != nullptr)
		{
			Elements.Add(AccessStreamComponent(Data.PreSkinPositionComponent, 14));
		}

		if (Data.LightMapCoordinateComponent.VertexBuffer)
		{
			Elements.Add(AccessStreamComponent(Data.LightMapCoordinateComponent, 15));
		}
		else if (Data.TextureCoordinates.Num())
		{
			Elements.Add(AccessStreamComponent(Data.TextureCoordinates[0], 15));
		}

		check(Streams.Num() > 0);

		InitDeclaration(Elements);
		check(IsValidRef(GetDeclaration()));

		if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform) || bCanUseGPUScene)
		{
			SCOPED_LOADTIMER(FLocalVertexFactory_InitRHI_CreateLocalVFUniformBuffer);
			UniformBuffer = CreateRealtimeMeshVFUniformBuffer(this, Data.LODLightmapDataIndex);
		}

		check(IsValidRef(GetDeclaration()));
	}
}

using namespace RealtimeMesh;

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FRealtimeMeshLocalVertexFactory, SF_Vertex, FRealtimeMeshVertexFactoryShaderParameters);
#if RHI_RAYTRACING
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FRealtimeMeshLocalVertexFactory, SF_RayHitGroup, FRealtimeMeshVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FRealtimeMeshLocalVertexFactory, SF_Compute, FRealtimeMeshVertexFactoryShaderParameters);
#endif // RHI_RAYTRACING

IMPLEMENT_VERTEX_FACTORY_TYPE(FRealtimeMeshLocalVertexFactory, "/Engine/Private/LocalVertexFactory.ush",
                              EVertexFactoryFlags::UsedWithMaterials
                              | EVertexFactoryFlags::SupportsDynamicLighting
                              | EVertexFactoryFlags::SupportsPrecisePrevWorldPos
                              | EVertexFactoryFlags::SupportsPositionOnly
                              | EVertexFactoryFlags::SupportsCachingMeshDrawCommands
                              | EVertexFactoryFlags::SupportsPrimitiveIdStream
                              | EVertexFactoryFlags::SupportsRayTracing
                              | EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
);
