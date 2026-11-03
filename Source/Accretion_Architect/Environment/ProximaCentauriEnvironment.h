// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProximaCentauriEnvironment.generated.h"

class UDirectionalLightComponent;
class UMaterialInterface;
class UPostProcessComponent;
class UProceduralMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture;

/** A planet that orbits the system's star. Scenery only (not consumable). */
USTRUCT(BlueprintType)
struct FAccretionPlanetDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FName Name;

	/** Planet radius (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (ClampMin = "0"))
	float Radius = 3000.f;

	/** Distance (cm) from the star. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (ClampMin = "0"))
	float OrbitRadius = 100000.f;

	/** Seconds per orbit (gameplay time, not real time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet", meta = (ClampMin = "1"))
	float OrbitPeriodSeconds = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	float StartAngleDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FLinearColor Color = FLinearColor::Gray;
};

/**
 * Space backdrop for the Proxima Centauri system, built entirely in code from engine content:
 * - a dark sky with a procedural starfield and Milky Way band that stays at "infinity" around the camera,
 *   so the black hole is seen by the stars it blocks;
 * - Proxima Centauri, a red dwarf with a glowing corona and dim red light (plus a faint fill light
 *   from Alpha Centauri so matter stays readable when backlit);
 * - its planets (Proxima b and d by default) orbiting the star;
 * - Alpha Centauri A and B as a bright pair in the sky;
 * - manual exposure so the scene stays dark instead of auto-brightening.
 * Daytime sky actors already in the level (atmosphere, fog, sky light, sun, clouds) are switched off.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AProximaCentauriEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AProximaCentauriEnvironment();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- Star field ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars", meta = (ClampMin = "0"))
	int32 BackgroundStarCount = 18000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars", meta = (ClampMin = "0"))
	int32 MilkyWayStarCount = 9000;

	/** Distance (cm) of the star sphere from the camera. Must be beyond every other object. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars", meta = (ClampMin = "1000"))
	float StarFieldRadius = 3.0e6f;

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

	/** Direction in the sky of the Alpha Centauri A/B pair. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Stars")
	FVector AlphaCentauriDirection = FVector(-0.55f, 0.65f, 0.3f);

	// --- Proxima Centauri ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star")
	FVector ProximaLocation = FVector(320000.f, -140000.f, 30000.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star", meta = (ClampMin = "0"))
	float ProximaRadius = 25000.f;

	/** HDR surface color of the red dwarf. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star")
	FLinearColor ProximaColor = FLinearColor(1.6f, 0.32f, 0.06f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star")
	FLinearColor ProximaCoronaColor = FLinearColor(0.12f, 0.018f, 0.004f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star")
	FLinearColor ProximaLightColor = FLinearColor(1.f, 0.55f, 0.35f);

	/** Directional light intensity (lux) from Proxima onto the play area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star", meta = (ClampMin = "0"))
	float ProximaLightIntensity = 4.f;

	/**
	 * Dim fill light from the direction of Alpha Centauri, so matter between the camera and Proxima
	 * is not a black silhouette on a black sky.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star")
	FLinearColor FillLightColor = FLinearColor(0.75f, 0.8f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star", meta = (ClampMin = "0"))
	float FillLightIntensity = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Environment|Star")
	TArray<FAccretionPlanetDefinition> Planets;

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
	TObjectPtr<UStaticMeshComponent> ProximaBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProximaCorona;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDirectionalLightComponent> ProximaLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDirectionalLightComponent> FillLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPostProcessComponent> PostProcess;

private:
	void BuildStarField();
	void SetUpProxima();
	void SpawnPlanets();
	void SetUpPostProcess();
	void SuppressLevelSkyAndSun();
	void UpdatePlanetOrbits();

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> VertexColorUnlitMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> UnlitColorMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> LitColorMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> AdditiveMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PlanetMeshes;
};
