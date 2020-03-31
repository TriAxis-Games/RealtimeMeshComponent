// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshCore.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshRenderable.h"
#include "Logging/MessageLog.h"


#define LOCTEXT_NAMESPACE "RuntimeMeshComponent"


bool FRuntimeMeshRenderableMeshData::HasValidMeshData(bool bPrintErrorMessage) const
{
	bool bStatus = true;
	if (Positions.Num() <= 3)
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

	if (Tangents.Num() > Positions.Num())
	{
		// Not really an error but, we'll ignore the extra data

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("NumTangents"), Tangents.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_TooManyTangents", "Supplied mesh data contains too many tangents. Tangent count should match vertex count. Extras will be ignored. Supplied vertices: {NumVerts}, Supplied tangents: {NumTangents}"), Arguments);
			FMessageLog("RuntimeMesh").Warning(Message);
		}
	}

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

	if (Triangles.Num() < 3 || Triangles.Num() % 3 != 0)
	{
		bStatus = false;

		if (bPrintErrorMessage)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("NumVerts"), Positions.Num());
			Arguments.Add(TEXT("NumTriangles"), Colors.Num());
			FText Message = FText::Format(LOCTEXT("InvalidMeshData_InvalidTriangles", "Supplied mesh data doesn't contain a valid number of triangles. Must be a multiple of 3. Supplied triangles: {NumTriangles}"), Arguments);
			FMessageLog("RuntimeMesh").Error(Message);
		}
	}

	return bStatus;
}




#undef LOCTEXT_NAMESPACE
