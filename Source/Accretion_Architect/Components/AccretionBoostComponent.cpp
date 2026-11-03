// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AccretionBoostComponent.h"
#include "Components/AccretionMassComponent.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/Actor.h"

UAccretionBoostComponent::UAccretionBoostComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAccretionBoostComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateCooldown(DeltaTime);
}

bool UAccretionBoostComponent::TryBoost(const FVector& Direction)
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	return TryBoostUsing(Owner->FindComponentByClass<UAccretionMassComponent>(), Owner->FindComponentByClass<UMovementComponent>(), Direction);
}

bool UAccretionBoostComponent::TryBoostUsing(UAccretionMassComponent* Mass, UMovementComponent* Movement, const FVector& Direction)
{
	const FVector BoostDirection = Direction.GetSafeNormal();
	if (!Mass || !Movement || BoostDirection.IsZero() || IsCoolingDown())
	{
		return false;
	}
	if (!Mass->TrySpendMass(MassCost))
	{
		return false;
	}

	Movement->Velocity += BoostDirection * BoostSpeed;
	CooldownRemaining = Cooldown;
	OnBoost.Broadcast();
	return true;
}

void UAccretionBoostComponent::UpdateCooldown(float DeltaTime)
{
	CooldownRemaining = FMath::Max(CooldownRemaining - FMath::Max(DeltaTime, 0.f), 0.f);
}

bool UAccretionBoostComponent::CanAfford(const UAccretionMassComponent* Mass) const
{
	return Mass && !Mass->IsStarved() && Mass->GetMass() - MassCost >= Mass->StarvationThreshold;
}
