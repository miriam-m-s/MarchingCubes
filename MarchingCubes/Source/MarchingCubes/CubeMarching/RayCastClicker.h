// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RayCastClicker.generated.h"
class UNiagaraSystem;
class ABurnActor;

UCLASS()
class MARCHINGCUBES_API ARayCastClicker : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARayCastClicker();
	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* ClickEffect;

	// UPROPERTY(EditAnywhere, Category = "Burn Actor")
	// TSubclassOf<ABurnActor> BurnActorBP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burn Actor")
	class ARuntimeVirtualTextureVolume* RVTVolumeRef;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	void HandleMouseClick();
};
