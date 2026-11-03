// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/AccretionBlackHolePawn.h"
#include "Components/AccretionBoostComponent.h"
#include "Components/AccretionDiskComponent.h"
#include "Components/AccretionGravityComponent.h"
#include "Components/AccretionMassComponent.h"
#include "Components/DiskPressureComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAccretionBlackHolePawn::AAccretionBlackHolePawn()
{
	// Ticks only for the victory camera.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
	Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Root;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(SphereMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlackMaterial(TEXT("/Engine/EngineDebugMaterials/BlackUnlitMaterial.BlackUnlitMaterial"));
	if (BlackMaterial.Succeeded())
	{
		BodyMesh->SetMaterial(0, BlackMaterial.Object);
	}

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Root);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 8.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// Speeds are set from the Base* properties times the size scale in UpdateSizeFromMass.
	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = BaseMaxSpeed;
	Movement->Acceleration = BaseAcceleration;
	Movement->Deceleration = BaseDeceleration;

	MassComponent = CreateDefaultSubobject<UAccretionMassComponent>(TEXT("Mass"));
	DiskPressureComponent = CreateDefaultSubobject<UDiskPressureComponent>(TEXT("DiskPressure"));
	GravityComponent = CreateDefaultSubobject<UAccretionGravityComponent>(TEXT("Gravity"));
	BoostComponent = CreateDefaultSubobject<UAccretionBoostComponent>(TEXT("Boost"));

	// Spins around the black hole, tipped toward the camera; scaled to the event horizon in UpdateSizeFromMass.
	DiskComponent = CreateDefaultSubobject<UAccretionDiskComponent>(TEXT("AccretionDisk"));
	DiskComponent->SetupAttachment(Root);

	// The camera follows the controller's rotation; the sphere itself does not need to rotate.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AAccretionBlackHolePawn::BeginPlay()
{
	Super::BeginPlay();

	GravityComponent->OnMatterConsumed.AddDynamic(this, &AAccretionBlackHolePawn::HandleMatterConsumed);
	MassComponent->OnMassChanged.AddDynamic(this, &AAccretionBlackHolePawn::HandleMassChanged);
	UpdateSizeFromMass();
}

void AAccretionBlackHolePawn::HandleMatterConsumed(float MatterMass, float RadialFraction)
{
	// Pressure is judged against the black hole's size before this meal.
	DiskPressureComponent->AddFeedPressure(MatterMass, RadialFraction, MassComponent->GetMass());
	MassComponent->AddMass(MatterMass * MassGainPerMatterMass);
	DiskComponent->NotifyMatterConsumed();
}

bool AAccretionBlackHolePawn::TryBoost(const FVector& Direction)
{
	return !bFrozen && BoostComponent->TryBoost(Direction);
}

void AAccretionBlackHolePawn::HandleMassChanged(float NewMass)
{
	UpdateSizeFromMass();
}

void AAccretionBlackHolePawn::UpdateSizeFromMass()
{
	const float EventHorizon = GravityComponent->GetEventHorizonRadius();
	Root->SetSphereRadius(EventHorizon);
	// The engine sphere mesh is 100 cm across (50 cm radius).
	BodyMesh->SetRelativeScale3D(FVector(EventHorizon / 50.f));
	if (!bPlayingVictoryCamera)
	{
		SpringArm->TargetArmLength = BaseCameraDistance + EventHorizon * CameraDistancePerEventHorizonRadius;
	}
	// The disk mesh is built in units of the event horizon radius.
	DiskComponent->SetRelativeScale3D(FVector(EventHorizon));

	// A bigger black hole crosses bigger distances at a similar pace.
	const float SizeScale = GravityComponent->GetSizeScale();
	Movement->MaxSpeed = BaseMaxSpeed * SizeScale;
	Movement->Acceleration = BaseAcceleration * SizeScale;
	Movement->Deceleration = BaseDeceleration * SizeScale;
}

void AAccretionBlackHolePawn::PlayVictoryCamera(float TargetCameraDistance)
{
	bPlayingVictoryCamera = true;
	VictoryCameraDistance = TargetCameraDistance;
	SetActorTickEnabled(true);
}

void AAccretionBlackHolePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bPlayingVictoryCamera)
	{
		// Ease out toward the galaxy-wide view (exponential approach, so it slows as it arrives).
		const float Alpha = 1.f - FMath::Exp(-VictoryCameraPullSpeed * DeltaTime);
		SpringArm->TargetArmLength = FMath::Lerp(SpringArm->TargetArmLength, VictoryCameraDistance, Alpha);
	}
}

void AAccretionBlackHolePawn::FreezeForRunEnd()
{
	// The disk keeps ticking so it fades out after the run ends.
	bFrozen = true;
	BoostComponent->SetComponentTickEnabled(false);
	Movement->StopMovementImmediately();
	Movement->SetComponentTickEnabled(false);
	MassComponent->SetComponentTickEnabled(false);
	DiskPressureComponent->SetComponentTickEnabled(false);
	GravityComponent->SetComponentTickEnabled(false);
}
