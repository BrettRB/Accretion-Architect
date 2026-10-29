// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/AccretionKeyBindings.h"
#include "AccretionControlsRowWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRebindRequested, EAccretionInputAction);

/** One line of the controls menu: action name plus a button showing (and rebinding) its key. Built in code. */
UCLASS()
class ACCRETION_ARCHITECT_API UAccretionControlsRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Must be called before the widget is first displayed. */
	void InitRow(EAccretionInputAction InAction);

	/** Updates the key label. bListening shows a prompt instead of the key. */
	void Refresh(const FKey& Key, bool bListening);

	FOnRebindRequested OnRebindRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION()
	void HandleKeyButtonClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> KeyText;

	EAccretionInputAction Action = EAccretionInputAction::MoveForward;
};
