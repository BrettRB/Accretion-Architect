// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Components/AccretionMassComponent.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
