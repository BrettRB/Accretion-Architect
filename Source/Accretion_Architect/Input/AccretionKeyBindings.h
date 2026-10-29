// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "AccretionKeyBindings.generated.h"

/** Player actions that can be rebound in the controls menu. */
UENUM(BlueprintType)
enum class EAccretionInputAction : uint8
{
	MoveForward,
	MoveBackward,
	MoveLeft,
	MoveRight,
	MoveUp,
	MoveDown,

	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EAccretionInputAction, EAccretionInputAction::Count);

/**
 * Default key bindings and the rules for rebinding them. Pure logic, no engine state,
 * so it can be unit tested and reused by any controls UI.
 */
struct ACCRETION_ARCHITECT_API FAccretionKeyBindings
{
	/** Key that opens/closes the controls menu. Reserved so players cannot lock themselves out. */
	static FKey GetControlsMenuKey() { return EKeys::Tab; }

	/** Key that cancels a rebind in progress. Reserved. */
	static FKey GetCancelKey() { return EKeys::Escape; }

	static TMap<EAccretionInputAction, FKey> GetDefaultBindings();

	static FText GetActionDisplayName(EAccretionInputAction Action);

	/** True for keyboard keys and mouse buttons that are not reserved and not analog axes. */
	static bool IsKeyAllowed(const FKey& Key);

	/**
	 * Binds Action to NewKey. If another action already uses NewKey, the two actions swap keys
	 * so every action always keeps exactly one key. Returns false (and changes nothing) if the key is not allowed.
	 */
	static bool Rebind(TMap<EAccretionInputAction, FKey>& Bindings, EAccretionInputAction Action, const FKey& NewKey);

	/** Fills in any missing or disallowed entries with defaults (e.g. after loading an older save). */
	static void Sanitize(TMap<EAccretionInputAction, FKey>& Bindings);
};
