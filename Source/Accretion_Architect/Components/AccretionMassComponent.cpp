// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AccretionMassComponent.h"

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

void UAccretionMassComponent::UpdateMass(float DeltaTime)
{
	if (bStarved || DeltaTime <= 0.f)
	{
		return;
	}

	SetMass(Mass * FMath::Max(1.f - DecayRatePerSecond * DeltaTime, 0.f));

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
