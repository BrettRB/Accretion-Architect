// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AccretionMassComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMassChanged, float, NewMass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStarved);

/**
 * Tracks the black hole's mass (the player's score). Mass decays over time;
 * dropping below the starvation threshold fires OnStarved once.
 */
UCLASS(ClassGroup = (Accretion), meta = (BlueprintSpawnableComponent))
class ACCRETION_ARCHITECT_API UAccretionMassComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAccretionMassComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Adds mass (negative values are ignored). */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Mass")
	void AddMass(float Amount);

	/** Applies decay for DeltaTime and checks for starvation. Called from TickComponent. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Mass")
	void UpdateMass(float DeltaTime);

	/** Restores the starting mass and clears the starved state. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Mass")
	void ResetMass();

	UFUNCTION(BlueprintPure, Category = "Accretion|Mass")
	float GetMass() const { return Mass; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Mass")
	bool IsStarved() const { return bStarved; }

	UPROPERTY(BlueprintAssignable, Category = "Accretion|Mass")
	FOnMassChanged OnMassChanged;

	UPROPERTY(BlueprintAssignable, Category = "Accretion|Mass")
	FOnStarved OnStarved;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Mass", meta = (ClampMin = "0"))
	float StartingMass = 100.f;

	/** Fraction of current mass lost per second. Larger black holes lose more in absolute terms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Mass", meta = (ClampMin = "0"))
	float DecayRatePerSecond = 0.01f;

	/** Falling below this mass ends the run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Mass", meta = (ClampMin = "0"))
	float StarvationThreshold = 20.f;

private:
	void SetMass(float NewMass);

	UPROPERTY(VisibleInstanceOnly, Category = "Accretion|Mass")
	float Mass = 100.f;

	bool bStarved = false;
};
