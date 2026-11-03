// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/AccretionMenuGameMode.h"
#include "Environment/AccretionSpaceEnvironment.h"
#include "Player/AccretionMenuPlayerController.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"

AAccretionMenuGameMode::AAccretionMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AAccretionMenuPlayerController::StaticClass();
	HUDClass = AHUD::StaticClass();
	EnvironmentClass = AAccretionSpaceEnvironment::StaticClass();
}

void AAccretionMenuGameMode::StartPlay()
{
	Super::StartPlay();

	if (EnvironmentClass && !TActorIterator<AActor>(GetWorld(), EnvironmentClass))
	{
		GetWorld()->SpawnActor<AActor>(EnvironmentClass);
	}
}
