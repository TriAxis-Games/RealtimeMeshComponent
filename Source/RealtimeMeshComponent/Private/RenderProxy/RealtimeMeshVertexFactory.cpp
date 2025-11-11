// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

/*=============================================================================
	RealtimeMeshVertexFactory.cpp: Local vertex factory implementation
=============================================================================*/

#include "RenderProxy/RealtimeMeshVertexFactory.h"


#include "SceneView.h"
#include "MeshBatch.h"
#include "SpeedTreeWind.h"
#include "Rendering/ColorVertexBuffer.h"
#include "MeshMaterialShader.h"
#include "RealtimeMeshComponentModule.h"
#include "SceneInterface.h"
#include "ProfilingDebugging/LoadTimeTracker.h"
#include "RenderProxy/RealtimeMeshProxyShared.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"

namespace RealtimeMesh
{

	IMPLEMENT_TYPE_LAYOUT(FRealtimeMeshVertexFactoryShaderParameters);

	class FRealtimeMeshSpeedTreeWindNullUniformBuffer : public TUniformBuffer<FSpeedTreeUniformParameters>
	{
		typedef TUniformBuffer<FSpeedTreeUniformParameters> Super;

	public:
		virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
	};

	void FRealtimeMeshSpeedTreeWindNullUniformBuffer::InitRHI(FRHICommandListBase& RHICmdList)
	{
		FSpeedTreeUniformParameters Parameters;
		FMemory::Memzero(Parameters);
		SetContentsNoUpdate(Parameters);

		Super::InitRHI(RHICmdList);
	}

	static TGlobalResource<FRealtimeMeshSpeedTreeWindNullUniformBuffer> GSpeedTreeWindNullUniformBuffer;

	void FRealtimeMeshNullColorVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
	{
#if RMC_ENGINE_ABOVE_5_6
		FRHIBufferCreateDesc VertexBufferDesc = FRHIBufferCreateDesc::CreateVertex(TEXT("FRealtimeMeshNullColorVertexBuffer"))
			.SetStride(0)
			.SetSize(sizeof(FColor))
			.SetUsage(BUF_Static | BUF_VertexBuffer | BUF_ShaderResource)
			.SetInitialState(ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask);
		VertexBufferRHI = RHICmdList.CreateBuffer(VertexBufferDesc);
#else		
		FRHIResourceCreateInfo CreateInfo(TEXT("FRealtimeMeshNullColorVertexBuffer"));
		VertexBufferRHI = RHICmdList.CreateBuffer(sizeof(FColor), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
#endif
		
		FColor* Vertices = static_cast<FColor*>(RHICmdList.LockBuffer(VertexBufferRHI, 0, sizeof(FColor), RLM_WriteOnly));
		Vertices[0] = FColor(255, 255, 255, 255);
		RHICmdList.UnlockBuffer(VertexBufferRHI);

		VertexBufferSRV = RHICmdList.CreateShaderResourceView(FShaderResourceViewInitializer(VertexBufferRHI, PF_R8G8B8A8));
		// VertexBufferSRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(FColor), PF_R8G8B8A8);
		
		FVertexBuffer::InitRHI(RHICmdList);
	}

	void FRealtimeMeshNullColorVertexBuffer::ReleaseRHI()
	{
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}

	void FRealtimeMeshNullTangentVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
	{
#if RMC_ENGINE_ABOVE_5_6
		FRHIBufferCreateDesc VertexBufferDesc = FRHIBufferCreateDesc::CreateVertex(TEXT("FRealtimeMeshNullTangentVertexBuffer"))
			.SetStride(0)
			.SetSize(sizeof(TRealtimeMeshTangents<FPackedRGBA16N>))
			.SetUsage(BUF_Static | BUF_VertexBuffer | BUF_ShaderResource)
			.SetInitialState(ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask);
		VertexBufferRHI = RHICmdList.CreateBuffer(VertexBufferDesc);
#else		
		FRHIResourceCreateInfo CreateInfo(TEXT("FRealtimeMeshNullTangentVertexBuffer"));
		VertexBufferRHI = RHICmdList.CreateBuffer(sizeof(TRealtimeMeshTangents<FPackedRGBA16N>), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
#endif

		TRealtimeMeshTangents<FPackedRGBA16N>* Vertices = static_cast<TRealtimeMeshTangents<FPackedRGBA16N>*>(RHICmdList.LockBuffer(VertexBufferRHI, 0, sizeof(TRealtimeMeshTangents<FPackedRGBA16N>), RLM_WriteOnly));
		Vertices[0] = TRealtimeMeshTangents<FPackedRGBA16N>(FVector3f::ZAxisVector, FVector3f::YAxisVector, FVector3f::XAxisVector);
		RHICmdList.UnlockBuffer(VertexBufferRHI);
		
		VertexBufferSRV = RHICmdList.CreateShaderResourceView(FShaderResourceViewInitializer(VertexBufferRHI, PF_R16G16B16A16_SINT));
		// VertexBufferSRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(FPackedRGBA16N), PF_R16G16B16A16_SINT);
		
		FVertexBuffer::InitRHI(RHICmdList);
	}

	void FRealtimeMeshNullTangentVertexBuffer::ReleaseRHI()
	{
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}

	void FRealtimeMeshNullTexCoordVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
	{
#if RMC_ENGINE_ABOVE_5_6
		FRHIBufferCreateDesc VertexBufferDesc = FRHIBufferCreateDesc::CreateVertex(TEXT("FRealtimeMeshNullTexCoordVertexBuffer"))
			.SetStride(0)
			.SetSize(sizeof(FVector2f))
			.SetUsage(BUF_Static | BUF_VertexBuffer | BUF_ShaderResource)
			.SetInitialState(ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask);
		VertexBufferRHI = RHICmdList.CreateBuffer(VertexBufferDesc);
#else		
		FRHIResourceCreateInfo CreateInfo(TEXT("FRealtimeMeshNullTexCoordVertexBuffer"));
		VertexBufferRHI = RHICmdList.CreateBuffer(sizeof(FVector2f), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
#endif

		FVector2f* Vertices = static_cast<FVector2f*>(RHICmdList.LockBuffer(VertexBufferRHI, 0, sizeof(FVector2f), RLM_WriteOnly));
		Vertices[0] = FVector2f::ZeroVector;
		RHICmdList.UnlockBuffer(VertexBufferRHI);
		
		VertexBufferSRV = RHICmdList.CreateShaderResourceView(FShaderResourceViewInitializer(VertexBufferRHI, PF_G32R32F));
		// VertexBufferSRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(FVector2f), PF_G32R32F);
		
		FVertexBuffer::InitRHI(RHICmdList);
	}

	void FRealtimeMeshNullTexCoordVertexBuffer::ReleaseRHI()
	{
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}

	TGlobalResource<FRealtimeMeshNullColorVertexBuffer, FRenderResource::EInitPhase::Pre> GRealtimeMeshNullColorVertexBuffer;
	TGlobalResource<FRealtimeMeshNullTangentVertexBuffer, FRenderResource::EInitPhase::Pre> GRealtimeMeshNullTangentVertexBuffer;
	TGlobalResource<FRealtimeMeshNullTexCoordVertexBuffer, FRenderResource::EInitPhase::Pre> GRealtimeMeshNullTexCoordVertexBuffer;

	

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


	FIndexBuffer& FRealtimeMeshLocalVertexFactory::GetIndexBuffer(bool& bDepthOnly, bool& bMatrixInverted, struct FRealtimeMeshResourceReferenceList& ActiveResources) const
	{
		if (bDepthOnly)
		{
			if (bMatrixInverted)
			{
				if (const auto ReversedDepth = ReversedDepthOnlyIndexBuffer.Pin())
				{
					ActiveResources.AddResource(ReversedDepth);
					return *ReversedDepth.Get();
				}
			}

			if (const auto DepthOnly = DepthOnlyIndexBuffer.Pin())
			{
				ActiveResources.AddResource(DepthOnly);
				bMatrixInverted = false;
				return *DepthOnly.Get();
			}
		}

		if (bMatrixInverted)
		{
			if (const auto Reversed = ReversedIndexBuffer.Pin())
			{
				ActiveResources.AddResource(Reversed);
				bDepthOnly = false;
				return *Reversed.Get();
			}
		}

		const auto Normal = IndexBuffer.Pin();
		check(Normal.IsValid());
		ActiveResources.AddResource(Normal);

		bMatrixInverted = false;
		bDepthOnly = false;
		return *Normal.Get();
	}

	
	bool FRealtimeMeshLocalVertexFactory::IsValidStreamRange(const FRealtimeMeshStreamRange& StreamRange) const
	{
		return ValidRange.Contains(StreamRange);
	}

	void FRealtimeMeshLocalVertexFactory::Initialize(FRHICommandListBase& RHICmdList, const TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>& Buffers)
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

		const TSharedPtr<FRealtimeMeshGPUBuffer> TangentBuffer = Buffers.FindRef(FRealtimeMeshStreams::Tangents);
		if (TangentBuffer && TangentBuffer->Num() > 0)
		{
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TangentBasisComponents[0], Buffers,
							 FRealtimeMeshStreams::TangentsStreamName, EVertexStreamUsage::ManualFetch, false, 0);
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TangentBasisComponents[1], Buffers,
							 FRealtimeMeshStreams::TangentsStreamName, EVertexStreamUsage::ManualFetch, false, 1);
			BindVertexBufferSRV(bIsValid, DataType.TangentsSRV, Buffers, FRealtimeMeshStreams::TangentsStreamName);
		}
		else
		{			
			DataType.TangentBasisComponents[0] = FVertexStreamComponent(&GRealtimeMeshNullTangentVertexBuffer, 0, 0, VET_Short4, EVertexStreamUsage::ManualFetch);
			DataType.TangentBasisComponents[1] = FVertexStreamComponent(&GRealtimeMeshNullTangentVertexBuffer, sizeof(FPackedRGBA16N), 0, VET_Short4, EVertexStreamUsage::ManualFetch);
			DataType.TangentsSRV = GRealtimeMeshNullTangentVertexBuffer.VertexBufferSRV;
		}

		// Bind Color		
		const TSharedPtr<FRealtimeMeshGPUBuffer> ColorBuffer = Buffers.FindRef(FRealtimeMeshStreams::Color);
		if (ColorBuffer && ColorBuffer->Num() > 0)
		{
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.ColorComponent, Buffers,
							 FRealtimeMeshStreams::ColorStreamName, EVertexStreamUsage::ManualFetch, true, 0, true);
			BindVertexBufferSRV(bIsValid, DataType.ColorComponentsSRV, Buffers, FRealtimeMeshStreams::ColorStreamName);
		}
		else
		{
			DataType.ColorComponent = FVertexStreamComponent(&GRealtimeMeshNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);
			DataType.ColorComponentsSRV = GRealtimeMeshNullColorVertexBuffer.VertexBufferSRV;			
		}

		// Bind TexCoords
		DataType.NumTexCoords = 0;
		DataType.TextureCoordinates.Empty();		
		const TSharedPtr<FRealtimeMeshGPUBuffer> TexCoordBuffer = Buffers.FindRef(FRealtimeMeshStreams::TexCoords);
		if (TexCoordBuffer && TexCoordBuffer->Num() > 0)
		{
			BindTexCoordsBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TextureCoordinates, DataType.NumTexCoords, Buffers,
							 FRealtimeMeshStreams::TexCoordsStreamName, EVertexStreamUsage::ManualFetch);
			BindVertexBufferSRV(bIsValid, DataType.TextureCoordinatesSRV, Buffers, FRealtimeMeshStreams::TexCoordsStreamName);
		}
		else
		{
			DataType.NumTexCoords = 1;
			DataType.TextureCoordinates.Add(FVertexStreamComponent(&GRealtimeMeshNullTexCoordVertexBuffer, 0, 0, VET_Float2, EVertexStreamUsage::ManualFetch));
			DataType.TextureCoordinatesSRV = GRealtimeMeshNullTexCoordVertexBuffer.VertexBufferSRV;	
		}

		// Bind all index buffers
		BindIndexBuffer(bIsValid, ValidIndexRange, IndexBuffer, Buffers, FRealtimeMeshStreams::TrianglesStreamName);
		bIsValid &= IndexBuffer != nullptr;
		//BindIndexBuffer(bIsValid, ValidIndexRange, ReversedIndexBuffer, Buffers, FRealtimeMeshStreamNames::ReversedTrianglesStreamName, true);
		//BindIndexBuffer(bIsValid, ValidIndexRange, DepthOnlyIndexBuffer, Buffers, FRealtimeMeshStreamNames::DepthOnlyTrianglesStreamName, true);
		//BindIndexBuffer(bIsValid, ValidIndexRange, ReversedDepthOnlyIndexBuffer, Buffers, FRealtimeMeshStreamNames::ReversedDepthOnlyTrianglesStreamName, true);

		// Update our valid range to the sum of the valid ranges

		ReleaseResource();

		if (bIsValid)
		{
			Data = DataType;
			ValidRange = FRealtimeMeshStreamRange(ValidVertexRange, ValidIndexRange);
			
			InitResource(RHICmdList);
		}
		else
		{
			Data = FDataType();
			ValidRange = FRealtimeMeshStreamRange(0, 0, 0, 0);
			InUseVertexBuffers.Empty();
		}
	}

	bool FRealtimeMeshLocalVertexFactory::GatherVertexBufferResources(FRealtimeMeshResourceReferenceList& ActiveResources) const
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
			ActiveResources.AddResource(Buffer);
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
		OutEnvironment.SetDefineIfUnset(TEXT("VF_SUPPORTS_SPEEDTREE_WIND"), TEXT("1"));

		if (RHISupportsManualVertexFetch(Parameters.Platform))
		{
			OutEnvironment.SetDefineIfUnset(TEXT("MANUAL_VERTEX_FETCH"), TEXT("1"));
		}
		
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
			&& ParameterMap.ContainsParameterAllocation(FPrimitiveUniformShaderParameters::FTypeInfo::GetStructMetadata()->GetShaderVariableName()))
		{
			OutErrors.AddUnique(*FString::Printf(
				TEXT(
					"Shader attempted to bind the Primitive uniform buffer even though Vertex Factory %s computes a PrimitiveId per-instance.  This will break auto-instancing.  Shaders should use GetPrimitiveData(Parameters).Member instead of Primitive.Member."),
				Type->GetName()));
		}
	}

	void FRealtimeMeshLocalVertexFactory::GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements)
	{		
		Elements.Add(FVertexElement(0, 0, VET_Float3, 0, 12, false));

		switch (VertexInputStreamType)
		{
		case EVertexInputStreamType::Default:
			{
				Elements.Add(FVertexElement(1, 0, VET_UInt, 13, 0, true));
				break;
			}
		case EVertexInputStreamType::PositionOnly:
			{
				Elements.Add(FVertexElement(1, 0, VET_UInt, 1, 0, true));
				break;
			}
		case EVertexInputStreamType::PositionAndNormalOnly:
			{
				Elements.Add(FVertexElement(1, 4, VET_PackedNormal, 2, 0, false));
				Elements.Add(FVertexElement(2, 0, VET_UInt, 1, 0, true));
				break;
			}
		default:
			checkNoEntry();
		}
	}

	void FRealtimeMeshLocalVertexFactory::GetVertexElements(ERHIFeatureLevel::Type FeatureLevel, EVertexInputStreamType InputStreamType, bool bSupportsManualVertexFetch,
		FDataType& Data, FVertexDeclarationElementList& Elements)
	{
		FVertexStreamList VertexStreams;
		int32 ColorStreamIndex;
		GetVertexElements(FeatureLevel, InputStreamType, bSupportsManualVertexFetch, Data, Elements, VertexStreams, ColorStreamIndex);

		// For ES3.1 attribute ID needs to be done differently
		check(FeatureLevel > ERHIFeatureLevel::ES3_1);
		Elements.Add(FVertexElement(VertexStreams.Num(), 0, VET_UInt, 13, 0, true));
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
	
	void FRealtimeMeshLocalVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
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
				AddPrimitiveIdStreamElement(InputStreamType, StreamElements, 1, 1);
				InitDeclaration(StreamElements, InputStreamType);
			};

			AddDeclaration(EVertexInputStreamType::PositionOnly, false);
			AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly, true);
		}

		FVertexDeclarationElementList Elements;
		
		const bool bUseManualVertexFetch = SupportsManualVertexFetch(GetFeatureLevel());
			
		GetVertexElements(GetFeatureLevel(), EVertexInputStreamType::Default, bUseManualVertexFetch, Data, Elements, Streams, ColorStreamIndex);
		
		AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, Elements, 13, 13);
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

	void FRealtimeMeshLocalVertexFactory::GetVertexElements(ERHIFeatureLevel::Type VFFeatureLevel, EVertexInputStreamType InputStreamType, bool bSupportsManualVertexFetch,
	                                                        FDataType& VFData, FVertexDeclarationElementList& Elements,
	                                                        FVertexStreamList& InOutStreams,
	                                                        int32& OutColorStreamIndex)
	{
		check(InputStreamType == EVertexInputStreamType::Default);

		if (VFData.PositionComponent.VertexBuffer != nullptr)
		{
			Elements.Add(AccessStreamComponent(VFData.PositionComponent, 0, InOutStreams));
		}

#if !WITH_EDITOR
	// Can't rely on manual vertex fetch in the editor to not add the unused elements because vertex factories created
	// with manual vertex fetch support can somehow still be used when booting up in for example ES3.1 preview mode
	// The vertex factories are then used during mobile rendering and will cause PSO creation failure.
	// First need to fix invalid usage of these vertex factories before this can be enabled again. (UE-165187)
	if (!bSupportsManualVertexFetch)
#endif // WITH_EDITOR
		{
			// Only the tangent and normal are used by the stream; the bitangent is derived in the shader.
			uint8 TangentBasisAttributes[2] = {1, 2};
			for (int32 AxisIndex = 0; AxisIndex < 2; AxisIndex++)
			{
				if (VFData.TangentBasisComponents[AxisIndex].VertexBuffer != nullptr)
				{
					Elements.Add(AccessStreamComponent(VFData.TangentBasisComponents[AxisIndex], TangentBasisAttributes[AxisIndex], InOutStreams));
				}
			}

			if (VFData.ColorComponentsSRV == nullptr)
			{
				VFData.ColorComponentsSRV = GNullColorVertexBuffer.VertexBufferSRV;
				VFData.ColorIndexMask = 0;
			}		

			if (VFData.ColorComponent.VertexBuffer)
			{
				Elements.Add(AccessStreamComponent(VFData.ColorComponent, 3, InOutStreams));
			}
			else
			{
				// If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
				// This wastes 4 bytes per vertex, but prevents having to compile out twice the number of vertex factories.
				FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);		
				Elements.Add(AccessStreamComponent(NullColorComponent, 3, InOutStreams));
			}
			OutColorStreamIndex = Elements.Last().StreamIndex;

			if (VFData.TextureCoordinates.Num())
			{
				const int32 BaseTexCoordAttribute = 4;
				for (int32 CoordinateIndex = 0; CoordinateIndex < VFData.TextureCoordinates.Num(); ++CoordinateIndex)
				{
					Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[CoordinateIndex], BaseTexCoordAttribute + CoordinateIndex, InOutStreams));
				}

				for (int32 CoordinateIndex = VFData.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; ++CoordinateIndex)
				{
					Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[VFData.TextureCoordinates.Num() - 1], BaseTexCoordAttribute + CoordinateIndex, InOutStreams));
				}
			}

			// Fill PreSkinPosition slot for GPUSkinPassThrough vertex factory, or else use a dummy buffer.
			FVertexStreamComponent NullComponent(&GNullVertexBuffer, 0, 0, VET_Float4);
			Elements.Add(AccessStreamComponent(VFData.PreSkinPositionComponent.VertexBuffer ? VFData.PreSkinPositionComponent : NullComponent, 14, InOutStreams));

			if (VFData.LightMapCoordinateComponent.VertexBuffer)
			{
				Elements.Add(AccessStreamComponent(VFData.LightMapCoordinateComponent, 15, InOutStreams));
			}
			else if (VFData.TextureCoordinates.Num())
			{
				Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[0], 15, InOutStreams));
			}
		}
	}

	void FRealtimeMeshLocalVertexFactory::ReleaseRHI()
	{
		UniformBuffer.SafeRelease();
		FVertexFactory::ReleaseRHI();
	}

	void FRealtimeMeshLocalVertexFactory::SetColorOverrideStream(FRHICommandList& RHICmdList, const FVertexBuffer* ColorVertexBuffer) const
	{
		checkf(ColorVertexBuffer->IsInitialized(), TEXT("Color Vertex buffer was not initialized! Name %s"), *ColorVertexBuffer->GetFriendlyName());
		checkf(IsInitialized() && EnumHasAnyFlags(EVertexStreamUsage::Overridden, Data.ColorComponent.VertexStreamUsage) && ColorStreamIndex > 0,
		       TEXT("Per-mesh colors with bad stream setup! Name %s"), *ColorVertexBuffer->GetFriendlyName());
		RHICmdList.SetStreamSource(ColorStreamIndex, ColorVertexBuffer->VertexBufferRHI, 0);
	}

	void FRealtimeMeshLocalVertexFactory::GetColorOverrideStream(const FVertexBuffer* ColorVertexBuffer, FVertexInputStreamArray& VertexStreams) const
	{
		checkf(ColorVertexBuffer->IsInitialized(), TEXT("Color Vertex buffer was not initialized! Name %s"), *ColorVertexBuffer->GetFriendlyName());
		checkf(IsInitialized() && EnumHasAnyFlags(EVertexStreamUsage::Overridden, Data.ColorComponent.VertexStreamUsage) && ColorStreamIndex > 0,
			   TEXT("Per-mesh colors with bad stream setup! Name %s"), *ColorVertexBuffer->GetFriendlyName());

		VertexStreams.Add(FVertexInputStream(ColorStreamIndex, 0, ColorVertexBuffer->VertexBufferRHI));
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
                              | EVertexFactoryFlags::SupportsManualVertexFetch
							  | EVertexFactoryFlags::SupportsPSOPrecaching
							  | EVertexFactoryFlags::SupportsLumenMeshCards
);