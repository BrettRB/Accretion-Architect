// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Universe/CelestialTypes.h"
#include "AccretionGravityComponent.generated.h"

class ACelestialBody;
class UAccretionMassComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatterConsumed, float, MatterMass, float, RadialFraction);

/**
 * Pulls nearby matter toward the owner. Matter keeps its own velocity, so gravity bends it into
 * orbits; a small drag inside the pull radius makes those orbits decay into spirals.
 * Matter that crosses the event horizon is consumed and reported through OnMatterConsumed.
 * Pull radius, event horizon, and strength all scale with the owner's mass component.
 *
 * Also handles celestial bodies (planets, moons, stars) whose surface is inside the pull radius:
 * bodies the black hole outweighs are tidally shredded into fragments; much lighter bodies are pulled
 * off their orbits and swallowed whole on touching the event horizon; heavier bodies are unaffected.
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

	/**
	 * Event horizon radius relative to a black hole of ReferenceMass. Speeds, accelerations, and
	 * distances that should grow with the black hole are multiplied by this.
	 */
	UFUNCTION(BlueprintPure, Category = "Accretion|Gravity")
	float GetSizeScale() const;

	/** Whether the owner can currently shred or swallow Body, or must grow first. */
	UFUNCTION(BlueprintPure, Category = "Accretion|Gravity")
	ECelestialConsumeMode GetConsumeModeFor(const ACelestialBody* Body) const;

	/** Owner mass needed to reach Mode for Body. */
	UFUNCTION(BlueprintPure, Category = "Accretion|Gravity")
	float GetMassRequiredFor(const ACelestialBody* Body, ECelestialConsumeMode Mode) const;

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

	/** Mass at which GetSizeScale() is 1 (the starting black hole). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Gravity", meta = (ClampMin = "1"))
	float ReferenceMass = 100.f;

	/** The black hole must be at least this many times a body's mass to tidally shred it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Celestial", meta = (ClampMin = "0"))
	float ShredMassRatio = 1.f;

	/** The black hole must be at least this many times a body's mass to swallow it whole. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Celestial", meta = (ClampMin = "0"))
	float SwallowMassRatio = 10.f;

	/** Sideways speed of shredded fragments as a fraction of circular orbit speed, so they spiral in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Celestial", meta = (ClampMin = "0"))
	float FragmentOrbitSpeedFactor = 0.7f;

	/**
	 * Most mass the black hole can strip from a body per second, as a fraction of its own mass. With the
	 * default pressure tuning, a smooth stream at this rate stays under the disk's vent rate.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Celestial", meta = (ClampMin = "0"))
	float MaxShredFractionPerSecond = 0.01f;

	/**
	 * How much a whole-body swallow's impact angle counts toward disk pressure (0 = always smooth,
	 * 1 = as head-on as it really is). Captured bodies fall almost straight in, so full weight would
	 * make every swallow a blowout.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Celestial", meta = (ClampMin = "0", ClampMax = "1"))
	float SwallowImpactWeight = 0.3f;

private:
	void UpdateCelestialBodies(const FVector& Center, const FVector& OwnerVelocity, double Mass, double PullRadius,
		double EventHorizon, double ScaledMaxAcceleration);

	float GetOwnerMass() const;

	UPROPERTY()
	TObjectPtr<UAccretionMassComponent> MassComponent;
};
