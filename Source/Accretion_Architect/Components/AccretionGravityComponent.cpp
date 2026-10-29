// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AccretionGravityComponent.h"
#include "Components/AccretionMassComponent.h"
#include "Core/AccretionMath.h"
#include "Matter/MatterActor.h"
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

void UAccretionGravityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetOwner();
	const FVector Center = Owner->GetActorLocation();
	const FVector OwnerVelocity = Owner->GetVelocity();
	const double Mass = GetOwnerMass();
	const double PullRadius = GetPullRadius();
	const double EventHorizon = GetEventHorizonRadius();

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

		const FVector Gravity = FAccretionMath::ComputeGravityAcceleration(Offset, Mass, GravityConstant, PullRadius, MinDistance, MaxAcceleration);
		const FVector Drag = -RelativeVelocity * OrbitalDrag;
		Matter->AddAcceleration(Gravity + Drag);
	}
}
