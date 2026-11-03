// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/AccretionHUD.h"
#include "Components/AccretionBoostComponent.h"
#include "Components/AccretionGravityComponent.h"
#include "Core/AccretionMath.h"
#include "Universe/AccretionUniverse.h"
#include "Universe/CelestialBody.h"
#include "EngineUtils.h"
#include "Components/AccretionMassComponent.h"
#include "Engine/LocalPlayer.h"
#include "Input/AccretionControlsSubsystem.h"
#include "Components/DiskPressureComponent.h"
#include "Core/AccretionGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace AccretionHUD
{
	static constexpr float MeterWidth = 300.f;
	static constexpr float MeterHeight = 18.f;
	static constexpr float Margin = 24.f;
}

void AAccretionHUD::DrawMeter(float X, float Y, float Fill, const FLinearColor& Color, const FString& Label, float MarkerFill)
{
	using namespace AccretionHUD;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y, MeterWidth, MeterHeight);
	DrawRect(Color, X, Y, MeterWidth * FMath::Clamp(Fill, 0.f, 1.f), MeterHeight);
	if (MarkerFill >= 0.f)
	{
		DrawRect(FLinearColor::White, X + MeterWidth * FMath::Clamp(MarkerFill, 0.f, 1.f) - 1.f, Y - 3.f, 2.f, MeterHeight + 6.f);
	}
	DrawText(Label, FLinearColor::White, X, Y - 20.f, GEngine->GetSmallFont());
}

void AAccretionHUD::DrawNearestBody(const APawn* Pawn, float X, float Y)
{
	const UAccretionGravityComponent* Gravity = Pawn->FindComponentByClass<UAccretionGravityComponent>();
	if (!Gravity)
	{
		return;
	}
	if (!Universe.IsValid())
	{
		TActorIterator<AAccretionUniverse> It(GetWorld());
		Universe = It ? *It : nullptr;
	}
	if (!Universe.IsValid())
	{
		return;
	}

	const float PullRadius = Gravity->GetPullRadius();
	const ACelestialBody* Body = Universe->FindNearestBody(Pawn->GetActorLocation(), PullRadius * NearestBodyRangeInPullRadii);
	if (!Body)
	{
		return;
	}

	const FCelestialClassDefinition* Definition = Universe->GetCatalog().Find(Body->GetBodyType());
	const FString TypeName = Definition ? Definition->DisplayName.ToString() : FString();
	auto Number = [](float Value) { return FText::AsNumber(FMath::RoundToInt32(Value)).ToString(); };

	DrawText(FString::Printf(TEXT("%s  -  %s  -  mass %s"), *Body->GetBodyName().ToUpper(), *TypeName, *Number(Body->GetMass())),
		FLinearColor::White, X, Y, GEngine->GetSmallFont());

	const bool bInRange = FVector::Dist(Pawn->GetActorLocation(), Body->GetActorLocation()) - Body->GetRadius() <= PullRadius;
	FString Status;
	FLinearColor Color;
	switch (Gravity->GetConsumeModeFor(Body))
	{
	case ECelestialConsumeMode::SwallowWhole:
		Status = bInRange ? TEXT("Swallowing whole") : TEXT("Can swallow whole - get closer");
		Color = FLinearColor(0.4f, 1.f, 0.5f);
		break;
	case ECelestialConsumeMode::Shred:
		Status = FString::Printf(TEXT("%s   (swallow whole at mass %s)"),
			bInRange ? TEXT("Tidal shredding - watch disk pressure") : TEXT("Can shred - get closer"),
			*Number(Gravity->GetMassRequiredFor(Body, ECelestialConsumeMode::SwallowWhole)));
		Color = FLinearColor(1.f, 0.8f, 0.3f);
		break;
	default:
		Status = FString::Printf(TEXT("Too massive - grow to %s to shred"), *Number(Gravity->GetMassRequiredFor(Body, ECelestialConsumeMode::Shred)));
		Color = FLinearColor(1.f, 0.45f, 0.35f);
		break;
	}
	DrawText(Status, Color, X, Y + 18.f, GEngine->GetSmallFont());
}

void AAccretionHUD::DrawHUD()
{
	Super::DrawHUD();
	using namespace AccretionHUD;

	const APawn* Pawn = GetOwningPawn();
	if (!Pawn || !Canvas)
	{
		return;
	}

	float Y = Margin + 20.f;

	const AAccretionGameMode* GameMode = GetWorld()->GetAuthGameMode<AAccretionGameMode>();
	if (GameMode && GameMode->GetRunEndReason() == EAccretionRunEndReason::Victory)
	{
		return; // the galaxy and victory screen take over
	}

	if (const UAccretionMassComponent* Mass = Pawn->FindComponentByClass<UAccretionMassComponent>())
	{
		// Log scale: every doubling of mass moves the bar the same amount, all the way to supermassive.
		const float TargetMass = GameMode ? GameMode->SupermassiveMass : 1.f;
		const float Progress = static_cast<float>(FAccretionMath::ComputeLogProgress(Mass->GetMass(), Mass->StartingMass, TargetMass));
		const FString Label = FString::Printf(TEXT("MASS  %s   %.0f%% to supermassive   (starve below %.0f)"),
			*FText::AsNumber(FMath::RoundToInt32(Mass->GetMass())).ToString(), Progress * 100.f, Mass->StarvationThreshold);
		DrawMeter(Margin, Y, Progress, FLinearColor(0.3f, 0.6f, 1.f), Label);
		Y += MeterHeight + 32.f;
	}

	if (const UDiskPressureComponent* Pressure = Pawn->FindComponentByClass<UDiskPressureComponent>())
	{
		const float Ratio = Pressure->GetPressureRatio();
		const FLinearColor Color = FLinearColor::LerpUsingHSV(FLinearColor(1.f, 0.85f, 0.2f), FLinearColor(1.f, 0.1f, 0.05f), Ratio);
		const FString Label = FString::Printf(TEXT("DISK PRESSURE  %.0f%%"), Ratio * 100.f);
		DrawMeter(Margin, Y, Ratio, Color, Label);
		Y += MeterHeight + 32.f;
	}

	const UAccretionBoostComponent* Boost = Pawn->FindComponentByClass<UAccretionBoostComponent>();
	const UAccretionMassComponent* BoostMass = Pawn->FindComponentByClass<UAccretionMassComponent>();
	if (Boost && BoostMass)
	{
		const ULocalPlayer* LocalPlayer = PlayerOwner ? PlayerOwner->GetLocalPlayer() : nullptr;
		const UAccretionControlsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UAccretionControlsSubsystem>() : nullptr;
		const FString KeyName = Controls ? Controls->GetKeyForAction(EAccretionInputAction::Boost).GetDisplayName().ToString() : TEXT("Space");

		FString Status;
		FLinearColor StatusColor = FLinearColor(0.5f, 0.9f, 1.f);
		if (Boost->IsCoolingDown())
		{
			Status = FString::Printf(TEXT("recharging %.1fs"), Boost->GetCooldownRemaining());
			StatusColor = FLinearColor(1.f, 1.f, 1.f, 0.5f);
		}
		else if (!Boost->CanAfford(BoostMass))
		{
			Status = TEXT("not enough mass");
			StatusColor = FLinearColor(1.f, 0.4f, 0.3f);
		}
		else
		{
			Status = TEXT("ready");
		}
		DrawText(FString::Printf(TEXT("BOOST [%s]  %s   (costs %s mass)"), *KeyName, *Status,
			*FText::AsNumber(FMath::RoundToInt32(Boost->GetMassCost(BoostMass->GetMass()))).ToString()),
			StatusColor, Margin, Y - 20.f, GEngine->GetSmallFont());
		Y += 28.f;
	}

	DrawNearestBody(Pawn, Margin, Y);

	DrawText(TEXT("Tab: Controls"), FLinearColor(1.f, 1.f, 1.f, 0.6f), Margin, Canvas->ClipY - Margin - 16.f, GEngine->GetSmallFont());

	// Victory has its own screen; losses get a centered message.
	if (GameMode && GameMode->HasRunEnded() && GameMode->GetRunEndReason() != EAccretionRunEndReason::Victory)
	{
		const bool bBlowout = GameMode->GetRunEndReason() == EAccretionRunEndReason::Blowout;
		const FString Message = bBlowout ? TEXT("DISK BLOWOUT") : TEXT("STARVED - EVAPORATED");
		UFont* Font = GEngine->GetLargeFont();
		float TextWidth = 0.f;
		float TextHeight = 0.f;
		GetTextSize(Message, TextWidth, TextHeight, Font, 2.f);
		DrawText(Message, bBlowout ? FLinearColor(1.f, 0.3f, 0.1f) : FLinearColor(0.6f, 0.7f, 1.f),
			(Canvas->ClipX - TextWidth) * 0.5f, (Canvas->ClipY - TextHeight) * 0.5f, Font, 2.f);
	}
}
