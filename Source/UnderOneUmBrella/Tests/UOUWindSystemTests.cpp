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

#endif
