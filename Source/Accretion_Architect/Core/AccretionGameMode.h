// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AccretionGameMode.generated.h"

class AMatterSpawner;

UENUM(BlueprintType)
enum class EAccretionRunEndReason : uint8
{
	None,
	Blowout,
	Starvation
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunEnded, EAccretionRunEndReason, Reason);

/**
 * Gray-box run rules: spawns the player's black hole and a matter spawner,
 * ends the run on disk blowout or starvation, and restarts the level after a short delay.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAccretionGameMode();

	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	UFUNCTION(BlueprintPure, Category = "Accretion|Run")
	EAccretionRunEndReason GetRunEndReason() const { return RunEndReason; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Run")
	bool HasRunEnded() const { return RunEndReason != EAccretionRunEndReason::None; }

	UPROPERTY(BlueprintAssignable, Category = "Accretion|Run")
	FOnRunEnded OnRunEnded;

	/** Seconds between the run ending and the level restarting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run", meta = (ClampMin = "0"))
	float RestartDelay = 3.f;

	/** Spawned at start if the level does not already contain a matter spawner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run")
	TSubclassOf<AMatterSpawner> MatterSpawnerClass;

private:
	UFUNCTION()
	void HandleBlowout();

	UFUNCTION()
	void HandleStarved();

	void EndRun(EAccretionRunEndReason Reason);
	void RestartRun();

	EAccretionRunEndReason RunEndReason = EAccretionRunEndReason::None;
	FTimerHandle RestartTimer;
};
