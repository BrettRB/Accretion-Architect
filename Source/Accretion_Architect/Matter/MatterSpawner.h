// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MatterSpawner.generated.h"

class AMatterActor;

/**
 * Keeps a population of matter drifting around the player. Replaces consumed matter and
 * recycles matter that drifts too far away. Gray-box stand-in for zone/wave spawning.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AMatterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMatterSpawner();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning")
	TSubclassOf<AMatterActor> MatterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning", meta = (ClampMin = "0"))
	int32 TargetMatterCount = 60;

	/** Matter spawns no closer than this to the player, so it does not appear inside the pull radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning", meta = (ClampMin = "0"))
	float MinSpawnDistance = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning", meta = (ClampMin = "0"))
	float MaxSpawnDistance = 15000.f;

	/** Matter farther than this from the player is moved back into the spawn shell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning", meta = (ClampMin = "0"))
	float RecycleDistance = 25000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning", meta = (ClampMin = "0"))
	float MinMatterMass = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning", meta = (ClampMin = "0"))
	float MaxMatterMass = 5.f;

	/** Maximum random drift speed (cm/s) given to new matter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning", meta = (ClampMin = "0"))
	float MaxDriftSpeed = 200.f;

	/** Seconds between population checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Spawning", meta = (ClampMin = "0.05"))
	float CheckInterval = 0.5f;

private:
	FVector GetFocusLocation() const;
	FVector RandomSpawnLocation(const FVector& Focus) const;
	FVector RandomDriftVelocity() const;
	void SpawnMatter(const FVector& Focus);

	UPROPERTY()
	TArray<TObjectPtr<AMatterActor>> SpawnedMatter;

	float TimeUntilCheck = 0.f;
};
