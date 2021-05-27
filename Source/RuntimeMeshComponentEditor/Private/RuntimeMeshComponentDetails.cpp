// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshComponentDetails.h"
#include "RuntimeMeshComponent.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicsEngine/BodySetup.h"
#include "IDetailCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "RawMesh.h"
#include "Engine/StaticMesh.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshActor.h"

#define LOCTEXT_NAMESPACE "RuntimeMeshComponentDetails"

TSharedRef<IDetailCustomization> FRuntimeMeshComponentDetails::MakeInstance()
{
	return MakeShareable(new FRuntimeMeshComponentDetails);
}

void FRuntimeMeshComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& RuntimeMeshCategory = DetailBuilder.EditCategory("RuntimeMesh");

	const FText ConvertToStaticMeshText = LOCTEXT("ConvertToStaticMesh", "Create StaticMesh");

	// Cache set of selected things
	SelectedObjectsList = DetailBuilder.GetDetailsView()->GetSelectedObjects();

	// Add the Create Static Mesh button
	RuntimeMeshCategory.AddCustomRow(ConvertToStaticMeshText, false)
		.NameContent()
		[
			SNullWidget::NullWidget
		]
		.ValueContent()
		.VAlign(VAlign_Center)
		.MaxDesiredWidth(250)
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.ToolTipText(LOCTEXT("ConvertToStaticMeshTooltip", "Create a new StaticMesh asset using current geometry from this RuntimeMeshComponent. Does not modify instance."))
			.OnClicked(this, &FRuntimeMeshComponentDetails::ClickedOnConvertToStaticMesh)
			.IsEnabled(this, &FRuntimeMeshComponentDetails::ConvertToStaticMeshEnabled)
			.Content()
			[
				SNew(STextBlock)
				.Text(ConvertToStaticMeshText)
			]
		];

	{
		// Add all the default properties
		TArray<TSharedRef<IPropertyHandle>> AllProperties;
		bool bSimpleProperties = true;
		bool bAdvancedProperties = false;
		// Add all properties in the category in order
		RuntimeMeshCategory.GetDefaultProperties(AllProperties, bSimpleProperties, bAdvancedProperties);
		for (auto& Property : AllProperties)
		{
			RuntimeMeshCategory.AddProperty(Property);
		}
	}

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	RuntimeMeshesReferenced.Empty();
	for (TWeakObjectPtr<UObject>& Object : ObjectsBeingCustomized)
	{
		URuntimeMeshComponent* Component = Cast<URuntimeMeshComponent>(Object.Get());
		if (ensure(Component))
		{
			if (Component->GetRuntimeMesh())
			{
				RuntimeMeshesReferenced.AddUnique(Component->GetRuntimeMesh());
			}
		}
	}

	
}

URuntimeMeshComponent* FRuntimeMeshComponentDetails::GetFirstSelectedRuntimeMeshComp() const
{
	// Find first selected valid RuntimeMeshComp
	URuntimeMeshComponent* RuntimeMeshComp = nullptr;
	for (const TWeakObjectPtr<UObject>& Object : SelectedObjectsList)
	{
		URuntimeMeshComponent* TestRuntimeComp = Cast<URuntimeMeshComponent>(Object.Get());
		// See if this one is good
		if (TestRuntimeComp != nullptr && !TestRuntimeComp->IsTemplate())
		{
			RuntimeMeshComp = TestRuntimeComp;
			break;
		}

		ARuntimeMeshActor* TestRuntimeActor = Cast<ARuntimeMeshActor>(Object.Get());
		if (TestRuntimeActor != nullptr && !TestRuntimeActor->IsTemplate())
		{
			RuntimeMeshComp = TestRuntimeActor->GetRuntimeMeshComponent();
			break;
		}
	}

	return RuntimeMeshComp;
}

bool FRuntimeMeshComponentDetails::ConvertToStaticMeshEnabled() const
{
	return GetFirstSelectedRuntimeMeshComp() != nullptr;
}

FReply FRuntimeMeshComponentDetails::ClickedOnConvertToStaticMesh()
{
	// Find first selected RuntimeMeshComp
	URuntimeMeshComponent* RuntimeMeshComp = GetFirstSelectedRuntimeMeshComp();
	URuntimeMesh* RuntimeMesh = RuntimeMeshComp->GetRuntimeMesh();
	URuntimeMeshProvider* MeshProvider = RuntimeMesh->GetProviderPtr();
	if (RuntimeMeshComp != nullptr && RuntimeMesh != nullptr && MeshProvider != nullptr)
	{
		FString NewNameSuggestion = FString(TEXT("RuntimeMeshComp"));
		FString PackageName = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
		FString Name;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

		TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
			SNew(SDlgPickAssetPath)
			.Title(LOCTEXT("ConvertToStaticMeshPickName", "Choose New StaticMesh Location"))
			.DefaultAssetPath(FText::FromString(PackageName));

		if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
		{
			// Get the full name of where we want to create the physics asset.
			FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
			FName MeshName(*FPackageName::GetLongPackageAssetName(UserPackageName));

			// Check if the user inputed a valid asset name, if they did not, give it the generated default name
			if (MeshName == NAME_None)
			{
				// Use the defaults that were already generated.
				UserPackageName = PackageName;
				MeshName = *Name;
			}

			// Create the package to save the static mesh
			UPackage* Package = CreatePackage(NULL, *UserPackageName);
			check(Package);

			// Create StaticMesh object
			UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
			StaticMesh->InitResources();

			StaticMesh->LightingGuid = FGuid::NewGuid();

			// Copy the material slots
			TArray<FStaticMaterial>& Materials = StaticMesh->StaticMaterials;
			const auto RMCMaterialSlots = RuntimeMesh->GetMaterialSlots();
			Materials.SetNum(RMCMaterialSlots.Num());
			for (int32 Index = 0; Index < RMCMaterialSlots.Num(); Index++)
			{
				UMaterialInterface* Mat = RuntimeMeshComp->OverrideMaterials.Num() > Index ? RuntimeMeshComp->OverrideMaterials[Index] : nullptr;
				Mat = Mat ? Mat : RMCMaterialSlots[Index].Material;
				Materials[Index] = FStaticMaterial(Mat, RMCMaterialSlots[Index].SlotName);
			}


			const auto LODConfig = RuntimeMesh->GetCopyOfConfiguration();

			for (int32 LODIndex = 0; LODIndex < LODConfig.Num(); LODIndex++)
			{
				const auto& LOD = LODConfig[LODIndex];

				// Raw mesh data we are filling in
				FRawMesh RawMesh;
				bool bUseHighPrecisionTangents = false;
				bool bUseFullPrecisionUVs = false;
				int32 MaxUVs = 1;

				int32 VertexBase = 0;

				for (const auto& SectionEntry : LOD.Sections)
				{
					const int32 SectionId = SectionEntry.Key;
					const auto& Section = SectionEntry.Value;

					// Here we need to direct query the provider the mesh is using
					// We also go ahead and use high precision tangents/uvs so we don't loose 
					// quality passing through the build pipeline after quantizing it once
					FRuntimeMeshRenderableMeshData MeshData(true, true, Section.NumTexCoords, true);
					if (MeshProvider->GetSectionMeshForLOD(LODIndex, SectionEntry.Key, MeshData))
					{
						MaxUVs = FMath::Max<int32>(MaxUVs, Section.NumTexCoords);
						
						// Fill out existing UV channels to start of this one
						for (int32 Index = 0; Index < MaxUVs; Index++)
						{
							RawMesh.WedgeTexCoords[Index].SetNumZeroed(RawMesh.WedgeIndices.Num());
						}

						// Copy the vertex positions
						int32 NumVertices = MeshData.Positions.Num();
						for (int32 Index = 0; Index < NumVertices; Index++)
						{
							RawMesh.VertexPositions.Add(MeshData.Positions.GetPosition(Index));
						}

						// Copy wedges
						int32 NumTris = MeshData.Triangles.Num();
						for (int32 Index = 0; Index < NumTris; Index++)
						{
							int32 VertexIndex = MeshData.Triangles.GetVertexIndex(Index);
							RawMesh.WedgeIndices.Add(VertexIndex + VertexBase);

							FVector TangentX, TangentY, TangentZ;
							MeshData.Tangents.GetTangents(VertexIndex, TangentX, TangentY, TangentZ);
							RawMesh.WedgeTangentX.Add(TangentX);
							RawMesh.WedgeTangentY.Add(TangentY);
							RawMesh.WedgeTangentZ.Add(TangentZ);

							for (int32 UVIndex = 0; UVIndex < Section.NumTexCoords; UVIndex++)
							{
								RawMesh.WedgeTexCoords[UVIndex].Add(MeshData.TexCoords.GetTexCoord(VertexIndex, UVIndex));
							}

							RawMesh.WedgeColors.Add(MeshData.Colors.GetColor(VertexIndex));
						}

						// Copy face info
						for (int32 TriIdx = 0; TriIdx < NumTris / 3; TriIdx++)
						{
							// Set the face material
							RawMesh.FaceMaterialIndices.Add(SectionId);
							RawMesh.FaceSmoothingMasks.Add(0); // Assume this is ignored as bRecomputeNormals is false
						}

						// Update offset for creating one big index/vertex buffer
						VertexBase += NumVertices;

					}
				}


				// Fill out the UV channels to the same length as the indices
				for (int32 Index = 0; Index < MaxUVs; Index++)
				{
					RawMesh.WedgeTexCoords[Index].SetNumZeroed(RawMesh.WedgeIndices.Num());
				}

				// If we got some valid data.
				if (RawMesh.IsValid())
				{
					// Add source to new StaticMesh
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 23
					FStaticMeshSourceModel* SrcModel = new (StaticMesh->GetSourceModels()) FStaticMeshSourceModel();
#else
					FStaticMeshSourceModel* SrcModel = new (StaticMesh->SourceModels) FStaticMeshSourceModel();
#endif
					SrcModel->BuildSettings.bRecomputeNormals = false;
					SrcModel->BuildSettings.bRecomputeTangents = false;
					SrcModel->BuildSettings.bRemoveDegenerates = true;
					SrcModel->BuildSettings.bUseHighPrecisionTangentBasis = bUseHighPrecisionTangents;
					SrcModel->BuildSettings.bUseFullPrecisionUVs = bUseFullPrecisionUVs;
					SrcModel->BuildSettings.bGenerateLightmapUVs = true;
					SrcModel->BuildSettings.SrcLightmapIndex = 0;
					SrcModel->BuildSettings.DstLightmapIndex = 1;
					SrcModel->RawMeshBulkData->SaveRawMesh(RawMesh);

					SrcModel->ScreenSize = LOD.Properties.ScreenSize;

					// Set the materials used for this static mesh
					int32 NumMaterials = StaticMesh->StaticMaterials.Num();

					// Set up the SectionInfoMap to enable collision
					for (int32 SectionIdx = 0; SectionIdx < NumMaterials; SectionIdx++)
					{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 23
						FMeshSectionInfoMap& SectionInfoMap = StaticMesh->GetSectionInfoMap();
#else
						FMeshSectionInfoMap& SectionInfoMap = StaticMesh->SectionInfoMap;
#endif

						FMeshSectionInfo Info = SectionInfoMap.Get(LODIndex, SectionIdx);
						Info.MaterialIndex = SectionIdx;
						// TODO: Is this the correct way to handle this by just turning on collision in the top level LOD?
						Info.bEnableCollision = LODIndex == 0; 
						SectionInfoMap.Set(LODIndex, SectionIdx, Info);
					}
				}
			}

			StaticMesh->StaticMaterials = Materials;

			// Configure body setup for working collision.
			StaticMesh->CreateBodySetup();
			StaticMesh->BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;

			// Build mesh from source
			StaticMesh->Build(false);

			// Make package dirty.
			StaticMesh->MarkPackageDirty();

			StaticMesh->PostEditChange();

			// Notify asset registry of new asset
			FAssetRegistryModule::AssetCreated(StaticMesh);
		}
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE