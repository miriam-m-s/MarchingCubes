#pragma once
class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class Chunk
{
	UProceduralMeshComponent* Mesh;
	TArray<float> TerrainMap;
	TArray<FVector> Vertices;
	TArray<bool> MeshBoolean;
	
	TArray<int32> Meshid;
	TArray<int32> Triangles;
	TMap<FVector, int32> VertexMap;
	FIntPoint chunkLocalSize;
	UInstancedStaticMeshComponent* GrassMesh;
	TArray<FLinearColor> vertexColors;

public:
	Chunk();
	TArray<FVector> GrassInstancePositions;
	void resetMeshData();
	// Métodos para acceder por referencia
	UProceduralMeshComponent*& GetMesh();
	UInstancedStaticMeshComponent*& GetGrassMesh();         // puntero por referencia
	TArray<float>& GetTerrainMap();
	TArray<FVector>& GetVertices();
	TArray<bool>& GetMeshBoolean();
	FIntPoint& GetChunkLocalSize();
	TArray<int32>& GetTriangles();
	TArray<int32>& GetMeshid();
	TArray<FLinearColor>& GetVertexColors();
	TMap<FVector, int32>& GetVertexMap();


	
};
