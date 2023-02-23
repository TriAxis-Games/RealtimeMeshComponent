// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshConfig.h"





const FRealtimeMeshStreamRange FRealtimeMeshStreamRange::Empty = FRealtimeMeshStreamRange(FInt32Range::Empty(), FInt32Range::Empty());

FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamRange& Range)
{
	Ar << Range.Vertices << Range.Indices;
	return Ar;	
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshTangent& Tangent)
{
	Ar << Tangent.TangentX << Tangent.bFlipTangentY;
	return Ar;	
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionConfig& Config)
{
	Ar << Config.MaterialSlot;
	Ar << Config.DrawType;
	Ar << Config.bIsVisible;
	Ar << Config.bCastsShadow;
	Ar << Config.bIsMainPassRenderable;
	Ar << Config.bForceOpaque;
	return Ar;	
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODConfig& Config)
{
	Ar << Config.bIsVisible;
	Ar << Config.ScreenSize;
	return Ar;	
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshConfig& Config)
{
	Ar << Config.ForcedLOD;
	return Ar;	
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODKey& Key)
{	
	Ar << Key.LODIndex;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionGroupKey& Key)
{	
	Ar << Key.LODIndex;
	Ar << Key.SectionGroupIndex;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionKey& Key)
{
	Ar << Key.LODIndex;
	Ar << Key.SectionGroupIndex;
	Ar << Key.SectionIndex;
	return Ar;
}
