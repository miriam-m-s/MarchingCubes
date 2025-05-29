
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "RayCastClicker.generated.h"

class UNiagaraSystem;
class APostProcessVolume;
class ABurnActor;
class UCurveFloat;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHoleEvent, FVector, ClickLocation);
UCLASS()
class MARCHINGCUBES_API ARayCastClicker : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARayCastClicker();
	UPROPERTY(EditAnywhere, Category = "Hole")
	UNiagaraSystem* ClickEffect;
	UPROPERTY(EditAnywhere, Category = "Hole")
	float Radius=5.0;
	// UPROPERTY(EditAnywhere, Category = "Burn Actor")
	// TSubclassOf<ABurnActor> BurnActorBP;
	UPROPERTY(EditAnywhere, Category = "Curve")
	UCurveFloat* FloatCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burn Actor")
	class ARuntimeVirtualTextureVolume* RVTVolumeRef;
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnHoleEvent HoleGen;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess")
	APostProcessVolume* PostProcessVolume;
	UFUNCTION()
	void HandlePostPro(float Value);
	void OnPostProFinished();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	FTimeline PostProTimeline;
	void HandleMouseClick();
	bool PostProEffect;
};
