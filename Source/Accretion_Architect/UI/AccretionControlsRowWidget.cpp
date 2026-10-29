// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/AccretionControlsRowWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "AccretionControlsRow"

void UAccretionControlsRowWidget::InitRow(EAccretionInputAction InAction)
{
	Action = InAction;
}

TSharedRef<SWidget> UAccretionControlsRowWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		WidgetTree->RootWidget = Row;

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FAccretionKeyBindings::GetActionDisplayName(Action));
		Label->SetFontSize(18.f);
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);

		USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
		ButtonSize->SetWidthOverride(220.f);
		Row->AddChildToHorizontalBox(ButtonSize);

		UButton* KeyButton = WidgetTree->ConstructWidget<UButton>();
		KeyButton->OnClicked.AddDynamic(this, &UAccretionControlsRowWidget::HandleKeyButtonClicked);
		ButtonSize->AddChild(KeyButton);

		KeyText = WidgetTree->ConstructWidget<UTextBlock>();
		KeyText->SetFontSize(18.f);
		KeyText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		KeyText->SetJustification(ETextJustify::Center);
		KeyButton->AddChild(KeyText);
	}
	return Super::RebuildWidget();
}

void UAccretionControlsRowWidget::Refresh(const FKey& Key, bool bListening)
{
	if (KeyText)
	{
		KeyText->SetText(bListening ? LOCTEXT("PressKey", "Press a key...") : Key.GetDisplayName());
	}
}

void UAccretionControlsRowWidget::HandleKeyButtonClicked()
{
	OnRebindRequested.Broadcast(Action);
}

#undef LOCTEXT_NAMESPACE
