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
