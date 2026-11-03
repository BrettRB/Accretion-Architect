// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "AccretionDiskComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture;

/**
 * Glowing accretion disk around the black hole. Invisible while the black hole is not feeding;
 * fades in when matter is consumed and fades back out once feeding stops.
 *
 * The disk is a procedural ring (additive, unlit) built in units of the component's scale:
 * the owner sets the scale to the event horizon radius, so the disk grows with the black hole.
 * Gray-box stand-in for a future Niagara disk effect.
 */
UCLASS(ClassGroup = (Accretion), meta = (BlueprintSpawnableComponent))
class ACCRETION_ARCHITECT_API UAccretionDiskComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UAccretionDiskComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Call whenever matter crosses the event horizon; keeps the disk lit for FeedHoldTime. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Disk")
	void NotifyMatterConsumed();

	/** Advances the fade toward lit (while feeding) or dark (after feeding stops). Called from TickComponent. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Disk")
	void UpdateFade(float DeltaTime);

	/** 0 = invisible, 1 = fully lit. */
	UFUNCTION(BlueprintPure, Category = "Accretion|Disk")
	float GetIntensity() const { return Intensity; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Disk")
	bool IsFeeding() const { return FeedHoldRemaining > 0.f; }

	/** Seconds the disk counts as "feeding" after the last consumed piece of matter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk", meta = (ClampMin = "0"))
	float FeedHoldTime = 1.f;

	/** Seconds to fade from dark to fully lit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk", meta = (ClampMin = "0.01"))
	float FadeInTime = 0.4f;

	/** Seconds to fade from fully lit to dark. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk", meta = (ClampMin = "0.01"))
	float FadeOutTime = 1.5f;

	/** Disk rotation speed in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk")
	float SpinDegreesPerSecond = 120.f;

	/**
	 * Degrees the disk tips from horizontal toward the camera, so it reads as an ellipse instead of
	 * vanishing when seen edge-on. 0 keeps it flat in the world's horizontal plane.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk", meta = (ClampMin = "0", ClampMax = "90"))
	float TiltTowardViewerDegrees = 30.f;

	/** Inner and outer disk radius in multiples of the component scale (the event horizon radius). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk", meta = (ClampMin = "0"))
	float InnerRadius = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk", meta = (ClampMin = "0"))
	float OuterRadius = 3.6f;

	/** Number of concentric bands used to grade the color from the hot inner edge to the cool outer edge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk", meta = (ClampMin = "1", ClampMax = "16"))
	int32 BandCount = 10;

	/** Bright spiral streaks that make the rotation visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk", meta = (ClampMin = "0", ClampMax = "16"))
	int32 SpiralArmCount = 5;

	/** HDR colors at full intensity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk")
	FLinearColor InnerColor = FLinearColor(1.6f, 1.15f, 0.75f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk")
	FLinearColor OuterColor = FLinearColor(0.45f, 0.08f, 0.012f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Disk")
	FLinearColor StreakColor = FLinearColor(1.1f, 0.5f, 0.12f);

private:
	void BuildDiskMesh();
	void ApplyIntensity();
	void UpdateOrientation();

	UPROPERTY()
	TObjectPtr<UMaterialInterface> DiskMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SectionMaterials;

	TArray<FLinearColor> SectionColors;
	float Intensity = 0.f;
	float FeedHoldRemaining = 0.f;
	float SpinDegrees = 0.f;
};
