// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/AccretionVictoryWidget.h"
#include "UI/AccretionWidgetHelpers.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "AccretionVictory"

TSharedRef<SWidget> UAccretionVictoryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UAccretionVictoryWidget::BuildLayout()
{
	using namespace AccretionWidgets;
	SetIsFocusable(true);

	// Light dimmer so the galaxy stays visible behind the panel.
	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>();
	Dimmer->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Dimmer->SetHorizontalAlignment(HAlign_Center);
	Dimmer->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dimmer;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(560.f);
	Dimmer->SetContent(PanelSize);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.06f, 0.85f));
	Panel->SetPadding(FMargin(40.f));
	PanelSize->AddChild(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);

	UTextBlock* Title = MakeText(WidgetTree, LOCTEXT("Title", "SUPERMASSIVE"), 44.f, FLinearColor(1.f, 0.78f, 0.4f));
	Title->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Title);

	UTextBlock* Subtitle = MakeText(WidgetTree,
		LOCTEXT("Subtitle", "You rival Sagittarius A*. A whole galaxy now orbits you."), 18.f, FLinearColor(0.8f, 0.85f, 0.95f));
	Subtitle->SetJustification(ETextJustify::Center);
	Subtitle->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(0.f, 8.f, 0.f, 28.f));

	StatsText = MakeText(WidgetTree, FText::GetEmpty(), 18.f);
	Column->AddChildToVerticalBox(StatsText)->SetPadding(FMargin(0.f, 0.f, 0.f, 32.f));

	UButton* ReturnButton = MakeButton(WidgetTree, LOCTEXT("Return", "Return to Menu"), 22.f);
	ReturnButton->OnClicked.AddDynamic(this, &UAccretionVictoryWidget::HandleReturnClicked);
	Column->AddChildToVerticalBox(ReturnButton);

	RefreshStats();
}

void UAccretionVictoryWidget::SetSummary(const FAccretionRunSummary& InSummary)
{
	Summary = InSummary;
	RefreshStats();
}

void UAccretionVictoryWidget::RefreshStats()
{
	if (!StatsText)
	{
		return;
	}

	const int32 TotalSeconds = FMath::FloorToInt32(Summary.RunSeconds);
	FNumberFormattingOptions WholeNumbers;
	WholeNumbers.MaximumFractionalDigits = 0;

	StatsText->SetText(FText::Format(
		LOCTEXT("Stats", "Final mass:  {0}\nRun time:  {1}:{2}\nStars consumed:  {3}\nPlanets consumed:  {4}\nMoons consumed:  {5}\nSystems visited:  {6}"),
		FText::AsNumber(Summary.FinalMass, &WholeNumbers),
		FText::AsNumber(TotalSeconds / 60),
		FText::FromString(FString::Printf(TEXT("%02d"), TotalSeconds % 60)),
		FText::AsNumber(Summary.StarsConsumed),
		FText::AsNumber(Summary.PlanetsConsumed),
		FText::AsNumber(Summary.MoonsConsumed),
		FText::AsNumber(Summary.SystemsVisited)));
}

void UAccretionVictoryWidget::HandleReturnClicked()
{
	AAccretionGameMode::OpenMainMenu(this);
}

#undef LOCTEXT_NAMESPACE
