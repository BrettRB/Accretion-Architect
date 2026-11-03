// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AccretionMassComponent.h"
#include "Core/AccretionMath.h"

UAccretionMassComponent::UAccretionMassComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Mass = StartingMass;
}

void UAccretionMassComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetMass();
}

void UAccretionMassComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateMass(DeltaTime);
}

void UAccretionMassComponent::AddMass(float Amount)
{
	if (bStarved || Amount <= 0.f)
	{
		return;
	}
	SetMass(Mass + Amount);
}

bool UAccretionMassComponent::TrySpendMass(float Amount)
{
	if (bStarved || Amount < 0.f || Mass - Amount < StarvationThreshold)
	{
		return false;
	}
	SetMass(Mass - Amount);
	return true;
}

void UAccretionMassComponent::UpdateMass(float DeltaTime)
{
	if (bStarved || DeltaTime <= 0.f)
	{
		return;
	}

	const float Decay = static_cast<float>(FAccretionMath::ComputeDecayPerSecond(Mass, DecayRatePerSecond, DecayReferenceMass, DecayMassExponent));
	SetMass(Mass - FMath::Min(Decay * DeltaTime, Mass));

	if (Mass < StarvationThreshold)
	{
		bStarved = true;
		OnStarved.Broadcast();
	}
}

void UAccretionMassComponent::ResetMass()
{
	bStarved = false;
	SetMass(StartingMass);
}

void UAccretionMassComponent::SetMass(float NewMass)
{
	Mass = FMath::Max(NewMass, 0.f);
	OnMassChanged.Broadcast(Mass);
}
