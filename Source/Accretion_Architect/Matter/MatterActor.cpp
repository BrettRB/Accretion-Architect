// Copyright Epic Games, Inc. All Rights Reserved.

#include "Matter/MatterActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMatterActor::AMatterActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
}

void AMatterActor::InitializeMatter(float InMatterMass, const FVector& InVelocity)
{
	MatterMass = FMath::Max(InMatterMass, 0.f);
	Velocity = InVelocity;
	SetActorScale3D(FVector(ScalePerCubeRootMass * FMath::Pow(FMath::Max(MatterMass, 0.01f), 1.f / 3.f)));
}

void AMatterActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Velocity += PendingAcceleration * DeltaTime;
	PendingAcceleration = FVector::ZeroVector;
	SetActorLocation(GetActorLocation() + Velocity * DeltaTime);
}

void AMatterActor::Consume()
{
	if (bConsumed)
	{
		return;
	}
	bConsumed = true;
	Destroy();
}
