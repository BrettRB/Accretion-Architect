// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AccretionHUD.generated.h"

class AAccretionUniverse;

/**
 * Gray-box HUD drawn on the canvas: mass (with progress toward supermassive on a log scale), disk
 * pressure, boost status, the nearest planet/star and whether it can be eaten yet, the run-end message,
 * and a controls hint. To be replaced by a UMG HUD later.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Bodies within this many pull radii (surface distance) are shown in the target readout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|HUD", meta = (ClampMin = "0"))
	float NearestBodyRangeInPullRadii = 6.f;

private:
	void DrawMeter(float X, float Y, float Fill, const FLinearColor& Color, const FString& Label, float MarkerFill = -1.f);
	void DrawNearestBody(const APawn* Pawn, float X, float Y);

	TWeakObjectPtr<AAccretionUniverse> Universe;
};
