// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/DiskPressureComponent.h"
#include "Core/AccretionMath.h"

UDiskPressureComponent::UDiskPressureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDiskPressureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePressure(DeltaTime);
}

void UDiskPressureComponent::AddPressure(float Amount)
{
	if (bBlownOut || Amount <= 0.f)
	{
		return;
	}

	SetPressure(FMath::Min(Pressure + Amount, MaxPressure));

	if (Pressure >= MaxPressure)
	{
		bBlownOut = true;
		OnBlowout.Broadcast();
	}
}

float UDiskPressureComponent::AddFeedPressure(float MatterMass, float RadialFraction)
{
	const float Amount = static_cast<float>(FAccretionMath::ComputeFeedPressure(
		MatterMass, PressurePerMass, SmoothFeedMultiplier, HeadOnFeedMultiplier, RadialFraction));
	AddPressure(Amount);
	return Amount;
}

void UDiskPressureComponent::UpdatePressure(float DeltaTime)
{
	if (bBlownOut || DeltaTime <= 0.f || Pressure <= 0.f)
	{
		return;
	}
	SetPressure(FMath::Max(Pressure - VentRatePerSecond * DeltaTime, 0.f));
}

void UDiskPressureComponent::ResetPressure()
{
	bBlownOut = false;
	SetPressure(0.f);
}

void UDiskPressureComponent::SetPressure(float NewPressure)
{
	Pressure = NewPressure;
	OnPressureChanged.Broadcast(Pressure);
}
