// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

/** Small builders for widgets constructed in code (menu and victory screens). */
namespace AccretionWidgets
{
	inline UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, float FontSize, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>();
		Block->SetText(Text);
		Block->SetFontSize(FontSize);
		Block->SetColorAndOpacity(FSlateColor(Color));
		return Block;
	}

	inline UButton* MakeButton(UWidgetTree* Tree, const FText& Text, float FontSize = 20.f)
	{
		UButton* Button = Tree->ConstructWidget<UButton>();
		UTextBlock* Label = MakeText(Tree, Text, FontSize, FLinearColor::Black);
		Label->SetJustification(ETextJustify::Center);
		Button->AddChild(Label);
		return Button;
	}
}
