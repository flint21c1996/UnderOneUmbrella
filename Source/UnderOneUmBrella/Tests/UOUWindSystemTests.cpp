// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "World/Wind/UOUWindInteractionSurfaceComponent.h"
#include "World/Wind/UOUWindTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUWindMirrorReflectionTest,
	"UnderOneUmbrella.Wind.Reflection.MirrorByHitNormal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindMirrorReflectionTest::RunTest(const FString& Parameters)
{
	UUOUWindInteractionSurfaceComponent* Surface =
		NewObject<UUOUWindInteractionSurfaceComponent>();
	TestNotNull(TEXT("Wind surface is created"), Surface);
	if (Surface == nullptr)
	{
		return false;
	}

	Surface->InteractionMode = EUOUWindInteractionMode::Reflecting;
	Surface->ReflectionNormalMode = EUOUWindReflectionNormalMode::HitNormal;

	const FVector IncomingDirection = FVector::ForwardVector;
	const FVector SurfaceNormal = FVector(-1.0f, 1.0f, 0.0f).GetSafeNormal();
	const FVector ReflectedDirection =
		Surface->GetOutgoingDirection(IncomingDirection, SurfaceNormal);

	TestTrue(
		TEXT("A 45-degree surface reflects +X wind toward +Y"),
		ReflectedDirection.Equals(FVector::RightVector, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUWindInteractionModeTest,
	"UnderOneUmbrella.Wind.Reflection.InteractionModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindInteractionModeTest::RunTest(const FString& Parameters)
{
	UUOUWindInteractionSurfaceComponent* Surface =
		NewObject<UUOUWindInteractionSurfaceComponent>();
	TestNotNull(TEXT("Wind surface is created"), Surface);
	if (Surface == nullptr)
	{
		return false;
	}

	Surface->SetInteractionMode(EUOUWindInteractionMode::Disabled);
	TestFalse(TEXT("Disabled surface does not block wind"), Surface->CanBlockWind());
	TestFalse(TEXT("Disabled surface does not reflect wind"), Surface->CanReflectWind());

	Surface->SetInteractionMode(EUOUWindInteractionMode::Blocking);
	TestTrue(TEXT("Blocking surface blocks wind"), Surface->CanBlockWind());
	TestFalse(TEXT("Blocking surface does not reflect wind"), Surface->CanReflectWind());

	Surface->SetInteractionMode(EUOUWindInteractionMode::Reflecting);
	TestTrue(TEXT("Reflecting surface blocks incoming wind"), Surface->CanBlockWind());
	TestTrue(TEXT("Reflecting surface creates outgoing wind"), Surface->CanReflectWind());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUWindPathSegmentLengthTest,
	"UnderOneUmbrella.Wind.Path.SegmentLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindPathSegmentLengthTest::RunTest(const FString& Parameters)
{
	FUOUWindPathSegment Segment;
	Segment.Start = FVector(10.0f, 20.0f, 30.0f);
	Segment.End = FVector(310.0f, 420.0f, 30.0f);
	TestEqual(TEXT("Segment length uses world-space endpoints"), Segment.GetLength(), 500.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUWindPulseCycleTest,
	"UnderOneUmbrella.Wind.Pulse.ThreeSecondsOnOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindPulseCycleTest::RunTest(const FString& Parameters)
{
	FUOUWindPulseRuntimeState PulseState;
	PulseState.Reset(true, 3.0f, 3.0f);

	TestTrue(TEXT("Pulse starts with wind"), PulseState.bIsBlowing);
	TestEqual(TEXT("Initial ON time is three seconds"), PulseState.TimeRemaining, 3.0f);

	TestFalse(TEXT("No phase change before three seconds"), PulseState.Advance(2.9f, 3.0f, 3.0f));
	TestTrue(TEXT("Wind is still blowing before the boundary"), PulseState.bIsBlowing);

	TestTrue(TEXT("Pulse changes to OFF at three seconds"), PulseState.Advance(0.1f, 3.0f, 3.0f));
	TestFalse(TEXT("Wind is resting during OFF phase"), PulseState.bIsBlowing);
	TestEqual(TEXT("OFF time resets to three seconds"), PulseState.TimeRemaining, 3.0f);

	TestTrue(TEXT("Pulse changes back to ON after three more seconds"), PulseState.Advance(3.0f, 3.0f, 3.0f));
	TestTrue(TEXT("Wind is blowing again"), PulseState.bIsBlowing);
	TestEqual(TEXT("Next ON time resets to three seconds"), PulseState.TimeRemaining, 3.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUWindPulseLargeDeltaTest,
	"UnderOneUmbrella.Wind.Pulse.LargeDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindPulseLargeDeltaTest::RunTest(const FString& Parameters)
{
	FUOUWindPulseRuntimeState PulseState;
	PulseState.Reset(true, 3.0f, 3.0f);

	TestFalse(
		TEXT("Two complete phases return to the same state"),
		PulseState.Advance(6.0f, 3.0f, 3.0f));
	TestTrue(TEXT("Wind returns to ON after a complete cycle"), PulseState.bIsBlowing);
	TestEqual(TEXT("Complete cycle restores full ON duration"), PulseState.TimeRemaining, 3.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUWindAdditiveAccelerationTest,
	"UnderOneUmbrella.Wind.Character.AdditiveAccelerationClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindAdditiveAccelerationTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Wind and character acceleration are added as vectors"),
		UOUWindMotion::CalculateClampedAdditiveAcceleration(
			FVector(600.0f, 0.0f, 0.0f),
			FVector(0.0f, 0.0f, 200.0f),
			1000.0f),
		FVector(600.0f, 0.0f, 200.0f));

	TestEqual(
		TEXT("Combined wind acceleration is clamped to the editor maximum"),
		UOUWindMotion::CalculateClampedAdditiveAcceleration(
			FVector(900.0f, 0.0f, 0.0f),
			FVector(600.0f, 0.0f, 0.0f),
			1000.0f),
		FVector(1000.0f, 0.0f, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUWindCharacterAccelerationTest,
	"UnderOneUmbrella.Wind.Character.DirectionalAcceleration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindCharacterAccelerationTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Continuous wind keeps accelerating after the optional target speed"),
		UOUWindMotion::CalculateDirectionalAcceleration(
			false,
			5000.0f,
			1200.0f,
			5000.0f,
			1.0f / 60.0f),
		5000.0f);

	TestEqual(
		TEXT("Limited wind caps acceleration near target speed"),
		UOUWindMotion::CalculateDirectionalAcceleration(
			true,
			1190.0f,
			1200.0f,
			5000.0f,
			0.02f),
		500.0f);

	TestEqual(
		TEXT("Limited wind stops accelerating after target speed"),
		UOUWindMotion::CalculateDirectionalAcceleration(
			true,
			1300.0f,
			1200.0f,
			5000.0f,
			1.0f / 60.0f),
		0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUWindEntryVelocityRetentionTest,
	"UnderOneUmbrella.Wind.Character.EntryVelocityRetention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindEntryVelocityRetentionTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Slow falling character retains fifteen percent of its old velocity"),
		UOUWindMotion::CalculateWindEntryVelocity(
			FVector(100.0f, 200.0f, -100.0f),
			FVector::ForwardVector,
			400.0f,
			0.5f,
			150.0f,
			800.0f,
			0.15f),
		FVector(565.0f, 30.0f, -15.0f));

	TestEqual(
		TEXT("Fast falling momentum is converted into wind direction speed"),
		UOUWindMotion::CalculateWindEntryVelocity(
			FVector(100.0f, 0.0f, -1000.0f),
			FVector::ForwardVector,
			400.0f,
			0.5f,
			150.0f,
			800.0f,
			0.15f),
		FVector(665.0f, 0.0f, -150.0f));

	TestEqual(
		TEXT("Wind entry vector is added after retaining existing momentum"),
		UOUWindMotion::CalculateWindEntryVelocity(
			FVector(600.0f, 0.0f, -100.0f),
			FVector::ForwardVector,
			400.0f,
			0.5f,
			150.0f,
			800.0f,
			0.15f),
		FVector(640.0f, 0.0f, -15.0f));

	TestEqual(
		TEXT("Reflected wind also retains fifteen percent before adding its new vector"),
		UOUWindMotion::CalculateWindEntryVelocity(
			FVector(1000.0f, 0.0f, -100.0f),
			-FVector::ForwardVector,
			400.0f,
			0.0f,
			150.0f,
			800.0f,
			0.15f),
		FVector(-400.0f, 0.0f, -15.0f));
	return true;
}

#endif
