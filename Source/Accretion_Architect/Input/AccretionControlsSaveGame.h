// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Input/AccretionKeyBindings.h"
#include "AccretionControlsSaveGame.generated.h"

/** The player's control preferences, saved locally between sessions. */
UCLASS()
class ACCRETION_ARCHITECT_API UAccretionControlsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	TMap<EAccretionInputAction, FKey> KeyBindings;

	UPROPERTY(SaveGame)
	float MouseSensitivity = 1.f;

	UPROPERTY(SaveGame)
	bool bInvertMouseY = false;
};
