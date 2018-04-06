// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Classes/Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "AI/Navigation/NavigationSystem.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "ConstructorHelpers.h"


// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(VRRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent >(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent >(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
	 
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (BlinkerMaterialBase != nullptr)
	{
		BlinkerInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerInstance);
	}

	if (ControllerBPClass) 
	{
		LeftController = GetWorld()->SpawnActor<AHandController>(ControllerBPClass);
		if (LeftController != nullptr)
		{
			LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
			LeftController->SetHand(EControllerHand::Left);
		}
		RightController = GetWorld()->SpawnActor<AHandController>(ControllerBPClass);
		if (RightController != nullptr)
		{
			RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
			RightController->SetHand(EControllerHand::Right);
		}
	}

	if(LeftController != nullptr && RightController != nullptr)
	{
		LeftController->PairController(RightController);
	}
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);

	UpdateDestinationMarker();

	UpdateBlinkers();
}

bool AVRCharacter::FindTeleportDestination(FVector & OutLocation)
{
	FHitResult HitResult;
	FVector Start = RightController->GetActorLocation();
	FVector Look = RightController->GetActorForwardVector();
	Look = Look.RotateAngleAxis(45, RightController->GetActorRightVector());
	FVector End = Start + Look * TeleportProjectileSpeed;

	FPredictProjectilePathParams TeleportPathParams = FPredictProjectilePathParams(
		TeleportProjectileRadius, 
		Start, 
		Look * TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this
	);

	FPredictProjectilePathResult TeleportPathResult;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, TeleportPathParams, TeleportPathResult);
	if (!bHit) return false;

	DrawTeleportPath(TeleportPathResult);

	const FVector& HitLocation = TeleportPathResult.HitResult.Location;
	FNavLocation NavSystemHitLocation;
	UNavigationSystem* NavigationSystem = GetWorld()->GetNavigationSystem();

	bool bNavHit = NavigationSystem->ProjectPointToNavigation(HitLocation, NavSystemHitLocation, TeleportProjectionExtent);
	if (!bNavHit) return false;

	OutLocation = HitLocation;
	return bHit && bNavHit;
}

void AVRCharacter::UpdateDestinationMarker()
{
	FVector HitLocation;
	bool bHasDestination = FindTeleportDestination(HitLocation);
	if (bHasDestination)
	{
			DestinationMarker->SetWorldLocation(HitLocation);
			DestinationMarker->SetVisibility(true);
	}
	else
	{
		DestinationMarker->SetVisibility(false);

		FPredictProjectilePathResult EmptyPath = FPredictProjectilePathResult();
		DrawTeleportPath(EmptyPath);
	}
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction("Teleport", IE_Released, this, &AVRCharacter::BeginTeleport);
	PlayerInputComponent->BindAction("GripLeft", IE_Pressed, this, &AVRCharacter::GripLeft);
	PlayerInputComponent->BindAction("GripLeft", IE_Released, this, &AVRCharacter::ReleaseLeft);
	PlayerInputComponent->BindAction("GripRight", IE_Pressed, this, &AVRCharacter::GripRight);
	PlayerInputComponent->BindAction("GripRight", IE_Released, this, &AVRCharacter::ReleaseRight);
}

void AVRCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity != nullptr)
	{
		float Speed = GetVelocity().Size();
		float Radius = RadiusVsVelocity->GetFloatValue(Speed);

		BlinkerInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

		FVector2D Center = GetBlinkerCenter();
		BlinkerInstance->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0));
	}
}

void AVRCharacter::DrawTeleportPath(FPredictProjectilePathResult TeleportPathResult)
{
	UpdateSpline(TeleportPathResult);

	// populate the meshes required for the mesh pool
	for (int32 i = 0; i < TeleportPathResult.PathData.Num(); ++i)
	{
		if (TeleportArchMeshPool.Num() <= i)
			{
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArchMesh);
			SplineMesh->SetMaterial(0, TeleportArchMaterial);
			SplineMesh->RegisterComponent();
			
			TeleportArchMeshPool.Add(SplineMesh);
			}
	}

	//All meshes should be not visible by default. We will set visibility only when we pull one out and place it in the world
	for (int32 i = 0; i < TeleportArchMeshPool.Num(); i++)
	{
		TeleportArchMeshPool[i]->SetVisibility(false);
	}

	//Pull meshes from pool, set their locations/tangents, and make them visible
	for (int32 j = 0; j < (TeleportPath->GetNumberOfSplinePoints() - 1); j++)
	{
		USplineMeshComponent* SplineMesh = TeleportArchMeshPool[j];

		FVector StartLocation, EndLocation, StartTangent, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(j, StartLocation, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(j + 1, EndLocation, EndTangent);

		SplineMesh->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, true);
		SplineMesh->SetVisibility(true);
	}
}

void AVRCharacter::UpdateSpline(FPredictProjectilePathResult TeleportPathResult)
{
	TArray<FSplinePoint> SplinePoints;
	float InputKey = 0;
	TeleportPath->ClearSplinePoints(false);

	for (FPredictProjectilePathPointData Point : TeleportPathResult.PathData)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Point.Location);
		FSplinePoint SplinePoint = FSplinePoint(InputKey, LocalPosition, ESplinePointType::Curve);
		SplinePoints.Add(SplinePoint);
		++InputKey;
	}

	TeleportPath->AddPoints(SplinePoints, false);
	TeleportPath->UpdateSpline();
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero())	{ return FVector2D(0.5, 0.5); }

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if(PlayerController == nullptr){ return FVector2D(0.5, 0.5); }
	FVector WorldStationaryLocation;

	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}

	FVector2D ScreenLocation;
	if (PlayerController->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenLocation))
	{
		int32 ViewportSizeX;
		int32 ViewportSizeY;
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
		return FVector2D(ScreenLocation.X / ViewportSizeX, ScreenLocation.Y / ViewportSizeY);
	}
	else{ return FVector2D(0.5, 0.5); }
}

void AVRCharacter::MoveForward(float AxisIn)
{
	AddMovementInput(Camera->GetForwardVector(), AxisIn);
}

void AVRCharacter::MoveRight(float AxisIn)
{
	AddMovementInput(Camera->GetRightVector(), AxisIn);
}

void AVRCharacter::BeginTeleport()
{
	float FromAlpha = 0;
	float ToAlpha = 1;
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (PlayerController != nullptr)
	{
		APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;

		CameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeDuration, FLinearColor::Black, false, true);
		FTimerManager& TimerManager = GetWorldTimerManager();
		FTimerHandle FadeTimer;
		TimerManager.SetTimer(FadeTimer, this, &AVRCharacter::FinishTeleport, TeleportFadeDuration, false);
	}
}

void AVRCharacter::FinishTeleport()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController != nullptr)
	{
		APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;

		FVector Destination = DestinationMarker->GetComponentLocation();
		Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
		SetActorLocation(Destination);

		float FromAlpha = 1;
		float ToAlpha = 0;
		CameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeDuration, FLinearColor::Black, false, true);
	}
}