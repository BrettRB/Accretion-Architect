// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/ProximaCentauriEnvironment.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace ProximaEnvironment
{
	static const FName ColorParameter(TEXT("Color"));
	static const FName TextureParameter(TEXT("Texture"));

	/** The engine sphere mesh has a 50 cm radius. */
	constexpr float SphereMeshRadius = 50.f;

	/** Corona radius as a multiple of the star radius. */
	constexpr float CoronaScale = 1.3f;

	/** Roughly normal-distributed value with mean 0 and standard deviation ~1. */
	float Gaussian(FRandomStream& Random)
	{
		return (Random.FRand() + Random.FRand() + Random.FRand() - 1.5f) * 2.f;
	}

	/** Star tint by spectral class, weighted toward the cooler, more common stars. */
	FLinearColor RandomStarTint(FRandomStream& Random)
	{
		const float Roll = Random.FRand();
		if (Roll < 0.10f) return FLinearColor(0.65f, 0.75f, 1.f);  // O/B: blue-white
		if (Roll < 0.40f) return FLinearColor(0.9f, 0.93f, 1.f);   // A/F: white
		if (Roll < 0.70f) return FLinearColor(1.f, 0.93f, 0.78f);  // G: yellow-white
		if (Roll < 0.90f) return FLinearColor(1.f, 0.75f, 0.5f);   // K: orange
		return FLinearColor(1.f, 0.55f, 0.4f);                     // M: red
	}

	/** Builds tiny double-sided octahedra; seen from the center of the star sphere they read as points of light. */
	struct FStarMeshBuilder
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FLinearColor> Colors;

		void AddStar(const FVector& Direction, float Distance, float AngularSize, const FLinearColor& Color)
		{
			const FVector Dir = Direction.GetSafeNormal();
			if (Dir.IsZero())
			{
				return;
			}
			FVector U, V;
			Dir.FindBestAxisVectors(U, V);

			const FVector Center = Dir * Distance;
			const float Size = Distance * AngularSize;
			const int32 Base = Vertices.Num();
			Vertices.Append({ Center + U * Size, Center - U * Size, Center + V * Size, Center - V * Size, Center + Dir * Size, Center - Dir * Size });
			for (int32 Index = 0; Index < 6; ++Index)
			{
				Colors.Add(Color);
			}

			static constexpr int32 Faces[8][3] = {
				{ 4, 0, 2 }, { 4, 2, 1 }, { 4, 1, 3 }, { 4, 3, 0 },
				{ 5, 2, 0 }, { 5, 1, 2 }, { 5, 3, 1 }, { 5, 0, 3 },
			};
			for (const auto& Face : Faces)
			{
				Triangles.Append({ Base + Face[0], Base + Face[1], Base + Face[2] });
				Triangles.Append({ Base + Face[0], Base + Face[2], Base + Face[1] });
			}
		}
	};
}

AProximaCentauriEnvironment::AProximaCentauriEnvironment()
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

	ProximaBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProximaBody"));
	ProximaBody->SetupAttachment(Root);
	ProximaBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProximaBody->SetCastShadow(false);

	ProximaCorona = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProximaCorona"));
	ProximaCorona->SetupAttachment(ProximaBody);
	ProximaCorona->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProximaCorona->SetCastShadow(false);
	ProximaCorona->SetRelativeScale3D(FVector(ProximaEnvironment::CoronaScale));

	ProximaLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("ProximaLight"));
	ProximaLight->SetupAttachment(Root);
	ProximaLight->SetMobility(EComponentMobility::Movable);
	ProximaLight->SetCastShadows(false);
	ProximaLight->SetAtmosphereSunLight(false);
	// Proxima is the main light for forward shading/translucency; the fill light defers to it.
	ProximaLight->ForwardShadingPriority = 1;

	FillLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(Root);
	FillLight->SetMobility(EComponentMobility::Movable);
	FillLight->SetCastShadows(false);
	FillLight->SetAtmosphereSunLight(false);
	FillLight->SetAffectTranslucentLighting(false);

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	// Above any post process volume the level may already contain.
	PostProcess->Priority = 100.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	SphereMesh = Sphere.Object;
	// Unlit opaque, EmissiveColor = vertex color.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VertexColorUnlit(TEXT("/Engine/EngineDebugMaterials/VertexColorViewMode_ColorOnly.VertexColorViewMode_ColorOnly"));
	VertexColorUnlitMaterial = VertexColorUnlit.Object;
	// Unlit opaque, EmissiveColor = "Color" parameter.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> UnlitColor(TEXT("/Engine/EngineDebugMaterials/LevelColorationUnlitMaterial.LevelColorationUnlitMaterial"));
	UnlitColorMaterial = UnlitColor.Object;
	// Default lit opaque, BaseColor = "Color" parameter.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LitColor(TEXT("/Engine/EngineDebugMaterials/M_SimpleOpaque.M_SimpleOpaque"));
	LitColorMaterial = LitColor.Object;
	// Unlit additive, EmissiveColor = "Texture" * "Color".
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Additive(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	AdditiveMaterial = Additive.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;

	ProximaBody->SetStaticMesh(SphereMesh);
	ProximaCorona->SetStaticMesh(SphereMesh);

	// Proxima d and Proxima b, scaled down to gameplay distances.
	FAccretionPlanetDefinition ProximaD;
	ProximaD.Name = TEXT("Proxima d");
	ProximaD.Radius = 1800.f;
	ProximaD.OrbitRadius = 70000.f;
	ProximaD.OrbitPeriodSeconds = 240.f;
	ProximaD.StartAngleDegrees = 120.f;
	ProximaD.Color = FLinearColor(0.42f, 0.4f, 0.38f);
	Planets.Add(ProximaD);

	FAccretionPlanetDefinition ProximaB;
	ProximaB.Name = TEXT("Proxima b");
	ProximaB.Radius = 3200.f;
	ProximaB.OrbitRadius = 130000.f;
	ProximaB.OrbitPeriodSeconds = 560.f;
	ProximaB.StartAngleDegrees = 200.f;
	ProximaB.Color = FLinearColor(0.45f, 0.28f, 0.2f);
	Planets.Add(ProximaB);
}

void AProximaCentauriEnvironment::BeginPlay()
{
	Super::BeginPlay();

	if (bSuppressLevelSkyAndSun)
	{
		SuppressLevelSkyAndSun();
	}
	BuildStarField();
	SetUpProxima();
	SpawnPlanets();
	SetUpPostProcess();
}

void AProximaCentauriEnvironment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keep the star sphere centered on the camera so stars behave as if infinitely far away.
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		StarField->SetWorldLocation(Camera->GetCameraLocation());
	}
	UpdatePlanetOrbits();
}

void AProximaCentauriEnvironment::BuildStarField()
{
	using namespace ProximaEnvironment;

	FRandomStream Random(StarSeed);
	FStarMeshBuilder Builder;
	const float MinSize = FMath::Min(MinStarAngularSize, MaxStarAngularSize);
	const float MaxSize = FMath::Max(MinStarAngularSize, MaxStarAngularSize);

	// Scattered background stars: most are faint, a few are bright.
	for (int32 Index = 0; Index < BackgroundStarCount; ++Index)
	{
		const float Brightness = 0.12f + 0.88f * FMath::Pow(Random.FRand(), 4.f);
		const float Size = FMath::Lerp(MinSize, MaxSize, FMath::Sqrt(Brightness));
		Builder.AddStar(Random.GetUnitVector(), StarFieldRadius, Size, RandomStarTint(Random) * Brightness);
	}

	// Milky Way: a dense band of faint stars along a great circle, thicker toward the galactic core.
	const FQuat BandRotation = MilkyWayOrientation.Quaternion();
	for (int32 Index = 0; Index < MilkyWayStarCount; ++Index)
	{
		const bool bCore = Random.FRand() < 0.3f;
		const float Longitude = bCore ? Gaussian(Random) * 0.35f : Random.FRandRange(-UE_PI, UE_PI);
		const float Latitude = Gaussian(Random) * (bCore ? 0.1f : 0.05f);
		const FVector BandDirection(FMath::Cos(Latitude) * FMath::Cos(Longitude), FMath::Cos(Latitude) * FMath::Sin(Longitude), FMath::Sin(Latitude));

		const float Brightness = 0.08f + 0.5f * FMath::Pow(Random.FRand(), 3.f);
		const float Size = FMath::Lerp(MinSize, MaxSize, Brightness) * 0.85f;
		const FLinearColor Tint = FLinearColor::LerpUsingHSV(FLinearColor(0.85f, 0.88f, 1.f), FLinearColor(1.f, 0.88f, 0.72f), Random.FRand());
		Builder.AddStar(BandRotation.RotateVector(BandDirection), StarFieldRadius, Size, Tint * Brightness);
	}

	// Alpha Centauri A (yellow, G2) and B (orange, K1): the brightest stars in Proxima's sky.
	const FVector AlphaDirection = AlphaCentauriDirection.GetSafeNormal();
	FVector Side, Up;
	AlphaDirection.FindBestAxisVectors(Side, Up);
	Builder.AddStar(AlphaDirection, StarFieldRadius, MaxSize * 2.6f, FLinearColor(1.f, 0.95f, 0.8f));
	Builder.AddStar((AlphaDirection + Side * 0.012f).GetSafeNormal(), StarFieldRadius, MaxSize * 2.f, FLinearColor(1.f, 0.72f, 0.45f));

	StarField->ClearAllMeshSections();
	StarField->CreateMeshSection_LinearColor(0, Builder.Vertices, Builder.Triangles, TArray<FVector>(), TArray<FVector2D>(),
		Builder.Colors, TArray<FProcMeshTangent>(), false);
	StarField->SetMaterial(0, VertexColorUnlitMaterial);
}

void AProximaCentauriEnvironment::SetUpProxima()
{
	using namespace ProximaEnvironment;

	ProximaBody->SetRelativeLocation(ProximaLocation);
	ProximaBody->SetRelativeScale3D(FVector(ProximaRadius / SphereMeshRadius));

	if (UMaterialInstanceDynamic* Surface = UnlitColorMaterial ? UMaterialInstanceDynamic::Create(UnlitColorMaterial, this) : nullptr)
	{
		Surface->SetVectorParameterValue(ColorParameter, ProximaColor);
		ProximaBody->SetMaterial(0, Surface);
	}
	if (UMaterialInstanceDynamic* Corona = AdditiveMaterial ? UMaterialInstanceDynamic::Create(AdditiveMaterial, this) : nullptr)
	{
		Corona->SetTextureParameterValue(TextureParameter, WhiteTexture);
		Corona->SetVectorParameterValue(ColorParameter, ProximaCoronaColor);
		ProximaCorona->SetMaterial(0, Corona);
	}

	// Light travels from the star toward the play area around the origin.
	ProximaLight->SetWorldRotation((-ProximaLocation).Rotation());
	ProximaLight->SetIntensity(ProximaLightIntensity);
	ProximaLight->SetLightColor(ProximaLightColor);

	FillLight->SetWorldRotation((-AlphaCentauriDirection).Rotation());
	FillLight->SetIntensity(FillLightIntensity);
	FillLight->SetLightColor(FillLightColor);
}

void AProximaCentauriEnvironment::SpawnPlanets()
{
	using namespace ProximaEnvironment;

	for (const FAccretionPlanetDefinition& Planet : Planets)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, NAME_None);
		Mesh->SetupAttachment(Root);
		Mesh->SetStaticMesh(SphereMesh);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetRelativeScale3D(FVector(Planet.Radius / SphereMeshRadius));
		if (UMaterialInstanceDynamic* Surface = LitColorMaterial ? UMaterialInstanceDynamic::Create(LitColorMaterial, this) : nullptr)
		{
			Surface->SetVectorParameterValue(ColorParameter, Planet.Color);
			Mesh->SetMaterial(0, Surface);
		}
		Mesh->RegisterComponent();
		PlanetMeshes.Add(Mesh);
	}
	UpdatePlanetOrbits();
}

void AProximaCentauriEnvironment::UpdatePlanetOrbits()
{
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	for (int32 Index = 0; Index < PlanetMeshes.Num() && Index < Planets.Num(); ++Index)
	{
		const FAccretionPlanetDefinition& Planet = Planets[Index];
		const float Angle = FMath::DegreesToRadians(Planet.StartAngleDegrees + 360.f * Time / FMath::Max(Planet.OrbitPeriodSeconds, 1.f));
		PlanetMeshes[Index]->SetRelativeLocation(ProximaLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Planet.OrbitRadius);
	}
}

void AProximaCentauriEnvironment::SetUpPostProcess()
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

void AProximaCentauriEnvironment::SuppressLevelSkyAndSun()
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
