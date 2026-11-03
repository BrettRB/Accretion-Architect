// Copyright Epic Games, Inc. All Rights Reserved.

#include "Matter/MatterSpawner.h"
#include "Matter/MatterActor.h"
#include "Components/AccretionGravityComponent.h"
#include "Components/AccretionMassComponent.h"
#include "Core/AccretionMath.h"
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

void AMatterSpawner::UpdateScales()
{
	SizeScale = 1.f;
	MassScale = 1.f;
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (const UAccretionGravityComponent* Gravity = PlayerPawn ? PlayerPawn->FindComponentByClass<UAccretionGravityComponent>() : nullptr)
	{
		SizeScale = Gravity->GetSizeScale();
	}
	if (const UAccretionMassComponent* Mass = PlayerPawn ? PlayerPawn->FindComponentByClass<UAccretionMassComponent>() : nullptr)
	{
		MassScale = FMath::Max(static_cast<float>(FAccretionMath::ComputeMassScale(Mass->GetMass(), ReferenceMass, MatterMassExponent)), 1.f);
	}
}

FVector AMatterSpawner::RandomSpawnLocation(const FVector& Focus) const
{
	const float Distance = FMath::FRandRange(MinSpawnDistance, FMath::Max(MinSpawnDistance, MaxSpawnDistance)) * SizeScale;
	return Focus + FMath::VRand() * Distance;
}

float AMatterSpawner::RandomMatterMass() const
{
	return FMath::FRandRange(MinMatterMass, FMath::Max(MinMatterMass, MaxMatterMass)) * MassScale;
}

FVector AMatterSpawner::RandomDriftVelocity() const
{
	return FMath::VRand() * FMath::FRandRange(0.f, MaxDriftSpeed) * SizeScale;
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
		Matter->InitializeMatter(RandomMatterMass(), RandomDriftVelocity());
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

	UpdateScales();
	const FVector Focus = GetFocusLocation();
	const double RecycleDistanceSquared = FMath::Square(static_cast<double>(RecycleDistance) * SizeScale);

	SpawnedMatter.RemoveAll([](const TObjectPtr<AMatterActor>& Matter)
	{
		return !IsValid(Matter) || Matter->IsConsumed();
	});

	for (AMatterActor* Matter : SpawnedMatter)
	{
		if (FVector::DistSquared(Matter->GetActorLocation(), Focus) > RecycleDistanceSquared)
		{
			// Recycled rocks are re-rolled at the current size, so old pebbles do not linger.
			Matter->SetActorLocation(RandomSpawnLocation(Focus));
			Matter->InitializeMatter(RandomMatterMass(), RandomDriftVelocity());
		}
	}

	while (SpawnedMatter.Num() < TargetMatterCount)
	{
		SpawnMatter(Focus);
	}
}
