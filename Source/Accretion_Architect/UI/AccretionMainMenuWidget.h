// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AccretionMainMenuWidget.generated.h"

/**
 * Main menu, built in code: title, the starting black hole (one option for now; more types will be
 * added with meta-progression), Start Run, and Quit.
 */
UCLASS()
class ACCRETION_ARCHITECT_API UAccretionMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
