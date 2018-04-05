// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandController.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AHandController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHandController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetHand(EControllerHand Hand);

private:
	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* MotionController;
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* ControllerMeshComponent;
	
};