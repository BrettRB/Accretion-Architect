// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AccretionGravityComponent.h"
#include "Components/AccretionMassComponent.h"
#include "Core/AccretionMath.h"
#include "Matter/MatterActor.h"
#include "Universe/CelestialBody.h"
#include "EngineUtils.h"

UAccretionGravityComponent::UAccretionGravityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAccretionGravityComponent::BeginPlay()
{
	Super::BeginPlay();
	MassComponent = GetOwner()->FindComponentByClass<UAccretionMassComponent>();
}

float UAccretionGravityComponent::GetOwnerMass() const
{
	return MassComponent ? MassComponent->GetMass() : 0.f;
}

float UAccretionGravityComponent::GetPullRadius() const
{
	return static_cast<float>(FAccretionMath::ComputePullRadius(GetOwnerMass(), BasePullRadius, PullRadiusPerSqrtMass));
}

float UAccretionGravityComponent::GetEventHorizonRadius() const
{
	return static_cast<float>(FAccretionMath::ComputeEventHorizonRadius(GetOwnerMass(), BaseEventHorizonRadius, EventHorizonRadiusPerSqrtMass));
}

float UAccretionGravityComponent::GetSizeScale() const
{
	const double ReferenceHorizon = FAccretionMath::ComputeEventHorizonRadius(ReferenceMass, BaseEventHorizonRadius, EventHorizonRadiusPerSqrtMass);
	return ReferenceHorizon > 0.0 ? FMath::Max(static_cast<float>(GetEventHorizonRadius() / ReferenceHorizon), 1.f) : 1.f;
}

ECelestialConsumeMode UAccretionGravityComponent::GetConsumeModeFor(const ACelestialBody* Body) const
{
	return Body ? FAccretionMath::GetConsumeMode(GetOwnerMass(), Body->GetMass(), ShredMassRatio, SwallowMassRatio) : ECelestialConsumeMode::TooMassive;
}

float UAccretionGravityComponent::GetMassRequiredFor(const ACelestialBody* Body, ECelestialConsumeMode Mode) const
{
	return Body ? static_cast<float>(FAccretionMath::GetMassRequiredFor(Mode, Body->GetMass(), ShredMassRatio, SwallowMassRatio)) : 0.f;
}

void UAccretionGravityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetOwner();
	const FVector Center = Owner->GetActorLocation();
	const FVector OwnerVelocity = Owner->GetVelocity();
	const double Mass = GetOwnerMass();
	const double PullRadius = GetPullRadius();
	const double EventHorizon = GetEventHorizonRadius();
	// A bigger black hole pulls harder, so distances that grow with it are still crossed in similar time.
	const double ScaledMaxAcceleration = MaxAcceleration * GetSizeScale();

	for (TActorIterator<AMatterActor> It(GetWorld()); It; ++It)
	{
		AMatterActor* Matter = *It;
		if (!IsValid(Matter) || Matter->IsConsumed())
		{
			continue;
		}

		const FVector Offset = Matter->GetActorLocation() - Center;
		const double Distance = Offset.Size();
		if (Distance > PullRadius)
		{
			continue;
		}

		const FVector RelativeVelocity = Matter->GetVelocity() - OwnerVelocity;

		if (Distance <= EventHorizon)
		{
			const float RadialFraction = static_cast<float>(FAccretionMath::ComputeRadialFraction(RelativeVelocity, Offset));
			const float MatterMass = Matter->GetMatterMass();
			Matter->Consume();
			OnMatterConsumed.Broadcast(MatterMass, RadialFraction);
			continue;
		}

		const FVector Gravity = FAccretionMath::ComputeGravityAcceleration(Offset, Mass, GravityConstant, PullRadius, MinDistance, ScaledMaxAcceleration);
		const FVector Drag = -RelativeVelocity * OrbitalDrag;
		Matter->AddAcceleration(Gravity + Drag);
	}

	UpdateCelestialBodies(Center, OwnerVelocity, Mass, PullRadius, EventHorizon, ScaledMaxAcceleration);
}

void UAccretionGravityComponent::UpdateCelestialBodies(const FVector& Center, const FVector& OwnerVelocity, double Mass,
	double PullRadius, double EventHorizon, double ScaledMaxAcceleration)
{
	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	for (TActorIterator<ACelestialBody> It(GetWorld()); It; ++It)
	{
		ACelestialBody* Body = *It;
		if (!IsValid(Body) || Body->IsConsumed())
		{
			continue;
		}

		const FVector Offset = Body->GetActorLocation() - Center;
		const double Distance = Offset.Size();
		const double BodyRadius = Body->GetRadius();
		const double SurfaceDistance = Distance - BodyRadius;
		if (SurfaceDistance > PullRadius)
		{
			continue;
		}

		switch (GetConsumeModeFor(Body))
		{
		case ECelestialConsumeMode::Shred:
		{
			// Fragments start near the body's surface; give them enough sideways speed to orbit, then drag spirals them in.
			const double FragmentDistance = FMath::Max(SurfaceDistance, MinDistance);
			const double Pull = FMath::Min(GravityConstant * Mass / (FragmentDistance * FragmentDistance), ScaledMaxAcceleration);
			const float FragmentSpeed = static_cast<float>(FMath::Sqrt(Pull * FragmentDistance) * FragmentOrbitSpeedFactor);
			Body->Shred(DeltaTime, Center, OwnerVelocity, FragmentSpeed, static_cast<float>(Mass * MaxShredFractionPerSecond));
			break;
		}
		case ECelestialConsumeMode::SwallowWhole:
		{
			if (SurfaceDistance <= EventHorizon)
			{
				const FVector RelativeVelocity = Body->GetVelocity() - OwnerVelocity;
				const float RadialFraction = static_cast<float>(FAccretionMath::ComputeRadialFraction(RelativeVelocity, Offset));
				const float BodyMass = Body->ConsumeWhole();
				OnMatterConsumed.Broadcast(BodyMass, RadialFraction * SwallowImpactWeight);
				break;
			}
			if (!Body->IsCaptured())
			{
				Body->Capture();
			}
			// Pull on the body's center even though only its surface is within reach.
			const FVector Gravity = FAccretionMath::ComputeGravityAcceleration(Offset, Mass, GravityConstant, PullRadius + BodyRadius, MinDistance, ScaledMaxAcceleration);
			const FVector Drag = -(Body->GetVelocity() - OwnerVelocity) * OrbitalDrag;
			Body->AddAcceleration(Gravity + Drag);
			break;
		}
		default:
			break; // too massive: the black hole has to grow first
		}
	}
}
