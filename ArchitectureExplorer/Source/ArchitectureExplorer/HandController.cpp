// Fill out your copyright notice in the Description page of Project Settings.

#include "HandController.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Haptics/HapticFeedbackEffect_Base.h"


// Sets default values
AHandController::AHandController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	SetRootComponent(MotionController);

	ControllerMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ControllerMeshComponent"));
	ControllerMeshComponent->SetupAttachment(MotionController);
}

// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &AHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AHandController::ActorEndOverlap);
}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsClimbing)
	{
		APawn* Player = Cast<APawn>(GetAttachParentActor());
		if (Player)
		{
			FVector Offset = GetActorLocation() - ClimbingStartLocation;
			Player->AddActorWorldOffset(-Offset);
		}
	}

}

void AHandController::PairController(AHandController* Controller)
{
	OtherController = Controller;
	OtherController->OtherController = this;
}

void AHandController::SetHand(EControllerHand Hand)
{
	MotionController->Hand = Hand;
}

void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bool bNewCanClimb = CanClimb();
	if (!bCanClimb && bNewCanClimb)
	{
		APawn* Player = Cast<APawn>(GetAttachParentActor());
		if (Player && HapticEffect)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
			if (PlayerController)
			{
				PlayerController->PlayHapticEffect(HapticEffect, MotionController->Hand);
			}
		}
	}
	bCanClimb = bNewCanClimb;
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bCanClimb = CanClimb();
}

bool AHandController::CanClimb() const
{
	TArray <AActor*>  OverlappingActors;
	this->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor->ActorHasTag(TEXT("Climbable")))
		{
			return true;
		}
	}
	return false;
}

void AHandController::Grip()
{
	if (CanClimb() && !bIsClimbing)
	{
		bIsClimbing = true;
		ClimbingStartLocation = GetActorLocation();
		OtherController->bIsClimbing = false;
		ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character != nullptr)
		{
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		}
	}
}

void AHandController::Release()
{
	if (bIsClimbing)
	{
		bIsClimbing = false;

		ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character != nullptr)
		{
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		}
	}

}