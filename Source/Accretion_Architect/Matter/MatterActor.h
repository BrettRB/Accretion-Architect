// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MatterActor.generated.h"

class UStaticMeshComponent;

/**
 * A piece of consumable matter (gray-box: a sphere). Moves under its own velocity;
 * gravity sources add acceleration each frame instead of moving it directly,
 * so it follows orbits rather than snapping to the player.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AMatterActor : public AActor
{
	GENERATED_BODY()

public:
	AMatterActor();

	virtual void Tick(float DeltaTime) override;
	virtual FVector GetVelocity() const override { return Velocity; }

	/** Sets mass and scales the mesh to match. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Matter")
	void InitializeMatter(float InMatterMass, const FVector& InVelocity);

	/** Accumulates acceleration applied on the next tick. */
	void AddAcceleration(const FVector& Acceleration) { PendingAcceleration += Acceleration; }

	void SetVelocity(const FVector& NewVelocity) { Velocity = NewVelocity; }

	/** Removes the matter from play. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Matter")
	void Consume();

	UFUNCTION(BlueprintPure, Category = "Accretion|Matter")
	float GetMatterMass() const { return MatterMass; }

	bool IsConsumed() const { return bConsumed; }

	/** Mesh scale per unit of mass (cube-root scaling keeps sizes readable). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Matter", meta = (ClampMin = "0"))
	float ScalePerCubeRootMass = 0.6f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Accretion|Matter")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Matter", meta = (ClampMin = "0"))
	float MatterMass = 3.f;

private:
	FVector Velocity = FVector::ZeroVector;
	FVector PendingAcceleration = FVector::ZeroVector;
	bool bConsumed = false;
};
