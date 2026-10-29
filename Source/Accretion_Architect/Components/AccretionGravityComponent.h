// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AccretionGravityComponent.generated.h"

class UAccretionMassComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatterConsumed, float, MatterMass, float, RadialFraction);

/**
 * Pulls nearby matter toward the owner. Matter keeps its own velocity, so gravity bends it into
 * orbits; a small drag inside the pull radius makes those orbits decay into spirals.
 * Matter that crosses the event horizon is consumed and reported through OnMatterConsumed.
 * Pull radius, event horizon, and strength all scale with the owner's mass component.
 */
UCLASS(ClassGroup = (Accretion), meta = (BlueprintSpawnableComponent))
class ACCRETION_ARCHITECT_API UAccretionGravityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAccretionGravityComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Accretion|Gravity")
	float GetPullRadius() const;

	UFUNCTION(BlueprintPure, Category = "Accretion|Gravity")
	float GetEventHorizonRadius() const;

	/** Fired when matter crosses the event horizon. RadialFraction: 0 = smooth spiral, 1 = head-on. */
	UPROPERTY(BlueprintAssignable, Category = "Accretion|Gravity")
	FOnMatterConsumed OnMatterConsumed;

	/** G in G * Mass / r^2 (cm^3 / s^2 per unit mass). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "0"))
	float GravityConstant = 2.7e7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "0"))
	float BasePullRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "0"))
	float PullRadiusPerSqrtMass = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "0"))
	float BaseEventHorizonRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "0"))
	float EventHorizonRadiusPerSqrtMass = 15.f;

	/** Distance floor used in the inverse-square falloff to avoid infinite acceleration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "1"))
	float MinDistance = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "0"))
	float MaxAcceleration = 6000.f;

	/** Drag (1/s) on matter velocity relative to the black hole, inside the pull radius. Turns orbits into spirals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "0"))
	float OrbitalDrag = 0.15f;

private:
	float GetOwnerMass() const;

	UPROPERTY()
	TObjectPtr<UAccretionMassComponent> MassComponent;
};
