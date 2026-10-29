// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/AccretionControlsSubsystem.h"
#include "Input/AccretionControlsSaveGame.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

namespace AccretionControls
{
	static const FString SaveSlotName = TEXT("AccretionControls");
	static constexpr int32 SaveUserIndex = 0;
}

void UAccretionControlsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UEnhancedInputLocalPlayerSubsystem>();
	Super::Initialize(Collection);

	CreateInputObjects();
	LoadSettings();
	RebuildMappings();
}

void UAccretionControlsSubsystem::CreateInputObjects()
{
	for (const EAccretionInputAction Action : TEnumRange<EAccretionInputAction>())
	{
		UInputAction* InputAction = NewObject<UInputAction>(this);
		InputAction->ValueType = EInputActionValueType::Boolean;
		MoveActions.Add(Action, InputAction);
	}

	LookAction = NewObject<UInputAction>(this, TEXT("IA_Look"));
	LookAction->ValueType = EInputActionValueType::Axis2D;

	ToggleControlsMenuAction = NewObject<UInputAction>(this, TEXT("IA_ToggleControlsMenu"));
	ToggleControlsMenuAction->ValueType = EInputActionValueType::Boolean;
	ToggleControlsMenuAction->bTriggerWhenPaused = true;

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_AccretionDefault"));
}

void UAccretionControlsSubsystem::LoadSettings()
{
	KeyBindings = FAccretionKeyBindings::GetDefaultBindings();

	if (UGameplayStatics::DoesSaveGameExist(AccretionControls::SaveSlotName, AccretionControls::SaveUserIndex))
	{
		if (const UAccretionControlsSaveGame* Save = Cast<UAccretionControlsSaveGame>(
			UGameplayStatics::LoadGameFromSlot(AccretionControls::SaveSlotName, AccretionControls::SaveUserIndex)))
		{
			KeyBindings = Save->KeyBindings;
			MouseSensitivity = FMath::Clamp(Save->MouseSensitivity, MinMouseSensitivity, MaxMouseSensitivity);
			bInvertMouseY = Save->bInvertMouseY;
		}
	}

	FAccretionKeyBindings::Sanitize(KeyBindings);
}

void UAccretionControlsSubsystem::SaveSettings() const
{
	UAccretionControlsSaveGame* Save = Cast<UAccretionControlsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UAccretionControlsSaveGame::StaticClass()));
	Save->KeyBindings = KeyBindings;
	Save->MouseSensitivity = MouseSensitivity;
	Save->bInvertMouseY = bInvertMouseY;
	UGameplayStatics::SaveGameToSlot(Save, AccretionControls::SaveSlotName, AccretionControls::SaveUserIndex);
}

void UAccretionControlsSubsystem::RebuildMappings()
{
	MappingContext->UnmapAll();

	for (const TPair<EAccretionInputAction, FKey>& Pair : KeyBindings)
	{
		MappingContext->MapKey(MoveActions.FindRef(Pair.Key), Pair.Value);
	}
	MappingContext->MapKey(LookAction, EKeys::Mouse2D);
	MappingContext->MapKey(ToggleControlsMenuAction, FAccretionKeyBindings::GetControlsMenuKey());

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		FModifyContextOptions Options;
		Options.bForceImmediately = true;
		InputSubsystem->RequestRebuildControlMappings(Options);
	}
}

void UAccretionControlsSubsystem::ApplyMappingContext()
{
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		if (!InputSubsystem->HasMappingContext(MappingContext))
		{
			InputSubsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

const UInputAction* UAccretionControlsSubsystem::GetMoveAction(EAccretionInputAction Action) const
{
	return MoveActions.FindRef(Action);
}

FKey UAccretionControlsSubsystem::GetKeyForAction(EAccretionInputAction Action) const
{
	return KeyBindings.FindRef(Action);
}

bool UAccretionControlsSubsystem::RebindAction(EAccretionInputAction Action, FKey NewKey)
{
	if (!FAccretionKeyBindings::Rebind(KeyBindings, Action, NewKey))
	{
		return false;
	}
	RebuildMappings();
	SaveSettings();
	OnControlsChanged.Broadcast();
	return true;
}

void UAccretionControlsSubsystem::ResetToDefaults()
{
	KeyBindings = FAccretionKeyBindings::GetDefaultBindings();
	MouseSensitivity = 1.f;
	bInvertMouseY = false;
	RebuildMappings();
	SaveSettings();
	OnControlsChanged.Broadcast();
}

void UAccretionControlsSubsystem::SetMouseSensitivity(float NewSensitivity)
{
	MouseSensitivity = FMath::Clamp(NewSensitivity, MinMouseSensitivity, MaxMouseSensitivity);
	SaveSettings();
	OnControlsChanged.Broadcast();
}

void UAccretionControlsSubsystem::SetInvertMouseY(bool bInvert)
{
	bInvertMouseY = bInvert;
	SaveSettings();
	OnControlsChanged.Broadcast();
}
