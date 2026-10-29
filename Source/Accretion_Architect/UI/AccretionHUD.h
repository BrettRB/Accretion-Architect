// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AccretionHUD.generated.h"

/**
 * Gray-box HUD drawn on the canvas: mass and disk pressure meters, run-end message,
 * and a controls hint. To be replaced by a UMG HUD later.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Mass that fills the mass meter. Purely a display scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|HUD", meta = (ClampMin = "1"))
	float MassMeterMax = 300.f;

private:
	void DrawMeter(float X, float Y, float Fill, const FLinearColor& Color, const FString& Label, float MarkerFill = -1.f);
};
