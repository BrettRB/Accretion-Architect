// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AccretionPlayerController.generated.h"

class UAccretionControlsMenuWidget;
class UAccretionControlsSubsystem;
struct FInputActionValue;

/**
 * Keyboard and mouse control for the black hole. Movement is relative to the camera:
 * forward/back follow where the camera looks (including pitch), left/right and up/down follow
 * the camera's right and up axes. Also opens the controls menu (Tab).
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Accretion|Controls")
	void ToggleControlsMenu();

	UFUNCTION(BlueprintPure, Category = "Accretion|Controls")
	bool IsControlsMenuOpen() const;

	/** Base look speed (degrees per mouse unit) before the player's sensitivity multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Controls", meta = (ClampMin = "0"))
	float BaseLookRate = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UAccretionControlsSubsystem* GetControlsSubsystem() const;
	void HandleLook(const FInputActionValue& Value);
	void OpenControlsMenu();
	void CloseControlsMenu();

	UPROPERTY(Transient)
	TObjectPtr<UAccretionControlsMenuWidget> ControlsMenu;
};
