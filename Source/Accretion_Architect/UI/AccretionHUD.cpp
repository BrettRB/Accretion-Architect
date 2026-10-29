// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/AccretionHUD.h"
#include "Components/AccretionMassComponent.h"
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

	if (const UAccretionMassComponent* Mass = Pawn->FindComponentByClass<UAccretionMassComponent>())
	{
		const FString Label = FString::Printf(TEXT("MASS  %.1f   (starve below %.0f)"), Mass->GetMass(), Mass->StarvationThreshold);
		DrawMeter(Margin, Y, Mass->GetMass() / MassMeterMax, FLinearColor(0.3f, 0.6f, 1.f), Label, Mass->StarvationThreshold / MassMeterMax);
		Y += MeterHeight + 32.f;
	}

	if (const UDiskPressureComponent* Pressure = Pawn->FindComponentByClass<UDiskPressureComponent>())
	{
		const float Ratio = Pressure->GetPressureRatio();
		const FLinearColor Color = FLinearColor::LerpUsingHSV(FLinearColor(1.f, 0.85f, 0.2f), FLinearColor(1.f, 0.1f, 0.05f), Ratio);
		const FString Label = FString::Printf(TEXT("DISK PRESSURE  %.0f%%"), Ratio * 100.f);
		DrawMeter(Margin, Y, Ratio, Color, Label);
	}

	DrawText(TEXT("Tab: Controls"), FLinearColor(1.f, 1.f, 1.f, 0.6f), Margin, Canvas->ClipY - Margin - 16.f, GEngine->GetSmallFont());

	const AAccretionGameMode* GameMode = GetWorld()->GetAuthGameMode<AAccretionGameMode>();
	if (GameMode && GameMode->HasRunEnded())
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
