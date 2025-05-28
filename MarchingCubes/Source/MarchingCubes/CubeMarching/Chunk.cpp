#include "Chunk.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"

Chunk::Chunk():Mesh(nullptr),GrassMesh(nullptr)
{
}




void Chunk::resetMeshData()
{
	Vertices.Empty();
	Triangles.Empty();
	VertexMap.Empty();
	vertexColors.Empty();
	MeshBoolean.Empty();

	
}

UProceduralMeshComponent*& Chunk::GetMesh()
{
	return Mesh;
}

UInstancedStaticMeshComponent*& Chunk::GetGrassMesh()
{
	return GrassMesh;
}

TArray<float>& Chunk::GetTerrainMap()
{
	return TerrainMap;
}

TArray<FVector>& Chunk::GetVertices()
{
	return Vertices;
}

TArray<bool>& Chunk::GetMeshBoolean()
{
	return MeshBoolean;
}

FIntPoint& Chunk::GetChunkLocalSize()
{
	return chunkLocalSize;
}

TArray<int32>& Chunk::GetTriangles()
{
	return Triangles;
}

TArray<int32>& Chunk::GetMeshid()
{
	return Meshid;
}

TArray<FLinearColor>& Chunk::GetVertexColors()
{
	return vertexColors;
}

TMap<FVector, int32>& Chunk::GetVertexMap()
{
	return VertexMap;
}
