// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Universe/CelestialTypes.h"

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

	/**
	 * (Mass / ReferenceMass) ^ Exponent, the factor most "scale with size" rules use.
	 * Returns 1 for non-positive inputs so callers never divide by zero.
	 */
	static double ComputeMassScale(double Mass, double ReferenceMass, double Exponent);

	/**
	 * Mass lost per second. Equals DecayRate * Mass at the reference mass, and grows more slowly
	 * than mass above it (Exponent < 1), so large black holes are not starved by their own size.
	 */
	static double ComputeDecayPerSecond(double Mass, double DecayRate, double ReferenceMass, double Exponent);

	/**
	 * How a black hole of BlackHoleMass can eat a body of BodyMass. Bodies at least ShredMassRatio
	 * times lighter can be tidally shredded; bodies at least SwallowMassRatio times lighter are swallowed whole.
	 */
	static ECelestialConsumeMode GetConsumeMode(double BlackHoleMass, double BodyMass, double ShredMassRatio, double SwallowMassRatio);

	/** Black hole mass needed to reach Mode for a body of BodyMass (0 for TooMassive). */
	static double GetMassRequiredFor(ECelestialConsumeMode Mode, double BodyMass, double ShredMassRatio, double SwallowMassRatio);

	/**
	 * Progress from StartingMass to TargetMass on a log scale (0..1), so each doubling counts the same.
	 */
	static double ComputeLogProgress(double Mass, double StartingMass, double TargetMass);

	/**
	 * Orbital period (seconds of game time) for a circular orbit of OrbitRadius around HostMass.
	 * Kepler's third law (T proportional to r^1.5 / sqrt(M)) with a gameplay constant.
	 */
	static double ComputeOrbitPeriod(double OrbitRadius, double HostMass, double PeriodConstant, double MinPeriod);
};
