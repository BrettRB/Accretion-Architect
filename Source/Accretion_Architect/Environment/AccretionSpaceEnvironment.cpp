// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AccretionSpaceEnvironment.h"
#include "Environment/StarMeshBuilder.h"
#include "Core/AccretionEngineContent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AAccretionSpaceEnvironment::AAccretionSpaceEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;
	// Follow the camera after it has moved this frame so the stars never lag behind.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	StarField = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StarField"));
	StarField->SetupAttachment(Root);
	StarField->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StarField->SetCastShadow(false);
	StarField->SetUsingAbsoluteLocation(true);

	FillLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(Root);
	FillLight->SetMobility(EComponentMobility::Movable);
	FillLight->SetCastShadows(false);
	FillLight->SetAtmosphereSunLight(false);

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	// Above any post process volume the level may already contain.
	PostProcess->Priority = 100.f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VertexColorUnlit(AccretionEngineContent::VertexColorUnlitMaterial);
	VertexColorUnlitMaterial = VertexColorUnlit.Object;
}

void AAccretionSpaceEnvironment::BeginPlay()
{
	Super::BeginPlay();

	if (bSuppressLevelSkyAndSun)
	{
		SuppressLevelSkyAndSun();
	}
	BuildStarField();
	SetUpPostProcess();

	FillLight->SetWorldRotation(FillLightRotation);
	FillLight->SetIntensity(FillLightIntensity);
	FillLight->SetLightColor(FillLightColor);
}

void AAccretionSpaceEnvironment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keep the star sphere centered on the camera so stars behave as if infinitely far away.
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		StarField->SetWorldLocation(Camera->GetCameraLocation());
	}
}

void AAccretionSpaceEnvironment::BuildStarField()
{
	FRandomStream Random(StarSeed);
	FStarMeshBuilder Builder;
	const float MinSize = FMath::Min(MinStarAngularSize, MaxStarAngularSize);
	const float MaxSize = FMath::Max(MinStarAngularSize, MaxStarAngularSize);

	// Scattered background stars: most are faint, a few are bright.
	for (int32 Index = 0; Index < BackgroundStarCount; ++Index)
	{
		const float Brightness = 0.12f + 0.88f * FMath::Pow(Random.FRand(), 4.f);
		const float Size = FMath::Lerp(MinSize, MaxSize, FMath::Sqrt(Brightness));
		Builder.AddSkyStar(Random.GetUnitVector(), StarFieldRadius, Size, FStarMeshBuilder::RandomStarTint(Random) * Brightness);
	}

	// Milky Way: a dense band of faint stars along a great circle, thicker toward the galactic core.
	const FQuat BandRotation = MilkyWayOrientation.Quaternion();
	for (int32 Index = 0; Index < MilkyWayStarCount; ++Index)
	{
		const bool bCore = Random.FRand() < 0.3f;
		const float Longitude = bCore ? FStarMeshBuilder::Gaussian(Random) * 0.35f : Random.FRandRange(-UE_PI, UE_PI);
		const float Latitude = FStarMeshBuilder::Gaussian(Random) * (bCore ? 0.1f : 0.05f);
		const FVector BandDirection(FMath::Cos(Latitude) * FMath::Cos(Longitude), FMath::Cos(Latitude) * FMath::Sin(Longitude), FMath::Sin(Latitude));

		const float Brightness = 0.08f + 0.5f * FMath::Pow(Random.FRand(), 3.f);
		const float Size = FMath::Lerp(MinSize, MaxSize, Brightness) * 0.85f;
		const FLinearColor Tint = FLinearColor::LerpUsingHSV(FLinearColor(0.85f, 0.88f, 1.f), FLinearColor(1.f, 0.88f, 0.72f), Random.FRand());
		Builder.AddSkyStar(BandRotation.RotateVector(BandDirection), StarFieldRadius, Size, Tint * Brightness);
	}

	StarField->ClearAllMeshSections();
	StarField->CreateMeshSection_LinearColor(0, Builder.Vertices, Builder.Triangles, TArray<FVector>(), TArray<FVector2D>(),
		Builder.Colors, TArray<FProcMeshTangent>(), false);
	StarField->SetMaterial(0, VertexColorUnlitMaterial);
}

void AAccretionSpaceEnvironment::SetUpPostProcess()
{
	FPostProcessSettings& Settings = PostProcess->Settings;

	// Fixed exposure: auto exposure would brighten the black sky to gray.
	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Settings.AutoExposureApplyPhysicalCameraExposure = false;
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = ExposureBias;

	Settings.bOverride_BloomIntensity = true;
	Settings.BloomIntensity = BloomIntensity;

	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.5f;
}

void AAccretionSpaceEnvironment::SuppressLevelSkyAndSun()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (*It == this)
		{
			continue;
		}
		for (UActorComponent* Component : It->GetComponents())
		{
			const bool bDaytimeSky = Component && (
				Component->IsA<USkyAtmosphereComponent>() ||
				Component->IsA<UExponentialHeightFogComponent>() ||
				Component->IsA<USkyLightComponent>() ||
				Component->IsA<UDirectionalLightComponent>() ||
				Component->IsA<UVolumetricCloudComponent>());
			if (USceneComponent* SceneComponent = bDaytimeSky ? Cast<USceneComponent>(Component) : nullptr)
			{
				SceneComponent->SetVisibility(false);
				SceneComponent->SetHiddenInGame(true);
			}
		}
	}
}
