// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/AccretionGameMode.h"
#include "AccretionVictoryWidget.generated.h"

class UTextBlock;

/** Victory screen shown once the black hole is supermassive: run stats and a Return to Menu button. */
UCLASS()
class ACCRETION_ARCHITECT_API UAccretionVictoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSummary(const FAccretionRunSummary& InSummary);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
	void RefreshStats();

	UFUNCTION()
	void HandleReturnClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsText;

	FAccretionRunSummary Summary;
};
