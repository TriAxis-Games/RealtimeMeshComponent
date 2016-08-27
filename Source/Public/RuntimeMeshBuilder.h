// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshComponent.h"

class IRuntimeMeshVerticesBuilder
{
public:
	IRuntimeMeshVerticesBuilder() { }
	virtual ~IRuntimeMeshVerticesBuilder() { }


	virtual bool HasPositionComponent() = 0;
	virtual bool HasNormalComponent() = 0;
	virtual bool HasTangentComponent() = 0;
	virtual bool HasColorComponent() = 0;
	virtual bool HasUVComponent(int32 Index) = 0;
	virtual bool HasHighPrecisionNormals() = 0;
	virtual bool HasHighPrecisionUVs() = 0;

	virtual void SetPosition(const FVector& InPosition) = 0;
	virtual void SetNormal(const FVector& InNormal) = 0;
	virtual void SetTangent(const FVector& InTangent) = 0;
	virtual void SetColor(const FColor& InColor) = 0;
	virtual void SetUV(int32 Index, const FVector2D& InUV) = 0;

	virtual FVector GetPosition() = 0;
	virtual FVector GetNormal() = 0;
	virtual FVector GetTangent() = 0;
	virtual FColor GetColor() = 0;
	virtual FVector2D GetUV(int32 Index) = 0;

	virtual int32 Length() = 0;
	virtual void Seek(int32 Position) = 0;
	virtual int32 MoveNext() = 0;
};





template<typename VertexType>
class FRuntimeMeshPackedVerticesBuilder : public IRuntimeMeshVerticesBuilder
{
private:
	TArray<VertexType>* Vertices;
	int32 CurrentPosition;
	bool bOwnsVertexArray;
public:

	FRuntimeMeshPackedVerticesBuilder()
		: bOwnsVertexArray(true), Vertices(new TArray<VertexType>()), CurrentPosition(-1)
	{ }
	FRuntimeMeshPackedVerticesBuilder(TArray<VertexType>*& InVertices)
		: bOwnsVertexArray(false), Vertices(InVertices), CurrentPosition(-1)
	{ }
	virtual ~FRuntimeMeshPackedVerticesBuilder() override
	{
		if (bOwnsVertexArray)
		{
			delete Vertices;
		}
	}


	virtual bool HasPositionComponent() override { return FRuntimeMeshVertexTraits<VertexType>::HasPosition; }
	virtual bool HasNormalComponent() override { return FRuntimeMeshVertexTraits<VertexType>::HasNormal; }
	virtual bool HasTangentComponent() override { return FRuntimeMeshVertexTraits<VertexType>::HasTangent; }
	virtual bool HasColorComponent() override { return FRuntimeMeshVertexTraits<VertexType>::HasColor; }
	virtual bool HasUVComponent(int32 Index) override
	{
		switch (Index)
		{
		case 0:
			return FRuntimeMeshVertexTraits<VertexType>::HasUV0;
		case 1:
			return FRuntimeMeshVertexTraits<VertexType>::HasUV1;
		case 2:
			return FRuntimeMeshVertexTraits<VertexType>::HasUV2;
		case 3:
			return FRuntimeMeshVertexTraits<VertexType>::HasUV3;
		case 4:
			return FRuntimeMeshVertexTraits<VertexType>::HasUV4;
		case 5:
			return FRuntimeMeshVertexTraits<VertexType>::HasUV5;
		case 6:
			return FRuntimeMeshVertexTraits<VertexType>::HasUV6;
		case 7:
			return FRuntimeMeshVertexTraits<VertexType>::HasUV7;
		}
		return false;
	}
	virtual bool HasHighPrecisionNormals() override { return FRuntimeMeshVertexTraits<VertexType>::HasHighPrecisionNormals; }
	virtual bool HasHighPrecisionUVs() override { return FRuntimeMeshVertexTraits<VertexType>::HasHighPrecisionUVs; }

	virtual void SetPosition(const FVector& InPosition) override { SetPositionInternal<VertexType>((*Vertices)[CurrentPosition], InPosition); }
	virtual void SetNormal(const FVector& InNormal) override { SetNormalInternal<VertexType>((*Vertices)[CurrentPosition], InNormal); }
	virtual void SetTangent(const FVector& InTangent) override { SetTangentInternal<VertexType>((*Vertices)[CurrentPosition], InTangent); }
	virtual void SetColor(const FColor& InColor) override { SetColorInternal<VertexType>((*Vertices)[CurrentPosition], InColor); }
	virtual void SetUV(int32 Index, const FVector2D& InUV)
	{
		switch (Index)
		{
		case 0:
			SetUV0Internal<VertexType>((*Vertices)[CurrentPosition], InUV);
			return;
		case 1:
			SetUV1Internal<VertexType>((*Vertices)[CurrentPosition], InUV);
			return;
		case 2:
			SetUV2Internal<VertexType>((*Vertices)[CurrentPosition], InUV);
			return;
		case 3:
			SetUV3Internal<VertexType>((*Vertices)[CurrentPosition], InUV);
			return;
		case 4:
			SetUV4Internal<VertexType>((*Vertices)[CurrentPosition], InUV);
			return;
		case 5:
			SetUV5Internal<VertexType>((*Vertices)[CurrentPosition], InUV);
			return;
		case 6:
			SetUV6Internal<VertexType>((*Vertices)[CurrentPosition], InUV);
			return;
		case 7:
			SetUV7Internal<VertexType>((*Vertices)[CurrentPosition], InUV);
			return;
		}
	}

	virtual FVector GetPosition() override { return GetPositionInternal<VertexType>((*Vertices)[CurrentPosition]); }
	virtual FVector GetNormal() override { return GetNormalInternal<VertexType>((*Vertices)[CurrentPosition]); }
	virtual FVector GetTangent() override { return GetTangentInternal<VertexType>((*Vertices)[CurrentPosition]); }
	virtual FColor GetColor() override { return GetColorInternal<VertexType>((*Vertices)[CurrentPosition]); }
	virtual FVector2D GetUV(int32 Index) override
	{
		switch (Index)
		{
		case 0:
			return GetUV0Internal<VertexType>((*Vertices)[CurrentPosition]);
		case 1:
			return GetUV1Internal<VertexType>((*Vertices)[CurrentPosition]);
		case 2:
			return GetUV2Internal<VertexType>((*Vertices)[CurrentPosition]);
		case 3:
			return GetUV3Internal<VertexType>((*Vertices)[CurrentPosition]);
		case 4:
			return GetUV4Internal<VertexType>((*Vertices)[CurrentPosition]);
		case 5:
			return GetUV5Internal<VertexType>((*Vertices)[CurrentPosition]);
		case 6:
			return GetUV6Internal<VertexType>((*Vertices)[CurrentPosition]);
		case 7:
			return GetUV7Internal<VertexType>((*Vertices)[CurrentPosition]);
		}
		return FVector2D::ZeroVector;
	}

	virtual int32 Length() override { return Vertices->Num(); }
	virtual void Seek(int32 Position) override { CurrentPosition = Position; }
	virtual int32 MoveNext() override 
	{ 
		CurrentPosition++;
		if (CurrentPosition >= Vertices->Num())
		{
			Vertices->SetNumZeroed(CurrentPosition + 1, false);
		}
		return CurrentPosition; 
	}



private:
	template<typename Type>
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasPosition>::Type SetPositionInternal(Type& Vertex, const FVector& Position)
	{
		Vertex.Position = Position;
	}
	template<typename Type>
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasPosition>::Type SetPositionInternal(Type& Vertex, const FVector& Position)
	{

	}	
	template<typename Type>
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasPosition, FVector>::Type GetPositionInternal(Type& Vertex)
	{
		return Vertex.Position;
	}
	template<typename Type>
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasPosition, FVector>::Type GetPositionInternal(Type& Vertex)
	{
		return FVector::ZeroVector;
	}


	template<typename Type>
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasNormal>::Type SetNormalInternal(Type& Vertex, const FVector4& Normal)
	{
		Vertex.Normal = Normal;
	}
	template<typename Type>
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasNormal>::Type SetNormalInternal(Type& Vertex, const FVector4& Normal)
	{

	}
	template<typename Type>
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasNormal, FVector4>::Type GetNormalInternal(Type& Vertex)
	{
		return Vertex.Normal;
	}
	template<typename Type>
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasNormal, FVector4>::Type GetNormalInternal(Type& Vertex)
	{
		return FVector::ZeroVector;
	}


	template<typename Type>
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasTangent>::Type SetTangentInternal(Type& Vertex, const FVector4& Tangent)
	{
		Vertex.Tangent = Tangent;
	}
	template<typename Type>
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasTangent>::Type SetTangentInternal(Type& Vertex, const FVector4& Tangent)
	{

	}
	template<typename Type>
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasTangent, FVector4>::Type GetTangentInternal(Type& Vertex)
	{
		return Vertex.Tangent;
	}
	template<typename Type>
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasTangent, FVector4>::Type GetTangentInternal(Type& Vertex)
	{
		return FVector::ZeroVector;
	}


	template<typename Type>
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasColor>::Type SetColorInternal(Type& Vertex, const FColor& Color)
	{
		Vertex.Color = Color;
	}
	template<typename Type>
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasColor>::Type SetColorInternal(Type& Vertex, const FColor& Color)
	{

	}
	template<typename Type>
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasColor, FColor>::Type GetColorInternal(Type& Vertex)
	{
		return Vertex.Color;
	}
	template<typename Type>
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasColor, FColor>::Type GetColorInternal(Type& Vertex)
	{
		return FColor::Transparent;
	}



#define CreateUVChannelGetSetPair(Index)																											\
	template<typename Type>																															\
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV##Index>::Type SetUV##Index##Internal(Type& Vertex, const FVector2D& UV##Index)			\
	{																																				\
		Vertex.UV##Index = UV##Index;																												\
	}																																				\
	template<typename Type>																															\
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV##Index>::Type SetUV##Index##Internal(Type& Vertex, const FVector2D& UV##Index)			\
	{																																				\
	}																																				\
	template<typename Type>																															\
	static typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV##Index, FVector2D>::Type GetUV##Index##Internal(Type& Vertex)						\
	{																																				\
		return Vertex.UV##Index;																													\
	}																																				\
	template<typename Type>																															\
	static typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV##Index, FVector2D>::Type GetUV##Index##Internal(Type& Vertex)					\
	{																																				\
		return FVector2D::ZeroVector;																												\
	}																																				


	CreateUVChannelGetSetPair(0);
	CreateUVChannelGetSetPair(1);
	CreateUVChannelGetSetPair(2);
	CreateUVChannelGetSetPair(3);
	CreateUVChannelGetSetPair(4);
	CreateUVChannelGetSetPair(5);
	CreateUVChannelGetSetPair(6);
	CreateUVChannelGetSetPair(7);


#undef CreateUVChannelGetSetPair























};

// class FRuntimeMeshIndicesBuilder
// {
// 	bool bOwnsIndexArray;
// 	TArray<int32>* Indices;
// 	int32 CurrentPosition;
// public:
// 	void AddTriangle(int32 Index0, int32 Index1, int32 Index2);
// 
// 	int32 TriangleLength() { return Length() / 3; }
// 	int32 Length() { return Indices.Num(); }
// 	void Seek(int32 Position) { CurrentPosition = 0; }
// 	int32 MoveNext() { CurrentPosition++; }
// };
// 
// class FRuntimeMeshBuilder
// {
// 
// };



// void Test()
//{
//	FRuntimeMeshBuilder<FRuntimeMeshVertexSimple> Mesh;
//
//	// Create 3 vertices setting position, normal, and UV0
//	int32 Vertex1 = Mesh.SetVertex(FVector(0, 0, 0));
//	Mesh.SetNormal(FVector(1, 0, 0));
//	Mesh.SetUV(0, FVector2D(0, 0));
//	Mesh.MoveNext();
//
//	int32 Vertex2 = Mesh.AddVertex(FVector(1, 1, 1));
//	Mesh.SetNormal(FVector(1, 0, 0));
//	Mesh.SetUV(0, FVector2D(1, 1));
//	Mesh.MoveNext();
//
//	int32 Vertex3 = Mesh.AddVertex(FVector(2, 2, 2));
//	Mesh.SetNormal(FVector(1, 0, 0));
//	Mesh.SetUV(0, FVector2D(1, 1));
//	Mesh.MoveNext();
//
//
//	// Add the triangle
//	Mesh.AddTriangle(Vertex1, Vertex2, Vertex3);
//
//
//	// Update the 3 vertices positions, leaving everything else the same
//	Mesh.SeekVertices(0);
//	Mesh.SetVertex(FVector(5, 5, 5));
//	Mesh.MoveNext();
//	Mesh.SetVertex(FVector(6, 6, 6));
//	Mesh.MoveNext();
//	Mesh.SetVertex(FVector(7, 7, 7));
//	Mesh.MoveNext();
//
//
//
//	Mesh.SeekTriangles(0);
//	Mesh.AddTriangle(Vertex3, Vertex2, Vertex1);
//
//
//
//}