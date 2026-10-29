// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/AccretionGameMode.h"
#include "Components/AccretionMassComponent.h"
#include "Components/DiskPressureComponent.h"
#include "Matter/MatterSpawner.h"
#include "Player/AccretionBlackHolePawn.h"
#include "Player/AccretionPlayerController.h"
#include "UI/AccretionHUD.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogAccretionRun, Log, All);

AAccretionGameMode::AAccretionGameMode()
{
	DefaultPawnClass = AAccretionBlackHolePawn::StaticClass();
	PlayerControllerClass = AAccretionPlayerController::StaticClass();
	HUDClass = AAccretionHUD::StaticClass();
	MatterSpawnerClass = AMatterSpawner::StaticClass();
}

void AAccretionGameMode::StartPlay()
{
	Super::StartPlay();

	if (MatterSpawnerClass && !TActorIterator<AMatterSpawner>(GetWorld()))
	{
		GetWorld()->SpawnActor<AMatterSpawner>(MatterSpawnerClass);
	}
}

void AAccretionGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	const APawn* Pawn = NewPlayer ? NewPlayer->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	if (UDiskPressureComponent* Pressure = Pawn->FindComponentByClass<UDiskPressureComponent>())
	{
		Pressure->OnBlowout.AddUniqueDynamic(this, &AAccretionGameMode::HandleBlowout);
	}
	if (UAccretionMassComponent* Mass = Pawn->FindComponentByClass<UAccretionMassComponent>())
	{
		Mass->OnStarved.AddUniqueDynamic(this, &AAccretionGameMode::HandleStarved);
	}
}

void AAccretionGameMode::HandleBlowout()
{
	EndRun(EAccretionRunEndReason::Blowout);
}

void AAccretionGameMode::HandleStarved()
{
	EndRun(EAccretionRunEndReason::Starvation);
}

void AAccretionGameMode::EndRun(EAccretionRunEndReason Reason)
{
	if (HasRunEnded())
	{
		return;
	}
	RunEndReason = Reason;

	UE_LOG(LogAccretionRun, Log, TEXT("Run ended: %s"), *UEnum::GetValueAsString(Reason));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AAccretionBlackHolePawn* BlackHole = Cast<AAccretionBlackHolePawn>(It->Get() ? It->Get()->GetPawn() : nullptr))
		{
			BlackHole->FreezeForRunEnd();
		}
	}

	OnRunEnded.Broadcast(Reason);
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AAccretionGameMode::RestartRun, FMath::Max(RestartDelay, 0.01f), false);
}

void AAccretionGameMode::RestartRun()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}
