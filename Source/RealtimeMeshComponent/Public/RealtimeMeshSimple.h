// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMesh.h"
#include "RealtimeMeshConfig.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshSectionGroup.h"
#include "Interface_CollisionDataProviderCore.h"
#include "Data/RealtimeMeshBuilder.h"
#include "Data/RealtimeMeshDataStream.h"
#include "RealtimeMeshSimple.generated.h"


#define LOCTEXT_NAMESPACE "RealtimeMesh"

class URealtimeMeshSimple;
using namespace RealtimeMesh;


USTRUCT(BlueprintType, meta=(HasNativeMake="RealtimeMeshComponent.RealtimeMeshSimpleBlueprintFunctionLibrary.MakeRealtimeMeshSimpleStream"))
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSimpleMeshData
{
	GENERATED_BODY()
	;

public:
	FRealtimeMeshSimpleMeshData()
		: bUseHighPrecisionTangents(false)
		  , bUseHighPrecisionTexCoords(false)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<int32> Triangles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Positions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Normals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Tangents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector> Binormals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FLinearColor> LinearColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector2D> UV0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FColor> Colors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	bool bUseHighPrecisionTangents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	bool bUseHighPrecisionTexCoords;
};

UCLASS(meta=(ScriptName="RealtimeMeshSimpleLibrary"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshSimpleBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|Simple",
		meta=(AdvancedDisplay="Triangles,Positions,Normals,Tangents,Binormals,LinearColors,UV0,UV1,UV2,UV3,Colors,bUseHighPrecisionTangents,bUseHighPrecisionTexCoords",
			AutoCreateRefTerm="Triangles,Positions,Normals,Tangents,Binormals,LinearColors,UV0,UV1,UV2,UV3,Colors"))
	static FRealtimeMeshSimpleMeshData MakeRealtimeMeshSimpleStream(
		const TArray<int32>& Triangles,
		const TArray<FVector>& Positions,
		const TArray<FVector>& Normals,
		const TArray<FVector>& Tangents,
		const TArray<FVector>& Binormals,
		const TArray<FLinearColor>& LinearColors,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FVector2D>& UV2,
		const TArray<FVector2D>& UV3,
		const TArray<FColor>& Colors,
		bool bUseHighPrecisionTangents,
		bool bUseHighPrecisionTexCoords);
};


namespace RealtimeMesh
{
	class FRealtimeMeshSectionGroupSimple;

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionSimple : public FRealtimeMeshSection
	{
		bool bShouldCreateMeshCollision;
	public:
		FRealtimeMeshSectionSimple(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey& InKey);
		virtual ~FRealtimeMeshSectionSimple() override;

		bool HasCollision() const { return bShouldCreateMeshCollision; }
		void SetShouldCreateCollision(bool bNewShouldCreateMeshCollision);

		virtual void UpdateStreamRange(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamRange& InRange) override;
		
		virtual bool GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData);

		virtual bool Serialize(FArchive& Ar) override;
		virtual void Reset(FRealtimeMeshProxyCommandBatch& Commands) override;


	protected:
		virtual void HandleStreamsChanged(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshChangeType ChangeType) const;
		virtual FBoxSphereBounds3f CalculateBounds() const override;

		void MarkCollisionDirtyIfNecessary() const;
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupSimple : public FRealtimeMeshSectionGroup
	{
	private:
		FRealtimeMeshDataStreamRefSet Streams;
		bool bIsStandalone;

	public:
		FRealtimeMeshSectionGroupSimple(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey)
			: FRealtimeMeshSectionGroup(InSharedResources, InKey)
			, bIsStandalone(false)
		{
		}

		void FlagStandalone() { bIsStandalone = true; }
		bool IsStandalone() const { return bIsStandalone; }
		
		FRealtimeMeshSectionPtr GetStandaloneSection() const
		{
			FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
			if (IsStandalone())
			{
				check(Sections.Num() <= 1);
				return Sections.Num() > 0? *Sections.CreateConstIterator() : FRealtimeMeshSectionPtr(nullptr);
			}
			else
			{				
				FMessageLog("RealtimeMesh").Error(FText::Format(
					LOCTEXT("GetStandaloneSection_InvalidOnNonStandaloneGroup", "Unable to get standalone section of non standalone group {0} for mesh {1}"),
					FText::FromString(Key.ToString()), FText::FromName(SharedResources->GetMeshName())));
			}
			
			return nullptr;
		}
		
		bool HasStreams() const;

		FRealtimeMeshDataStreamPtr GetStream(FRealtimeMeshStreamKey StreamKey) const;
		FRealtimeMeshStreamRange GetStreamRange() const;

		TFuture<ERealtimeMeshProxyUpdateStatus> EditMeshData(TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshDataStreamRefSet&)> EditFunc);
		

		virtual void CreateOrUpdateStream(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshDataStream&& Stream) override;

		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateFromSimpleMesh(const FRealtimeMeshSimpleMeshData& MeshData);
		void UpdateFromSimpleMesh(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSimpleMeshData& MeshData);
		

		virtual bool GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData);
		virtual void InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands) override;
		
		virtual bool Serialize(FArchive& Ar) override;		
		virtual void Reset(FRealtimeMeshProxyCommandBatch& Commands) override;

	private:
		virtual void RemoveStream(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamKey& StreamKey) override;
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODSimple : public FRealtimeMeshLODData
	{
	public:
		FRealtimeMeshLODSimple(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey)
			: FRealtimeMeshLODData(InSharedResources, InKey)
		{
		}

		virtual bool GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData);
	};

	DECLARE_MULTICAST_DELEGATE(FRealtimeMeshSimpleCollisionDataChangedEvent);

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSharedResourcesSimple : public FRealtimeMeshSharedResources
	{
		FRealtimeMeshSimpleCollisionDataChangedEvent CollisionDataChangedEvent;

	public:
		FRealtimeMeshSimpleCollisionDataChangedEvent& OnCollisionDataChanged() { return CollisionDataChangedEvent; }
		void BroadcastCollisionDataChanged() const { CollisionDataChangedEvent.Broadcast(); }


		virtual FRealtimeMeshSectionRef CreateSection(const FRealtimeMeshSectionKey& InKey) const override
		{
			return MakeShared<FRealtimeMeshSectionSimple>(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey);
		}

		virtual FRealtimeMeshSectionGroupRef CreateSectionGroup(const FRealtimeMeshSectionGroupKey& InKey) const override
		{
			return MakeShared<FRealtimeMeshSectionGroupSimple>(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey);
		}

		virtual FRealtimeMeshLODDataRef CreateLOD(const FRealtimeMeshLODKey& InKey) const override
		{
			return MakeShared<FRealtimeMeshLODSimple>(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey);
		}

		virtual FRealtimeMeshRef CreateRealtimeMesh() const override;
		virtual FRealtimeMeshSharedResourcesRef CreateSharedResources() const override { return MakeShared<FRealtimeMeshSharedResourcesSimple>(); }
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSimple : public FRealtimeMesh
	{
	protected:
		FRealtimeMeshCollisionConfiguration CollisionConfig;
		FRealtimeMeshSimpleGeometry SimpleGeometry;
		mutable TSharedPtr<TPromise<ERealtimeMeshCollisionUpdateResult>> PendingCollisionPromise;

	public:
		FRealtimeMeshSimple(const FRealtimeMeshSharedResourcesRef& InSharedResources)
			: FRealtimeMesh(InSharedResources)
		{
			SharedResources->As<FRealtimeMeshSharedResourcesSimple>().OnCollisionDataChanged().AddRaw(this, &FRealtimeMeshSimple::MarkCollisionDirtyNoCallback);
		}

		virtual ~FRealtimeMeshSimple() override
		{
			SharedResources->As<FRealtimeMeshSharedResourcesSimple>().OnCollisionDataChanged().RemoveAll(this);
		}

		FRealtimeMeshCollisionConfiguration GetCollisionConfig() const;
		TFuture<ERealtimeMeshCollisionUpdateResult> SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig);
		FRealtimeMeshSimpleGeometry GetSimpleGeometry() const;
		TFuture<ERealtimeMeshCollisionUpdateResult> SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry);

		virtual bool GenerateCollisionMesh(FRealtimeMeshTriMeshData& CollisionData);

		virtual void Reset(FRealtimeMeshProxyCommandBatch& Commands, bool bRemoveRenderProxy) override;

		virtual bool Serialize(FArchive& Ar) override;

	protected:
		void MarkForEndOfFrameUpdate() const;
		TFuture<ERealtimeMeshCollisionUpdateResult> MarkCollisionDirty() const;
		void MarkCollisionDirtyNoCallback() const;

		virtual void ProcessEndOfFrameUpdates() override;

		friend class URealtimeMeshSimple;
	};
}


DECLARE_DYNAMIC_DELEGATE_OneParam(FRealtimeMeshSimpleCompletionCallback, ERealtimeMeshProxyUpdateStatus, ProxyUpdateResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FRealtimeMeshSimpleCollisionCompletionCallback, ERealtimeMeshCollisionUpdateResult, CollisionResult);


UCLASS(Blueprintable)
class REALTIMEMESHCOMPONENT_API URealtimeMeshSimple : public URealtimeMesh
{
	GENERATED_UCLASS_BODY()
protected:

public:
	TSharedRef<RealtimeMesh::FRealtimeMeshSimple> GetMeshData() const { return StaticCastSharedRef<RealtimeMesh::FRealtimeMeshSimple>(GetMesh()); };

	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);

	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData);
	
	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData);
	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData);

	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
														  const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision = false);

	TFuture<ERealtimeMeshProxyUpdateStatus> CreateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
																	const FRealtimeMeshSimpleMeshData& MeshData, bool bShouldCreateCollision = false);
	
	TFuture<ERealtimeMeshProxyUpdateStatus> CreateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
																	FRealtimeMeshStreamSet&& MeshData, bool bShouldCreateCollision = false);	
	TFuture<ERealtimeMeshProxyUpdateStatus> CreateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
																	const FRealtimeMeshStreamSet& MeshData, bool bShouldCreateCollision = false);

	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData);
	
	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData);
	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData);

	//TFuture<ERealtimeMeshProxyUpdateStatus> UpdateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSimpleMeshData& MeshData);

	//TFuture<ERealtimeMeshProxyUpdateStatus> UpdateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, FRealtimeMeshStreamSet&& MeshData);
	//TFuture<ERealtimeMeshProxyUpdateStatus> UpdateStandaloneSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamSet& MeshData);

	TFuture<ERealtimeMeshProxyUpdateStatus> EditMeshInPlace(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshDataStreamRefSet&)>);
	

	UE_DEPRECATED(all, "Use UpdateStandaloneSection instead")
	FRealtimeMeshSectionKey CreateMeshSection(const FRealtimeMeshLODKey& LODKey, const FRealtimeMeshSectionConfig& Config,
	                                          const FRealtimeMeshSimpleMeshData& MeshData, bool bShouldCreateCollision = false);

	UE_DEPRECATED(all, "Use CreateSectionGroup returning TFuture instead")
	FRealtimeMeshSectionGroupKey CreateSectionGroup(const FRealtimeMeshLODKey& LODKey, const FRealtimeMeshSimpleMeshData& MeshData);

	UE_DEPRECATED(all, "Use CreateSection returning TFuture instead")
	FRealtimeMeshSectionKey CreateSection(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionConfig& Config,
	                                      const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision = false);



	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	FRealtimeMeshSectionConfig GetSectionConfig(const FRealtimeMeshSectionKey& SectionKey) const;

	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config);
	
	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionSegment(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange);



	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	bool IsSectionVisible(const FRealtimeMeshSectionKey& SectionKey) const;

	TFuture<ERealtimeMeshProxyUpdateStatus> SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	bool IsSectionCastingShadow(const FRealtimeMeshSectionKey& SectionKey) const;

	TFuture<ERealtimeMeshProxyUpdateStatus> SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow);

	TFuture<ERealtimeMeshProxyUpdateStatus> RemoveSection(const FRealtimeMeshSectionKey& SectionKey);

	TFuture<ERealtimeMeshProxyUpdateStatus> RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshCollisionConfiguration GetCollisionConfig() const;

	TFuture<ERealtimeMeshCollisionUpdateResult> SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshSimpleGeometry GetSimpleGeometry() const;

	TFuture<ERealtimeMeshCollisionUpdateResult> SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry);













	
private:
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateSectionGroup", meta= (AutoCreateRefTerm = "CompletionCallback"))
	void CreateSectionGroup_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UE_DEPRECATED(all, "Use CreateSectionGroup instead")
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateUniqueSectionGroup", meta= (AutoCreateRefTerm = "LODKey, CompletionCallback"))
	FRealtimeMeshSectionGroupKey CreateSectionGroupUnique_Blueprint(const FRealtimeMeshLODKey& LODKey, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateSectionGroupWithSimpleMesh", meta = (AutoCreateRefTerm = "MeshData, CompletionCallback"))
	void CreateSectionGroupWithSimpleMesh_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData,
	                                                FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UE_DEPRECATED(all, "Use CreateSectionGroupWithSimpleMesh instead")
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateSectionGroupUniqueWithSimpleMesh",
		meta = (AutoCreateRefTerm = "LODKey, MeshData, CompletionCallback"))
	FRealtimeMeshSectionGroupKey CreateSectionGroupUniqueWithSimpleMesh_Blueprint(const FRealtimeMeshLODKey& LODKey, const FRealtimeMeshSimpleMeshData& MeshData,
	                                                                              FRealtimeMeshSimpleCompletionCallback CompletionCallback);


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateSection", meta = (AutoCreateRefTerm = "Config, StreamRange, CompletionCallback"))
	void CreateSection_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
	                             const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UE_DEPRECATED(all, "Use CreateSection instead")
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateSectionUnique", meta = (AutoCreateRefTerm = "Config, StreamRange, CompletionCallback"))
	FRealtimeMeshSectionKey CreateSectionUnique_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionConfig& Config,
	                                                      const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision,
	                                                      FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateStandaloneSectionWithSimpleMesh",
		meta = (AutoCreateRefTerm = "Config, MeshData, CompletionCallback"))
	void CreateStandaloneSectionWithSimpleMesh_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
	                                                     const FRealtimeMeshSimpleMeshData& MeshData,
	                                                     bool bShouldCreateCollision, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UE_DEPRECATED(all, "Use CreateStandaloneSectionWithSimpleMesh instead")
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateStandaloneSectionUniqueWithSimpleMesh",
		meta = (AutoCreateRefTerm = "Config, MeshData, CompletionCallback"))
	FRealtimeMeshSectionKey CreateStandaloneSectionUniqueWithSimpleMesh_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionConfig& Config,
	                                                                              const FRealtimeMeshSimpleMeshData& MeshData, bool bShouldCreateCollision,
	                                                                              FRealtimeMeshSimpleCompletionCallback CompletionCallback);


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="UpdateSectionGroupWithSimpleMesh", meta = (AutoCreateRefTerm = "MeshData, CompletionCallback"))
	void UpdateSectionGroupWithSimpleMesh_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData,
	                                                FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="UpdateStandaloneSectionWithSimpleMesh",
		meta = (AutoCreateRefTerm = "MeshData, CompletionCallback"))
	void UpdateStandaloneSectionWithSimpleMesh_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSimpleMeshData& MeshData,
	                                                     FRealtimeMeshSimpleCompletionCallback CompletionCallback);


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="UpdateSectionSegment", meta = (AutoCreateRefTerm = "SectionKey"))
	void UpdateSectionSegment_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="UpdateSectionConfig", meta = (AutoCreateRefTerm = "SectionKey"))
	void UpdateSectionConfig_Blueprint(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="SetSectionVisibility", meta = (AutoCreateRefTerm = "SectionKey"))
	void SetSectionVisibility_Blueprint(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="SetSectionCastShadow", meta = (AutoCreateRefTerm = "SectionKey"))
	void SetSectionCastShadow_Blueprint(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="RemoveSection", meta = (AutoCreateRefTerm = "SectionKey"))
	void RemoveSection_Blueprint(const FRealtimeMeshSectionKey& SectionKey, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="RemoveSectionGroup", meta = (AutoCreateRefTerm = "SectionGroupKey"))
	void RemoveSectionGroup_Blueprint(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FRealtimeMeshSimpleCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="SetCollisionConfig")
	void SetCollisionConfig_Blueprint(const FRealtimeMeshCollisionConfiguration& InCollisionConfig, FRealtimeMeshSimpleCollisionCompletionCallback CompletionCallback);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="SetSimpleGeometry")
	void SetSimpleGeometry_Blueprint(const FRealtimeMeshSimpleGeometry& InSimpleGeometry, FRealtimeMeshSimpleCollisionCompletionCallback CompletionCallback);


	
	virtual void Reset(bool bCreateNewMeshData) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
};

#undef LOCTEXT_NAMESPACE
