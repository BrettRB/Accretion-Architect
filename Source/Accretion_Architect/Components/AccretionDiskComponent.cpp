// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AccretionDiskComponent.h"
#include "Engine/Texture.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace AccretionDisk
{
	static const FName ColorParameter(TEXT("Color"));
	static const FName TextureParameter(TEXT("Texture"));
	constexpr int32 RingSegments = 96;
	constexpr int32 ArmSteps = 32;
	/** Radians the spiral arms wind per e-fold of radius. */
	constexpr float ArmTwist = 2.4f;
	/** Angular half-width of a spiral arm, in radians. */
	constexpr float ArmHalfWidth = 0.09f;

	/** Flat, double-sided triangle list in the XY plane, so the disk is visible from above and below. */
	struct FRingMeshBuilder
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;

		void AddQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D)
		{
			const int32 Base = Vertices.Add(A);
			Vertices.Add(B);
			Vertices.Add(C);
			Vertices.Add(D);
			Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
			Triangles.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
		}

		static FVector Polar(float Radius, float Angle)
		{
			return FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f);
		}
	};
}

UAccretionDiskComponent::UAccretionDiskComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	SetVisibility(false);

	// Unlit, additive engine material: EmissiveColor = Texture * Color. At zero color it adds nothing.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMaterial(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (EmissiveMaterial.Succeeded())
	{
		DiskMaterial = EmissiveMaterial.Object;
	}
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	if (White.Succeeded())
	{
		WhiteTexture = White.Object;
	}
}

void UAccretionDiskComponent::BeginPlay()
{
	Super::BeginPlay();
	BuildDiskMesh();
	ApplyIntensity();
}

void UAccretionDiskComponent::BuildDiskMesh()
{
	using namespace AccretionDisk;

	ClearAllMeshSections();
	SectionMaterials.Reset();
	SectionColors.Reset();

	auto AddSection = [this](const FRingMeshBuilder& Builder, const FLinearColor& Color)
	{
		const int32 SectionIndex = SectionColors.Num();
		CreateMeshSection_LinearColor(SectionIndex, Builder.Vertices, Builder.Triangles,
			TArray<FVector>(), TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);

		UMaterialInstanceDynamic* Material = DiskMaterial ? UMaterialInstanceDynamic::Create(DiskMaterial, this) : nullptr;
		if (Material && WhiteTexture)
		{
			Material->SetTextureParameterValue(TextureParameter, WhiteTexture);
		}
		SetMaterial(SectionIndex, Material);
		SectionMaterials.Add(Material);
		SectionColors.Add(Color);
	};

	// Concentric bands, hot and bright at the inner edge, cooling and dimming outward.
	const int32 Bands = FMath::Max(BandCount, 1);
	for (int32 Band = 0; Band < Bands; ++Band)
	{
		const float T0 = static_cast<float>(Band) / Bands;
		const float T1 = static_cast<float>(Band + 1) / Bands;
		const float R0 = FMath::Lerp(InnerRadius, OuterRadius, T0);
		const float R1 = FMath::Lerp(InnerRadius, OuterRadius, T1);

		FRingMeshBuilder Builder;
		for (int32 Segment = 0; Segment < RingSegments; ++Segment)
		{
			const float A0 = UE_TWO_PI * Segment / RingSegments;
			const float A1 = UE_TWO_PI * (Segment + 1) / RingSegments;
			Builder.AddQuad(FRingMeshBuilder::Polar(R0, A0), FRingMeshBuilder::Polar(R1, A0),
				FRingMeshBuilder::Polar(R1, A1), FRingMeshBuilder::Polar(R0, A1));
		}
		AddSection(Builder, FLinearColor::LerpUsingHSV(InnerColor, OuterColor, (T0 + T1) * 0.5f));
	}

	// Logarithmic spiral streaks on top of the bands so the spin reads clearly.
	if (SpiralArmCount > 0 && InnerRadius > 0.f)
	{
		FRingMeshBuilder Builder;
		for (int32 Arm = 0; Arm < SpiralArmCount; ++Arm)
		{
			const float ArmStart = UE_TWO_PI * Arm / SpiralArmCount;
			for (int32 Step = 0; Step < ArmSteps; ++Step)
			{
				const float R0 = FMath::Lerp(InnerRadius, OuterRadius, static_cast<float>(Step) / ArmSteps);
				const float R1 = FMath::Lerp(InnerRadius, OuterRadius, static_cast<float>(Step + 1) / ArmSteps);
				const float A0 = ArmStart - ArmTwist * FMath::Loge(R0 / InnerRadius);
				const float A1 = ArmStart - ArmTwist * FMath::Loge(R1 / InnerRadius);
				// Arms taper toward the outer edge.
				const float W0 = ArmHalfWidth * (1.f - 0.6f * Step / ArmSteps);
				const float W1 = ArmHalfWidth * (1.f - 0.6f * (Step + 1) / ArmSteps);
				Builder.AddQuad(FRingMeshBuilder::Polar(R0, A0 - W0), FRingMeshBuilder::Polar(R1, A1 - W1),
					FRingMeshBuilder::Polar(R1, A1 + W1), FRingMeshBuilder::Polar(R0, A0 + W0));
			}
		}
		AddSection(Builder, StreakColor);
	}
}

void UAccretionDiskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFade(DeltaTime);
	ApplyIntensity();
	if (Intensity > 0.f)
	{
		SpinDegrees = FMath::Fmod(SpinDegrees + SpinDegreesPerSecond * DeltaTime, 360.f);
		UpdateOrientation();
	}
}

void UAccretionDiskComponent::UpdateOrientation()
{
	// Default: flat in the world's horizontal plane.
	FVector Normal = FVector::UpVector;
	FVector Forward = FVector::ForwardVector;

	// Tip the disk toward the viewer so it never collapses to an invisible edge-on line.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (Controller)
	{
		const FVector View = Controller->GetControlRotation().Vector();
		const FVector TowardViewer = FVector(-View.X, -View.Y, 0.f).GetSafeNormal();
		if (!TowardViewer.IsZero())
		{
			const float Tilt = FMath::DegreesToRadians(TiltTowardViewerDegrees);
			Normal = FVector::UpVector * FMath::Cos(Tilt) + TowardViewer * FMath::Sin(Tilt);
			Forward = TowardViewer;
		}
	}

	const FQuat Base = FRotationMatrix::MakeFromZX(Normal, Forward).ToQuat();
	SetWorldRotation(Base * FQuat(FVector::UpVector, FMath::DegreesToRadians(SpinDegrees)));
}

void UAccretionDiskComponent::NotifyMatterConsumed()
{
	FeedHoldRemaining = FeedHoldTime;
}

void UAccretionDiskComponent::UpdateFade(float DeltaTime)
{
	DeltaTime = FMath::Max(DeltaTime, 0.f);
	const bool bFeeding = IsFeeding();
	FeedHoldRemaining = FMath::Max(FeedHoldRemaining - DeltaTime, 0.f);

	if (bFeeding)
	{
		Intensity = FMath::Min(Intensity + DeltaTime / FMath::Max(FadeInTime, 0.01f), 1.f);
	}
	else
	{
		Intensity = FMath::Max(Intensity - DeltaTime / FMath::Max(FadeOutTime, 0.01f), 0.f);
	}
}

void UAccretionDiskComponent::ApplyIntensity()
{
	// Ease so the fade looks smooth at both ends.
	const float Brightness = FMath::SmoothStep(0.f, 1.f, Intensity);
	for (int32 Index = 0; Index < SectionMaterials.Num(); ++Index)
	{
		if (UMaterialInstanceDynamic* Material = SectionMaterials[Index])
		{
			Material->SetVectorParameterValue(AccretionDisk::ColorParameter, SectionColors[Index] * Brightness);
		}
	}

	const bool bShouldBeVisible = Intensity > 0.f;
	if (IsVisible() != bShouldBeVisible)
	{
		SetVisibility(bShouldBeVisible);
	}
}
