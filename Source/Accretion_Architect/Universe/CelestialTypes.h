// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CelestialTypes.generated.h"

/** Kinds of planets, moons, and stars the universe can generate. */
UENUM(BlueprintType)
enum class ECelestialBodyType : uint8
{
	Moon,
	RockyPlanet,
	TerrestrialPlanet,
	IceGiant,
	GasGiant,
	RedDwarf,        // M
	OrangeDwarf,     // K
	YellowDwarf,     // G
	YellowWhiteStar, // F
	WhiteStar,       // A
	BlueGiant,       // B
	BlueSupergiant,  // O
	WhiteDwarf,
	RedGiant,

	Count UMETA(Hidden)
};

/** What the black hole can currently do to a body. */
UENUM(BlueprintType)
enum class ECelestialConsumeMode : uint8
{
	/** The body is too massive; the black hole must grow first. */
	TooMassive,
	/** The body is torn apart over time into a stream of fragments. */
	Shred,
	/** The body is pulled off its orbit and swallowed in one gulp. */
	SwallowWhole
};

/** Tuning for one body type. Ranges are rolled per body. Units: game mass, cm. */
USTRUCT(BlueprintType)
struct ACCRETION_ARCHITECT_API FCelestialClassDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	ECelestialBodyType Type = ECelestialBodyType::RockyPlanet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	bool bIsStar = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial", meta = (ClampMin = "0"))
	float MinMass = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial", meta = (ClampMin = "0"))
	float MaxMass = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial", meta = (ClampMin = "0"))
	float MinRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial", meta = (ClampMin = "0"))
	float MaxRadius = 1000.f;

	/** Surface color. Stars use it as HDR emissive color, planets as base color. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	FLinearColor Color = FLinearColor::Gray;

	/** Alternative colors picked at random (planets), e.g. different rocky surfaces. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	TArray<FLinearColor> AlternateColors;

	/** Stars: relative chance of this class being picked as a system's star. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial|Star", meta = (ClampMin = "0"))
	float StarWeight = 0.f;

	/** Stars: brightness of the light the star casts on its system. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial|Star", meta = (ClampMin = "0"))
	float LightIntensity = 0.f;

	/** Stars: distance (cm) beyond which planets form as giants instead of rocky worlds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial|Star", meta = (ClampMin = "0"))
	float FrostLineDistance = 0.f;

	/** Stars: how many planets the star can host. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial|Star", meta = (ClampMin = "0"))
	int32 MinPlanets = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial|Star", meta = (ClampMin = "0"))
	int32 MaxPlanets = 0;

	/** Planets: how many moons the planet can host. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial|Planet", meta = (ClampMin = "0"))
	int32 MaxMoons = 0;
};

/**
 * Definitions for every celestial body type. Code supplies sensible defaults; a Data Asset of this
 * class can override them without code changes.
 */
UCLASS(BlueprintType)
class ACCRETION_ARCHITECT_API UCelestialCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UCelestialCatalog();

	const FCelestialClassDefinition* Find(ECelestialBodyType Type) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	TArray<FCelestialClassDefinition> Classes;
};

/** One generated body: everything needed to spawn it. Parent indices refer to FStarSystemDesc::Bodies. */
USTRUCT(BlueprintType)
struct ACCRETION_ARCHITECT_API FCelestialBodyDesc
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	ECelestialBodyType Type = ECelestialBodyType::RockyPlanet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float Mass = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	FLinearColor Color = FLinearColor::Gray;

	/** Body this one orbits, or INDEX_NONE to orbit the system's center (barycenter). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	int32 ParentIndex = INDEX_NONE;

	/** 0 = sits at the center (single star). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float OrbitRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float OrbitPeriod = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float OrbitPhaseDegrees = 0.f;

	/** Small tilt of this orbit relative to the system plane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float OrbitInclinationDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	bool bIsStar = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float LightIntensity = 0.f;
};

/** A generated star system, before any actors exist. */
USTRUCT(BlueprintType)
struct ACCRETION_ARCHITECT_API FStarSystemDesc
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	TArray<FCelestialBodyDesc> Bodies;

	/** Radius (cm) containing every orbit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float Radius = 0.f;

	/** Orientation of the system's orbital plane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	FRotator OrbitPlane = FRotator::ZeroRotator;

	/** Asteroid belt (around the system center) if BeltOuterRadius > 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float BeltInnerRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Celestial")
	float BeltOuterRadius = 0.f;

	int32 NumStars() const;
};
