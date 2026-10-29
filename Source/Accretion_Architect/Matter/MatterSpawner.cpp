// Copyright Epic Games, Inc. All Rights Reserved.

#include "Matter/MatterSpawner.h"
#include "Matter/MatterActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AMatterSpawner::AMatterSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	MatterClass = AMatterActor::StaticClass();
}

void AMatterSpawner::BeginPlay()
{
	Super::BeginPlay();
	TimeUntilCheck = 0.f;
}

FVector AMatterSpawner::GetFocusLocation() const
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	return PlayerPawn ? PlayerPawn->GetActorLocation() : GetActorLocation();
}

FVector AMatterSpawner::RandomSpawnLocation(const FVector& Focus) const
{
	const float Distance = FMath::FRandRange(MinSpawnDistance, FMath::Max(MinSpawnDistance, MaxSpawnDistance));
	return Focus + FMath::VRand() * Distance;
}

FVector AMatterSpawner::RandomDriftVelocity() const
{
	return FMath::VRand() * FMath::FRandRange(0.f, MaxDriftSpeed);
}

void AMatterSpawner::SpawnMatter(const FVector& Focus)
{
	if (!MatterClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMatterActor* Matter = GetWorld()->SpawnActor<AMatterActor>(MatterClass, RandomSpawnLocation(Focus), FRotator::ZeroRotator, Params);
	if (Matter)
	{
		Matter->InitializeMatter(FMath::FRandRange(MinMatterMass, FMath::Max(MinMatterMass, MaxMatterMass)), RandomDriftVelocity());
		SpawnedMatter.Add(Matter);
	}
}

void AMatterSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeUntilCheck -= DeltaTime;
	if (TimeUntilCheck > 0.f)
	{
		return;
	}
	TimeUntilCheck = CheckInterval;

	const FVector Focus = GetFocusLocation();
	const double RecycleDistanceSquared = FMath::Square(static_cast<double>(RecycleDistance));

	SpawnedMatter.RemoveAll([](const TObjectPtr<AMatterActor>& Matter)
	{
		return !IsValid(Matter) || Matter->IsConsumed();
	});

	for (AMatterActor* Matter : SpawnedMatter)
	{
		if (FVector::DistSquared(Matter->GetActorLocation(), Focus) > RecycleDistanceSquared)
		{
			Matter->SetActorLocation(RandomSpawnLocation(Focus));
			Matter->SetVelocity(RandomDriftVelocity());
		}
	}

	while (SpawnedMatter.Num() < TargetMatterCount)
	{
		SpawnMatter(Focus);
	}
}
