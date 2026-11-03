// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/AccretionGameMode.h"
#include "Core/AccretionMenuGameMode.h"
#include "Components/AccretionGravityComponent.h"
#include "Components/AccretionMassComponent.h"
#include "Components/DiskPressureComponent.h"
#include "Environment/AccretionGalaxy.h"
#include "Environment/AccretionSpaceEnvironment.h"
#include "Matter/MatterSpawner.h"
#include "Player/AccretionBlackHolePawn.h"
#include "Player/AccretionPlayerController.h"
#include "UI/AccretionHUD.h"
#include "Universe/AccretionUniverse.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogAccretionRun, Log, All);

namespace AccretionRun
{
	/** Camera pitch (degrees, negative looks down) for the victory shot, so the galaxy is seen from above. */
	constexpr float VictoryCameraPitch = -28.f;

	void OpenCurrentLevelWithGameMode(const UObject* WorldContext, const UClass* GameModeClass)
	{
		const FString Options = FString::Printf(TEXT("game=%s"), *GameModeClass->GetPathName());
		UGameplayStatics::OpenLevel(WorldContext, FName(*UGameplayStatics::GetCurrentLevelName(WorldContext, true)), true, Options);
	}
}

AAccretionGameMode::AAccretionGameMode()
{
	DefaultPawnClass = AAccretionBlackHolePawn::StaticClass();
	PlayerControllerClass = AAccretionPlayerController::StaticClass();
	HUDClass = AAccretionHUD::StaticClass();
	MatterSpawnerClass = AMatterSpawner::StaticClass();
	EnvironmentClass = AAccretionSpaceEnvironment::StaticClass();
	UniverseClass = AAccretionUniverse::StaticClass();
	GalaxyClass = AAccretionGalaxy::StaticClass();
}

void AAccretionGameMode::OpenRun(const UObject* WorldContext)
{
	AccretionRun::OpenCurrentLevelWithGameMode(WorldContext, AAccretionGameMode::StaticClass());
}

void AAccretionGameMode::OpenMainMenu(const UObject* WorldContext)
{
	AccretionRun::OpenCurrentLevelWithGameMode(WorldContext, AAccretionMenuGameMode::StaticClass());
}

void AAccretionGameMode::StartPlay()
{
	Super::StartPlay();
	RunStartTime = GetWorld()->GetTimeSeconds();

	if (EnvironmentClass && !TActorIterator<AActor>(GetWorld(), EnvironmentClass))
	{
		GetWorld()->SpawnActor<AActor>(EnvironmentClass);
	}

	if (UniverseClass)
	{
		Universe = TActorIterator<AAccretionUniverse>(GetWorld()) ? *TActorIterator<AAccretionUniverse>(GetWorld()) : GetWorld()->SpawnActor<AAccretionUniverse>(UniverseClass);
	}

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
		Mass->OnMassChanged.AddUniqueDynamic(this, &AAccretionGameMode::HandleMassChanged);
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

void AAccretionGameMode::HandleMassChanged(float NewMass)
{
	if (NewMass >= SupermassiveMass)
	{
		EndRun(EAccretionRunEndReason::Victory);
	}
}

void AAccretionGameMode::EndRun(EAccretionRunEndReason Reason)
{
	if (HasRunEnded())
	{
		return;
	}
	RunEndReason = Reason;
	RunEndTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogAccretionRun, Log, TEXT("Run ended: %s"), *UEnum::GetValueAsString(Reason));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AAccretionBlackHolePawn* BlackHole = Cast<AAccretionBlackHolePawn>(It->Get() ? It->Get()->GetPawn() : nullptr))
		{
			BlackHole->FreezeForRunEnd();
		}
	}

	OnRunEnded.Broadcast(Reason);

	if (Reason == EAccretionRunEndReason::Victory)
	{
		BeginVictorySequence();
	}
	else
	{
		GetWorldTimerManager().SetTimer(RestartTimer, this, &AAccretionGameMode::RestartRun, FMath::Max(RestartDelay, 0.01f), false);
	}
}

void AAccretionGameMode::BeginVictorySequence()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		AAccretionBlackHolePawn* BlackHole = Cast<AAccretionBlackHolePawn>(Controller ? Controller->GetPawn() : nullptr);
		if (!BlackHole)
		{
			continue;
		}

		// A whole galaxy settles into orbit around the new supermassive black hole.
		const float GalaxyRadius = BlackHole->GetGravityComponent()->GetEventHorizonRadius() * GalaxyRadiusPerEventHorizon;
		if (GalaxyClass)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AAccretionGalaxy* Galaxy = GetWorld()->SpawnActor<AAccretionGalaxy>(GalaxyClass, BlackHole->GetActorLocation(), FRotator::ZeroRotator, Params))
			{
				Galaxy->FormGalaxy(GalaxyRadius);
			}
		}

		// Look down on the galaxy from above while the camera pulls back to take it all in.
		FRotator View = Controller->GetControlRotation();
		View.Pitch = AccretionRun::VictoryCameraPitch;
		Controller->SetControlRotation(View);
		BlackHole->PlayVictoryCamera(GalaxyRadius * 1.6f);
	}

	GetWorldTimerManager().SetTimer(VictoryTimer, this, &AAccretionGameMode::ShowVictoryScreen, FMath::Max(VictoryScreenDelay, 0.01f), false);
}

void AAccretionGameMode::ShowVictoryScreen()
{
	const FAccretionRunSummary Summary = GetRunSummary();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AAccretionPlayerController* Controller = Cast<AAccretionPlayerController>(It->Get()))
		{
			Controller->ShowVictoryScreen(Summary);
		}
	}
}

FAccretionRunSummary AAccretionGameMode::GetRunSummary() const
{
	FAccretionRunSummary Summary;
	const float EndTime = RunEndTime >= 0.f ? RunEndTime : GetWorld()->GetTimeSeconds();
	Summary.RunSeconds = EndTime - RunStartTime;

	if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (const UAccretionMassComponent* Mass = Pawn->FindComponentByClass<UAccretionMassComponent>())
		{
			Summary.FinalMass = Mass->GetMass();
		}
	}
	if (Universe)
	{
		const FAccretionUniverseStats& Stats = Universe->GetStats();
		Summary.StarsConsumed = Stats.StarsConsumed;
		Summary.PlanetsConsumed = Stats.PlanetsConsumed;
		Summary.MoonsConsumed = Stats.MoonsConsumed;
		Summary.SystemsVisited = Stats.SystemsVisited;
	}
	return Summary;
}

void AAccretionGameMode::RestartRun()
{
	OpenRun(this);
}
