// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	UPROPERTY()
	class AHandController* LeftController;
	UPROPERTY()
	class AHandController* RightController;
	UPROPERTY()
	class UCameraComponent* Camera;
	UPROPERTY()
	class USceneComponent* VRRoot;
	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;
	UPROPERTY()
	class UPostProcessComponent* PostProcessComponent;
	UPROPERTY()
	class UMaterialInstanceDynamic* BlinkerInstance;
	UPROPERTY()
	TArray<class USplineMeshComponent*> TeleportArchMeshPool;

private: // Configuration Parameters
	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 800;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10;

	UPROPERTY(EditAnywhere)
	float TeleportSimulationTime = 2;

	UPROPERTY(EditAnywhere)
	float TeleportFadeDuration = 0.5;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100, 100, 100);

	UPROPERTY(EditAnywhere)
	class UMaterialInterface * BlinkerMaterialBase;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditDefaultsOnly)
		class UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* TeleportArchMaterial;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AHandController> ControllerBPClass;

private:
	bool FindTeleportDestination(FVector & OutLocation);
	void MoveForward(float AxisIn);
	void MoveRight(float AxisIn);
	void UpdateDestinationMarker();
	void BeginTeleport();
	void FinishTeleport();
	void UpdateBlinkers();
	void DrawTeleportPath(struct FPredictProjectilePathResult TeleportPathResult);
	void UpdateSpline(FPredictProjectilePathResult TeleportPathResult);
	FVector2D GetBlinkerCenter();

};
