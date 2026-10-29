// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure gameplay formulas shared by the gravity, mass, and disk pressure components.
 * Kept free of world/actor state so they can be unit tested directly.
 * All values are gray-box placeholders; tune them through the component properties.
 */
struct ACCRETION_ARCHITECT_API FAccretionMath
{
	/** Radius (cm) inside which the black hole pulls matter. Grows with the square root of mass. */
	static double ComputePullRadius(double Mass, double BaseRadius, double RadiusPerSqrtMass);

	/** Radius (cm) at which matter is consumed. Grows with the square root of mass. */
	static double ComputeEventHorizonRadius(double Mass, double BaseRadius, double RadiusPerSqrtMass);

	/**
	 * Gravitational acceleration (cm/s^2) applied to matter at Offset from the black hole center
	 * (Offset = MatterLocation - Center). Strength is G * Mass / r^2, clamped to MaxAcceleration.
	 * Returns zero outside PullRadius.
	 */
	static FVector ComputeGravityAcceleration(const FVector& Offset, double Mass, double GravityConstant,
		double PullRadius, double MinDistance, double MaxAcceleration);

	/**
	 * How head-on an impact is, from 0 (purely tangential, a smooth spiral) to 1 (falling straight in).
	 * RelativeVelocity is matter velocity minus black hole velocity; Offset = MatterLocation - Center.
	 */
	static double ComputeRadialFraction(const FVector& RelativeVelocity, const FVector& Offset);

	/** Pressure added by consuming MatterMass, scaled between the smooth and head-on multipliers. */
	static double ComputeFeedPressure(double MatterMass, double PressurePerMass,
		double SmoothMultiplier, double HeadOnMultiplier, double RadialFraction);
};
