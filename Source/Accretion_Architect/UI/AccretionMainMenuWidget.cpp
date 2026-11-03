// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/AccretionMainMenuWidget.h"
#include "UI/AccretionWidgetHelpers.h"
#include "Core/AccretionGameMode.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "AccretionMainMenu"

TSharedRef<SWidget> UAccretionMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UAccretionMainMenuWidget::BuildLayout()
{
	using namespace AccretionWidgets;
	SetIsFocusable(true);

	// Transparent full-screen layer so the starfield shows; content sits on the left.
	UBorder* Layer = WidgetTree->ConstructWidget<UBorder>();
	Layer->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.25f));
	Layer->SetHorizontalAlignment(HAlign_Left);
	Layer->SetVerticalAlignment(VAlign_Center);
	Layer->SetPadding(FMargin(96.f, 0.f));
	WidgetTree->RootWidget = Layer;

	USizeBox* ColumnSize = WidgetTree->ConstructWidget<USizeBox>();
	ColumnSize->SetWidthOverride(460.f);
	Layer->SetContent(ColumnSize);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	ColumnSize->AddChild(Column);

	Column->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("Title", "ACCRETION ARCHITECT"), 40.f));
	Column->AddChildToVerticalBox(MakeText(WidgetTree,
		LOCTEXT("Tagline", "Grow from a stellar remnant into a supermassive black hole."), 16.f, FLinearColor(0.75f, 0.8f, 0.9f)))
		->SetPadding(FMargin(0.f, 4.f, 0.f, 40.f));

	// Starting black hole: a single type for now. Meta-progression will add more choices here.
	Column->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("StartingHeader", "STARTING BLACK HOLE"), 14.f, FLinearColor(1.f, 0.7f, 0.35f)));
	Column->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("StellarRemnant", "Stellar Remnant"), 22.f))
		->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	Column->AddChildToVerticalBox(MakeText(WidgetTree,
		LOCTEXT("StellarRemnantDesc", "Born in the Proxima Centauri system. Balanced gravity, disk, and decay."), 14.f, FLinearColor(0.7f, 0.7f, 0.75f)))
		->SetPadding(FMargin(0.f, 2.f, 0.f, 40.f));

	UButton* StartButton = MakeButton(WidgetTree, LOCTEXT("Start", "Start Run"), 22.f);
	StartButton->OnClicked.AddDynamic(this, &UAccretionMainMenuWidget::HandleStartClicked);
	Column->AddChildToVerticalBox(StartButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	UButton* QuitButton = MakeButton(WidgetTree, LOCTEXT("Quit", "Quit"), 18.f);
	QuitButton->OnClicked.AddDynamic(this, &UAccretionMainMenuWidget::HandleQuitClicked);
	Column->AddChildToVerticalBox(QuitButton);
}

void UAccretionMainMenuWidget::HandleStartClicked()
{
	AAccretionGameMode::OpenRun(this);
}

void UAccretionMainMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
