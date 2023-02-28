// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMesh.h"
#include "Data/RealtimeMeshConfig.h"
#include "Data/RealtimeMeshDataBuilder.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshSectionGroup.h"
#include "Interface_CollisionDataProviderCore.h"
#include "RealtimeMeshSimple.generated.h"


#define LOCTEXT_NAMESPACE "RealtimeMesh"




USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSimpleMeshData
{
	GENERATED_BODY();
public:

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


namespace RealtimeMesh
{
	class FRealtimeMeshSectionGroupSimple;

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionSimple : public FRealtimeMeshSectionData
	{
		bool bShouldCreateMeshCollision;

	public:
		FRealtimeMeshSectionSimple(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionKey InKey,
								 const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange);

		bool HasCollision() const { return bShouldCreateMeshCollision; }
		void SetShouldCreateCollision(bool bNewShouldCreateMeshCollision);

		virtual void OnStreamsChanged(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams, const TArray<FRealtimeMeshStreamKey>& RemovedStreams) override;
		virtual void UpdateStreamRange(const FRealtimeMeshStreamRange& InRange) override;

		virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData);
		virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const;

		virtual bool Serialize(FArchive& Ar) override;
	private:		
		FBoxSphereBounds3f RecalculateBounds() const;
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupSimple : public FRealtimeMeshSectionGroup
	{
	private:		
		TMap<FRealtimeMeshStreamKey, FRealtimeMeshDataStreamPtr> Streams;
	public:
		FRealtimeMeshSectionGroupSimple(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionGroupKey InID)
			: FRealtimeMeshSectionGroup(InClassFactory, InMesh, InID) { }

		bool HasStreams() const;
		
		FRealtimeMeshDataStreamPtr GetStream(FRealtimeMeshStreamKey StreamKey) const;

		FRealtimeMeshStreamRange GetBaseRange() const;

		virtual void CreateOrUpdateStream(FRealtimeMeshStreamKey StreamKey, const FRealtimeMeshDataStreamRef& InStream);

		virtual void SetStreamData(const FRealtimeMeshSimpleMeshData& MeshData);

		virtual FRealtimeMeshSectionGroupProxyInitializationParametersRef GetInitializationParams() const override;

		virtual bool Serialize(FArchive& Ar) override;
		
		
		virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData);
		virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const;
		
	private:
		virtual void SetAllStreams(const TMap<FRealtimeMeshStreamKey, FRealtimeMeshDataStreamPtr>& InStreams);
		virtual void ClearStream(FRealtimeMeshStreamKey StreamKey) override;
		virtual void RemoveStream(FRealtimeMeshStreamKey StreamKey) override;		
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODSimple : public FRealtimeMeshLODData
	{
	public:
		FRealtimeMeshLODSimple(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh, const FRealtimeMeshLODKey& InID,
			const FRealtimeMeshLODConfig& InConfig)
			: FRealtimeMeshLODData(InClassFactory, InMesh, InID, InConfig)
		{
			TypeName = "RealtimeMeshLOD-Simple";
		}

		virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData);
		virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const;
		
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshClassFactorySimple : public FRealtimeMeshClassFactory
	{
		virtual FRealtimeMeshSectionDataRef CreateSection(const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionKey InKey, const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InSegment) const override
		{
			return MakeShared<FRealtimeMeshSectionSimple>(this->AsShared(), InMesh, InKey, InConfig, InSegment);
		}
		
		virtual FRealtimeMeshSectionGroupRef CreateSectionGroup(const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionGroupKey InKey) const override
		{
			return MakeShared<FRealtimeMeshSectionGroupSimple>(this->AsShared(), InMesh, InKey);			
		}

		virtual FRealtimeMeshLODDataRef CreateLOD(const FRealtimeMeshRef& InMesh, FRealtimeMeshLODKey InKey, const FRealtimeMeshLODConfig& InConfig) const override
		{
			return MakeShared<FRealtimeMeshLODSimple>(this->AsShared(), InMesh, InKey, InConfig);
		}
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshDataSimple : public FRealtimeMesh
	{
	public:
		FRealtimeMeshDataSimple() : FRealtimeMesh(MakeShared<FRealtimeMeshClassFactorySimple>()) {
		TypeName = "RealtimeMesh-Simple"; }
					
		virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
		virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	};
}





UCLASS(Blueprintable)
class REALTIMEMESHCOMPONENT_API URealtimeMeshSimple : public URealtimeMesh
{
	GENERATED_UCLASS_BODY()
protected:
	TSharedRef<RealtimeMesh::FRealtimeMeshDataSimple> MeshRef;
	

	
public:
	virtual RealtimeMesh::FRealtimeMeshRef GetMesh() const override { return MeshRef; }
	TSharedRef<RealtimeMesh::FRealtimeMeshDataSimple> GetMeshData() const { return MeshRef; };

	virtual void Reset(bool bCreateNewMeshData) override;


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta= (AutoCreateRefTerm = "LODKey"))
	FRealtimeMeshSectionGroupKey CreateSectionGroup(const FRealtimeMeshLODKey& LODKey);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "LODKey, MeshData"))
	FRealtimeMeshSectionGroupKey CreateSectionGroupWithMesh(const FRealtimeMeshLODKey& LODKey, const FRealtimeMeshSimpleMeshData& MeshData);
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionGroupKey, MeshData"))
	void UpdateSectionGroupMesh(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSimpleMeshData& MeshData);
		
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionGroupKey"))
	void RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionGroupKey, Config, StreamRange"))
	FRealtimeMeshSectionKey CreateSectionInGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionConfig& Config,
		const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision = false);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "LODKey, Config, MeshData"))
	FRealtimeMeshSectionKey CreateMeshSection(const FRealtimeMeshLODKey& LODKey, const FRealtimeMeshSectionConfig& Config, const FRealtimeMeshSimpleMeshData& MeshData,
		bool bShouldCreateCollision = false);
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey, MeshData"))	
	void UpdateSectionMesh(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSimpleMeshData& MeshData);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))	
	void UpdateSectionSegment(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange);
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	FRealtimeMeshSectionConfig GetSectionConfig(const FRealtimeMeshSectionKey& SectionKey) const;
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	void UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config);
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	bool IsSectionVisible(const FRealtimeMeshSectionKey& SectionKey) const;
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	void SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible);
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	bool IsSectionCastingShadow(const FRealtimeMeshSectionKey& SectionKey) const;
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	void SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow);
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	void RemoveSection(const FRealtimeMeshSectionKey& SectionKey);	
};

#undef LOCTEXT_NAMESPACE