// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AccretionSpaceEnvironment.generated.h"

class UDirectionalLightComponent;
class UMaterialInterface;
class UPostProcessComponent;
class UProceduralMeshComponent;

/**
 * Deep-space backdrop shared by the menu and the run, built in code from engine content:
 * - a dark sky with a procedural starfield and Milky Way band that stays at "infinity" around the camera,
 *   so the black hole is seen by the stars it blocks;
 * - a faint fill light so matter stays readable between and against stars;
 * - manual exposure so the scene stays dark instead of auto-brightening.
 * Daytime sky actors already in the level (atmosphere, fog, sky light, sun, clouds) are switched off.
 * Stars, planets, and their light come from AAccretionUniverse (Proxima Centauri is the starting system).
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionSpaceEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AAccretionSpaceEnvironment();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- Star field ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars", meta = (ClampMin = "0"))
	int32 BackgroundStarCount = 18000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars", meta = (ClampMin = "0"))
	int32 MilkyWayStarCount = 9000;

	/** Distance (cm) of the star sphere from the camera. Must be beyond every other object, including the victory galaxy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars", meta = (ClampMin = "1000"))
	float StarFieldRadius = 1.0e8f;

	/** Star half-size range in radians (about 1-3 pixels at 1080p). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars", meta = (ClampMin = "0"))
	float MinStarAngularSize = 0.0007f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars", meta = (ClampMin = "0"))
	float MaxStarAngularSize = 0.0018f;

	/** Orientation of the Milky Way band across the sky. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars")
	FRotator MilkyWayOrientation = FRotator(25.f, 30.f, 60.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars")
	int32 StarSeed = 4242;

	// --- Lighting ---

	/** Dim fill light so matter is not a black silhouette when it is backlit by a star. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Lighting")
	FLinearColor FillLightColor = FLinearColor(0.75f, 0.8f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Lighting", meta = (ClampMin = "0"))
	float FillLightIntensity = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Lighting")
	FRotator FillLightRotation = FRotator(-25.f, -50.f, 0.f);

	// --- Camera / post process ---

	/** Manual exposure compensation (EV). Higher is brighter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Post Process")
	float ExposureBias = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Post Process", meta = (ClampMin = "0"))
	float BloomIntensity = 1.2f;

	/** Turn off atmosphere, fog, sky light, sun, and cloud components already placed in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment")
	bool bSuppressLevelSkyAndSun = true;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> StarField;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDirectionalLightComponent> FillLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPostProcessComponent> PostProcess;

private:
	void BuildStarField();
	void SetUpPostProcess();
	void SuppressLevelSkyAndSun();

	UPROPERTY()
	TObjectPtr<UMaterialInterface> VertexColorUnlitMaterial;
};
