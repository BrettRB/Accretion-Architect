// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/AccretionKeyBindings.h"
#include "AccretionControlsMenuWidget.generated.h"

class UAccretionControlsRowWidget;
class UAccretionControlsSubsystem;
class UCheckBox;
class USlider;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FOnControlsMenuCloseRequested);

/**
 * Controls menu: rebind movement keys, adjust mouse sensitivity, invert mouse Y, reset to defaults.
 * Built entirely in code. Changes are applied and saved immediately through UAccretionControlsSubsystem.
 */
UCLASS()
class ACCRETION_ARCHITECT_API UAccretionControlsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnControlsMenuCloseRequested OnCloseRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UAccretionControlsSubsystem* GetControlsSubsystem() const;
	void BuildLayout();
	void RefreshAll();
	void StartListening(EAccretionInputAction Action);
	void StopListening();
	void TryBindKey(const FKey& Key);

	UFUNCTION()
	void HandleSensitivityChanged(float Value);

	UFUNCTION()
	void HandleInvertChanged(bool bIsChecked);

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleControlsChanged();

	UPROPERTY(Transient)
	TMap<EAccretionInputAction, TObjectPtr<UAccretionControlsRowWidget>> Rows;

	UPROPERTY(Transient)
	TObjectPtr<USlider> SensitivitySlider;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SensitivityValueText;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> InvertCheckBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	TOptional<EAccretionInputAction> ListeningAction;

	/** Set while RefreshAll pushes values into widgets, so their change events are ignored. */
	bool bRefreshing = false;
};
