// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/AccretionBlackHolePawn.h"
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
	PrimaryActorTick.bCanEverTick = false;

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

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 2000.f;
	Movement->Acceleration = 2000.f;
	// Low deceleration so the black hole drifts to a stop instead of halting instantly.
	Movement->Deceleration = 500.f;

	MassComponent = CreateDefaultSubobject<UAccretionMassComponent>(TEXT("Mass"));
	DiskPressureComponent = CreateDefaultSubobject<UDiskPressureComponent>(TEXT("DiskPressure"));
	GravityComponent = CreateDefaultSubobject<UAccretionGravityComponent>(TEXT("Gravity"));

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
	MassComponent->AddMass(MatterMass * MassGainPerMatterMass);
	DiskPressureComponent->AddFeedPressure(MatterMass, RadialFraction);
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
	SpringArm->TargetArmLength = BaseCameraDistance + EventHorizon * CameraDistancePerEventHorizonRadius;
}

void AAccretionBlackHolePawn::FreezeForRunEnd()
{
	Movement->StopMovementImmediately();
	Movement->SetComponentTickEnabled(false);
	MassComponent->SetComponentTickEnabled(false);
	DiskPressureComponent->SetComponentTickEnabled(false);
	GravityComponent->SetComponentTickEnabled(false);
}
