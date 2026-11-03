// Copyright Epic Games, Inc. All Rights Reserved.

#include "Universe/CelestialBody.h"
#include "Core/AccretionEngineContent.h"
#include "Matter/MatterActor.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace CelestialBodyVisuals
{
	constexpr float CoronaScale = 1.3f;
	const FLinearColor CoronaTint(0.075f, 0.075f, 0.075f);
}

ACelestialBody::ACelestialBody()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;

	Corona = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Corona"));
	Corona->SetupAttachment(Mesh);
	Corona->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Corona->SetCastShadow(false);
	Corona->SetRelativeScale3D(FVector(CelestialBodyVisuals::CoronaScale));
	Corona->SetVisibility(false);

	StarLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StarLight"));
	StarLight->SetupAttachment(Mesh);
	StarLight->SetMobility(EComponentMobility::Movable);
	StarLight->SetCastShadows(false);
	// Gameplay falloff instead of physical inverse-square, so light reaches the whole system.
	StarLight->bUseInverseSquaredFalloff = false;
	StarLight->LightFalloffExponent = 2.f;
	StarLight->SetVisibility(false);
	// The light lives on the scaled mesh; keep its own scale neutral.
	StarLight->SetUsingAbsoluteScale(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(AccretionEngineContent::SphereMesh);
	Mesh->SetStaticMesh(Sphere.Object);
	Corona->SetStaticMesh(Sphere.Object);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Unlit(AccretionEngineContent::UnlitColorMaterial);
	UnlitColorMaterial = Unlit.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Lit(AccretionEngineContent::LitColorMaterial);
	LitColorMaterial = Lit.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Additive(AccretionEngineContent::AdditiveMaterial);
	AdditiveMaterial = Additive.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(AccretionEngineContent::WhiteTexture);
	WhiteTexture = White.Object;
}

void ACelestialBody::InitializeBody(const FCelestialBodyDesc& InDesc, uint64 InBodyId, ACelestialBody* InParent,
	const FVector& InSystemCenter, const FQuat& InSystemPlane, float InMassFraction, float LightRadius)
{
	using namespace AccretionEngineContent;

	Desc = InDesc;
	BodyId = InBodyId;
	Parent = InParent;
	SystemCenter = InSystemCenter;
	LastParentLocation = InParent ? InParent->GetActorLocation() : InSystemCenter;
	OrbitRotation = InSystemPlane * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Desc.OrbitInclinationDegrees));
	InitialMass = Desc.Mass;
	Mass = Desc.Mass * FMath::Clamp(InMassFraction, 0.f, 1.f);

	if (InParent)
	{
		// Move after the parent each frame so moons do not lag behind their planet.
		AddTickPrerequisiteActor(InParent);
	}

	UMaterialInterface* SurfaceBase = Desc.bIsStar ? UnlitColorMaterial : LitColorMaterial;
	SurfaceMaterial = SurfaceBase ? UMaterialInstanceDynamic::Create(SurfaceBase, this) : nullptr;
	if (SurfaceMaterial)
	{
		SurfaceMaterial->SetVectorParameterValue(ColorParameter, Desc.Color);
		Mesh->SetMaterial(0, SurfaceMaterial);
	}

	if (Desc.bIsStar)
	{
		if (UMaterialInstanceDynamic* Glow = AdditiveMaterial ? UMaterialInstanceDynamic::Create(AdditiveMaterial, this) : nullptr)
		{
			Glow->SetTextureParameterValue(TextureParameter, WhiteTexture);
			Glow->SetVectorParameterValue(ColorParameter, Desc.Color * CelestialBodyVisuals::CoronaTint);
			Corona->SetMaterial(0, Glow);
		}
		Corona->SetVisibility(true);

		// Light color is the star color normalized so its brightness comes from the intensity alone.
		const FLinearColor Tint = Desc.Color / FMath::Max(Desc.Color.GetMax(), UE_SMALL_NUMBER);
		BaseLightIntensity = Desc.LightIntensity;
		StarLight->SetLightColor(Tint);
		StarLight->SetAttenuationRadius(LightRadius);
		StarLight->SetVisibility(true);
	}

	SetActorLocation(ComputeOrbitLocation(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f));
	ApplyMassToVisuals();
}

float ACelestialBody::GetRadius() const
{
	// Volume tracks mass, so radius tracks its cube root.
	return Desc.Radius * FMath::Pow(FMath::Max(GetMassFraction(), 0.f), 1.f / 3.f);
}

FVector ACelestialBody::ComputeOrbitLocation(float Time) const
{
	const FVector Center = Parent.IsValid() ? Parent->GetActorLocation() : LastParentLocation;
	if (Desc.OrbitRadius <= 0.f)
	{
		return Center;
	}
	const float Angle = FMath::DegreesToRadians(Desc.OrbitPhaseDegrees + 360.f * Time / FMath::Max(Desc.OrbitPeriod, 1.f));
	const FVector Local(FMath::Cos(Angle) * Desc.OrbitRadius, FMath::Sin(Angle) * Desc.OrbitRadius, 0.f);
	return Center + OrbitRotation.RotateVector(Local);
}

void ACelestialBody::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bRemoved || DeltaTime <= 0.f)
	{
		return;
	}

	if (bCaptured)
	{
		Velocity += PendingAcceleration * DeltaTime;
		PendingAcceleration = FVector::ZeroVector;
		SetActorLocation(GetActorLocation() + Velocity * DeltaTime);
		return;
	}

	if (Parent.IsValid())
	{
		LastParentLocation = Parent->GetActorLocation();
	}
	const FVector OldLocation = GetActorLocation();
	const FVector NewLocation = ComputeOrbitLocation(GetWorld()->GetTimeSeconds());
	Velocity = (NewLocation - OldLocation) / DeltaTime;
	SetActorLocation(NewLocation);
}

void ACelestialBody::Capture()
{
	// Keeps its current orbital velocity, so it falls in along a curve.
	bCaptured = true;
}

void ACelestialBody::Shred(float DeltaTime, const FVector& BlackHoleLocation, const FVector& BlackHoleVelocity, float FragmentSpeed, float MaxMassPerSecond)
{
	if (bRemoved || DeltaTime <= 0.f || InitialMass <= 0.f)
	{
		return;
	}

	const float FragmentMass = InitialMass / FMath::Max(FragmentsPerBody, 1);
	const float MassPerSecond = FMath::Min(InitialMass / FMath::Max(ShredSeconds, 1.f), FMath::Max(MaxMassPerSecond, 0.f));
	ShredAccumulator += MassPerSecond * DeltaTime;

	while (ShredAccumulator >= FragmentMass && !bRemoved)
	{
		ShredAccumulator -= FragmentMass;
		const float Torn = FMath::Min(FragmentMass, Mass);
		Mass -= Torn;
		SpawnFragment(Torn, BlackHoleLocation, BlackHoleVelocity, FragmentSpeed);

		if (Mass <= InitialMass * MinRemainingFraction)
		{
			// The last remnant goes in one piece.
			if (Mass > 0.f)
			{
				SpawnFragment(Mass, BlackHoleLocation, BlackHoleVelocity, FragmentSpeed);
				Mass = 0.f;
			}
			Remove(true);
			return;
		}
	}
	ApplyMassToVisuals();
}

void ACelestialBody::SpawnFragment(float FragmentMass, const FVector& BlackHoleLocation, const FVector& BlackHoleVelocity, float FragmentSpeed)
{
	UWorld* World = GetWorld();
	if (!World || FragmentMass <= 0.f)
	{
		return;
	}

	const FVector ToBlackHole = (BlackHoleLocation - GetActorLocation()).GetSafeNormal();
	FVector Sideways = FVector::CrossProduct(ToBlackHole, OrbitRotation.GetUpVector()).GetSafeNormal();
	if (Sideways.IsZero())
	{
		Sideways = FVector::CrossProduct(ToBlackHole, FVector::ForwardVector).GetSafeNormal();
	}

	// Torn from the side facing the black hole, with a little scatter.
	const float Radius = GetRadius();
	const FVector Location = GetActorLocation() + ToBlackHole * Radius + FMath::VRand() * Radius * 0.25f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AMatterActor* Fragment = World->SpawnActor<AMatterActor>(AMatterActor::StaticClass(), Location, FRotator::ZeroRotator, Params))
	{
		const FVector FragmentVelocity = BlackHoleVelocity + Sideways * FragmentSpeed * FMath::FRandRange(0.8f, 1.1f);
		Fragment->InitializeMatter(FragmentMass, FragmentVelocity);
		Fragment->SetSurfaceMaterial(SurfaceMaterial);
		Fragment->SetLifeSpan(FragmentLifeSeconds);
	}
}

float ACelestialBody::ConsumeWhole()
{
	if (bRemoved)
	{
		return 0.f;
	}
	const float Eaten = Mass;
	Mass = 0.f;
	Remove(true);
	return Eaten;
}

void ACelestialBody::ApplyMassToVisuals()
{
	Mesh->SetWorldScale3D(FVector(GetRadius() / AccretionEngineContent::SphereMeshRadius));
	if (Desc.bIsStar)
	{
		StarLight->SetIntensity(BaseLightIntensity * GetMassFraction());
	}
}

void ACelestialBody::Remove(bool bWasConsumed)
{
	if (bRemoved)
	{
		return;
	}
	bRemoved = true;
	bConsumed = bWasConsumed;
	OnRemoved.Broadcast(this, bWasConsumed);
	Destroy();
}

void ACelestialBody::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnRemoved.Clear();
	Super::EndPlay(EndPlayReason);
}
