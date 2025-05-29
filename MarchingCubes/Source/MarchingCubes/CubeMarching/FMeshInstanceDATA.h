#pragma once

// #include "CoreMinimal.h"
class UStaticMesh;
#include "FMeshInstanceDATA.generated.h"

USTRUCT(BlueprintType)
struct FMeshInstanceDATA
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass")
	UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass")
	float MinScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grass")
	float MaxScale = 1.0f;
};
