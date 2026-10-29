// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DiskPressureComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPressureChanged, float, NewPressure);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBlowout);

/**
 * Accretion disk pressure. Feeding raises pressure (smooth spirals less than head-on impacts),
 * pressure vents over time, and reaching MaxPressure fires OnBlowout once.
 */
UCLASS(ClassGroup = (Accretion), meta = (BlueprintSpawnableComponent))
class ACCRETION_ARCHITECT_API UDiskPressureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDiskPressureComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Adds raw pressure and checks for blowout. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Pressure")
	void AddPressure(float Amount);

	/**
	 * Adds pressure for consuming MatterMass. RadialFraction is 0 for a smooth spiral and 1 for a head-on impact.
	 * Returns the pressure that was added.
	 */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Pressure")
	float AddFeedPressure(float MatterMass, float RadialFraction);

	/** Vents pressure for DeltaTime. Called from TickComponent. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Pressure")
	void UpdatePressure(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Accretion|Pressure")
	void ResetPressure();

	UFUNCTION(BlueprintPure, Category = "Accretion|Pressure")
	float GetPressure() const { return Pressure; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Pressure")
	float GetPressureRatio() const { return MaxPressure > 0.f ? Pressure / MaxPressure : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Pressure")
	bool IsBlownOut() const { return bBlownOut; }

	UPROPERTY(BlueprintAssignable, Category = "Accretion|Pressure")
	FOnPressureChanged OnPressureChanged;

	UPROPERTY(BlueprintAssignable, Category = "Accretion|Pressure")
	FOnBlowout OnBlowout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Pressure", meta = (ClampMin = "1"))
	float MaxPressure = 100.f;

	/** Pressure vented per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Pressure", meta = (ClampMin = "0"))
	float VentRatePerSecond = 4.f;

	/** Base pressure per unit of consumed matter mass, before the smooth/head-on multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Pressure", meta = (ClampMin = "0"))
	float PressurePerMass = 4.f;

	/** Multiplier for matter that spirals in tangentially. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Pressure", meta = (ClampMin = "0"))
	float SmoothFeedMultiplier = 0.5f;

	/** Multiplier for matter that falls straight in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Pressure", meta = (ClampMin = "0"))
	float HeadOnFeedMultiplier = 3.f;

private:
	void SetPressure(float NewPressure);

	UPROPERTY(VisibleInstanceOnly, Category = "Accretion|Pressure")
	float Pressure = 0.f;

	bool bBlownOut = false;
};
