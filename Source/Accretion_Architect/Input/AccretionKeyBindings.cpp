// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/AccretionKeyBindings.h"

#define LOCTEXT_NAMESPACE "AccretionKeyBindings"

TMap<EAccretionInputAction, FKey> FAccretionKeyBindings::GetDefaultBindings()
{
	return {
		{ EAccretionInputAction::MoveForward, EKeys::W },
		{ EAccretionInputAction::MoveBackward, EKeys::S },
		{ EAccretionInputAction::MoveLeft, EKeys::A },
		{ EAccretionInputAction::MoveRight, EKeys::D },
		{ EAccretionInputAction::MoveUp, EKeys::SpaceBar },
		{ EAccretionInputAction::MoveDown, EKeys::LeftControl },
	};
}

FText FAccretionKeyBindings::GetActionDisplayName(EAccretionInputAction Action)
{
	switch (Action)
	{
	case EAccretionInputAction::MoveForward:  return LOCTEXT("MoveForward", "Move Forward");
	case EAccretionInputAction::MoveBackward: return LOCTEXT("MoveBackward", "Move Backward");
	case EAccretionInputAction::MoveLeft:     return LOCTEXT("MoveLeft", "Move Left");
	case EAccretionInputAction::MoveRight:    return LOCTEXT("MoveRight", "Move Right");
	case EAccretionInputAction::MoveUp:       return LOCTEXT("MoveUp", "Move Up");
	case EAccretionInputAction::MoveDown:     return LOCTEXT("MoveDown", "Move Down");
	default:                                  return FText::GetEmpty();
	}
}

bool FAccretionKeyBindings::IsKeyAllowed(const FKey& Key)
{
	if (!Key.IsValid() || Key == GetControlsMenuKey() || Key == GetCancelKey())
	{
		return false;
	}
	if (Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D() || Key.IsGamepadKey() || Key.IsTouch())
	{
		return false;
	}
	return true;
}

bool FAccretionKeyBindings::Rebind(TMap<EAccretionInputAction, FKey>& Bindings, EAccretionInputAction Action, const FKey& NewKey)
{
	if (!IsKeyAllowed(NewKey))
	{
		return false;
	}

	const FKey OldKey = Bindings.FindRef(Action);
	for (TPair<EAccretionInputAction, FKey>& Pair : Bindings)
	{
		if (Pair.Key != Action && Pair.Value == NewKey)
		{
			Pair.Value = OldKey;
		}
	}
	Bindings.Add(Action, NewKey);
	return true;
}

void FAccretionKeyBindings::Sanitize(TMap<EAccretionInputAction, FKey>& Bindings)
{
	const TMap<EAccretionInputAction, FKey> Defaults = GetDefaultBindings();
	TSet<FKey> UsedKeys;

	for (const EAccretionInputAction Action : TEnumRange<EAccretionInputAction>())
	{
		const FKey* Existing = Bindings.Find(Action);
		if (!Existing || !IsKeyAllowed(*Existing) || UsedKeys.Contains(*Existing))
		{
			// Any problem with the saved bindings resets everything, so we never end up with duplicate keys.
			Bindings = Defaults;
			return;
		}
		UsedKeys.Add(*Existing);
	}
}

#undef LOCTEXT_NAMESPACE
