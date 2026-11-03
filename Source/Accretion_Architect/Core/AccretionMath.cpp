// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/AccretionMath.h"

double FAccretionMath::ComputePullRadius(double Mass, double BaseRadius, double RadiusPerSqrtMass)
{
	return BaseRadius + RadiusPerSqrtMass * FMath::Sqrt(FMath::Max(Mass, 0.0));
}

double FAccretionMath::ComputeEventHorizonRadius(double Mass, double BaseRadius, double RadiusPerSqrtMass)
{
	return BaseRadius + RadiusPerSqrtMass * FMath::Sqrt(FMath::Max(Mass, 0.0));
}

FVector FAccretionMath::ComputeGravityAcceleration(const FVector& Offset, double Mass, double GravityConstant,
	double PullRadius, double MinDistance, double MaxAcceleration)
{
	const double Distance = Offset.Size();
	if (Distance > PullRadius || Distance <= UE_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	const double ClampedDistance = FMath::Max(Distance, MinDistance);
	const double Strength = FMath::Min(GravityConstant * Mass / (ClampedDistance * ClampedDistance), MaxAcceleration);

	// Pull toward the center, which is the opposite of the offset.
	return -Offset / Distance * Strength;
}

double FAccretionMath::ComputeRadialFraction(const FVector& RelativeVelocity, const FVector& Offset)
{
	const double Speed = RelativeVelocity.Size();
	const double Distance = Offset.Size();
	if (Speed <= UE_KINDA_SMALL_NUMBER || Distance <= UE_SMALL_NUMBER)
	{
		return 0.0;
	}

	const FVector Inward = -Offset / Distance;
	return FMath::Clamp(FVector::DotProduct(RelativeVelocity / Speed, Inward), 0.0, 1.0);
}

double FAccretionMath::ComputeFeedPressure(double MatterMass, double PressurePerMass,
	double SmoothMultiplier, double HeadOnMultiplier, double RadialFraction)
{
	const double Multiplier = FMath::Lerp(SmoothMultiplier, HeadOnMultiplier, FMath::Clamp(RadialFraction, 0.0, 1.0));
	return FMath::Max(MatterMass, 0.0) * PressurePerMass * Multiplier;
}

double FAccretionMath::ComputeMassScale(double Mass, double ReferenceMass, double Exponent)
{
	if (Mass <= 0.0 || ReferenceMass <= 0.0)
	{
		return 1.0;
	}
	return FMath::Pow(Mass / ReferenceMass, Exponent);
}

double FAccretionMath::ComputeDecayPerSecond(double Mass, double DecayRate, double ReferenceMass, double Exponent)
{
	if (Mass <= 0.0 || ReferenceMass <= 0.0)
	{
		return 0.0;
	}
	return DecayRate * ReferenceMass * ComputeMassScale(Mass, ReferenceMass, Exponent);
}

ECelestialConsumeMode FAccretionMath::GetConsumeMode(double BlackHoleMass, double BodyMass, double ShredMassRatio, double SwallowMassRatio)
{
	if (BlackHoleMass >= BodyMass * SwallowMassRatio)
	{
		return ECelestialConsumeMode::SwallowWhole;
	}
	if (BlackHoleMass >= BodyMass * ShredMassRatio)
	{
		return ECelestialConsumeMode::Shred;
	}
	return ECelestialConsumeMode::TooMassive;
}

double FAccretionMath::GetMassRequiredFor(ECelestialConsumeMode Mode, double BodyMass, double ShredMassRatio, double SwallowMassRatio)
{
	switch (Mode)
	{
	case ECelestialConsumeMode::Shred:        return BodyMass * ShredMassRatio;
	case ECelestialConsumeMode::SwallowWhole: return BodyMass * SwallowMassRatio;
	default:                                  return 0.0;
	}
}

double FAccretionMath::ComputeLogProgress(double Mass, double StartingMass, double TargetMass)
{
	if (StartingMass <= 0.0 || TargetMass <= StartingMass)
	{
		return Mass >= TargetMass ? 1.0 : 0.0;
	}
	const double Progress = FMath::Loge(FMath::Max(Mass, StartingMass) / StartingMass) / FMath::Loge(TargetMass / StartingMass);
	return FMath::Clamp(Progress, 0.0, 1.0);
}

double FAccretionMath::ComputeOrbitPeriod(double OrbitRadius, double HostMass, double PeriodConstant, double MinPeriod)
{
	if (OrbitRadius <= 0.0 || HostMass <= 0.0)
	{
		return MinPeriod;
	}
	return FMath::Max(PeriodConstant * FMath::Pow(OrbitRadius, 1.5) / FMath::Sqrt(HostMass), MinPeriod);
}
