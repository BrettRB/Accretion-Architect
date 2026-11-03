// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AccretionGalaxy.h"
#include "Environment/StarMeshBuilder.h"
#include "Core/AccretionEngineContent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace AccretionGalaxy
{
	/** Stars are built in a unit galaxy of this radius, then the actor is scaled to the real radius. */
	constexpr float ModelRadius = 10000.f;
	constexpr float BulgeShare = 0.18f;
	constexpr float ArmShare = 0.6f;
	/** Star half-size as a fraction of the galaxy radius. */
	constexpr float StarSize = 0.0016f;
}

AAccretionGalaxy::AAccretionGalaxy()
{
	PrimaryActorTick.bCanEverTick = true;

	Stars = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Stars"));
	Stars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stars->SetCastShadow(false);
	RootComponent = Stars;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VertexColorUnlit(AccretionEngineContent::VertexColorUnlitMaterial);
	VertexColorUnlitMaterial = VertexColorUnlit.Object;
}

void AAccretionGalaxy::FormGalaxy(float InRadius)
{
	Radius = FMath::Max(InRadius, 1.f);
	Age = 0.f;
	bFormed = true;
	BuildStars();
	SetActorScale3D(FVector(KINDA_SMALL_NUMBER));
}

void AAccretionGalaxy::BuildStars()
{
	using namespace AccretionGalaxy;

	FRandomStream Random(Seed);
	FStarMeshBuilder Builder;
	const float R = ModelRadius;
	const float Size = R * StarSize;

	for (int32 Index = 0; Index < StarCount; ++Index)
	{
		const float Kind = Random.FRand();
		FVector Location;
		FLinearColor Color;
		float StarScale = 1.f;

		if (Kind < BulgeShare)
		{
			// Central bulge: a dense, flattened ball of old yellow-white stars.
			Location = FVector(FStarMeshBuilder::Gaussian(Random), FStarMeshBuilder::Gaussian(Random), FStarMeshBuilder::Gaussian(Random) * 0.6f) * R * 0.07f;
			Color = FLinearColor::LerpUsingHSV(FLinearColor(1.f, 0.95f, 0.8f), FLinearColor(1.f, 0.8f, 0.55f), Random.FRand()) * Random.FRandRange(0.6f, 1.f);
			StarScale = 1.3f;
		}
		else if (Kind < BulgeShare + ArmShare)
		{
			// Spiral arms: logarithmic spirals of young blue-white stars, scattered more toward the rim.
			const int32 Arm = Random.RandRange(0, ArmCount - 1);
			const float Distance = FMath::Lerp(0.08f, 1.f, FMath::Pow(Random.FRand(), 0.8f));
			const float Angle = UE_TWO_PI * Arm / ArmCount + ArmWinding * FMath::Loge(Distance / 0.08f);
			const float Scatter = 0.035f + 0.05f * Distance;
			Location = FVector(FMath::Cos(Angle) * Distance + FStarMeshBuilder::Gaussian(Random) * Scatter,
				FMath::Sin(Angle) * Distance + FStarMeshBuilder::Gaussian(Random) * Scatter,
				FStarMeshBuilder::Gaussian(Random) * 0.012f) * R;
			Color = (Random.FRand() < 0.85f ? FLinearColor(0.7f, 0.8f, 1.f) : FLinearColor(1.f, 0.6f, 0.75f)) * Random.FRandRange(0.4f, 1.f);
		}
		else
		{
			// Disk: older, fainter stars spread smoothly between the arms.
			const float Distance = FMath::Min(-FMath::Loge(FMath::Max(Random.FRand(), 0.001f)) * 0.3f, 1.1f);
			const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
			Location = FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, FStarMeshBuilder::Gaussian(Random) * 0.02f) * R;
			Color = FStarMeshBuilder::RandomStarTint(Random) * Random.FRandRange(0.2f, 0.55f);
			StarScale = 0.8f;
		}

		Builder.AddStar(Location, Size * StarScale * Random.FRandRange(0.6f, 1.2f), Color, Random.GetUnitVector());
	}

	Stars->ClearAllMeshSections();
	Stars->CreateMeshSection_LinearColor(0, Builder.Vertices, Builder.Triangles, TArray<FVector>(), TArray<FVector2D>(),
		Builder.Colors, TArray<FProcMeshTangent>(), false);
	Stars->SetMaterial(0, VertexColorUnlitMaterial);
}

void AAccretionGalaxy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bFormed)
	{
		return;
	}

	Age += DeltaTime;
	// Ease-out growth: matter gathers quickly, then settles.
	const float Growth = 1.f - FMath::Pow(1.f - FMath::Clamp(Age / GrowSeconds, 0.f, 1.f), 3.f);
	SetActorScale3D(FVector(FMath::Max(Growth, KINDA_SMALL_NUMBER) * Radius / AccretionGalaxy::ModelRadius));
	AddActorLocalRotation(FRotator(0.f, RotationDegreesPerSecond * DeltaTime, 0.f));
}
