// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshCore.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshCollision.h"
#include "Logging/MessageLog.h"
#include "RuntimeMeshComponentSettings.h"


#define LOCTEXT_NAMESPACE "RuntimeMeshComponent"

FRuntimeMeshSectionProperties::FRuntimeMeshSectionProperties()
	: UpdateFrequency(ERuntimeMeshUpdateFrequency::Infrequent)
	, MaterialSlot(0)
	, bIsVisible(true)
	, bIsMainPassRenderable(true)
	, bCastsShadow(true)
	, bForceOpaque(false)
	, bUseHighPrecisionTangents(false)
	, bUseHighPrecisionTexCoords(false)
	, NumTexCoords(1)
	, bWants32BitIndices(false)
{
	const URuntimeMeshComponentSettings* Settings = GetDefault<URuntimeMeshComponentSettings>();
	
	if (Settings)
	{
		UpdateFrequency = Settings->DefaultUpdateFrequency;
		bWants32BitIndices = Settings->bUse32BitIndicesByDefault;
		bUseHighPrecisionTangents = Settings->bUseHighPrecisionTangentsByDefault;
		bUseHighPrecisionTexCoords = Settings->bUseHighPrecisionTexCoordsByDefault;
	}
}


bool FRuntimeMeshRenderableMeshData::HasValidMeshData(bool bPrintErrorMessage) const
{
	bool bStatus = true;

	// Check that we have a full triangle at least worth of positions
	if (Positions.Num() < 3)
	{
		bStatus = false;

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_Positions", "Supplied mesh data doesn't contain enough vertices. Supplied vertices: {NumVerts}, Must be at least 3."), Arguments);
			FMessageLog("RuntimeMesh").Error(Message);
		}
	}

	// Make sure we have all the tangents for the positions
	if (Tangents.Num() < Positions.Num())
	{
		bStatus = false;

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("NumTangents"), Tangents.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_TooFewTangents", "Supplied mesh data doesn't contain enough tangents. Tangent count should match vertex count. Supplied vertices: {NumVerts}, Supplied tangents: {NumTangents}"), Arguments);
			FMessageLog("RuntimeMesh").Error(Message);
		}
	}

	// Make sure we don't have too many tangents
	if (Tangents.Num() > Positions.Num())
	{
		// Not really an error but we'll ignore the extra data

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("NumTangents"), Tangents.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_TooManyTangents", "Supplied mesh data contains too many tangents. Tangent count should match vertex count. Extras will be ignored. Supplied vertices: {NumVerts}, Supplied tangents: {NumTangents}"), Arguments);
			FMessageLog("RuntimeMesh").Warning(Message);
		}
	}

	// Make sure we have enough tex coords
	if (TexCoords.Num() < Positions.Num())
	{
		bStatus = false;

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("NumTexCoords"), TexCoords.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_TooFewTexCoords", "Supplied mesh data doesn't contain enough texture coordinates. Texture coordinate count should match vertex count. Supplied vertices: {NumVerts}, Supplied texture coordinates: {NumTexCoords}"), Arguments);
			FMessageLog("RuntimeMesh").Error(Message);
		}
	}

	// Make sure we don't have too many tex coords
	if (TexCoords.Num() > Positions.Num())
	{
		// Not really an error but, we'll ignore the extra data		

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("NumTexCoords"), TexCoords.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_TooManyTexCoords", "Supplied mesh data contains too many texture coordinates. Texture coordinate count should match vertex count. Extras will be ignored. Supplied vertices: {NumVerts}, Supplied texture coordinates: {NumTexCoords}"), Arguments);
			FMessageLog("RuntimeMesh").Warning(Message);
		}
	}

	// Make sure we have enough colors
	if (Colors.Num() < Positions.Num())
	{
		bStatus = false;

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("NumColors"), Colors.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_TooFewColors", "Supplied mesh data doesn't contain enough colors. Color count should match vertex count. Supplied vertices: {NumVerts}, Supplied colors: {NumColors}"), Arguments);
			FMessageLog("RuntimeMesh").Error(Message);
		}
	}

	// Make sure we don't have too many colors
	if (Colors.Num() > Positions.Num())
	{
		// Not really an error but, we'll ignore the extra data	

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("NumColors"), Colors.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_TooManyColors", "Supplied mesh data contains too many colors. Color count should match vertex count. Supplied vertices: {NumVerts}, Supplied colors: {NumColors}"), Arguments);
			FMessageLog("RuntimeMesh").Warning(Message);
		}
	}

	// Make sure we have enough indices to form at valid number of triangles
	if (Triangles.Num() < 3 || Triangles.Num() % 3 != 0)
	{
		bStatus = false;

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumTriangles"), Colors.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_InvalidTriangles", "Supplied mesh data doesn't contain a valid number of triangles. Must be a multiple of 3. Supplied triangles: {NumTriangles}"), Arguments);
			FMessageLog("RuntimeMesh").Error(Message);
		}
	}

	// Make sure we're using 32 bit indices if there's more than 2^16-1 vertices in this mesh
	if (!Triangles.IsHighPrecision() && Positions.Num() > MAX_uint16)
	{
		bStatus = false;

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("MaxVerts"), MAX_uint16);
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_InvalidTrianglesPrecision", "Supplied mesh uses 16 bit indices, but has more vertices than can be indexed. Must use 32 bit indices for this mesh. Supplied Vertices: {NumVerts}  Max Vertices on 16 bit indices: {MaxVerts}"), Arguments);
			FMessageLog("RuntimeMesh").Error(Message);
		}
	}

	return bStatus;
}


FRuntimeMeshCollisionSettings::FRuntimeMeshCollisionSettings()
	: bUseComplexAsSimple(true)
	, bUseAsyncCooking(false)
	, CookingMode(ERuntimeMeshCollisionCookingMode::CollisionPerformance)
{
	const URuntimeMeshComponentSettings* Settings = GetDefault<URuntimeMeshComponentSettings>();

	if (Settings)
	{
		bUseAsyncCooking = Settings->bCookCollisionAsync;
		CookingMode = Settings->DefaultCookingMode;
	}
}




FRuntimeMeshRenderableCollisionData::FRuntimeMeshRenderableCollisionData(const FRuntimeMeshRenderableMeshData& InRenderable)
{
	// Copy vertices
	int32 NumVertices = InRenderable.Positions.Num();
	Vertices.SetNum(NumVertices);
	for (int32 Index = 0; Index < NumVertices; Index++)
	{
		Vertices.SetPosition(Index, InRenderable.Positions.GetPosition(Index));
	}

	// Copy UVs
	int32 NumTexCoords = InRenderable.TexCoords.Num();
	int32 NumChannels = InRenderable.TexCoords.NumChannels();
	TexCoords.SetNum(NumChannels, NumTexCoords);
	for (int32 Index = 0; Index < NumTexCoords; Index++)
	{
		for (int32 ChannelId = 0; ChannelId < NumChannels; ChannelId++)
		{
			TexCoords.SetTexCoord(ChannelId, Index, InRenderable.TexCoords.GetTexCoord(Index, ChannelId));
		}
	}

	// Copy Triangles
	int32 NumTriangles = InRenderable.Triangles.NumTriangles();
	Triangles.SetNum(NumTriangles);
	for (int32 Index = 0; Index < NumTriangles; Index++)
	{
		Triangles.SetTriangleIndices(Index,
			InRenderable.Triangles.GetVertexIndex(Index * 3 + 0),
			InRenderable.Triangles.GetVertexIndex(Index * 3 + 1),
			InRenderable.Triangles.GetVertexIndex(Index * 3 + 2));
	}
}















#undef LOCTEXT_NAMESPACE
