// Fill out your copyright notice in the Description page of Project Settings.


#include "RayCastClicker.h"
#include "Engine/PostProcessVolume.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "Marching.h"

// Sets default values
ARayCastClicker::ARayCastClicker():PostProEffect(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void ARayCastClicker::HandlePostPro(float Value)
{
	
	if (PostProcessVolume)
	{
		
		UE_LOG(LogTemp, Warning, TEXT("post Process Volume: %f"),Value);
		PostProcessVolume->Settings.bOverride_SceneFringeIntensity = true;
		PostProcessVolume->Settings.SceneFringeIntensity = Value; // Value viene de la curva
	}
}
void ARayCastClicker::OnPostProFinished()
{
	

	if (PostProcessVolume)
	{
		// Apaga el efecto si lo deseas
		PostProcessVolume->Settings.SceneFringeIntensity = 0.0f;
	}
}

// Called when the game starts or when spawned
void ARayCastClicker::BeginPlay()
{
	Super::BeginPlay();
	EnableInput(GetWorld()->GetFirstPlayerController());
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}
	if (InputComponent)
	{
		InputComponent->BindAction("LeftClick", IE_Pressed, this, &ARayCastClicker::HandleMouseClick);
	}
	if (FloatCurve)
	{
		FOnTimelineFloat ProgressFunction;
		ProgressFunction.BindUFunction(this, FName("HandlePostPro"));
		PostProTimeline.AddInterpFloat(FloatCurve, ProgressFunction);

		FOnTimelineEvent FinishedFunction;
		FinishedFunction.BindUFunction(this, FName("OnPostProFinished"));
		PostProTimeline.SetTimelineFinishedFunc(FinishedFunction);

		PostProTimeline.SetLooping(false);
	}
}

// Called every frame
void ARayCastClicker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	HoleGen.Broadcast(FVector(0, 0, 0));
	// ACTUALIZA EL TIMELINE AQUÍ
	PostProTimeline.TickTimeline(DeltaTime);
	// if (!PostProEffect)return;
	// float CurveValue = FloatCurve->GetFloatValue(ElapsedTime);
}
void ARayCastClicker::HandleMouseClick()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		FVector Start = WorldLocation;
		FVector End = Start + WorldDirection * 10000.0f;

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.bTraceComplex = true;
		Params.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
		{
			FVector HitLocation = HitResult.ImpactPoint;
			UE_LOG(LogTemp, Warning, TEXT("¡Colisión en: %s"), *HitLocation.ToString());
			DrawDebugSphere(GetWorld(), HitLocation, 20.0f, 12, FColor::Green, false, 2.0f);

			// Verificamos si el actor golpeado es de tipo AMarching o está contenido dentro de uno
			AMarching* Terrain = Cast<AMarching>(HitResult.GetActor());
			if (!Terrain && HitResult.Component.IsValid())
			{
				Terrain = Cast<AMarching>(HitResult.GetComponent()->GetOwner());
			}

			if (Terrain)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					ClickEffect,
					HitLocation,
					FRotator::ZeroRotator,
					FVector(1.0f), // Escala
					true,          // AutoDestroy
					true,          // AutoActivate
					ENCPoolMethod::None,
					true           // PreciseLocation
				);
				
				Terrain->GenerateHole(HitLocation,Radius);
				PostProEffect = true;
				if (FloatCurve && !PostProTimeline.IsPlaying())
				{
					PostProTimeline.PlayFromStart();
				}
				
				// FActorSpawnParameters SpawnParams;
				// GetWorld()->SpawnActor<AExpandingSphere>(AExpandingSphere::StaticClass(), HitLocation, FRotator::ZeroRotator, SpawnParams);
				
				// if (BurnActorBP)
				// {
				// 	FActorSpawnParameters SpawnParams;
				// 	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				//
				// 	FVector SpawnLocation = FVector(HitLocation.X, HitLocation.Y,  HitLocation.Z + 500);
				// 	FRotator SpawnRotation = FRotator::ZeroRotator;
				//
				// 	ABurnActor* Instancia = GetWorld()->SpawnActor<ABurnActor>(BurnActorBP, SpawnLocation, SpawnRotation, SpawnParams);
				// 	// Instancia->StartBurn();
				// 	Instancia->SetRVTVolume(RVTVolumeRef);
				// }

			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("No se pudo hacer cast a AMarching desde el actor golpeado."));
			}
		}
	}
}

