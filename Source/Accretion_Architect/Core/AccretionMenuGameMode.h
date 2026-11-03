// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AccretionMenuGameMode.generated.h"

/**
 * Main menu: no black hole, just the space backdrop behind the menu widget. Runs start from here,
 * and the victory screen returns here. Starting options (black hole types) will be chosen here later.
 */
UCLASS()
class ACCRETION_ARCHITECT_API AAccretionMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAccretionMenuGameMode();

	virtual void StartPlay() override;

	/** Space backdrop shown behind the menu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accretion|Menu")
	TSubclassOf<AActor> EnvironmentClass;

protected:
	/** The menu has no pawn to spawn. */
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override { return false; }
};
