// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/AccretionPlayerController.h"
#include "Input/AccretionControlsSubsystem.h"
#include "Player/AccretionBlackHolePawn.h"
#include "Components/AccretionGravityComponent.h"
#include "Components/AccretionMassComponent.h"
#include "Universe/CelestialBody.h"
#include "EngineUtils.h"
#include "UI/AccretionControlsMenuWidget.h"
#include "UI/AccretionVictoryWidget.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"

UAccretionControlsSubsystem* AAccretionPlayerController::GetControlsSubsystem() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UAccretionControlsSubsystem>() : nullptr;
}

void AAccretionPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void AAccretionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UAccretionControlsSubsystem* Controls = GetControlsSubsystem();
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Controls || !EnhancedInput)
	{
		return;
	}

	Controls->ApplyMappingContext();

	for (const EAccretionInputAction Action : TEnumRange<EAccretionInputAction>())
	{
		EnhancedInput->BindActionValue(Controls->GetMoveAction(Action));
	}
	EnhancedInput->BindAction(Controls->GetLookAction(), ETriggerEvent::Triggered, this, &AAccretionPlayerController::HandleLook);
	EnhancedInput->BindAction(Controls->GetToggleControlsMenuAction(), ETriggerEvent::Started, this, &AAccretionPlayerController::ToggleControlsMenu);
	EnhancedInput->BindAction(Controls->GetMoveAction(EAccretionInputAction::Boost), ETriggerEvent::Started, this, &AAccretionPlayerController::HandleBoost);
}

void AAccretionPlayerController::HandleBoost()
{
	AAccretionBlackHolePawn* BlackHole = Cast<AAccretionBlackHolePawn>(GetPawn());
	if (BlackHole && !IsControlsMenuOpen())
	{
		// Boost toward where the camera is looking.
		BlackHole->TryBoost(GetControlRotation().Vector());
	}
}

void AAccretionPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	APawn* ControlledPawn = GetPawn();
	const UAccretionControlsSubsystem* Controls = GetControlsSubsystem();
	const UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!ControlledPawn || !Controls || !EnhancedInput || IsControlsMenuOpen())
	{
		return;
	}

	auto Axis = [&](EAccretionInputAction Positive, EAccretionInputAction Negative)
	{
		const float Pos = EnhancedInput->GetBoundActionValue(Controls->GetMoveAction(Positive)).Get<bool>() ? 1.f : 0.f;
		const float Neg = EnhancedInput->GetBoundActionValue(Controls->GetMoveAction(Negative)).Get<bool>() ? 1.f : 0.f;
		return Pos - Neg;
	};

	// The movement plane follows the camera: full control rotation, including pitch.
	const FRotationMatrix CameraAxes(GetControlRotation());
	const FVector Direction =
		CameraAxes.GetUnitAxis(EAxis::X) * Axis(EAccretionInputAction::MoveForward, EAccretionInputAction::MoveBackward) +
		CameraAxes.GetUnitAxis(EAxis::Y) * Axis(EAccretionInputAction::MoveRight, EAccretionInputAction::MoveLeft) +
		CameraAxes.GetUnitAxis(EAxis::Z) * Axis(EAccretionInputAction::MoveUp, EAccretionInputAction::MoveDown);

	if (!Direction.IsNearlyZero())
	{
		ControlledPawn->AddMovementInput(Direction.GetSafeNormal());
	}
}

void AAccretionPlayerController::HandleLook(const FInputActionValue& Value)
{
	const UAccretionControlsSubsystem* Controls = GetControlsSubsystem();
	if (!Controls || IsControlsMenuOpen())
	{
		return;
	}

	const FVector2D Look = Value.Get<FVector2D>();
	const float Rate = BaseLookRate * Controls->GetMouseSensitivity();
	AddYawInput(Look.X * Rate);
	AddPitchInput(Look.Y * Rate * (Controls->GetInvertMouseY() ? -1.f : 1.f));
}

bool AAccretionPlayerController::IsControlsMenuOpen() const
{
	return ControlsMenu && ControlsMenu->IsInViewport();
}

void AAccretionPlayerController::AccretionAddMass(float Amount)
{
#if !UE_BUILD_SHIPPING
	if (const APawn* ControlledPawn = GetPawn())
	{
		if (UAccretionMassComponent* Mass = ControlledPawn->FindComponentByClass<UAccretionMassComponent>())
		{
			Mass->AddMass(Amount);
		}
	}
#endif
}

void AAccretionPlayerController::AccretionGoTo(const FString& BodyName)
{
#if !UE_BUILD_SHIPPING
	APawn* ControlledPawn = GetPawn();
	const UAccretionGravityComponent* Gravity = ControlledPawn ? ControlledPawn->FindComponentByClass<UAccretionGravityComponent>() : nullptr;
	if (!Gravity)
	{
		return;
	}
	for (TActorIterator<ACelestialBody> It(GetWorld()); It; ++It)
	{
		if (It->GetBodyName().Contains(BodyName))
		{
			// Inside the pull radius (so it starts interacting at once), on the side facing the player.
			const FVector Away = (ControlledPawn->GetActorLocation() - It->GetActorLocation()).GetSafeNormal();
			ControlledPawn->SetActorLocation(It->GetActorLocation() + Away * (It->GetRadius() + Gravity->GetPullRadius() * 0.7f));
			SetControlRotation((-Away).Rotation());
			return;
		}
	}
#endif
}

void AAccretionPlayerController::ShowVictoryScreen(const FAccretionRunSummary& Summary)
{
	if (IsControlsMenuOpen())
	{
		CloseControlsMenu();
	}

	if (!VictoryScreen)
	{
		VictoryScreen = CreateWidget<UAccretionVictoryWidget>(this, UAccretionVictoryWidget::StaticClass());
	}
	if (!VictoryScreen)
	{
		return;
	}
	VictoryScreen->SetSummary(Summary);
	if (!VictoryScreen->IsInViewport())
	{
		VictoryScreen->AddToViewport(200);
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(VictoryScreen->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
	FlushPressedKeys();
}

void AAccretionPlayerController::ToggleControlsMenu()
{
	if (VictoryScreen && VictoryScreen->IsInViewport())
	{
		return;
	}
	if (IsControlsMenuOpen())
	{
		CloseControlsMenu();
	}
	else
	{
		OpenControlsMenu();
	}
}

void AAccretionPlayerController::OpenControlsMenu()
{
	if (!ControlsMenu)
	{
		ControlsMenu = CreateWidget<UAccretionControlsMenuWidget>(this, UAccretionControlsMenuWidget::StaticClass());
		ControlsMenu->OnCloseRequested.AddUObject(this, &AAccretionPlayerController::CloseControlsMenu);
	}

	ControlsMenu->AddToViewport(100);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ControlsMenu->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
	SetPause(true);
	// The game never sees the key-up for the key that opened the menu; release everything so it is not stuck down.
	FlushPressedKeys();
}

void AAccretionPlayerController::CloseControlsMenu()
{
	if (ControlsMenu)
	{
		ControlsMenu->RemoveFromParent();
	}

	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	SetPause(false);
	FlushPressedKeys();
}
