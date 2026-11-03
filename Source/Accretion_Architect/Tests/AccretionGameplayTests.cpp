// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Components/AccretionBoostComponent.h"
#include "Components/AccretionDiskComponent.h"
#include "Components/AccretionMassComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/DiskPressureComponent.h"
#include "Core/AccretionMath.h"
#include "Input/AccretionKeyBindings.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace AccretionTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

// ---------------------------------------------------------------------------
// Gravity math
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionGravityFalloffTest, "AccretionArchitect.Gravity.FalloffAndRadius", AccretionTests::Flags)
bool FAccretionGravityFalloffTest::RunTest(const FString& Parameters)
{
	const double G = 1000.0, Mass = 100.0, PullRadius = 5000.0, MinDistance = 10.0, MaxAccel = 1e9;

	const FVector Near = FAccretionMath::ComputeGravityAcceleration(FVector(1000, 0, 0), Mass, G, PullRadius, MinDistance, MaxAccel);
	const FVector Far = FAccretionMath::ComputeGravityAcceleration(FVector(2000, 0, 0), Mass, G, PullRadius, MinDistance, MaxAccel);
	const FVector Outside = FAccretionMath::ComputeGravityAcceleration(FVector(6000, 0, 0), Mass, G, PullRadius, MinDistance, MaxAccel);

	TestTrue(TEXT("Gravity points toward the center"), Near.X < 0.0);
	TestEqual(TEXT("Inverse-square: doubling distance quarters the pull"), Near.Size() / Far.Size(), 4.0, 1e-6);
	TestTrue(TEXT("No pull outside the pull radius"), Outside.IsZero());

	const FVector Clamped = FAccretionMath::ComputeGravityAcceleration(FVector(1, 0, 0), Mass, G, PullRadius, MinDistance, 50.0);
	TestEqual(TEXT("Acceleration is clamped to the maximum"), Clamped.Size(), 50.0, 1e-6);

	const FVector Heavier = FAccretionMath::ComputeGravityAcceleration(FVector(1000, 0, 0), Mass * 2.0, G, PullRadius, MinDistance, MaxAccel);
	TestEqual(TEXT("More mass pulls harder"), Heavier.Size() / Near.Size(), 2.0, 1e-6);

	TestTrue(TEXT("Pull radius grows with mass"),
		FAccretionMath::ComputePullRadius(400.0, 1500.0, 150.0) > FAccretionMath::ComputePullRadius(100.0, 1500.0, 150.0));
	TestTrue(TEXT("Event horizon grows with mass"),
		FAccretionMath::ComputeEventHorizonRadius(400.0, 150.0, 15.0) > FAccretionMath::ComputeEventHorizonRadius(100.0, 150.0, 15.0));
	return true;
}

// ---------------------------------------------------------------------------
// Feeding / impact angle
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionSmoothVsHeadOnTest, "AccretionArchitect.Pressure.SmoothVsHeadOn", AccretionTests::Flags)
bool FAccretionSmoothVsHeadOnTest::RunTest(const FString& Parameters)
{
	const FVector Offset(1000, 0, 0); // matter sits on +X, center is at the origin

	TestEqual(TEXT("Falling straight in is fully head-on"),
		FAccretionMath::ComputeRadialFraction(FVector(-500, 0, 0), Offset), 1.0, 1e-6);
	TestEqual(TEXT("Moving tangentially is fully smooth"),
		FAccretionMath::ComputeRadialFraction(FVector(0, 500, 0), Offset), 0.0, 1e-6);
	TestEqual(TEXT("Moving away counts as smooth"),
		FAccretionMath::ComputeRadialFraction(FVector(500, 0, 0), Offset), 0.0, 1e-6);

	const double Smooth = FAccretionMath::ComputeFeedPressure(3.0, 4.0, 0.5, 3.0, 0.0);
	const double HeadOn = FAccretionMath::ComputeFeedPressure(3.0, 4.0, 0.5, 3.0, 1.0);
	TestEqual(TEXT("Smooth feed pressure"), Smooth, 6.0, 1e-6);
	TestEqual(TEXT("Head-on feed pressure"), HeadOn, 36.0, 1e-6);
	TestTrue(TEXT("Smooth feeding adds less pressure than head-on"), Smooth < HeadOn);

	UDiskPressureComponent* Pressure = NewObject<UDiskPressureComponent>();
	const float SmoothAdded = Pressure->AddFeedPressure(3.f, 0.f);
	Pressure->ResetPressure();
	const float HeadOnAdded = Pressure->AddFeedPressure(3.f, 1.f);
	TestTrue(TEXT("Component applies the smooth/head-on multipliers"), SmoothAdded < HeadOnAdded);
	return true;
}

// ---------------------------------------------------------------------------
// Disk pressure component
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionPressureVentAndBlowoutTest, "AccretionArchitect.Pressure.VentAndBlowout", AccretionTests::Flags)
bool FAccretionPressureVentAndBlowoutTest::RunTest(const FString& Parameters)
{
	UDiskPressureComponent* Pressure = NewObject<UDiskPressureComponent>();
	Pressure->MaxPressure = 100.f;
	Pressure->VentRatePerSecond = 10.f;

	Pressure->AddPressure(30.f);
	Pressure->UpdatePressure(1.f);
	TestEqual(TEXT("Pressure vents over time"), Pressure->GetPressure(), 20.f, 1e-4f);

	Pressure->UpdatePressure(10.f);
	TestEqual(TEXT("Venting never goes below zero"), Pressure->GetPressure(), 0.f, 1e-4f);

	Pressure->AddPressure(99.f);
	TestFalse(TEXT("No blowout below max"), Pressure->IsBlownOut());

	Pressure->AddPressure(5.f);
	TestTrue(TEXT("Blowout at max pressure"), Pressure->IsBlownOut());
	TestEqual(TEXT("Pressure is capped at max"), Pressure->GetPressure(), 100.f, 1e-4f);

	Pressure->UpdatePressure(5.f);
	TestEqual(TEXT("No venting after blowout"), Pressure->GetPressure(), 100.f, 1e-4f);

	Pressure->ResetPressure();
	TestFalse(TEXT("Reset clears blowout"), Pressure->IsBlownOut());
	TestEqual(TEXT("Reset clears pressure"), Pressure->GetPressure(), 0.f, 1e-4f);
	return true;
}

// ---------------------------------------------------------------------------
// Mass component
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionMassDecayAndStarvationTest, "AccretionArchitect.Mass.DecayAndStarvation", AccretionTests::Flags)
bool FAccretionMassDecayAndStarvationTest::RunTest(const FString& Parameters)
{
	UAccretionMassComponent* Mass = NewObject<UAccretionMassComponent>();
	Mass->StartingMass = 100.f;
	Mass->DecayRatePerSecond = 0.1f;
	Mass->StarvationThreshold = 20.f;
	Mass->ResetMass();

	Mass->UpdateMass(1.f);
	TestEqual(TEXT("Mass decays by the decay rate"), Mass->GetMass(), 90.f, 1e-3f);

	Mass->AddMass(10.f);
	TestEqual(TEXT("Feeding adds mass"), Mass->GetMass(), 100.f, 1e-3f);

	Mass->AddMass(-50.f);
	TestEqual(TEXT("Negative mass gain is ignored"), Mass->GetMass(), 100.f, 1e-3f);

	for (int32 Step = 0; Step < 200 && !Mass->IsStarved(); ++Step)
	{
		Mass->UpdateMass(0.5f);
	}
	TestTrue(TEXT("Starves once mass decays below the threshold"), Mass->IsStarved());
	TestTrue(TEXT("Starved mass is below the threshold"), Mass->GetMass() < 20.f);

	const float StarvedMass = Mass->GetMass();
	Mass->AddMass(100.f);
	TestEqual(TEXT("Cannot feed after starving"), Mass->GetMass(), StarvedMass, 1e-3f);

	Mass->ResetMass();
	TestFalse(TEXT("Reset clears starvation"), Mass->IsStarved());
	TestEqual(TEXT("Reset restores starting mass"), Mass->GetMass(), 100.f, 1e-3f);
	return true;
}

// ---------------------------------------------------------------------------
// Customizable controls
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionKeyRebindTest, "AccretionArchitect.Controls.Rebind", AccretionTests::Flags)
bool FAccretionKeyRebindTest::RunTest(const FString& Parameters)
{
	TMap<EAccretionInputAction, FKey> Bindings = FAccretionKeyBindings::GetDefaultBindings();
	TestEqual(TEXT("Every action has a default key"), Bindings.Num(), static_cast<int32>(EAccretionInputAction::Count));
	TestEqual(TEXT("Default forward is W"), Bindings[EAccretionInputAction::MoveForward], EKeys::W);

	TestTrue(TEXT("Rebind to a free key"), FAccretionKeyBindings::Rebind(Bindings, EAccretionInputAction::MoveForward, EKeys::Up));
	TestEqual(TEXT("Forward is now Up"), Bindings[EAccretionInputAction::MoveForward], EKeys::Up);

	TestTrue(TEXT("Rebind to a key another action uses"), FAccretionKeyBindings::Rebind(Bindings, EAccretionInputAction::MoveForward, EKeys::S));
	TestEqual(TEXT("Forward takes S"), Bindings[EAccretionInputAction::MoveForward], EKeys::S);
	TestEqual(TEXT("Backward swaps to forward's old key"), Bindings[EAccretionInputAction::MoveBackward], EKeys::Up);

	TestTrue(TEXT("Mouse buttons are allowed"), FAccretionKeyBindings::Rebind(Bindings, EAccretionInputAction::MoveUp, EKeys::RightMouseButton));

	const TMap<EAccretionInputAction, FKey> Before = Bindings;
	TestFalse(TEXT("Menu key is reserved"), FAccretionKeyBindings::Rebind(Bindings, EAccretionInputAction::MoveLeft, FAccretionKeyBindings::GetControlsMenuKey()));
	TestFalse(TEXT("Cancel key is reserved"), FAccretionKeyBindings::Rebind(Bindings, EAccretionInputAction::MoveLeft, FAccretionKeyBindings::GetCancelKey()));
	TestFalse(TEXT("Mouse axes are not allowed"), FAccretionKeyBindings::Rebind(Bindings, EAccretionInputAction::MoveLeft, EKeys::MouseX));
	TestFalse(TEXT("Gamepad keys are not allowed"), FAccretionKeyBindings::Rebind(Bindings, EAccretionInputAction::MoveLeft, EKeys::Gamepad_FaceButton_Bottom));
	TestTrue(TEXT("Rejected rebinds change nothing"), Bindings.OrderIndependentCompareEqual(Before));

	TSet<FKey> UniqueKeys;
	for (const TPair<EAccretionInputAction, FKey>& Pair : Bindings)
	{
		UniqueKeys.Add(Pair.Value);
	}
	TestEqual(TEXT("No two actions share a key"), UniqueKeys.Num(), Bindings.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionKeySanitizeTest, "AccretionArchitect.Controls.Sanitize", AccretionTests::Flags)
bool FAccretionKeySanitizeTest::RunTest(const FString& Parameters)
{
	const TMap<EAccretionInputAction, FKey> Defaults = FAccretionKeyBindings::GetDefaultBindings();

	TMap<EAccretionInputAction, FKey> Valid = Defaults;
	FAccretionKeyBindings::Rebind(Valid, EAccretionInputAction::MoveForward, EKeys::Up);
	const TMap<EAccretionInputAction, FKey> ValidCopy = Valid;
	FAccretionKeyBindings::Sanitize(Valid);
	TestTrue(TEXT("Valid custom bindings are kept"), Valid.OrderIndependentCompareEqual(ValidCopy));

	TMap<EAccretionInputAction, FKey> Missing;
	Missing.Add(EAccretionInputAction::MoveForward, EKeys::Up);
	FAccretionKeyBindings::Sanitize(Missing);
	TestTrue(TEXT("Incomplete bindings reset to defaults"), Missing.OrderIndependentCompareEqual(Defaults));

	TMap<EAccretionInputAction, FKey> Duplicate = Defaults;
	Duplicate[EAccretionInputAction::MoveBackward] = EKeys::W;
	FAccretionKeyBindings::Sanitize(Duplicate);
	TestTrue(TEXT("Duplicate keys reset to defaults"), Duplicate.OrderIndependentCompareEqual(Defaults));

	TMap<EAccretionInputAction, FKey> Reserved = Defaults;
	Reserved[EAccretionInputAction::MoveLeft] = FAccretionKeyBindings::GetControlsMenuKey();
	FAccretionKeyBindings::Sanitize(Reserved);
	TestTrue(TEXT("Reserved keys reset to defaults"), Reserved.OrderIndependentCompareEqual(Defaults));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionBoostDefaultKeyTest, "AccretionArchitect.Controls.BoostDefaultKey", AccretionTests::Flags)
bool FAccretionBoostDefaultKeyTest::RunTest(const FString& Parameters)
{
	const TMap<EAccretionInputAction, FKey> Defaults = FAccretionKeyBindings::GetDefaultBindings();
	TestEqual(TEXT("Boost defaults to the space bar"), Defaults[EAccretionInputAction::Boost], EKeys::SpaceBar);
	TestNotEqual(TEXT("Move Up no longer uses the space bar"), Defaults[EAccretionInputAction::MoveUp], EKeys::SpaceBar);

	// A save from before Boost existed has no Boost entry and Move Up on Space: it must reset cleanly.
	TMap<EAccretionInputAction, FKey> OldSave = Defaults;
	OldSave.Remove(EAccretionInputAction::Boost);
	OldSave[EAccretionInputAction::MoveUp] = EKeys::SpaceBar;
	FAccretionKeyBindings::Sanitize(OldSave);
	TestTrue(TEXT("Old saves without Boost reset to defaults"), OldSave.OrderIndependentCompareEqual(Defaults));
	return true;
}

// ---------------------------------------------------------------------------
// Boost
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionSpendMassTest, "AccretionArchitect.Mass.SpendMass", AccretionTests::Flags)
bool FAccretionSpendMassTest::RunTest(const FString& Parameters)
{
	UAccretionMassComponent* Mass = NewObject<UAccretionMassComponent>();
	Mass->StartingMass = 30.f;
	Mass->StarvationThreshold = 20.f;
	Mass->ResetMass();

	TestTrue(TEXT("Can spend mass above the threshold"), Mass->TrySpendMass(5.f));
	TestEqual(TEXT("Spending reduces mass"), Mass->GetMass(), 25.f, 1e-3f);
	TestTrue(TEXT("Can spend down to exactly the threshold"), Mass->TrySpendMass(5.f));
	TestFalse(TEXT("Cannot spend below the threshold"), Mass->TrySpendMass(0.5f));
	TestEqual(TEXT("Refused spend changes nothing"), Mass->GetMass(), 20.f, 1e-3f);
	TestFalse(TEXT("Negative spend is refused"), Mass->TrySpendMass(-5.f));
	TestFalse(TEXT("Spending never starves the black hole"), Mass->IsStarved());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionBoostTest, "AccretionArchitect.Boost.CostAndCooldown", AccretionTests::Flags)
bool FAccretionBoostTest::RunTest(const FString& Parameters)
{
	UAccretionMassComponent* Mass = NewObject<UAccretionMassComponent>();
	Mass->StartingMass = 100.f;
	Mass->StarvationThreshold = 20.f;
	Mass->ResetMass();

	UFloatingPawnMovement* Movement = NewObject<UFloatingPawnMovement>();
	UAccretionBoostComponent* Boost = NewObject<UAccretionBoostComponent>();
	Boost->MassCost = 10.f;
	Boost->BoostSpeed = 3000.f;
	Boost->Cooldown = 2.f;

	TestTrue(TEXT("Boost succeeds when ready"), Boost->TryBoostUsing(Mass, Movement, FVector(0, 2, 0)));
	TestEqual(TEXT("Boost spends mass"), Mass->GetMass(), 90.f, 1e-3f);
	TestTrue(TEXT("Boost adds speed in the (normalized) direction"), Movement->Velocity.Equals(FVector(0, 3000, 0), 1e-3));

	TestTrue(TEXT("Cooldown starts after boosting"), Boost->IsCoolingDown());
	TestFalse(TEXT("Cannot boost while cooling down"), Boost->TryBoostUsing(Mass, Movement, FVector(1, 0, 0)));
	TestEqual(TEXT("Refused boost costs nothing"), Mass->GetMass(), 90.f, 1e-3f);

	Boost->UpdateCooldown(1.f);
	TestTrue(TEXT("Still cooling down partway through"), Boost->IsCoolingDown());
	Boost->UpdateCooldown(1.f);
	TestFalse(TEXT("Cooldown finishes"), Boost->IsCoolingDown());

	TestFalse(TEXT("Zero direction is refused"), Boost->TryBoostUsing(Mass, Movement, FVector::ZeroVector));

	Mass->StartingMass = 25.f;
	Mass->ResetMass();
	TestFalse(TEXT("Cannot afford a boost that would starve"), Boost->CanAfford(Mass));
	TestFalse(TEXT("Boost refused when it would drop mass below the threshold"), Boost->TryBoostUsing(Mass, Movement, FVector(1, 0, 0)));
	TestEqual(TEXT("Refused boost keeps mass"), Mass->GetMass(), 25.f, 1e-3f);
	return true;
}

// ---------------------------------------------------------------------------
// Accretion disk fade
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccretionDiskFadeTest, "AccretionArchitect.Disk.FadeInAndOut", AccretionTests::Flags)
bool FAccretionDiskFadeTest::RunTest(const FString& Parameters)
{
	UAccretionDiskComponent* Disk = NewObject<UAccretionDiskComponent>();
	Disk->FeedHoldTime = 1.f;
	Disk->FadeInTime = 0.5f;
	Disk->FadeOutTime = 2.f;

	Disk->UpdateFade(1.f);
	TestEqual(TEXT("Disk is dark before feeding"), Disk->GetIntensity(), 0.f, 1e-4f);

	Disk->NotifyMatterConsumed();
	Disk->UpdateFade(0.25f);
	TestEqual(TEXT("Disk fades in while feeding"), Disk->GetIntensity(), 0.5f, 1e-4f);
	Disk->UpdateFade(0.25f);
	TestEqual(TEXT("Disk reaches full intensity"), Disk->GetIntensity(), 1.f, 1e-4f);

	Disk->NotifyMatterConsumed();
	Disk->UpdateFade(0.9f);
	TestEqual(TEXT("Disk stays lit while matter keeps arriving"), Disk->GetIntensity(), 1.f, 1e-4f);

	Disk->UpdateFade(0.2f); // hold time runs out during this step
	Disk->UpdateFade(1.f);
	TestEqual(TEXT("Disk fades out after feeding stops"), Disk->GetIntensity(), 0.5f, 1e-4f);
	TestFalse(TEXT("No longer feeding"), Disk->IsFeeding());

	Disk->UpdateFade(5.f);
	TestEqual(TEXT("Disk fades to fully dark"), Disk->GetIntensity(), 0.f, 1e-4f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
