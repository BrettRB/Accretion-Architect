// Copyright Epic Games, Inc. All Rights Reserved.

#include "Universe/StarSystemGenerator.h"
#include "Core/AccretionMath.h"

namespace StarSystemGeneration
{
	using T = ECelestialBodyType;

	const TCHAR* const PlanetLetters[] = { TEXT("b"), TEXT("c"), TEXT("d"), TEXT("e"), TEXT("f"), TEXT("g"), TEXT("h"), TEXT("i"), TEXT("j") };
	const TCHAR* const MoonNumerals[] = { TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI") };
	const TCHAR* const StarLetters[] = { TEXT("A"), TEXT("B"), TEXT("C") };

	FString MakeSystemName(FRandomStream& Random)
	{
		switch (Random.RandRange(0, 4))
		{
		case 0:  return FString::Printf(TEXT("GJ %d"), Random.RandRange(100, 9999));
		case 1:  return FString::Printf(TEXT("HD %d"), Random.RandRange(1000, 299999));
		case 2:  return FString::Printf(TEXT("Kepler-%d"), Random.RandRange(10, 1999));
		case 3:  return FString::Printf(TEXT("TOI-%d"), Random.RandRange(100, 6999));
		default: return FString::Printf(TEXT("LHS %d"), Random.RandRange(1000, 5999));
		}
	}

	/** Picks a star class by weight among classes no heavier than MaxMass. */
	const FCelestialClassDefinition* PickStarClass(FRandomStream& Random, const UCelestialCatalog& Catalog, float MaxMass)
	{
		float TotalWeight = 0.f;
		for (const FCelestialClassDefinition& Definition : Catalog.Classes)
		{
			if (Definition.bIsStar && Definition.MinMass <= MaxMass)
			{
				TotalWeight += Definition.StarWeight;
			}
		}
		float Roll = Random.FRandRange(0.f, TotalWeight);
		const FCelestialClassDefinition* Fallback = nullptr;
		for (const FCelestialClassDefinition& Definition : Catalog.Classes)
		{
			if (Definition.bIsStar && Definition.MinMass <= MaxMass && Definition.StarWeight > 0.f)
			{
				Fallback = &Definition;
				Roll -= Definition.StarWeight;
				if (Roll <= 0.f)
				{
					return &Definition;
				}
			}
		}
		return Fallback;
	}

	FLinearColor PickColor(FRandomStream& Random, const FCelestialClassDefinition& Definition)
	{
		const FLinearColor Base = Definition.AlternateColors.Num() > 0
			? Definition.AlternateColors[Random.RandRange(0, Definition.AlternateColors.Num() - 1)]
			: Definition.Color;
		// Slight per-body variation so neighbors of the same type are not identical.
		const float Jitter = Random.FRandRange(0.85f, 1.15f);
		return FLinearColor(Base.R * Jitter, Base.G * Random.FRandRange(0.9f, 1.1f) * Jitter, Base.B * Jitter, 1.f);
	}

	FCelestialBodyDesc MakeBody(FRandomStream& Random, const FCelestialClassDefinition& Definition, const FString& Name, float MaxMass)
	{
		FCelestialBodyDesc Body;
		Body.Name = Name;
		Body.Type = Definition.Type;
		Body.bIsStar = Definition.bIsStar;
		Body.Mass = FMath::Min(Random.FRandRange(Definition.MinMass, Definition.MaxMass), MaxMass);
		// Heavier bodies of a class are also larger.
		const float MassAlpha = Definition.MaxMass > Definition.MinMass
			? FMath::Clamp((Body.Mass - Definition.MinMass) / (Definition.MaxMass - Definition.MinMass), 0.f, 1.f) : 0.5f;
		Body.Radius = FMath::Lerp(Definition.MinRadius, Definition.MaxRadius, FMath::Clamp(MassAlpha + Random.FRandRange(-0.15f, 0.15f), 0.f, 1.f));
		Body.Color = Definition.bIsStar ? Definition.Color : PickColor(Random, Definition);
		Body.LightIntensity = Definition.LightIntensity;
		Body.OrbitPhaseDegrees = Random.FRandRange(0.f, 360.f);
		return Body;
	}

	/** Planet type by where it forms relative to the frost line and what the host can hold. */
	T PickPlanetType(FRandomStream& Random, bool bBeyondFrostLine, ECelestialBodyType HostType)
	{
		if (HostType == T::WhiteDwarf)
		{
			return T::RockyPlanet; // survivors of the red giant phase are rocky cores
		}
		if (!bBeyondFrostLine)
		{
			return Random.FRand() < 0.55f ? T::RockyPlanet : T::TerrestrialPlanet;
		}
		const float Roll = Random.FRand();
		return Roll < 0.45f ? T::GasGiant : (Roll < 0.85f ? T::IceGiant : T::RockyPlanet);
	}

	/** Next lighter planet type, used when a roll is too heavy for the host star. */
	bool DowngradePlanetType(T& Type)
	{
		switch (Type)
		{
		case T::GasGiant:          Type = T::IceGiant; return true;
		case T::IceGiant:          Type = T::TerrestrialPlanet; return true;
		case T::TerrestrialPlanet: Type = T::RockyPlanet; return true;
		default:                   return false;
		}
	}

	float HostMassOf(const FStarSystemDesc& System, int32 HostIndex)
	{
		if (HostIndex != INDEX_NONE)
		{
			return System.Bodies[HostIndex].Mass;
		}
		float Total = 0.f;
		for (const FCelestialBodyDesc& Body : System.Bodies)
		{
			Total += Body.bIsStar ? Body.Mass : 0.f;
		}
		return Total;
	}

	/** Lightest star in the system: planets must stay much lighter than it. */
	float LightestStarMass(const FStarSystemDesc& System)
	{
		float Lightest = TNumericLimits<float>::Max();
		for (const FCelestialBodyDesc& Body : System.Bodies)
		{
			if (Body.bIsStar)
			{
				Lightest = FMath::Min(Lightest, Body.Mass);
			}
		}
		return Lightest;
	}

	void AddMoons(FRandomStream& Random, const UCelestialCatalog& Catalog, const FStarSystemGenerationSettings& Settings,
		FStarSystemDesc& System, int32 PlanetIndex, float MaxMoonOrbit)
	{
		const FCelestialClassDefinition* PlanetClass = Catalog.Find(System.Bodies[PlanetIndex].Type);
		const FCelestialClassDefinition* MoonClass = Catalog.Find(T::Moon);
		if (!PlanetClass || !MoonClass || PlanetClass->MaxMoons <= 0)
		{
			return;
		}

		const int32 MoonCount = Random.RandRange(0, PlanetClass->MaxMoons);
		float Orbit = System.Bodies[PlanetIndex].Radius * 2.5f + MoonClass->MaxRadius;
		for (int32 MoonIndex = 0; MoonIndex < MoonCount && MoonIndex < UE_ARRAY_COUNT(MoonNumerals); ++MoonIndex)
		{
			if (Orbit > MaxMoonOrbit)
			{
				break;
			}
			const FCelestialBodyDesc& Planet = System.Bodies[PlanetIndex];
			FCelestialBodyDesc Moon = MakeBody(Random, *MoonClass,
				FString::Printf(TEXT("%s %s"), *Planet.Name, MoonNumerals[MoonIndex]), Planet.Mass * Settings.MaxMoonToPlanetMass);
			Moon.ParentIndex = PlanetIndex;
			Moon.OrbitRadius = Orbit;
			Moon.OrbitPeriod = FAccretionMath::ComputeOrbitPeriod(Orbit, Planet.Mass, Settings.OrbitPeriodConstant, Settings.MinOrbitPeriod);
			Moon.OrbitInclinationDegrees = Random.FRandRange(-8.f, 8.f);
			System.Bodies.Add(Moon);
			Orbit = Orbit * Random.FRandRange(1.4f, 1.8f) + MoonClass->MaxRadius;
		}
	}
}

FStarSystemDesc FStarSystemGenerator::Generate(int32 Seed, const UCelestialCatalog& Catalog, const FStarSystemGenerationSettings& Settings)
{
	using namespace StarSystemGeneration;

	FRandomStream Random(Seed);
	FStarSystemDesc System;
	System.Name = MakeSystemName(Random);
	System.OrbitPlane = FRotator(Random.FRandRange(-25.f, 25.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-25.f, 25.f));
	const float MaxRadius = Settings.MaxSystemRadius;

	// --- Stars ---
	const float MultiplicityRoll = Random.FRand();
	const int32 StarCount = MultiplicityRoll < Settings.TripleChance ? 3 : (MultiplicityRoll < Settings.TripleChance + Settings.BinaryChance ? 2 : 1);

	const FCelestialClassDefinition* PrimaryClass = PickStarClass(Random, Catalog, TNumericLimits<float>::Max());
	if (!PrimaryClass)
	{
		return System;
	}
	auto StarName = [&](int32 Index) { return StarCount > 1 ? FString::Printf(TEXT("%s %s"), *System.Name, StarLetters[Index]) : System.Name; };

	System.Bodies.Add(MakeBody(Random, *PrimaryClass, StarName(0), TNumericLimits<float>::Max()));
	for (int32 Index = 1; Index < StarCount; ++Index)
	{
		const float PrimaryMass = System.Bodies[0].Mass;
		if (const FCelestialClassDefinition* CompanionClass = PickStarClass(Random, Catalog, PrimaryMass))
		{
			// Companions are never heavier than the primary.
			System.Bodies.Add(MakeBody(Random, *CompanionClass, StarName(Index), PrimaryMass));
		}
	}
	int32 Stars = System.Bodies.Num();

	// Pair separation, shrunk to fit the system. Big stars that cannot fit as a pair drop the extra stars
	// (a triple first becomes a binary, then a single star).
	bool bClosePair = Stars == 3 || Random.FRand() < 0.5f;
	float Separation = 0.f;
	while (Stars > 1)
	{
		const FCelestialBodyDesc& A = System.Bodies[0];
		const FCelestialBodyDesc& B = System.Bodies[1];
		const float RadiusSum = A.Radius + B.Radius;
		Separation = bClosePair
			// A triple's inner pair must stay tight so the third star can circle it from far out.
			? FMath::Min(RadiusSum * Random.FRandRange(3.f, 6.f), MaxRadius * (Stars == 3 ? 0.15f : 0.35f))
			: FMath::Min(MaxRadius * Random.FRandRange(0.5f, 0.8f), MaxRadius - FMath::Max(A.Radius, B.Radius));
		if (Separation >= RadiusSum * 2.5f)
		{
			break;
		}
		System.Bodies.SetNum(--Stars);
		if (Stars == 1)
		{
			System.Bodies[0].Name = System.Name;
		}
	}

	// Where planets can go: host (INDEX_NONE = around the center) and the allowed orbit range.
	int32 PlanetHost = 0;
	float InnerOrbit = 0.f;
	// Leaves room for moons around the outermost planet.
	float OuterOrbit = MaxRadius * 0.8f;
	bool bBeltAllowed = true;

	if (Stars == 1)
	{
		InnerOrbit = System.Bodies[0].Radius * 2.5f + Random.FRandRange(10000.f, 25000.f);
	}
	else
	{
		FCelestialBodyDesc& A = System.Bodies[0];
		FCelestialBodyDesc& B = System.Bodies[1];
		const float PairMass = A.Mass + B.Mass;

		// Both stars circle their barycenter on opposite sides.
		const float Period = FAccretionMath::ComputeOrbitPeriod(Separation, PairMass, Settings.OrbitPeriodConstant, Settings.MinOrbitPeriod);
		A.OrbitRadius = Separation * B.Mass / PairMass;
		B.OrbitRadius = Separation * A.Mass / PairMass;
		A.OrbitPeriod = B.OrbitPeriod = Period;
		B.OrbitPhaseDegrees = A.OrbitPhaseDegrees + 180.f;

		if (bClosePair)
		{
			// Circumbinary planets only stay stable well outside the pair.
			PlanetHost = INDEX_NONE;
			InnerOrbit = Separation * 3.f;
		}
		else
		{
			// Wide pair: planets circle the primary, well inside the companion's orbit.
			PlanetHost = 0;
			InnerOrbit = A.Radius * 2.5f + Random.FRandRange(10000.f, 20000.f);
			OuterOrbit = Separation / 3.f;
			bBeltAllowed = false;
		}

		if (Stars == 3)
		{
			// Hierarchical triple: the third star circles the inner pair from far out.
			FCelestialBodyDesc& C = System.Bodies[2];
			C.OrbitRadius = MaxRadius * Random.FRandRange(0.65f, 0.85f) - C.Radius;
			C.OrbitPeriod = FAccretionMath::ComputeOrbitPeriod(C.OrbitRadius, PairMass + C.Mass, Settings.OrbitPeriodConstant, Settings.MinOrbitPeriod);
			OuterOrbit = C.OrbitRadius / 3.f;
		}
	}

	// --- Planets ---
	// Copied: System.Bodies grows below, which would invalidate a reference.
	const FCelestialBodyDesc HostStar = System.Bodies[0];
	const FCelestialClassDefinition* HostClass = Catalog.Find(HostStar.Type);
	const float HostMass = HostMassOf(System, PlanetHost);
	const float MaxPlanetMass = LightestStarMass(System) * Settings.MaxPlanetToStarMass;
	// A pair of stars is brighter than one, pushing the frost line out.
	const float FrostLine = HostClass ? HostClass->FrostLineDistance * (PlanetHost == INDEX_NONE && Stars > 1 ? 1.3f : 1.f) : 0.f;
	const int32 PlanetCount = HostClass ? Random.RandRange(HostClass->MinPlanets, HostClass->MaxPlanets) : 0;
	const bool bGiantHost = HostStar.Type == T::RedGiant;

	float Orbit = InnerOrbit;
	if (bGiantHost)
	{
		// A red giant has engulfed its inner system.
		Orbit = FMath::Max(Orbit, FrostLine);
	}

	int32 PlanetNumber = 0;
	bool bPlacedBelt = !bBeltAllowed || HostStar.Type == T::WhiteDwarf || Random.FRand() >= Settings.BeltChance;
	TArray<int32> PlanetIndices;

	for (int32 Attempt = 0; Attempt < PlanetCount && PlanetNumber < UE_ARRAY_COUNT(PlanetLetters); ++Attempt)
	{
		const bool bBeyondFrost = Orbit >= FrostLine;

		// Debris belt at the frost line, between the rocky and giant zones.
		if (bBeyondFrost && !bPlacedBelt && PlanetIndices.Num() > 0)
		{
			System.BeltInnerRadius = Orbit;
			System.BeltOuterRadius = Orbit * 1.2f;
			Orbit = System.BeltOuterRadius * Random.FRandRange(1.2f, 1.4f);
			bPlacedBelt = true;
		}

		T Type = PickPlanetType(Random, Orbit >= FrostLine, HostStar.Type);
		const FCelestialClassDefinition* PlanetClass = Catalog.Find(Type);
		while (PlanetClass && PlanetClass->MinMass > MaxPlanetMass && DowngradePlanetType(Type))
		{
			PlanetClass = Catalog.Find(Type);
		}
		if (!PlanetClass || PlanetClass->MinMass > MaxPlanetMass)
		{
			continue; // the star is too light to hold any planet of this kind
		}
		if (Orbit + PlanetClass->MaxRadius > OuterOrbit)
		{
			break;
		}

		const FString HostName = PlanetHost == INDEX_NONE && Stars > 1 ? FString::Printf(TEXT("%s (AB)"), *System.Name) : HostStar.Name;
		FCelestialBodyDesc Planet = MakeBody(Random, *PlanetClass, FString::Printf(TEXT("%s %s"), *HostName, PlanetLetters[PlanetNumber]), MaxPlanetMass);
		Planet.ParentIndex = PlanetHost;
		Planet.OrbitRadius = Orbit;
		Planet.OrbitPeriod = FAccretionMath::ComputeOrbitPeriod(Orbit, HostMass, Settings.OrbitPeriodConstant, Settings.MinOrbitPeriod);
		Planet.OrbitInclinationDegrees = Random.FRandRange(-4.f, 4.f);
		PlanetIndices.Add(System.Bodies.Add(Planet));
		++PlanetNumber;

		const float NextOrbit = Orbit * Random.FRandRange(Settings.MinOrbitSpacing, Settings.MaxOrbitSpacing) + Planet.Radius * 2.f;
		// Moons stay well inside the gap to the neighboring orbits.
		AddMoons(Random, Catalog, Settings, System, PlanetIndices.Last(), FMath::Min(Orbit * 0.12f, (NextOrbit - Orbit) * 0.3f));
		Orbit = NextOrbit;
	}

	UpdateSystemRadius(System);
	return System;
}

FStarSystemDesc FStarSystemGenerator::MakeProximaCentauri(const UCelestialCatalog& Catalog, const FStarSystemGenerationSettings& Settings)
{
	using T = ECelestialBodyType;

	FStarSystemDesc System;
	System.Name = TEXT("Proxima Centauri");
	System.OrbitPlane = FRotator(4.f, 20.f, -6.f);

	FCelestialBodyDesc Star;
	Star.Name = TEXT("Proxima Centauri");
	Star.Type = T::RedDwarf;
	Star.bIsStar = true;
	Star.Mass = 60000.f;
	Star.Radius = 16000.f;
	if (const FCelestialClassDefinition* Definition = Catalog.Find(T::RedDwarf))
	{
		Star.Color = Definition->Color;
		Star.LightIntensity = Definition->LightIntensity;
	}
	System.Bodies.Add(Star);

	auto AddPlanet = [&](const TCHAR* Name, T Type, float Mass, float Radius, float Orbit, float Phase, const FLinearColor& Color)
	{
		FCelestialBodyDesc Planet;
		Planet.Name = Name;
		Planet.Type = Type;
		Planet.Mass = Mass;
		Planet.Radius = Radius;
		Planet.Color = Color;
		Planet.ParentIndex = 0;
		Planet.OrbitRadius = Orbit;
		Planet.OrbitPhaseDegrees = Phase;
		Planet.OrbitPeriod = FAccretionMath::ComputeOrbitPeriod(Orbit, Star.Mass, Settings.OrbitPeriodConstant, Settings.MinOrbitPeriod);
		System.Bodies.Add(Planet);
	};
	// Real planets, scaled to gameplay: d is a small rocky world, b an Earth-mass world in the habitable zone.
	AddPlanet(TEXT("Proxima d"), T::RockyPlanet, 250.f, 1800.f, 50000.f, 140.f, FLinearColor(0.42f, 0.4f, 0.38f));
	AddPlanet(TEXT("Proxima b"), T::TerrestrialPlanet, 900.f, 3200.f, 85000.f, 210.f, FLinearColor(0.45f, 0.28f, 0.2f));

	// Proxima has observed cold dust; here it is a belt of small debris to feed on early.
	System.BeltInnerRadius = 115000.f;
	System.BeltOuterRadius = 145000.f;

	UpdateSystemRadius(System);
	return System;
}

FStarSystemDesc FStarSystemGenerator::MakeAlphaCentauri(const UCelestialCatalog& Catalog, const FStarSystemGenerationSettings& Settings)
{
	using T = ECelestialBodyType;

	FStarSystemDesc System;
	System.Name = TEXT("Alpha Centauri");
	System.OrbitPlane = FRotator(-10.f, 75.f, 12.f);

	auto MakeStar = [&](const TCHAR* Name, T Type, float Mass, float Radius)
	{
		FCelestialBodyDesc Star;
		Star.Name = Name;
		Star.Type = Type;
		Star.bIsStar = true;
		Star.Mass = Mass;
		Star.Radius = Radius;
		if (const FCelestialClassDefinition* Definition = Catalog.Find(Type))
		{
			Star.Color = Definition->Color;
			Star.LightIntensity = Definition->LightIntensity;
		}
		return Star;
	};

	// A (G2V) and B (K1V) circle their barycenter.
	FCelestialBodyDesc A = MakeStar(TEXT("Alpha Centauri A"), T::YellowDwarf, 420000.f, 26000.f);
	FCelestialBodyDesc B = MakeStar(TEXT("Alpha Centauri B"), T::OrangeDwarf, 280000.f, 22000.f);
	const float Separation = 240000.f;
	const float PairMass = A.Mass + B.Mass;
	A.OrbitRadius = Separation * B.Mass / PairMass;
	B.OrbitRadius = Separation * A.Mass / PairMass;
	A.OrbitPeriod = B.OrbitPeriod = FAccretionMath::ComputeOrbitPeriod(Separation, PairMass, Settings.OrbitPeriodConstant, Settings.MinOrbitPeriod);
	A.OrbitPhaseDegrees = 30.f;
	B.OrbitPhaseDegrees = 210.f;
	System.Bodies.Add(A);
	System.Bodies.Add(B);

	// Candidate gas giant around A (S-type orbit, well inside B's distance).
	FCelestialBodyDesc Ab;
	Ab.Name = TEXT("Alpha Centauri Ab");
	Ab.Type = T::GasGiant;
	Ab.Mass = 25000.f;
	Ab.Radius = 13000.f;
	Ab.Color = FLinearColor(0.7f, 0.58f, 0.42f);
	Ab.ParentIndex = 0;
	Ab.OrbitRadius = 70000.f;
	Ab.OrbitPhaseDegrees = 90.f;
	Ab.OrbitPeriod = FAccretionMath::ComputeOrbitPeriod(Ab.OrbitRadius, A.Mass, Settings.OrbitPeriodConstant, Settings.MinOrbitPeriod);
	System.Bodies.Add(Ab);

	UpdateSystemRadius(System);
	return System;
}

float FStarSystemGenerator::ComputeBodyExtent(const FStarSystemDesc& System, int32 BodyIndex)
{
	float CenterDistance = 0.f;
	for (int32 Index = BodyIndex, Guard = 0; Index != INDEX_NONE && System.Bodies.IsValidIndex(Index) && Guard < 8; ++Guard)
	{
		CenterDistance += System.Bodies[Index].OrbitRadius;
		Index = System.Bodies[Index].ParentIndex;
	}
	return CenterDistance + System.Bodies[BodyIndex].Radius;
}

void FStarSystemGenerator::UpdateSystemRadius(FStarSystemDesc& System)
{
	float Radius = System.BeltOuterRadius;
	for (int32 Index = 0; Index < System.Bodies.Num(); ++Index)
	{
		Radius = FMath::Max(Radius, ComputeBodyExtent(System, Index));
	}
	System.Radius = Radius;
}
