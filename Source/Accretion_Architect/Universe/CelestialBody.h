// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Universe/CelestialTypes.h"
#include "CelestialBody.generated.h"

class ACelestialBody;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture;

/** Fired when a body leaves play. bConsumed is true if the black hole ate it (false when unloaded). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCelestialBodyRemoved, ACelestialBody* /*Body*/, bool /*bConsumed*/);

/**
 * A star, planet, or moon. Follows a circular orbit around its parent body (or the system center)
 * until a black hole interacts with it:
 * - Shred: mass is torn off over time as fragments (matter actors) that spiral into the black hole;
 *   the body shrinks and is gone once nearly all of its mass is stripped.
 * - Capture + swallow: the body leaves its orbit, falls toward the black hole, and is eaten whole.
 */
UCLASS()
class ACCRETION_ARCHITECT_API ACelestialBody : public AActor
{
	GENERATED_BODY()

public:
	ACelestialBody();

	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FVector GetVelocity() const override { return Velocity; }

	/**
	 * Sets up the body. Parent may be null (orbits SystemCenter). MassFraction < 1 restores a body that
	 * was partly shredded before it was unloaded. LightRadius is the reach of a star's light.
	 */
	void InitializeBody(const FCelestialBodyDesc& InDesc, uint64 InBodyId, ACelestialBody* InParent,
		const FVector& InSystemCenter, const FQuat& InSystemPlane, float InMassFraction, float LightRadius);

	UFUNCTION(BlueprintPure, Category = "Celestial")
	float GetMass() const { return Mass; }

	UFUNCTION(BlueprintPure, Category = "Celestial")
	float GetInitialMass() const { return InitialMass; }

	/** Current radius (shrinks while being shredded). */
	UFUNCTION(BlueprintPure, Category = "Celestial")
	float GetRadius() const;

	UFUNCTION(BlueprintPure, Category = "Celestial")
	bool IsStar() const { return Desc.bIsStar; }

	UFUNCTION(BlueprintPure, Category = "Celestial")
	ECelestialBodyType GetBodyType() const { return Desc.Type; }

	UFUNCTION(BlueprintPure, Category = "Celestial")
	FString GetBodyName() const { return Desc.Name; }

	UFUNCTION(BlueprintPure, Category = "Celestial")
	bool IsCaptured() const { return bCaptured; }

	bool IsConsumed() const { return bConsumed; }
	uint64 GetBodyId() const { return BodyId; }
	float GetMassFraction() const { return InitialMass > 0.f ? Mass / InitialMass : 0.f; }

	/** Pulls the body off its orbit; from now on it moves under its own velocity plus applied acceleration. */
	void Capture();

	/** Accumulates acceleration applied on the next tick (captured bodies only). */
	void AddAcceleration(const FVector& Acceleration) { PendingAcceleration += Acceleration; }

	/**
	 * Tears mass off for DeltaTime and launches it toward the black hole as fragments with sideways
	 * speed FragmentSpeed, so they spiral in rather than falling straight. At most MaxMassPerSecond is torn
	 * off (a black hole can only strip so much at once), so big bodies take longer.
	 */
	void Shred(float DeltaTime, const FVector& BlackHoleLocation, const FVector& BlackHoleVelocity, float FragmentSpeed, float MaxMassPerSecond);

	/** Removes the whole body. Returns the mass eaten. */
	float ConsumeWhole();

	FOnCelestialBodyRemoved OnRemoved;

	/** Fastest a body can be shredded completely (seconds); big bodies are slower, limited by the black hole. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial", meta = (ClampMin = "1"))
	float ShredSeconds = 8.f;

	/** How many fragments a full body breaks into. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial", meta = (ClampMin = "1"))
	int32 FragmentsPerBody = 60;

	/** Below this fraction of its starting mass the remnant is torn apart completely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial", meta = (ClampMin = "0", ClampMax = "1"))
	float MinRemainingFraction = 0.03f;

	/** Seconds a stray fragment lasts before it dissipates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celestial", meta = (ClampMin = "1"))
	float FragmentLifeSeconds = 120.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Corona;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> StarLight;

private:
	FVector ComputeOrbitLocation(float Time) const;
	void SpawnFragment(float FragmentMass, const FVector& BlackHoleLocation, const FVector& BlackHoleVelocity, float FragmentSpeed);
	void ApplyMassToVisuals();
	void Remove(bool bWasConsumed);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> UnlitColorMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> LitColorMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> AdditiveMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SurfaceMaterial;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACelestialBody> Parent;

	FCelestialBodyDesc Desc;
	uint64 BodyId = 0;
	FVector SystemCenter = FVector::ZeroVector;
	FVector LastParentLocation = FVector::ZeroVector;
	FQuat OrbitRotation = FQuat::Identity;
	FVector Velocity = FVector::ZeroVector;
	FVector PendingAcceleration = FVector::ZeroVector;
	float InitialMass = 0.f;
	float Mass = 0.f;
	float ShredAccumulator = 0.f;
	float BaseLightIntensity = 0.f;
	bool bCaptured = false;
	bool bConsumed = false;
	bool bRemoved = false;
};
