﻿// Core Unreal includes
#include "Marching.h"
#include "Chunk.h"
#include "ProceduralMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"

// VT and Asset tools
#include "VT/RuntimeVirtualTextureVolume.h"
#include "AssetRegistry/AssetRegistryModule.h"

// Field system and geometry tools
#include "Field/FieldSystemNoiseAlgo.h"
#include "Operations/EmbedSurfacePath.h"

// Frame types
#include "FrameTypes.h"

AMarching::AMarching():RuntimeVolume(nullptr)
{
}

void AMarching::UpdateChunksAffectedByHole(FVector HitLocation, int centerX, int centerY, const int Radius)
{
	int radiusInVoxels = Radius;
	int minX = FMath::FloorToInt((centerX - radiusInVoxels) / float(ChunkSize.X));
	int maxX = FMath::FloorToInt((centerX + radiusInVoxels) / float(ChunkSize.X));
	int minY = FMath::FloorToInt((centerY - radiusInVoxels) / float(ChunkSize.Y));
	int maxY = FMath::FloorToInt((centerY + radiusInVoxels) / float(ChunkSize.Y));

	for (int chunkX = minX; chunkX <= maxX; ++chunkX)
	{
		for (int chunkY = minY; chunkY <= maxY; ++chunkY)
		{
			FIntPoint chunkCoord(chunkX, chunkY);
			if (!Chunks.Contains(chunkCoord)) continue;

			Chunk* CurrentChunk = Chunks[chunkCoord];
			if (!CurrentChunk) continue;

			if (CurrentChunk->GetMesh()) CurrentChunk->GetMesh()->DestroyComponent();

			for (FoliageInstance& GrassMesh : CurrentChunk->GetGrassMesh())
			{
				TArray<int32> IndicesToRemove;

				for (int32 i = 0; i < GrassMesh.MeshComponent->GetInstanceCount(); ++i)
				{
					FTransform InstanceTransform;
					GrassMesh.MeshComponent->GetInstanceTransform(i, InstanceTransform, true);

					FVector Position = InstanceTransform.GetLocation();
					float Distance = FVector::Dist(Position, HitLocation);

					if (Distance <= Radius * TriangleScale)
					{
						IndicesToRemove.Add(i);
					}
				}

				// Orden descendente para evitar problemas de desplazamiento de índices
				IndicesToRemove.Sort(TGreater<int32>());

				for (int32 i : IndicesToRemove)
				{
					GrassMesh.MeshComponent->RemoveInstance(i);
					CurrentChunk->GrassInstancePositions.RemoveAt(i);
					CurrentChunk->GetMeshid().RemoveAt(i);
				}
			}

			// Recrear malla
			UProceduralMeshComponent* NewMesh = NewObject<UProceduralMeshComponent>(this);
			NewMesh->RegisterComponent();
			NewMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			CurrentChunk->GetMesh() = NewMesh;
			CurrentChunk->resetMeshData();

			IterateChunkVoxels(chunkX, chunkY, CurrentChunk->GetChunkLocalSize());
			BuildMesh(chunkCoord);
		}
	}
}

void AMarching::GenerateHole(FVector HitLocation,const float Radius)
{
	FVector localPos = HitLocation / TriangleScale;


	int centerX = FMath::FloorToInt(localPos.X);
	int centerY = FMath::FloorToInt(localPos.Y);
	int centerZ = FMath::FloorToInt(localPos.Z);
	

	if (!IsInBounds(centerX, centerY, centerZ))return;
	
	ApplySphericalHole(centerX, centerY, centerZ, Radius);
	
	UpdateChunksAffectedByHole(HitLocation, centerX, centerY, Radius);
	
}
FORCEINLINE bool AMarching::IsInBounds(int x, int y, int z) const
{
	return x >= 0 && x < GridSize.X &&
		   y >= 0 && y < GridSize.Y &&
		   z >= 0 && z < GridSize.Z;
}
void AMarching::ApplySphericalHole(int centerX, int centerY, int centerZ, int radius)
{
	for (int x = centerX - radius; x <= centerX + radius; x++)
	{
		for (int y = centerY - radius; y <= centerY + radius; y++)
		{
			for (int z = centerZ - radius; z <= centerZ + radius; z++)
			{
				if (!IsInBounds(x, y, z)) continue;

				float dx = x - centerX;
				float dy = y - centerY;
				float dz = z - centerZ;
				float distSquared = dx * dx + dy * dy + dz * dz;

				if (distSquared <= radius * radius)
				{
					float dist = FMath::Sqrt(distSquared);
					float t = 1.0f - (dist / radius); // 1.0 en el centro, 0.0 en el borde
					int index = getTerrainIndex(x, y, z);
					if (TerrainMap.IsValidIndex(index))
					{
						TerrainMap[index].value += t * 5.0f;
						TerrainMap[index].vertexColor=t;
					}
				}
			}
		}
	}
}

void AMarching::BeginPlay()
{
	Super::BeginPlay();
	DeleteTerrain();
	UE_LOG(LogTemp, Warning, TEXT("TerrainMap size after creation: %d"), TerrainMap.Num());
	GenerateTerrain();

}





void AMarching::GenerateTerrain()
{
	//RuntimeVolume
	
	DeleteTerrain();
	
	TerrainMap.SetNum((GridSize.X + 1) * (GridSize.Y + 1) * (GridSize.Z + 1));
	
	UE_LOG(LogTemp, Warning, TEXT("Create Terrain"));
	CreateTerrain();
	UE_LOG(LogTemp, Warning, TEXT("TerrainMap size after creation: %d"), TerrainMap.Num());


	//si el chunk es mas grande que grid se queda el grid con su tamaño 
	ChunkSize.X = FMath::Min(GridSize.X, ChunkSize.X);
	ChunkSize.Y = FMath::Min(GridSize.Y, ChunkSize.Y);

	//chunks enteros que hay en el terreno
	
	NumChunks = FIntPoint (
		(GridSize.X + ChunkSize.X - 1) / ChunkSize.X,
		(GridSize.Y + ChunkSize.Y - 1) / ChunkSize.Y
	
	);

	// Calcular el tamaño de grid sobrante que no llena un chunk completo (por si lo necesitas)
	Remainder = FIntPoint(
		GridSize.X % ChunkSize.X,
		GridSize.Y % ChunkSize.Y
		
	);

	
	
	CubeIteration(true);
}
void AMarching::generateChunk(FIntPoint  chunkCoord,FIntPoint LocalChunkSize)
{
	UE_LOG(LogTemp, Warning, TEXT("generateChunk%d"),1);


	Chunks.Add(chunkCoord, new Chunk());
	

	Chunk* CurrentChunk = Chunks[chunkCoord];

	

	
	
	UProceduralMeshComponent* NewMesh = NewObject<UProceduralMeshComponent>(this);
	NewMesh->RegisterComponent();
	NewMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	CurrentChunk->GetMesh() = NewMesh;

	
	Chunks[chunkCoord]->GetChunkLocalSize() = LocalChunkSize;
	TArray<FoliageInstance> grassMesh=Chunks[chunkCoord]->GetGrassMesh();
	for (int i=0;i<StaticMeshes.Num();i++)
	{
		UInstancedStaticMeshComponent* fol=NewObject<UInstancedStaticMeshComponent>(this);
		fol->RegisterComponent();
		fol->SetStaticMesh(StaticMeshes[i].Mesh);
		fol->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		fol->SetCastShadow(false);
		fol->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
		Chunks[chunkCoord]->GetGrassMesh().Add(FoliageInstance(fol,StaticMeshes[i]));
	}
	

	
	
}

void AMarching::DeleteTerrain()
{
	
	
	TerrainMap.Empty();
	UE_LOG(LogTemp, Warning, TEXT("empty size after creation: %d"), TerrainMap.Num());

	UE_LOG(LogTemp, Warning, TEXT("Adios Terrene"));
	for (auto& ChunkPair : Chunks)
	{
		if (ChunkPair.Value)
		{
			// Destroy the main mesh component
			if (ChunkPair.Value->GetMesh())
			{
				ChunkPair.Value->GetMesh()->DestroyComponent();
				ChunkPair.Value->GetMesh()=nullptr; 
			}

	
			TArray<FoliageInstance>& GrassMeshes = ChunkPair.Value->GetGrassMesh();
			for (FoliageInstance GrassMesh : GrassMeshes)
			{
				if (GrassMesh.MeshComponent)
				{
					GrassMesh.MeshComponent->ClearInstances();
					GrassMesh.MeshComponent->DestroyComponent();
				}
			}
			GrassMeshes.Empty(); 

			delete ChunkPair.Value;
			ChunkPair.Value = nullptr;
		}
	}


	Chunks.Empty();

	

	

}


void AMarching::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMarching::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Material)
		for (auto& ChunkPair : Chunks)
		{
			if (ChunkPair.Value)
				if (ChunkPair.Value->GetMesh())
				
					ChunkPair.Value->GetMesh()->SetMaterial(0,Material);
		}
			
		
	
}

void AMarching::AddInstanceGrass(Chunk* CurrentChunk, UInstancedStaticMeshComponent* GrassMesh, FMeshInstanceDATA ChosenMesh, FVector WorldLocation)
{
	FTransform InstanceTransform;
	InstanceTransform.SetLocation(WorldLocation);
	InstanceTransform.SetRotation(FQuat::MakeFromEuler(FVector(0, 0, FMath::RandRange(0.f, 360.f))));
	float randScaleConstant = FMath::RandRange(ChosenMesh.MinScale, ChosenMesh.MaxScale);
	InstanceTransform.SetScale3D(FVector(randScaleConstant, randScaleConstant, randScaleConstant));

	int32 GrassId = GrassMesh->AddInstance(InstanceTransform);
	CurrentChunk->GetMeshid().Add(GrassId);
	CurrentChunk->GrassInstancePositions.Add(WorldLocation);
}

void AMarching::GenerateFoliage(FIntPoint chunkCoordinates)
{
	if (StaticMeshes.Num() <= 0||!Chunks.Contains(chunkCoordinates))return;
	
	Chunk* CurrentChunk = Chunks[chunkCoordinates];
	if (!CurrentChunk)return;
	if (CurrentChunk->GetGrassMesh().Num()==0)return;
	
	const TArray<FVector>& Vertices = CurrentChunk->GetVertices();
	TArray<int32>&Triangles=CurrentChunk->GetTriangles();
	UE_LOG(LogTemp, Warning, TEXT("Chunk tiene %d vértices."), Vertices.Num());
	 
	
	int count = 0;
	CurrentChunk->GrassInstancePositions.Empty();  // Limpia si ya tenía
	for (int i=0;i<Triangles.Num();i+=3)
	{
		int32 index1=Triangles[i];
		int32 index2=Triangles[i+1];
		int32 index3=Triangles[i+2];

		FVector v1=Vertices[index1];
		FVector v2=Vertices[index2];
		FVector v3=Vertices[index3];
		
		//calculo de baricentro
		FVector b1=(v1+v2+v3)/3;
		FVector WorldLocation = b1;
		//calculo del area
		float Area = 0.5f * FVector::CrossProduct(v2 - v1, v3 - v1).Size();
		int GrassInstances=int(Area*Density*DensityMultiplyer);
		for (int j=0;j<GrassInstances;j++)
		{
			FoliageInstance ChosenMesh = CurrentChunk->GetGrassMesh()[FMath::RandRange(0, StaticMeshes.Num() - 1)];
			float u = FMath::FRand();
			float v = FMath::FRand();
			if (u + v > 1.0f)
			{
				u = 1.0f - u;
				v = 1.0f - v;
			}
			FVector RandomPoint = v1 + u * (v2 - v1) + v * (v3 - v1);
			AddInstanceGrass(CurrentChunk, ChosenMesh.MeshComponent, ChosenMesh.InstanceData, RandomPoint);  
			
		}
		
	}
		
	



	
}


void AMarching::CreateTerrain()
{
	
	UE_LOG(LogTemp, Warning, TEXT("Terrain Map Size: %d"), TerrainMap.Num());
	
	/*
		This loops iterates over a 3D grid of points 
		to assign a noise value or density to each voxel corner.

		Example for a 2D slice (X-Z plane), GridSize = 2:
		
		Grid of points:
		
		o---o---o   --> X (3 points for 2 cells)
		|   |   |
		o---o---o
		|   |   |
		o---o---o
		
		Each "o" is a vertex (corner), and each square formed by 4 "o" points is a cube (in 2D, a cell).
		To get 2 cubes in X, you need 3 points (2+1).
		The same applies in 3D.

		This loop prepares those points for later cube generation and marching cubes logic.
	*/

	for (int x=0;x<GridSize.X+1;x++)
	{
		
		for (int y=0;y<GridSize.Y+1;y++)
		{
			
		
				
				float height = FMath::PerlinNoise2D(FVector2D((x / 16.f * 1.5f + 0.001f)*noiseScale, (y / 16.f * 1.5f + 0.001f)*noiseScale));
				height = (height + 1.f) * 0.5f; // Remap [-1,1] to [0,1]
				height *= GridSize.Z; // Escalar a la altura máxima
				for (int z = 0; z < GridSize.Z + 1; z++)
				{
				
					float density = z - height; // Positivo: aire, Negativo: sólido
					TerrainMap[getTerrainIndex(x, y, z)].value = density;
				}

			
		}
	}
}

void AMarching::BuildMesh(FIntPoint   chunkCoordinates)
{
	Chunk* CurrentChunk = Chunks[chunkCoordinates];

	// UE_LOG(LogTemp, Warning, TEXT("BuildMesh called"));
	 UE_LOG(LogTemp, Warning, TEXT("Vertices count: %d"), CurrentChunk->GetVertices().Num());
	// UE_LOG(LogTemp, Warning, TEXT("Triangles count: %d"), Triangles.Num());

	TArray<FVector> normals;
	TArray<FVector2D> uvs;
	TArray<FProcMeshTangent> tangents;
	

	// Inicializa las normales, UVs y demás arrays
	//UE_LOG(LogTemp, Warning, TEXT("Initializing mesh data arrays"));
	//normals.Init(FVector(0, 0, 1), Vertices.Num());
	normals.SetNumZeroed(CurrentChunk->GetVertices().Num());
	for (int i = 0; i < CurrentChunk->GetTriangles().Num(); i += 3)
	{
		int i0 = CurrentChunk->GetTriangles()[i];
		int i1 = CurrentChunk->GetTriangles()[i + 1];
		int i2 = CurrentChunk->GetTriangles()[i + 2];
	
		const FVector& v0 = CurrentChunk->GetVertices()[i0];
		const FVector& v1 =CurrentChunk->GetVertices()[i1];
		const FVector& v2 = CurrentChunk->GetVertices()[i2];
	
		FVector faceNormal = FVector::CrossProduct(v1 - v0, v2 - v0).GetSafeNormal();
	
		normals[i0] -= faceNormal;
		normals[i1] -= faceNormal;
		normals[i2] -= faceNormal;
	}
	
	// Paso 3: normalizar cada normal de vértice
	for (int i = 0; i < normals.Num(); ++i)
	{
	
		normals[i].Normalize();
	}
	uvs.SetNum(CurrentChunk->GetVertices().Num());

	

	FVector ChunkOrigin = FVector(chunkCoordinates.X * ChunkSize.X,
								  chunkCoordinates.Y * ChunkSize.Y,
								    0) * TriangleScale; 

	for (int32 i = 0; i < CurrentChunk->GetVertices().Num(); ++i)
	{
		//lo desplazamos como si su sistema de coordenadas fuera los chunks
		FVector LocalVertex = CurrentChunk->GetVertices()[i] - ChunkOrigin;
		//Normalizamos para sacar sus uvs
		float U = LocalVertex.X / (ChunkSize.X * TriangleScale);
		float V = LocalVertex.Y / (ChunkSize.Y * TriangleScale);

		uvs[i] = FVector2D(U, V);
	}
	tangents.Init(FProcMeshTangent(1, 0, 0), CurrentChunk->GetVertices().Num());
	// float R = FMath::FRand(); // Valor entre 0.0 y 1.0
	// float G = FMath::FRand();
	// float B = FMath::FRand();
	//
	// TArray<FLinearColor> linear;
	// linear.Init(FLinearColor(R, G, B), CurrentChunk->GetVertices().Num());
	// UE_LOG(LogTemp, Warning, TEXT("Calling CreateMeshSection_LinearColor"));
	// Crea la malla usando los datos
	CurrentChunk->GetMesh()->CreateMeshSection_LinearColor(0, CurrentChunk->GetVertices(),CurrentChunk->GetTriangles(), normals, uvs, CurrentChunk->GetVertexColors(), tangents, CollisionMesh);
	if (Material)
	{
		CurrentChunk->GetMesh()->SetMaterial(0,Material);
	}
	// UE_LOG(LogTemp, Warning, TEXT("Mesh section created successfully"));
	CurrentChunk->GetMesh()->GetRuntimeVirtualTextures();
}

 uint8 AMarching::GetConfigurationIndex(float* cube)
{
	uint8 configIndex = 0;
	for (int i = 0; i < 8; ++i)
	{
		if ( cube[i] > SurfaceLevel) 
		{
			//desplazar bits a la izquierda si i es igual 3 0000 1000 y el or "suma" los bits con el anterior
			configIndex |= (1 << i);
			
		}
	}

	return configIndex; 


}
void AMarching::MarchCube(FVector pos,float* cube,FIntPoint chunkCoordinates)
{
	uint8 configIndex = GetConfigurationIndex(cube);
	if (configIndex == 0 || configIndex == 255) return;

	int edgeIndex = 0;
	// maximo numero de trinagulos que puede haber en una intersecion de un cubo con estas 256 posibles combinociones 
	for (int i = 0; i < 5; i++)
	{
		
		int vertIndices[3];

		// Recoge los 3 vértices del triángulo primero
		for (int j = 0; j < 3; j++)
		{
			int indice = TriangleTable[configIndex][edgeIndex];
			if (indice == -1) return;

			FVector vert1 = pos + EdgeTable[indice][0];
			FVector vert2 = pos + EdgeTable[indice][1];
			float noise1 = TerrainMap[getTerrainIndex(vert1.X, vert1.Y, vert1.Z)].value;
			float noise2 = TerrainMap[getTerrainIndex(vert2.X, vert2.Y, vert2.Z)].value;
			float color1 = TerrainMap[getTerrainIndex(vert1.X, vert1.Y, vert1.Z)].vertexColor;
			float color2 = TerrainMap[getTerrainIndex(vert2.X, vert2.Y, vert2.Z)].vertexColor;
			float color=FMath::Max(color1,color2);
			float t = (SurfaceLevel - noise1) / (noise2 - noise1);
			FVector vertice = FMath::Lerp(vert1, vert2, t);
			FVector snapped = vertice.GridSnap(0.01f);

			int* valor = Chunks[chunkCoordinates]->GetVertexMap().Find(snapped);
			if (valor)
			{
				vertIndices[j] = *valor;
			}
			else
			{
				Chunks[chunkCoordinates]->GetVertices().Add(vertice * TriangleScale);
				Chunks[chunkCoordinates]->GetVertexColors().Add(FLinearColor(color,0.0f,0.0f));
				int newIndex = Chunks[chunkCoordinates]->GetVertices().Num() - 1;
				Chunks[chunkCoordinates]->GetVertexMap().Add(snapped, newIndex);
				vertIndices[j] = newIndex;
	
			}

			edgeIndex++;
		}

		
		 Chunks[chunkCoordinates]->GetTriangles().Add(vertIndices[0]);
		 Chunks[chunkCoordinates]->GetTriangles().Add(vertIndices[2]);
		 Chunks[chunkCoordinates]->GetTriangles().Add(vertIndices[1]);
	}

}




FORCEINLINE const int AMarching::getTerrainIndex( int x, int y, int z)
{
	return x + y * (GridSize.X + 1) + z * (GridSize.X + 1) * (GridSize.Y + 1);

}


void AMarching::IterateChunkVoxels(int i, int j, FIntPoint LocalChunkSize)
{
	int startX = i * ChunkSize.X;
	int startY = j * ChunkSize.Y;
			

	int endX = startX + LocalChunkSize.X;
	int endY = startY + LocalChunkSize.Y;
				
				
				
				
	for (int x = startX; x < endX; x++)
	{
		for (int y = startY; y < endY; y++)
		{
			for (int z = 0; z < GridSize.Z; z++)
			{
				float cube[8];

				for (int cornerIndex = 0; cornerIndex < 8; cornerIndex++)
				{
					FVector corner = FVector(x, y, z) + CornerTable[cornerIndex];
					cube[cornerIndex] = TerrainMap[getTerrainIndex(corner.X, corner.Y, corner.Z)].value;
				}

				MarchCube(FVector(x, y, z), cube,FIntPoint(i, j));
			}
		}
	}
}

void AMarching::CubeIteration(bool genFoliage)
{
	for (int i = 0; i < NumChunks.X; i++)
	{
		for (int j = 0; j < NumChunks.Y; j++)
		{
			
				// Calcular tamaño real del chunk
				FIntPoint LocalChunkSize = ChunkSize;
				if (i == NumChunks.X - 1 && Remainder.X > 0) LocalChunkSize.X = Remainder.X;
				if (j == NumChunks.Y - 1 && Remainder.Y > 0) LocalChunkSize.Y = Remainder.Y;
				

				generateChunk(FIntPoint(i, j), LocalChunkSize);
			
				IterateChunkVoxels(i, j, LocalChunkSize);
				BuildMesh(FIntPoint(i, j));
				if (genFoliage)
				GenerateFoliage(FIntPoint(i, j));
				
			}
		}
	}




