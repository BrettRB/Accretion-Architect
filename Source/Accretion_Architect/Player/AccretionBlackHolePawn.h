// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AccretionBlackHolePawn.generated.h"

class UAccretionGravityComponent;
class UAccretionMassComponent;
class UCameraComponent;
class UDiskPressureComponent;
class UFloatingPawnMovement;
class USphereComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/**
 * The player's black hole. Coordinates movement, camera, and its gameplay components;
 * gravity, mass, and disk pressure logic live in their own components.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionBlackHolePawn : public APawn
{
	GENERATED_BODY()

public:
	AAccretionBlackHolePawn();

	virtual void BeginPlay() override;

	/** Stops movement, feeding, and decay after the run ends. */
	UFUNCTION(BlueprintCallable, Category = "Accretion")
	void FreezeForRunEnd();

	UAccretionMassComponent* GetMassComponent() const { return MassComponent; }
	UDiskPressureComponent* GetDiskPressureComponent() const { return DiskPressureComponent; }
	UAccretionGravityComponent* GetGravityComponent() const { return GravityComponent; }

	/** Mass gained per unit of consumed matter mass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion", meta = (ClampMin = "0"))
	float MassGainPerMatterMass = 1.f;

	/** Camera distance as a multiple of the event horizon radius, so the view pulls back as the black hole grows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Camera", meta = (ClampMin = "0"))
	float CameraDistancePerEventHorizonRadius = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Camera", meta = (ClampMin = "0"))
	float BaseCameraDistance = 600.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAccretionMassComponent> MassComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDiskPressureComponent> DiskPressureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAccretionGravityComponent> GravityComponent;

private:
	UFUNCTION()
	void HandleMatterConsumed(float MatterMass, float RadialFraction);

	UFUNCTION()
	void HandleMassChanged(float NewMass);

	void UpdateSizeFromMass();
};
