// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AccretionGameMode.generated.h"

class AAccretionGalaxy;
class AAccretionUniverse;
class AMatterSpawner;

UENUM(BlueprintType)
enum class EAccretionRunEndReason : uint8
{
	None,
	Blowout,
	Starvation,
	/** The black hole became supermassive. */
	Victory
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunEnded, EAccretionRunEndReason, Reason);

/** End-of-run numbers shown on the victory screen. */
USTRUCT(BlueprintType)
struct ACCRETION_ARCHITECT_API FAccretionRunSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Accretion|Run")
	float FinalMass = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Accretion|Run")
	float RunSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Accretion|Run")
	int32 StarsConsumed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Accretion|Run")
	int32 PlanetsConsumed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Accretion|Run")
	int32 MoonsConsumed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Accretion|Run")
	int32 SystemsVisited = 0;
};

/**
 * Run rules: spawns the player's black hole, the space backdrop, the streaming universe, and a matter
 * spawner. The run is won by reaching SupermassiveMass (a Sagittarius A*-class black hole): a galaxy forms
 * around the black hole and the victory screen offers a return to the main menu. Disk blowout or
 * starvation ends the run and restarts it after a short delay.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAccretionGameMode();

	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** Opens the current level as a new run. */
	static void OpenRun(const UObject* WorldContext);

	/** Opens the current level as the main menu. */
	static void OpenMainMenu(const UObject* WorldContext);

	UFUNCTION(BlueprintPure, Category = "Accretion|Run")
	EAccretionRunEndReason GetRunEndReason() const { return RunEndReason; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Run")
	bool HasRunEnded() const { return RunEndReason != EAccretionRunEndReason::None; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Run")
	FAccretionRunSummary GetRunSummary() const;

	UPROPERTY(BlueprintAssignable, Category = "Accretion|Run")
	FOnRunEnded OnRunEnded;

	/** Mass at which the black hole is supermassive and the run is won. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run", meta = (ClampMin = "1"))
	float SupermassiveMass = 4300000.f;

	/** Seconds between a loss and the level restarting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run", meta = (ClampMin = "0"))
	float RestartDelay = 3.f;

	/** Seconds of galaxy formation before the victory screen appears. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run", meta = (ClampMin = "0"))
	float VictoryScreenDelay = 6.f;

	/** Victory galaxy radius as a multiple of the black hole's event horizon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run", meta = (ClampMin = "1"))
	float GalaxyRadiusPerEventHorizon = 200.f;

	/** Spawned at start if the level does not already contain a matter spawner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run")
	TSubclassOf<AMatterSpawner> MatterSpawnerClass;

	/** Space backdrop (sky, fill light, exposure). Spawned at start if the level does not already contain one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run")
	TSubclassOf<AActor> EnvironmentClass;

	/** Streams the star systems around the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run")
	TSubclassOf<AAccretionUniverse> UniverseClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Run")
	TSubclassOf<AAccretionGalaxy> GalaxyClass;

private:
	UFUNCTION()
	void HandleBlowout();

	UFUNCTION()
	void HandleStarved();

	UFUNCTION()
	void HandleMassChanged(float NewMass);

	void EndRun(EAccretionRunEndReason Reason);
	void RestartRun();
	void BeginVictorySequence();
	void ShowVictoryScreen();

	UPROPERTY(Transient)
	TObjectPtr<AAccretionUniverse> Universe;

	EAccretionRunEndReason RunEndReason = EAccretionRunEndReason::None;
	FTimerHandle RestartTimer;
	FTimerHandle VictoryTimer;
	float RunStartTime = 0.f;
	float RunEndTime = -1.f;
};
