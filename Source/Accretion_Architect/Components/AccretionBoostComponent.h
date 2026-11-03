// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AccretionBoostComponent.generated.h"

class UAccretionMassComponent;
class UMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAccretionBoost);

/**
 * Burst of speed that costs mass. Adds BoostSpeed to the owner's velocity in the given direction;
 * velocity above the movement component's max speed then bleeds off through its deceleration.
 * A boost is refused while cooling down or if paying for it would starve the black hole.
 */
UCLASS(ClassGroup = (Accretion), meta = (BlueprintSpawnableComponent))
class ACCRETION_ARCHITECT_API UAccretionBoostComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAccretionBoostComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Boosts the owner using its mass and movement components. Returns true if the boost happened. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Boost")
	bool TryBoost(const FVector& Direction);

	/**
	 * Boost logic with explicit dependencies (used by TryBoost and by tests). SpeedScale multiplies
	 * BoostSpeed (the owner passes its size scale so boosts grow with the black hole).
	 */
	bool TryBoostUsing(UAccretionMassComponent* Mass, UMovementComponent* Movement, const FVector& Direction, float SpeedScale = 1.f);

	/** Mass a boost costs at the given current mass. */
	UFUNCTION(BlueprintPure, Category = "Accretion|Boost")
	float GetMassCost(float CurrentMass) const;

	/** Counts down the cooldown. Called from TickComponent. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Boost")
	void UpdateCooldown(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Accretion|Boost")
	bool IsCoolingDown() const { return CooldownRemaining > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Boost")
	float GetCooldownRemaining() const { return CooldownRemaining; }

	/** True if Mass can pay for a boost without dropping below its starvation threshold. */
	UFUNCTION(BlueprintPure, Category = "Accretion|Boost")
	bool CanAfford(const UAccretionMassComponent* Mass) const;

	UPROPERTY(BlueprintAssignable, Category = "Accretion|Boost")
	FOnAccretionBoost OnBoost;

	/** Flat mass spent per boost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Boost", meta = (ClampMin = "0"))
	float MassCost = 0.f;

	/** Share of current mass spent per boost, so boosting costs the same at any size (5 at the starting 100). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Boost", meta = (ClampMin = "0", ClampMax = "1"))
	float MassCostFraction = 0.05f;

	/** Speed (cm/s) added in the boost direction, before the size scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Boost", meta = (ClampMin = "0"))
	float BoostSpeed = 4000.f;

	/** Seconds between boosts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Boost", meta = (ClampMin = "0"))
	float Cooldown = 1.5f;

private:
	float CooldownRemaining = 0.f;
};
