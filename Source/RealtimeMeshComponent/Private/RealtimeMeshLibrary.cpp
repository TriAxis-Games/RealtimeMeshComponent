// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshLibrary.h"

#include <MeshExport.h>

#include "IndexTypes.h"
#include "IntVectorTypes.h"
#include "MathUtil.h"
#include "RealtimeMeshSimple.h"

FRealtimeMeshLODKey URealtimeMeshBlueprintFunctionLibrary::Conv_IntToRealtimeMeshLODKey(int32 LODIndex)
{
    return FRealtimeMeshLODKey(LODIndex);
}

FRealtimeMeshLODKey URealtimeMeshBlueprintFunctionLibrary::MakeLODKey(int32 LODIndex)
{
    return FRealtimeMeshLODKey(LODIndex);
}

FRealtimeMeshSectionGroupKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionGroupKey(FRealtimeMeshLODKey LODKey, int32 SectionGroupIndex)
{
    return FRealtimeMeshSectionGroupKey(LODKey, SectionGroupIndex);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKey(FRealtimeMeshSectionGroupKey SectionGroupKey, int32 SectionIndex)
{
    return FRealtimeMeshSectionKey(SectionGroupKey, SectionIndex);
}

void URealtimeMeshBlueprintFunctionLibrary::BreakLODKey(const FRealtimeMeshLODKey& LODKey, int32& LODIndex)
{
    LODIndex = RealtimeMesh::FRealtimeMeshKeyHelpers::GetLODIndex(LODKey);
}

FRealtimeMeshStreamRange URealtimeMeshBlueprintFunctionLibrary::MakeStreamRange(int32 VerticesLowerInclusive,
    int32 VerticesUpperExclusive, int32 IndicesLowerInclusive, int32 IndicesUpperExclusive)
{
    return FRealtimeMeshStreamRange(VerticesLowerInclusive, VerticesUpperExclusive, IndicesLowerInclusive, IndicesUpperExclusive);
}

static void ConvertQuadToTriangles(TArray<int32>& Triangles, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3)
{
    Triangles.Add(Vert0);
    Triangles.Add(Vert1);
    Triangles.Add(Vert3);

    Triangles.Add(Vert1);
    Triangles.Add(Vert2);
    Triangles.Add(Vert3);
}

FRealtimeMeshSimpleMeshData& URealtimeMeshBlueprintFunctionLibrary::AppendBoxMesh(FVector BoxRadius, FTransform BoxTransform, FRealtimeMeshSimpleMeshData& MeshData)
{
    // Generate verts
    FVector BoxVerts[8];
    BoxVerts[0] = BoxTransform.TransformPosition(FVector(-BoxRadius.X, BoxRadius.Y, BoxRadius.Z));
    BoxVerts[1] = BoxTransform.TransformPosition(FVector(BoxRadius.X, BoxRadius.Y, BoxRadius.Z));
    BoxVerts[2] = BoxTransform.TransformPosition(FVector(BoxRadius.X, -BoxRadius.Y, BoxRadius.Z));
    BoxVerts[3] = BoxTransform.TransformPosition(FVector(-BoxRadius.X, -BoxRadius.Y, BoxRadius.Z));

    BoxVerts[4] = BoxTransform.TransformPosition(FVector(-BoxRadius.X, BoxRadius.Y, -BoxRadius.Z));
    BoxVerts[5] = BoxTransform.TransformPosition(FVector(BoxRadius.X, BoxRadius.Y, -BoxRadius.Z));
    BoxVerts[6] = BoxTransform.TransformPosition(FVector(BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z));
    BoxVerts[7] = BoxTransform.TransformPosition(FVector(-BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z));

    // Generate triangles (from quads)
    const int32 StartVertex = MeshData.Positions.Num();
    const int32 NumVerts = 24; // 6 faces x 4 verts per face
    const int32 NumIndices = 36;

    // Make sure the secondary arrays are the same length, zeroing them if necessary
    MeshData.Normals.SetNumZeroed(StartVertex);
    MeshData.Tangents.SetNumZeroed(StartVertex);
    MeshData.UV0.SetNumZeroed(StartVertex);
    
    MeshData.Positions.Reserve(StartVertex + NumVerts);
    MeshData.Normals.Reserve(StartVertex + NumVerts);
    MeshData.Tangents.Reserve(StartVertex + NumVerts);
    MeshData.UV0.Reserve(StartVertex + NumVerts);
    MeshData.Triangles.Reserve(MeshData.Triangles.Num() + NumIndices);

    const auto WriteToNextFour = [](TArray<FVector>& Array, const FVector& Value)
    {
        Array.Add(Value);
        Array.Add(Value);
        Array.Add(Value);
        Array.Add(Value);
    };
    
    const auto WriteQuadPositions = [&MeshData](const FVector& VertA, const FVector& VertB, const FVector& VertC, const FVector& VertD)
    {
        MeshData.Positions.Add(VertA);
        MeshData.Positions.Add(VertB);
        MeshData.Positions.Add(VertC);
        MeshData.Positions.Add(VertD);
    };

    WriteQuadPositions(BoxVerts[0], BoxVerts[1], BoxVerts[2], BoxVerts[3]);
    WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(0.0f, 0.0f, 1.0f)));
    WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(0.0f, -1.0f, 0.0f)));
    ConvertQuadToTriangles(MeshData.Triangles, StartVertex + 0, StartVertex + 1, StartVertex + 2, StartVertex + 3);

    WriteQuadPositions(BoxVerts[4], BoxVerts[0], BoxVerts[3], BoxVerts[7]);
    WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(-1.0, 0.0, 0.0)));
    WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(0.0f, -1.0f, 0.0f)));
    ConvertQuadToTriangles(MeshData.Triangles, StartVertex + 4, StartVertex + 5, StartVertex + 6, StartVertex + 7);

    WriteQuadPositions(BoxVerts[5], BoxVerts[1], BoxVerts[0], BoxVerts[4]);
    WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(0.0, 1.0, 0.0)));
    WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(-1.0f, 0.0f, 0.0f)));
    ConvertQuadToTriangles(MeshData.Triangles, StartVertex + 8, StartVertex + 9, StartVertex + 10, StartVertex + 11);

    WriteQuadPositions(BoxVerts[6], BoxVerts[2], BoxVerts[1], BoxVerts[5]);
    WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(1.0, 0.0, 0.0)));
    WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(0.0f, 1.0f, 0.0f)));
    ConvertQuadToTriangles(MeshData.Triangles, StartVertex + 12, StartVertex + 13, StartVertex + 14, StartVertex + 15);

    WriteQuadPositions(BoxVerts[7], BoxVerts[3], BoxVerts[2], BoxVerts[6]);
    WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(0.0, -1.0, 0.0)));
    WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(1.0f, 0.0f, 0.0f)));
    ConvertQuadToTriangles(MeshData.Triangles, StartVertex + 16, StartVertex + 17, StartVertex + 18, StartVertex + 19);
    
    WriteQuadPositions(BoxVerts[7], BoxVerts[6], BoxVerts[5], BoxVerts[4]);
    WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(0.0, 0.0, -1.0)));
    WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(0.0f, 1.0f, 0.0f)));
    ConvertQuadToTriangles(MeshData.Triangles, StartVertex + 20, StartVertex + 21, StartVertex + 22, StartVertex + 23);

    // UVs
    for (int32 Index = 0; Index < 6; Index++)
    {
        MeshData.UV0.Add(FVector2D(0.0f, 0.0f));
        MeshData.UV0.Add(FVector2D(0.0f, 1.0f));
        MeshData.UV0.Add(FVector2D(1.0f, 1.0f));
        MeshData.UV0.Add(FVector2D(1.0f, 0.0f));
    }

    return MeshData;
}

FRealtimeMeshSimpleMeshData& URealtimeMeshBlueprintFunctionLibrary::AppendCapsuleMesh(
    FRealtimeMeshSimpleMeshData& MeshData, const FTransform& Transform, float Radius, float CylinderLength,
    int32 HemisphereSteps, int32 CircleSteps, int32 CylinderSteps)
{
    constexpr float ZeroTolerance = 1e-6;
    checkf(Radius >= ZeroTolerance
        , TEXT("AppendCapsuleMesh: Radius %f, must be bigger or equal %f")
        , Radius
        , ZeroTolerance
        )
    checkf(CylinderLength >= ZeroTolerance
        , TEXT("AppendCapsuleMesh: CylinderLength %f, must be bigger or equal %f")
        , CylinderLength
        , ZeroTolerance
        )
    checkf(HemisphereSteps >= 2
        , TEXT("AppendCapsuleMesh: HemisphereSteps %d, must be bigger or queal 2")
        , HemisphereSteps
        )
    checkf(CircleSteps >= 3
        , TEXT("AppendCapsuleMesh: CircleSteps %d, must be bigger or equal 3")
        , CylinderSteps
        )
    checkf(CylinderSteps >= 1
        , TEXT("AppendCapsuleMesh: CylinderSteps %d, must be bigger or equal 1")
        , CylinderSteps
        )
    // num vertical steps excluding poles: 2 * HemisphereSteps - 2 + CylinderSteps - 1
    const int32 NumHeightSteps = 2 * HemisphereSteps - 2 + CylinderSteps + 1;
    const int32 NumVertices = NumHeightSteps * CircleSteps + 2;
    // 3 entries in Tris for every triangle: 3 *
    // for the number of rectangles: * CircleSteps
    // for the number of triangles: * 2
    // for the poles: + 2 * CircleSteps
    const int32 NumTris = 3 * (NumHeightSteps * CircleSteps * 2 + 2 * CircleSteps);

    const int32 StartVertex = MeshData.Positions.Num();
    MeshData.Normals.SetNumZeroed(StartVertex);
    MeshData.Tangents.SetNumZeroed(StartVertex);

    MeshData.Positions.Reserve(StartVertex + NumVertices);
    MeshData.Normals.Reserve(StartVertex + NumVertices);
    MeshData.Tangents.Reserve(StartVertex + NumVertices);
    
    MeshData.Triangles.Reserve(StartVertex + NumTris);

    {
        auto AddVertex = [&MeshData, &Transform](FVector Pos, FVector Normal)
        {
            MeshData.Positions.Add(Transform.TransformPosition(Pos));
            MeshData.Normals.Add(Transform.TransformVectorNoScale(Normal));
        };
        
        const double DeltaPhi   = FMathd::HalfPi / static_cast<double>(HemisphereSteps);
        const double DeltaTheta = FMathd::TwoPi  / static_cast<double>(CircleSteps);

        FVector Offset = FVector(0, 0, CylinderLength) / 2.;

        // northern hemisphere, skipping the pole
        for (int32 p = 1; p < HemisphereSteps; ++p)
        {
            for (int32 t = 0; t < CircleSteps; ++t)
            {
                FVector Normal = SphericalToCartesian(1., t * DeltaTheta, p * DeltaPhi);
                AddVertex(Normal * Radius + Offset, Normal);
            }
        }

        // cylinder section
        for (int32 h = 0; h < CylinderSteps + 1; ++h)
        {
            for(int32 t = 0; t < CircleSteps; ++t)
            {
                FVector Normal = SphericalToCartesian(1., t * DeltaTheta, FMathd::HalfPi);
                AddVertex
                    (Normal * Radius + Offset * (1. - 2. * static_cast<double>(h) / CylinderSteps)
                    , Normal
                    );
            }
        }

        // southern hemisphere, skipping the pole
        for (int32 p = 1; p < HemisphereSteps; ++p)
        {
            for (int32 t = 0; t < CircleSteps; ++t)
            {
                FVector Normal = SphericalToCartesian(1., t * DeltaTheta, FMathd::HalfPi + p * DeltaPhi);
                AddVertex(Normal * Radius - Offset, Normal);
            }
        }
        // north-pole
        AddVertex( FVector::UnitZ() * Radius + Offset,  FVector::UnitZ());
        // south-pole
        AddVertex(-FVector::UnitZ() * Radius - Offset, -FVector::UnitZ());
    }

    {
        MeshData.UV0.SetNum(StartVertex + NumVertices, false);
        
        // generate the UV's
        const float DeltaH   = 1.0 / static_cast<float>(NumHeightSteps);
        const float DeltaTheta = 1.0 / static_cast<float>(CircleSteps);
        int32 UVid = StartVertex;

        for(int p = 1; p < NumHeightSteps + 1; ++p)
        {
            for (int t = 0; t < CircleSteps; ++t)
            {
                MeshData.UV0[UVid++] = FVector2D(t * DeltaTheta, p * DeltaH);
            }
            // TODO: how to set multiple uvs for same vertex
            // MeshData.UV0[UVid++] = FVector2D(0, p * DeltaPhi);
        }
        // TODO: how to set multiple uvs for same vertex
        // north-pole
        MeshData.UV0[UVid++] = FVector2D(1., 0.0);
        // south-pole
        MeshData.UV0[UVid] = FVector2D(1., 1.0);
    }
        
    {
        int32 Corners[4] =   { 0, 1, CircleSteps + 1, CircleSteps};
        for (int32 p = 0; p < NumHeightSteps - 1; ++p)
        {
            for (int32 t = 0; t < CircleSteps - 1; ++t)
            {
                ConvertQuadToTriangles
                    (MeshData.Triangles
                    , Corners[0]
                    , Corners[1]
                    , Corners[2]
                    , Corners[3]
                    );
                for (int32& i : Corners) ++i; 
            }
            ConvertQuadToTriangles
                ( MeshData.Triangles
                , Corners[0]
                , Corners[1] - CircleSteps
                , Corners[2] - CircleSteps
                , Corners[3]
                );
            for (int32& i : Corners) ++i;
        }
    }

    {
        // Triangles that connect to north pole
        for (int32 t = 0; t < CircleSteps; ++t)
        {
            MeshData.Triangles.Add(NumVertices - 2);
            MeshData.Triangles.Add((t + 1) % CircleSteps);
            MeshData.Triangles.Add(t);
        }

        // Triangles that connect to South pole
        const int32 Offset   = NumVertices - 2 - CircleSteps;
        for (int32 t = 0; t < CircleSteps; ++t)
        {
            MeshData.Triangles.Add(NumVertices - 1);
            MeshData.Triangles.Add(t + Offset);
            MeshData.Triangles.Add((t + 1) % CircleSteps + Offset);
        }
    }
    
    return MeshData;
}

template<typename Type>
static void AppendVertexArrayIfContains(TArray<Type>& Destination, const TArray<Type>& Source, int32 VertexOffset, int32 FinalLength)
{
    if (Source.Num() > 0)
    {
        Destination.SetNumZeroed(VertexOffset);
        Destination.Append(Source.GetData(), FMath::Min(FinalLength - VertexOffset, Source.Num()));		
    }	
}

static void AppendTransformedTangentArray(TArray<FVector>& Destination, const TArray<FVector>& Source, int32 VertexOffset, int32 FinalLength, const FTransform& Transform)
{
    if (Source.Num() > 0)
    {
        const int32 NumToCopy = FMath::Min(FinalLength - VertexOffset, Source.Num());
        Destination.SetNumZeroed(FinalLength);
        for (int32 Index = 0; Index < NumToCopy; Index++)
        {
            Destination[VertexOffset + Index] = Transform.TransformVector(Source[Index]);
        }
    }	
}


FRealtimeMeshSimpleMeshData& URealtimeMeshBlueprintFunctionLibrary::AppendMesh(FRealtimeMeshSimpleMeshData& TargetMeshData, const FRealtimeMeshSimpleMeshData& MeshDataToAdd, const FTransform& Transform)
{
    const int32 StartVertex = TargetMeshData.Positions.Num();

    // Skip slower transform logic if transform == identity
    if (Transform.Equals(FTransform::Identity))
    {
        TargetMeshData.Positions.Append(MeshDataToAdd.Positions);
        AppendVertexArrayIfContains(TargetMeshData.Normals, MeshDataToAdd.Normals, StartVertex, TargetMeshData.Positions.Num());
        AppendVertexArrayIfContains(TargetMeshData.Binormals, MeshDataToAdd.Binormals, StartVertex, TargetMeshData.Positions.Num());
        AppendVertexArrayIfContains(TargetMeshData.Tangents, MeshDataToAdd.Tangents, StartVertex, TargetMeshData.Positions.Num());
    }
    else
    {
        TargetMeshData.Positions.Reserve(TargetMeshData.Positions.Num() + MeshDataToAdd.Positions.Num());
        for (int32 Index = 0; Index < MeshDataToAdd.Positions.Num(); Index++)
        {
            TargetMeshData.Positions.Add(Transform.TransformPosition(MeshDataToAdd.Positions[Index]));
        }
        AppendTransformedTangentArray(TargetMeshData.Normals, MeshDataToAdd.Normals, StartVertex, TargetMeshData.Positions.Num(), Transform);
        AppendTransformedTangentArray(TargetMeshData.Binormals, MeshDataToAdd.Binormals, StartVertex, TargetMeshData.Positions.Num(), Transform);
        AppendTransformedTangentArray(TargetMeshData.Tangents, MeshDataToAdd.Tangents, StartVertex, TargetMeshData.Positions.Num(), Transform);
    }
        
    AppendVertexArrayIfContains(TargetMeshData.Colors, MeshDataToAdd.Colors, StartVertex, TargetMeshData.Positions.Num());
    AppendVertexArrayIfContains(TargetMeshData.LinearColors, MeshDataToAdd.LinearColors, StartVertex, TargetMeshData.Positions.Num());

    AppendVertexArrayIfContains(TargetMeshData.UV0, MeshDataToAdd.UV0, StartVertex, TargetMeshData.Positions.Num());
    AppendVertexArrayIfContains(TargetMeshData.UV1, MeshDataToAdd.UV1, StartVertex, TargetMeshData.Positions.Num());
    AppendVertexArrayIfContains(TargetMeshData.UV2, MeshDataToAdd.UV2, StartVertex, TargetMeshData.Positions.Num());
    AppendVertexArrayIfContains(TargetMeshData.UV3, MeshDataToAdd.UV3, StartVertex, TargetMeshData.Positions.Num());

    return TargetMeshData;
}
