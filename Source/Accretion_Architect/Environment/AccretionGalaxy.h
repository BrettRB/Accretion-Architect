// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AccretionGalaxy.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * Spiral galaxy that forms around the black hole when it becomes supermassive: a bright central bulge,
 * blue-white spiral arms, and a fainter disk of older stars, rotating slowly around the black hole.
 * Grows from nothing to full size over GrowSeconds.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionGalaxy : public AActor
{
	GENERATED_BODY()

public:
	AAccretionGalaxy();

	virtual void Tick(float DeltaTime) override;

	/** Builds the galaxy with the given radius (cm) and starts it growing. */
	UFUNCTION(BlueprintCallable, Category = "Galaxy")
	void FormGalaxy(float InRadius);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy", meta = (ClampMin = "100"))
	int32 StarCount = 40000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy", meta = (ClampMin = "1", ClampMax = "8"))
	int32 ArmCount = 4;

	/** Tightness of the spiral arms: radians of turn per e-fold of radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy", meta = (ClampMin = "0"))
	float ArmWinding = 2.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy", meta = (ClampMin = "0.1"))
	float GrowSeconds = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
	float RotationDegreesPerSecond = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
	int32 Seed = 1977;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> Stars;

private:
	void BuildStars();

	UPROPERTY()
	TObjectPtr<UMaterialInterface> VertexColorUnlitMaterial;

	float Radius = 1.f;
	float Age = 0.f;
	bool bFormed = false;
};
