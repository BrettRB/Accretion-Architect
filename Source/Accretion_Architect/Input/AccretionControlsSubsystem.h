// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Input/AccretionKeyBindings.h"
#include "AccretionControlsSubsystem.generated.h"

class UInputAction;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAccretionControlsChanged);

/**
 * Owns the player's Enhanced Input actions and mapping context (created in code, no assets),
 * applies the player's saved key bindings and mouse preferences, and saves changes.
 */
UCLASS()
class ACCRETION_ARCHITECT_API UAccretionControlsSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Adds the mapping context to the local player's Enhanced Input subsystem. */
	void ApplyMappingContext();

	const UInputAction* GetMoveAction(EAccretionInputAction Action) const;
	const UInputAction* GetLookAction() const { return LookAction; }
	const UInputAction* GetToggleControlsMenuAction() const { return ToggleControlsMenuAction; }

	UFUNCTION(BlueprintPure, Category = "Accretion|Controls")
	FKey GetKeyForAction(EAccretionInputAction Action) const;

	/** Rebinds an action (swapping with any action that used the key) and saves. Returns false if the key is not allowed. */
	UFUNCTION(BlueprintCallable, Category = "Accretion|Controls")
	bool RebindAction(EAccretionInputAction Action, FKey NewKey);

	UFUNCTION(BlueprintCallable, Category = "Accretion|Controls")
	void ResetToDefaults();

	UFUNCTION(BlueprintPure, Category = "Accretion|Controls")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	UFUNCTION(BlueprintCallable, Category = "Accretion|Controls")
	void SetMouseSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintPure, Category = "Accretion|Controls")
	bool GetInvertMouseY() const { return bInvertMouseY; }

	UFUNCTION(BlueprintCallable, Category = "Accretion|Controls")
	void SetInvertMouseY(bool bInvert);

	UPROPERTY(BlueprintAssignable, Category = "Accretion|Controls")
	FOnAccretionControlsChanged OnControlsChanged;

	static constexpr float MinMouseSensitivity = 0.1f;
	static constexpr float MaxMouseSensitivity = 3.f;

private:
	void CreateInputObjects();
	void LoadSettings();
	void SaveSettings() const;
	void RebuildMappings();

	UPROPERTY(Transient)
	TMap<EAccretionInputAction, TObjectPtr<UInputAction>> MoveActions;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleControlsMenuAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	TMap<EAccretionInputAction, FKey> KeyBindings;
	float MouseSensitivity = 1.f;
	bool bInvertMouseY = false;
};
