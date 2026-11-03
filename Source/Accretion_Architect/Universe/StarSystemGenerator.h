// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Universe/CelestialTypes.h"

/** Knobs for star system generation. Distances in cm, periods in seconds of game time. */
struct ACCRETION_ARCHITECT_API FStarSystemGenerationSettings
{
	/** Every orbit must fit inside this radius (keeps systems inside their sector). */
	float MaxSystemRadius = 360000.f;

	/** Chance a system has two stars / three stars. The rest are single. */
	float BinaryChance = 0.3f;
	float TripleChance = 0.05f;

	/** Chance of an asteroid belt at the frost line. */
	float BeltChance = 0.4f;

	/** Each planet orbits this much farther out than the previous one (Titius-Bode-like spacing). */
	float MinOrbitSpacing = 1.4f;
	float MaxOrbitSpacing = 1.9f;

	/** A planet can be at most this fraction of its host star's mass; a moon this fraction of its planet's. */
	float MaxPlanetToStarMass = 0.1f;
	float MaxMoonToPlanetMass = 0.2f;

	/** Kepler's-law constant and floor for orbital periods. */
	float OrbitPeriodConstant = 0.0044f;
	float MinOrbitPeriod = 30.f;
};

/**
 * Builds star systems as plain data, deterministically from a seed. Follows simple astrophysical rules:
 * - one star in most systems, sometimes a binary (close pair with planets circling both, or wide pair
 *   with planets around the primary), rarely a hierarchical triple; never a star orbiting a planet;
 * - companion stars are no heavier than the primary;
 * - rocky worlds form inside the frost line, giants mostly outside it;
 * - planets are much lighter than their star, moons much lighter than their planet;
 * - orbits are spaced outward geometrically and stay within the system radius.
 */
struct ACCRETION_ARCHITECT_API FStarSystemGenerator
{
	static FStarSystemDesc Generate(int32 Seed, const UCelestialCatalog& Catalog, const FStarSystemGenerationSettings& Settings);

	/** The starting system: Proxima Centauri with Proxima d, Proxima b, and a debris belt. */
	static FStarSystemDesc MakeProximaCentauri(const UCelestialCatalog& Catalog, const FStarSystemGenerationSettings& Settings);

	/** The neighboring system: the Alpha Centauri A/B binary with the candidate gas giant Alpha Centauri Ab. */
	static FStarSystemDesc MakeAlphaCentauri(const UCelestialCatalog& Catalog, const FStarSystemGenerationSettings& Settings);

	/** Distance from the system center to the far edge of a body, following its parent chain. */
	static float ComputeBodyExtent(const FStarSystemDesc& System, int32 BodyIndex);

	/** Recomputes System.Radius from its bodies and belt. */
	static void UpdateSystemRadius(FStarSystemDesc& System);
};
