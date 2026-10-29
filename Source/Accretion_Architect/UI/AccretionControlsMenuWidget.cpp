// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/AccretionControlsMenuWidget.h"
#include "UI/AccretionControlsRowWidget.h"
#include "Input/AccretionControlsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

#define LOCTEXT_NAMESPACE "AccretionControlsMenu"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, float FontSize, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>();
		Block->SetText(Text);
		Block->SetFontSize(FontSize);
		Block->SetColorAndOpacity(FSlateColor(Color));
		return Block;
	}

	UButton* MakeButton(UWidgetTree* Tree, const FText& Text)
	{
		UButton* Button = Tree->ConstructWidget<UButton>();
		UTextBlock* Label = MakeText(Tree, Text, 18.f, FLinearColor::Black);
		Label->SetJustification(ETextJustify::Center);
		Button->AddChild(Label);
		return Button;
	}

	UHorizontalBox* MakeLabeledRow(UWidgetTree* Tree, const FText& Label)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>();
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(MakeText(Tree, Label, 18.f));
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		return Row;
	}
}

UAccretionControlsSubsystem* UAccretionControlsMenuWidget::GetControlsSubsystem() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UAccretionControlsSubsystem>() : nullptr;
}

TSharedRef<SWidget> UAccretionControlsMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UAccretionControlsMenuWidget::BuildLayout()
{
	SetIsFocusable(true);

	// Full-screen dimmer with a centered panel.
	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>();
	Dimmer->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Dimmer->SetHorizontalAlignment(HAlign_Center);
	Dimmer->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dimmer;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(560.f);
	Dimmer->SetContent(PanelSize);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.08f, 0.95f));
	Panel->SetPadding(FMargin(32.f));
	PanelSize->AddChild(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);

	const FMargin RowPadding(0.f, 6.f);

	Column->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("Title", "Controls"), 28.f))->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));

	for (const EAccretionInputAction Action : TEnumRange<EAccretionInputAction>())
	{
		UAccretionControlsRowWidget* Row = WidgetTree->ConstructWidget<UAccretionControlsRowWidget>(UAccretionControlsRowWidget::StaticClass());
		Row->InitRow(Action);
		Row->OnRebindRequested.AddUObject(this, &UAccretionControlsMenuWidget::StartListening);
		Column->AddChildToVerticalBox(Row)->SetPadding(RowPadding);
		Rows.Add(Action, Row);
	}

	// Mouse sensitivity.
	UHorizontalBox* SensitivityRow = MakeLabeledRow(WidgetTree, LOCTEXT("Sensitivity", "Mouse Sensitivity"));
	USizeBox* SliderSize = WidgetTree->ConstructWidget<USizeBox>();
	SliderSize->SetWidthOverride(160.f);
	SensitivitySlider = WidgetTree->ConstructWidget<USlider>();
	SensitivitySlider->SetMinValue(UAccretionControlsSubsystem::MinMouseSensitivity);
	SensitivitySlider->SetMaxValue(UAccretionControlsSubsystem::MaxMouseSensitivity);
	SensitivitySlider->SetStepSize(0.05f);
	SensitivitySlider->OnValueChanged.AddDynamic(this, &UAccretionControlsMenuWidget::HandleSensitivityChanged);
	SliderSize->AddChild(SensitivitySlider);
	SensitivityRow->AddChildToHorizontalBox(SliderSize)->SetVerticalAlignment(VAlign_Center);
	USizeBox* ValueSize = WidgetTree->ConstructWidget<USizeBox>();
	ValueSize->SetWidthOverride(60.f);
	SensitivityValueText = MakeText(WidgetTree, FText::GetEmpty(), 18.f);
	SensitivityValueText->SetJustification(ETextJustify::Right);
	ValueSize->AddChild(SensitivityValueText);
	SensitivityRow->AddChildToHorizontalBox(ValueSize)->SetVerticalAlignment(VAlign_Center);
	Column->AddChildToVerticalBox(SensitivityRow)->SetPadding(FMargin(0.f, 16.f, 0.f, 6.f));

	// Invert mouse Y.
	UHorizontalBox* InvertRow = MakeLabeledRow(WidgetTree, LOCTEXT("InvertY", "Invert Mouse Y"));
	InvertCheckBox = WidgetTree->ConstructWidget<UCheckBox>();
	InvertCheckBox->OnCheckStateChanged.AddDynamic(this, &UAccretionControlsMenuWidget::HandleInvertChanged);
	InvertRow->AddChildToHorizontalBox(InvertCheckBox)->SetVerticalAlignment(VAlign_Center);
	Column->AddChildToVerticalBox(InvertRow)->SetPadding(RowPadding);

	StatusText = MakeText(WidgetTree, FText::GetEmpty(), 16.f, FLinearColor(1.f, 0.8f, 0.3f));
	Column->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	// Reset / Close.
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* ResetButton = MakeButton(WidgetTree, LOCTEXT("Reset", "Reset to Defaults"));
	ResetButton->OnClicked.AddDynamic(this, &UAccretionControlsMenuWidget::HandleResetClicked);
	UHorizontalBoxSlot* ResetSlot = ButtonRow->AddChildToHorizontalBox(ResetButton);
	ResetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ResetSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	UButton* CloseButton = MakeButton(WidgetTree, LOCTEXT("Close", "Close (Tab)"));
	CloseButton->OnClicked.AddDynamic(this, &UAccretionControlsMenuWidget::HandleCloseClicked);
	ButtonRow->AddChildToHorizontalBox(CloseButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Column->AddChildToVerticalBox(ButtonRow)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
}

void UAccretionControlsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UAccretionControlsSubsystem* Controls = GetControlsSubsystem())
	{
		Controls->OnControlsChanged.AddUniqueDynamic(this, &UAccretionControlsMenuWidget::HandleControlsChanged);
	}
	ListeningAction.Reset();
	StatusText->SetText(LOCTEXT("Hint", "Click a key to rebind it. Changes save automatically."));
	RefreshAll();
}

void UAccretionControlsMenuWidget::NativeDestruct()
{
	if (UAccretionControlsSubsystem* Controls = GetControlsSubsystem())
	{
		Controls->OnControlsChanged.RemoveDynamic(this, &UAccretionControlsMenuWidget::HandleControlsChanged);
	}
	Super::NativeDestruct();
}

void UAccretionControlsMenuWidget::RefreshAll()
{
	const UAccretionControlsSubsystem* Controls = GetControlsSubsystem();
	if (!Controls)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bRefreshing, true);

	for (const TPair<EAccretionInputAction, TObjectPtr<UAccretionControlsRowWidget>>& Pair : Rows)
	{
		const bool bListening = ListeningAction.IsSet() && ListeningAction.GetValue() == Pair.Key;
		Pair.Value->Refresh(Controls->GetKeyForAction(Pair.Key), bListening);
	}

	const float Sensitivity = Controls->GetMouseSensitivity();
	SensitivitySlider->SetValue(Sensitivity);
	FNumberFormattingOptions Format;
	Format.MinimumFractionalDigits = 2;
	Format.MaximumFractionalDigits = 2;
	SensitivityValueText->SetText(FText::AsNumber(Sensitivity, &Format));
	InvertCheckBox->SetIsChecked(Controls->GetInvertMouseY());
}

void UAccretionControlsMenuWidget::StartListening(EAccretionInputAction Action)
{
	ListeningAction = Action;
	StatusText->SetText(FText::Format(LOCTEXT("Listening", "Press a key or mouse button for {0}. Esc to cancel."),
		FAccretionKeyBindings::GetActionDisplayName(Action)));
	RefreshAll();
	SetKeyboardFocus();
}

void UAccretionControlsMenuWidget::StopListening()
{
	ListeningAction.Reset();
	RefreshAll();
}

void UAccretionControlsMenuWidget::TryBindKey(const FKey& Key)
{
	UAccretionControlsSubsystem* Controls = GetControlsSubsystem();
	if (!Controls || !ListeningAction.IsSet())
	{
		return;
	}

	const EAccretionInputAction Action = ListeningAction.GetValue();
	if (Controls->RebindAction(Action, Key))
	{
		StatusText->SetText(FText::Format(LOCTEXT("Bound", "{0} bound to {1}."),
			FAccretionKeyBindings::GetActionDisplayName(Action), Key.GetDisplayName()));
		StopListening();
	}
	else
	{
		StatusText->SetText(FText::Format(LOCTEXT("NotAllowed", "{0} can't be used. Press another key, or Esc to cancel."),
			Key.GetDisplayName()));
	}
}

FReply UAccretionControlsMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (ListeningAction.IsSet())
	{
		if (Key == FAccretionKeyBindings::GetCancelKey())
		{
			StatusText->SetText(LOCTEXT("Cancelled", "Rebind cancelled."));
			StopListening();
		}
		else
		{
			TryBindKey(Key);
		}
		return FReply::Handled();
	}

	if (Key == FAccretionKeyBindings::GetControlsMenuKey() || Key == FAccretionKeyBindings::GetCancelKey())
	{
		OnCloseRequested.Broadcast();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UAccretionControlsMenuWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ListeningAction.IsSet())
	{
		TryBindKey(InMouseEvent.GetEffectingButton());
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UAccretionControlsMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Clicking empty space keeps focus in the menu so Tab/Esc still work.
	return FReply::Handled().SetUserFocus(TakeWidget());
}

void UAccretionControlsMenuWidget::HandleSensitivityChanged(float Value)
{
	if (bRefreshing)
	{
		return;
	}
	if (UAccretionControlsSubsystem* Controls = GetControlsSubsystem())
	{
		Controls->SetMouseSensitivity(Value);
	}
}

void UAccretionControlsMenuWidget::HandleInvertChanged(bool bIsChecked)
{
	if (bRefreshing)
	{
		return;
	}
	if (UAccretionControlsSubsystem* Controls = GetControlsSubsystem())
	{
		Controls->SetInvertMouseY(bIsChecked);
	}
}

void UAccretionControlsMenuWidget::HandleResetClicked()
{
	if (UAccretionControlsSubsystem* Controls = GetControlsSubsystem())
	{
		Controls->ResetToDefaults();
		StatusText->SetText(LOCTEXT("ResetDone", "Controls reset to defaults."));
	}
}

void UAccretionControlsMenuWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

void UAccretionControlsMenuWidget::HandleControlsChanged()
{
	RefreshAll();
}

#undef LOCTEXT_NAMESPACE
