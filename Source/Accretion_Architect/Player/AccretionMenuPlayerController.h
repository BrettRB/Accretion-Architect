// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AccretionMenuPlayerController.generated.h"

class UAccretionMainMenuWidget;

/** Shows the main menu with a mouse cursor and slowly pans the starfield behind it. */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAccretionMenuPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	/** Degrees per second the backdrop drifts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Menu")
	float BackdropDriftDegreesPerSecond = 1.5f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAccretionMainMenuWidget> MainMenu;
};
