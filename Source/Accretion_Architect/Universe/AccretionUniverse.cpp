// Copyright Epic Games, Inc. All Rights Reserved.

#include "Universe/AccretionUniverse.h"
#include "Universe/CelestialBody.h"
#include "Matter/MatterActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogAccretionUniverse, Log, All);

namespace AccretionUniverse
{
	constexpr float UpdateInterval = 0.5f;
	/** Fraction of the sector a system may fill (radius), leaving empty space between neighbors. */
	constexpr float MaxSystemRadiusFraction = 0.45f;
	/** Mass of the star the belt rock range is tuned for (Proxima). */
	constexpr float BeltReferenceStarMass = 60000.f;
	/** Belts appear inside this multiple of the system radius and vanish beyond the larger one. */
	constexpr float BeltSpawnRange = 1.6f;
	constexpr float BeltDespawnRange = 2.2f;

	int32 ChebyshevDistance(const FIntVector& A, const FIntVector& B)
	{
		return FMath::Max3(FMath::Abs(A.X - B.X), FMath::Abs(A.Y - B.Y), FMath::Abs(A.Z - B.Z));
	}
}

AAccretionUniverse::AAccretionUniverse()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

const UCelestialCatalog& AAccretionUniverse::GetCatalog() const
{
	return Catalog ? *Catalog : *GetDefault<UCelestialCatalog>();
}

void AAccretionUniverse::BeginPlay()
{
	Super::BeginPlay();

	RunSeed = Seed != 0 ? Seed : (FMath::Rand() | 1);
	GenerationSettings.MaxSystemRadius = SectorSize * AccretionUniverse::MaxSystemRadiusFraction;
	UE_LOG(LogAccretionUniverse, Log, TEXT("Universe seed %d"), RunSeed);

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	UpdateSectors(Player ? Player->GetActorLocation() : FVector::ZeroVector);
}

void AAccretionUniverse::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeUntilUpdate -= DeltaTime;
	if (TimeUntilUpdate > 0.f)
	{
		return;
	}
	TimeUntilUpdate = AccretionUniverse::UpdateInterval;

	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		UpdateSectors(Player->GetActorLocation());
		UpdateSystemsNearPlayer(Player->GetActorLocation());
	}
}

FIntVector AAccretionUniverse::GetSectorAt(const FVector& Location) const
{
	auto Axis = [this](double Value) { return FMath::FloorToInt32((Value + SectorSize * 0.5) / SectorSize); };
	return FIntVector(Axis(Location.X), Axis(Location.Y), Axis(Location.Z));
}

FVector AAccretionUniverse::GetSectorCenter(const FIntVector& Sector) const
{
	return FVector(Sector) * SectorSize;
}

uint64 AAccretionUniverse::MakeBodyId(const FIntVector& Sector, int32 BodyIndex)
{
	// 18 bits per sector axis (two's complement) + 10 bits of body index.
	constexpr uint64 AxisMask = (1ull << 18) - 1;
	return ((static_cast<uint64>(Sector.X) & AxisMask) << 46) | ((static_cast<uint64>(Sector.Y) & AxisMask) << 28)
		| ((static_cast<uint64>(Sector.Z) & AxisMask) << 10) | (static_cast<uint64>(BodyIndex) & 1023ull);
}

bool AAccretionUniverse::DescribeSector(const FIntVector& Sector, FStarSystemDesc& OutSystem, FVector& OutCenter) const
{
	const UCelestialCatalog& Definitions = GetCatalog();

	if (Sector == GetSectorAt(ProximaLocation))
	{
		OutSystem = FStarSystemGenerator::MakeProximaCentauri(Definitions, GenerationSettings);
		OutCenter = ProximaLocation;
		return true;
	}
	if (Sector == GetSectorAt(AlphaCentauriLocation))
	{
		OutSystem = FStarSystemGenerator::MakeAlphaCentauri(Definitions, GenerationSettings);
		OutCenter = AlphaCentauriLocation;
		return true;
	}

	FRandomStream Random(HashCombine(static_cast<uint32>(RunSeed), GetTypeHash(Sector)));
	if (Random.FRand() >= SystemChance)
	{
		return false;
	}

	OutSystem = FStarSystemGenerator::Generate(Random.RandRange(1, MAX_int32 - 1), Definitions, GenerationSettings);
	if (OutSystem.Bodies.Num() == 0)
	{
		return false;
	}

	// Anywhere in the sector that keeps the whole system inside it.
	const float Slack = FMath::Max(SectorSize * 0.5f - OutSystem.Radius, 0.f);
	OutCenter = GetSectorCenter(Sector) + FVector(Random.FRandRange(-Slack, Slack), Random.FRandRange(-Slack, Slack), Random.FRandRange(-Slack, Slack));
	return true;
}

void AAccretionUniverse::UpdateSectors(const FVector& PlayerLocation)
{
	const FIntVector Current = GetSectorAt(PlayerLocation);
	if (Current == PlayerSector)
	{
		return;
	}
	PlayerSector = Current;

	TArray<FIntVector> ToUnload;
	for (const TPair<FIntVector, FLoadedSector>& Pair : LoadedSectors)
	{
		if (AccretionUniverse::ChebyshevDistance(Pair.Key, Current) > UnloadRadiusSectors)
		{
			ToUnload.Add(Pair.Key);
		}
	}
	for (const FIntVector& Sector : ToUnload)
	{
		UnloadSector(Sector);
	}

	const int32 Radius = LoadRadiusSectors;
	for (int32 X = -Radius; X <= Radius; ++X)
	{
		for (int32 Y = -Radius; Y <= Radius; ++Y)
		{
			for (int32 Z = -Radius; Z <= Radius; ++Z)
			{
				const FIntVector Sector = Current + FIntVector(X, Y, Z);
				if (!LoadedSectors.Contains(Sector))
				{
					LoadSector(Sector);
				}
			}
		}
	}
}

void AAccretionUniverse::LoadSector(const FIntVector& Sector)
{
	FLoadedSector& Loaded = LoadedSectors.Add(Sector);
	Loaded.bHasSystem = DescribeSector(Sector, Loaded.System, Loaded.Center);
	if (!Loaded.bHasSystem)
	{
		return;
	}

	const FStarSystemDesc& System = Loaded.System;
	const FQuat Plane = System.OrbitPlane.Quaternion();
	// Stars light their whole system plus a margin.
	const float LightRadius = System.Radius * 1.5f + 100000.f;

	TArray<ACelestialBody*> Spawned;
	Spawned.SetNumZeroed(System.Bodies.Num());

	for (int32 Index = 0; Index < System.Bodies.Num(); ++Index)
	{
		const uint64 BodyId = MakeBodyId(Sector, Index);
		const float* SavedFraction = BodyMassFractions.Find(BodyId);
		const float Fraction = SavedFraction ? *SavedFraction : 1.f;
		if (Fraction <= 0.f)
		{
			continue;
		}

		// If the parent was eaten, orbit what it orbited, at its distance.
		FCelestialBodyDesc BodyDesc = System.Bodies[Index];
		int32 ParentIndex = BodyDesc.ParentIndex;
		while (ParentIndex != INDEX_NONE && !Spawned[ParentIndex])
		{
			BodyDesc.OrbitRadius += System.Bodies[ParentIndex].OrbitRadius;
			ParentIndex = System.Bodies[ParentIndex].ParentIndex;
		}
		ACelestialBody* ParentActor = ParentIndex != INDEX_NONE ? Spawned[ParentIndex] : nullptr;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACelestialBody* Body = GetWorld()->SpawnActor<ACelestialBody>(ACelestialBody::StaticClass(), Loaded.Center, FRotator::ZeroRotator, Params);
		if (!Body)
		{
			continue;
		}
		Body->InitializeBody(BodyDesc, BodyId, ParentActor, Loaded.Center, Plane, Fraction, LightRadius);
		Body->OnRemoved.AddUObject(this, &AAccretionUniverse::HandleBodyRemoved);
		Spawned[Index] = Body;
		Loaded.Bodies.Add(Body);
	}
}

void AAccretionUniverse::UnloadSector(const FIntVector& Sector)
{
	FLoadedSector* Loaded = LoadedSectors.Find(Sector);
	if (!Loaded)
	{
		return;
	}

	for (const TWeakObjectPtr<ACelestialBody>& WeakBody : Loaded->Bodies)
	{
		if (ACelestialBody* Body = WeakBody.Get())
		{
			// Remember partial shredding so the body comes back the same size.
			if (Body->GetMassFraction() < 1.f)
			{
				BodyMassFractions.Add(Body->GetBodyId(), Body->GetMassFraction());
			}
			Body->OnRemoved.RemoveAll(this);
			Body->Destroy();
		}
	}
	DespawnBelt(Sector, *Loaded);
	LoadedSectors.Remove(Sector);
}

void AAccretionUniverse::UpdateSystemsNearPlayer(const FVector& PlayerLocation)
{
	for (TPair<FIntVector, FLoadedSector>& Pair : LoadedSectors)
	{
		FLoadedSector& Loaded = Pair.Value;
		if (!Loaded.bHasSystem)
		{
			continue;
		}

		const float Distance = FVector::Dist(PlayerLocation, Loaded.Center);
		if (Distance <= Loaded.System.Radius && !VisitedSectors.Contains(Pair.Key))
		{
			VisitedSectors.Add(Pair.Key);
			Stats.SystemsVisited = VisitedSectors.Num();
			UE_LOG(LogAccretionUniverse, Log, TEXT("Entered system %s"), *Loaded.System.Name);
		}

		if (Loaded.System.BeltOuterRadius > 0.f)
		{
			if (!Loaded.bBeltSpawned && Distance <= Loaded.System.Radius * AccretionUniverse::BeltSpawnRange)
			{
				SpawnBelt(Pair.Key, Loaded);
			}
			else if (Loaded.bBeltSpawned && Distance > Loaded.System.Radius * AccretionUniverse::BeltDespawnRange)
			{
				DespawnBelt(Pair.Key, Loaded);
			}
		}
	}
}

void AAccretionUniverse::SpawnBelt(const FIntVector& Sector, FLoadedSector& Loaded)
{
	Loaded.bBeltSpawned = true;
	const int32* Remaining = BeltRocksRemaining.Find(Sector);
	const int32 Count = Remaining ? *Remaining : BeltRockCount;
	if (Count <= 0 || Loaded.System.Bodies.Num() == 0)
	{
		return;
	}

	FRandomStream Random(HashCombine(static_cast<uint32>(RunSeed), GetTypeHash(Sector) + 7919u));
	const FQuat Plane = Loaded.System.OrbitPlane.Quaternion();
	const float MassScale = FMath::Sqrt(FMath::Max(Loaded.System.Bodies[0].Mass, 1.f) / AccretionUniverse::BeltReferenceStarMass);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const float Radius = Random.FRandRange(Loaded.System.BeltInnerRadius, Loaded.System.BeltOuterRadius);
		const FVector Local(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Random.FRandRange(-0.03f, 0.03f) * Radius);
		const FVector Location = Loaded.Center + Plane.RotateVector(Local);

		if (AMatterActor* Rock = GetWorld()->SpawnActor<AMatterActor>(AMatterActor::StaticClass(), Location, FRotator::ZeroRotator, Params))
		{
			Rock->InitializeMatter(Random.FRandRange(BeltRockMinMass, BeltRockMaxMass) * MassScale, Random.GetUnitVector() * Random.FRandRange(0.f, 50.f));
			Loaded.BeltRocks.Add(Rock);
		}
	}
}

void AAccretionUniverse::DespawnBelt(const FIntVector& Sector, FLoadedSector& Loaded)
{
	if (!Loaded.bBeltSpawned)
	{
		return;
	}

	int32 Remaining = 0;
	for (const TWeakObjectPtr<AMatterActor>& WeakRock : Loaded.BeltRocks)
	{
		if (AMatterActor* Rock = WeakRock.Get())
		{
			if (!Rock->IsConsumed())
			{
				++Remaining;
			}
			Rock->Destroy();
		}
	}
	BeltRocksRemaining.Add(Sector, Remaining);
	Loaded.BeltRocks.Reset();
	Loaded.bBeltSpawned = false;
}

void AAccretionUniverse::HandleBodyRemoved(ACelestialBody* Body, bool bConsumed)
{
	if (!Body || !bConsumed)
	{
		return;
	}

	BodyMassFractions.Add(Body->GetBodyId(), 0.f);
	if (Body->IsStar())
	{
		++Stats.StarsConsumed;
	}
	else if (Body->GetBodyType() == ECelestialBodyType::Moon)
	{
		++Stats.MoonsConsumed;
	}
	else
	{
		++Stats.PlanetsConsumed;
	}
	UE_LOG(LogAccretionUniverse, Log, TEXT("Consumed %s"), *Body->GetBodyName());
}

ACelestialBody* AAccretionUniverse::FindNearestBody(const FVector& Location, float MaxSurfaceDistance) const
{
	ACelestialBody* Nearest = nullptr;
	float NearestDistance = MaxSurfaceDistance;
	for (const TPair<FIntVector, FLoadedSector>& Pair : LoadedSectors)
	{
		for (const TWeakObjectPtr<ACelestialBody>& WeakBody : Pair.Value.Bodies)
		{
			ACelestialBody* Body = WeakBody.Get();
			if (!Body || Body->IsConsumed())
			{
				continue;
			}
			const float SurfaceDistance = FVector::Dist(Location, Body->GetActorLocation()) - Body->GetRadius();
			if (SurfaceDistance <= NearestDistance)
			{
				NearestDistance = SurfaceDistance;
				Nearest = Body;
			}
		}
	}
	return Nearest;
}
