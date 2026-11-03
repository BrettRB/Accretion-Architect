// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/AccretionMenuPlayerController.h"
#include "UI/AccretionMainMenuWidget.h"
#include "Blueprint/UserWidget.h"

AAccretionMenuPlayerController::AAccretionMenuPlayerController()
{
	// No pawn in the menu: the camera views from the controller itself.
	bAutoManageActiveCameraTarget = true;
}

void AAccretionMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	SetControlRotation(FRotator(10.f, 0.f, 0.f));
	MainMenu = CreateWidget<UAccretionMainMenuWidget>(this, UAccretionMainMenuWidget::StaticClass());
	if (MainMenu)
	{
		MainMenu->AddToViewport();
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MainMenu->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	SetShowMouseCursor(true);
}

void AAccretionMenuPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	FRotator View = GetControlRotation();
	View.Yaw += BackdropDriftDegreesPerSecond * DeltaTime;
	SetControlRotation(View);
}
