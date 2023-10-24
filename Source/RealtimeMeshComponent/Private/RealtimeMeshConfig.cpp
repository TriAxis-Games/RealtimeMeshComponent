// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"


const FRealtimeMeshStreamRange FRealtimeMeshStreamRange::Empty = FRealtimeMeshStreamRange(FInt32Range::Empty(), FInt32Range::Empty());

FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamRange& Range)
{
	Ar << Range.Vertices << Range.Indices;
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
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DataRestructure)
	{
		uint8 OldLODIndex;
		Ar << OldLODIndex;
		Key.LODIndex = OldLODIndex;
	}
	else
	{
		Ar << Key.LODIndex;
	}
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionGroupKey& Key)
{
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DataRestructure)
	{
		uint8 OldLODIndex;
		Ar << OldLODIndex;
		Key.LODIndex = OldLODIndex;
		uint8 OldGroupIndex;
		Ar << OldGroupIndex;
		Key.GroupName = FName("RM-Legacy-Group", OldGroupIndex);
	}
	else
	{
		Ar << Key.LODIndex;
		Ar << Key.GroupName;
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionKey& Key)
{
	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::DataRestructure)
	{
		uint8 OldLODIndex;
		Ar << OldLODIndex;
		Key.LODIndex = OldLODIndex;
		uint8 OldGroupIndex;
		Ar << OldGroupIndex;
		Key.GroupName = FName("RM-Legacy-Group", OldGroupIndex);
		uint16 OldSectionIndex;
		Ar << OldSectionIndex;
		Key.SectionName = FName("RM-Legacy-Section", OldSectionIndex);
	}
	else
	{
		Ar << Key.LODIndex;
		Ar << Key.GroupName;
		Ar << Key.SectionName;
	}

	return Ar;
}

FRealtimeMeshSectionGroupKey FRealtimeMeshSectionGroupKey::Create(const FRealtimeMeshLODKey& LODKey, FName GroupName)
{
	return FRealtimeMeshSectionGroupKey(LODKey, GroupName);
}

FRealtimeMeshSectionGroupKey FRealtimeMeshSectionGroupKey::Create(const FRealtimeMeshLODKey& LODKey, int32 GroupID)
{
	return FRealtimeMeshSectionGroupKey(LODKey, FName("Group_", GroupID));
}

FRealtimeMeshSectionGroupKey FRealtimeMeshSectionGroupKey::CreateUnique(const FRealtimeMeshLODKey& LODKey)
{
	return FRealtimeMeshSectionGroupKey(LODKey, FName("Group_" + FGuid::NewGuid().ToString()));
}

bool FRealtimeMeshSectionKey::IsPolyGroupKey() const
{
	return GroupName.ToString().StartsWith("Section_PolyGroup");
}

FRealtimeMeshSectionKey FRealtimeMeshSectionKey::Create(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FName SectionName)
{
	return FRealtimeMeshSectionKey(SectionGroupKey.LOD(), SectionGroupKey.GroupName, SectionName);
}

FRealtimeMeshSectionKey FRealtimeMeshSectionKey::Create(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 SectionID)
{
	return FRealtimeMeshSectionKey(SectionGroupKey.LOD(), SectionGroupKey.GroupName, FName("Section_", SectionID));
}

FRealtimeMeshSectionKey FRealtimeMeshSectionKey::CreateUnique(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
{
	return FRealtimeMeshSectionKey(SectionGroupKey.LOD(), SectionGroupKey.GroupName, FName("Section_" + FGuid::NewGuid().ToString()));
}

FRealtimeMeshSectionKey FRealtimeMeshSectionKey::CreateForPolyGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 PolyGroup)
{
	return FRealtimeMeshSectionKey(SectionGroupKey.LOD(), SectionGroupKey.GroupName, FName("Section_PolyGroup", PolyGroup));	
}
