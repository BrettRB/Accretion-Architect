// Copyright Epic Games, Inc. All Rights Reserved.

#include "Universe/CelestialTypes.h"

#define LOCTEXT_NAMESPACE "CelestialTypes"

namespace CelestialDefaults
{
	FCelestialClassDefinition Body(ECelestialBodyType Type, const FText& Name, float MinMass, float MaxMass,
		float MinRadius, float MaxRadius, const FLinearColor& Color)
	{
		FCelestialClassDefinition Definition;
		Definition.Type = Type;
		Definition.DisplayName = Name;
		Definition.MinMass = MinMass;
		Definition.MaxMass = MaxMass;
		Definition.MinRadius = MinRadius;
		Definition.MaxRadius = MaxRadius;
		Definition.Color = Color;
		return Definition;
	}

	FCelestialClassDefinition Planet(ECelestialBodyType Type, const FText& Name, float MinMass, float MaxMass,
		float MinRadius, float MaxRadius, int32 MaxMoons, TArray<FLinearColor> Colors)
	{
		FCelestialClassDefinition Definition = Body(Type, Name, MinMass, MaxMass, MinRadius, MaxRadius, Colors[0]);
		Definition.AlternateColors = MoveTemp(Colors);
		Definition.MaxMoons = MaxMoons;
		return Definition;
	}

	FCelestialClassDefinition Star(ECelestialBodyType Type, const FText& Name, float MinMass, float MaxMass,
		float MinRadius, float MaxRadius, const FLinearColor& Color, float Weight, float Light, float FrostLine,
		int32 MinPlanets, int32 MaxPlanets)
	{
		FCelestialClassDefinition Definition = Body(Type, Name, MinMass, MaxMass, MinRadius, MaxRadius, Color);
		Definition.bIsStar = true;
		Definition.StarWeight = Weight;
		Definition.LightIntensity = Light;
		Definition.FrostLineDistance = FrostLine;
		Definition.MinPlanets = MinPlanets;
		Definition.MaxPlanets = MaxPlanets;
		return Definition;
	}
}

UCelestialCatalog::UCelestialCatalog()
{
	using namespace CelestialDefaults;
	using T = ECelestialBodyType;

	// Masses climb in steps of roughly 3-10x so the black hole graduates from moons to planets to stars.
	Classes = {
		Planet(T::Moon, LOCTEXT("Moon", "Moon"), 30.f, 150.f, 500.f, 1400.f, 0,
			{ FLinearColor(0.45f, 0.45f, 0.45f), FLinearColor(0.5f, 0.45f, 0.38f), FLinearColor(0.6f, 0.58f, 0.55f) }),
		Planet(T::RockyPlanet, LOCTEXT("Rocky", "Rocky Planet"), 150.f, 600.f, 1500.f, 3000.f, 1,
			{ FLinearColor(0.42f, 0.4f, 0.38f), FLinearColor(0.55f, 0.3f, 0.18f), FLinearColor(0.5f, 0.42f, 0.32f) }),
		Planet(T::TerrestrialPlanet, LOCTEXT("Terrestrial", "Terrestrial Planet"), 600.f, 2000.f, 3000.f, 4500.f, 2,
			{ FLinearColor(0.15f, 0.3f, 0.5f), FLinearColor(0.45f, 0.28f, 0.2f), FLinearColor(0.35f, 0.42f, 0.3f) }),
		Planet(T::IceGiant, LOCTEXT("IceGiant", "Ice Giant"), 5000.f, 15000.f, 8000.f, 11000.f, 3,
			{ FLinearColor(0.35f, 0.6f, 0.75f), FLinearColor(0.25f, 0.4f, 0.8f) }),
		Planet(T::GasGiant, LOCTEXT("GasGiant", "Gas Giant"), 15000.f, 60000.f, 12000.f, 20000.f, 4,
			{ FLinearColor(0.7f, 0.55f, 0.38f), FLinearColor(0.75f, 0.65f, 0.5f), FLinearColor(0.65f, 0.4f, 0.25f) }),

		// Weights roughly follow how common each star is, nudged toward variety.
		Star(T::RedDwarf, LOCTEXT("RedDwarf", "Red Dwarf (M)"), 40000.f, 150000.f, 12000.f, 18000.f,
			FLinearColor(1.6f, 0.32f, 0.06f), 60.f, 4.f, 60000.f, 0, 3),
		Star(T::OrangeDwarf, LOCTEXT("OrangeDwarf", "Orange Dwarf (K)"), 150000.f, 300000.f, 18000.f, 24000.f,
			FLinearColor(2.f, 0.9f, 0.35f), 14.f, 6.f, 90000.f, 1, 6),
		Star(T::YellowDwarf, LOCTEXT("YellowDwarf", "Yellow Dwarf (G)"), 300000.f, 450000.f, 24000.f, 30000.f,
			FLinearColor(2.4f, 1.9f, 1.1f), 10.f, 8.f, 120000.f, 2, 8),
		Star(T::YellowWhiteStar, LOCTEXT("YellowWhite", "Yellow-White Star (F)"), 450000.f, 650000.f, 30000.f, 38000.f,
			FLinearColor(2.4f, 2.2f, 1.7f), 6.f, 10.f, 150000.f, 2, 7),
		Star(T::WhiteStar, LOCTEXT("White", "White Star (A)"), 650000.f, 1000000.f, 38000.f, 50000.f,
			FLinearColor(2.f, 2.1f, 2.6f), 4.f, 12.f, 190000.f, 0, 4),
		Star(T::BlueGiant, LOCTEXT("BlueGiant", "Blue Giant (B)"), 1000000.f, 2500000.f, 55000.f, 80000.f,
			FLinearColor(1.4f, 1.7f, 3.f), 2.f, 16.f, 260000.f, 0, 2),
		Star(T::BlueSupergiant, LOCTEXT("BlueSupergiant", "Blue Supergiant (O)"), 3000000.f, 6000000.f, 80000.f, 110000.f,
			FLinearColor(1.2f, 1.5f, 3.4f), 0.5f, 20.f, 320000.f, 0, 1),
		Star(T::WhiteDwarf, LOCTEXT("WhiteDwarf", "White Dwarf"), 200000.f, 500000.f, 2500.f, 4000.f,
			FLinearColor(2.5f, 2.6f, 3.f), 2.5f, 3.f, 20000.f, 0, 1),
		Star(T::RedGiant, LOCTEXT("RedGiant", "Red Giant"), 300000.f, 800000.f, 100000.f, 140000.f,
			FLinearColor(1.8f, 0.55f, 0.15f), 1.5f, 8.f, 250000.f, 0, 2),
	};
}

const FCelestialClassDefinition* UCelestialCatalog::Find(ECelestialBodyType Type) const
{
	return Classes.FindByPredicate([Type](const FCelestialClassDefinition& Definition) { return Definition.Type == Type; });
}

int32 FStarSystemDesc::NumStars() const
{
	int32 Count = 0;
	for (const FCelestialBodyDesc& Body : Bodies)
	{
		Count += Body.bIsStar ? 1 : 0;
	}
	return Count;
}

#undef LOCTEXT_NAMESPACE
