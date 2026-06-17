// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Environment/UOUFlyingSwarmEffectActor.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUPaperPlaneSwarmTimingTest,
	"UnderOneUmBrella.World.Environment.PaperPlaneSwarm.Timing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUPaperPlaneSwarmTimingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("FlightAlpha starts at 0"), AUOUFlyingSwarmEffectActor::CalculateFlightAlpha(0.0f, 2.5f), 0.0f);
	TestEqual(TEXT("FlightAlpha reaches 0.5 at half flight duration"), AUOUFlyingSwarmEffectActor::CalculateFlightAlpha(1.25f, 2.5f), 0.5f);
	TestEqual(TEXT("FlightAlpha reaches 1 at flight duration"), AUOUFlyingSwarmEffectActor::CalculateFlightAlpha(2.5f, 2.5f), 1.0f);
	TestEqual(TEXT("FlightAlpha is clamped before start"), AUOUFlyingSwarmEffectActor::CalculateFlightAlpha(-1.0f, 2.5f), 0.0f);
	TestEqual(TEXT("FlightAlpha is clamped after duration"), AUOUFlyingSwarmEffectActor::CalculateFlightAlpha(10.0f, 2.5f), 1.0f);

	TestEqual(TEXT("WrapAlpha stays 0 before wrap start"), AUOUFlyingSwarmEffectActor::CalculateWrapAlpha(1.5f, 2.0f, 1.5f), 0.0f);
	TestEqual(TEXT("WrapAlpha starts at 0 on wrap start"), AUOUFlyingSwarmEffectActor::CalculateWrapAlpha(2.0f, 2.0f, 1.5f), 0.0f);
	TestEqual(TEXT("WrapAlpha reaches 0.5 at half wrap duration"), AUOUFlyingSwarmEffectActor::CalculateWrapAlpha(2.75f, 2.0f, 1.5f), 0.5f);
	TestEqual(TEXT("WrapAlpha reaches 1 after wrap duration"), AUOUFlyingSwarmEffectActor::CalculateWrapAlpha(3.5f, 2.0f, 1.5f), 1.0f);
	TestEqual(TEXT("WrapAlpha is clamped after wrap duration"), AUOUFlyingSwarmEffectActor::CalculateWrapAlpha(10.0f, 2.0f, 1.5f), 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUPaperPlaneSwarmMotionTest,
	"UnderOneUmBrella.World.Environment.PaperPlaneSwarm.Motion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUPaperPlaneSwarmMotionTest::RunTest(const FString& Parameters)
{
	auto TestVectorNear = [this](const TCHAR* What, const FVector& Actual, const FVector& Expected)
	{
		constexpr float Tolerance = 0.01f;
		TestTrue(What, Actual.Equals(Expected, Tolerance));
	};

	FUOUPaperPlaneSwarmMotionInput MotionInput;
	MotionInput.StartPosition = FVector::ZeroVector;
	MotionInput.TargetPosition = FVector(1000.0f, 0.0f, 0.0f);
	MotionInput.WrapRadius = 350.0f;
	MotionInput.WrapHeight = 250.0f;
	MotionInput.OrbitSpeed = UE_PI;
	MotionInput.WobbleRightAmount = 0.0f;
	MotionInput.WobbleUpAmount = 0.0f;
	MotionInput.FarReachAlpha = 0.58f;
	MotionInput.TargetForward = FVector::ZeroVector;
	MotionInput.TargetRight = FVector::ZeroVector;
	MotionInput.TargetUp = FVector::UpVector;
	MotionInput.PreviousPosition = FVector::ZeroVector;
	MotionInput.DeltaTime = 0.5f;

	FUOUPaperPlaneSwarmParticleRandom ParticleRandom;
	ParticleRandom.RandomPhase = 0.0f;
	ParticleRandom.RandomDelay = 0.0f;
	ParticleRandom.RandomSpeed = 1.0f;
	ParticleRandom.RandomRadius = 0.0f;
	ParticleRandom.RandomHeight = 100.0f;
	ParticleRandom.SideOffset = 50.0f;
	ParticleRandom.HeightOffset = 300.0f;
	ParticleRandom.PatternIndex = static_cast<int32>(EUOUPaperPlaneSwarmFlightPattern::WideGlide);
	ParticleRandom.PatternPhase = 0.0f;
	ParticleRandom.SwoopAmount = 1.0f;
	ParticleRandom.SwoopHeight = 1.0f;
	ParticleRandom.SwoopSideAmount = 1.0f;
	ParticleRandom.SwoopSpeed = 1.0f;
	ParticleRandom.FarPoint = FVector(0.0f, -2000.0f, 1000.0f);

	FUOUPaperPlaneSwarmMotionResult Result = AUOUFlyingSwarmEffectActor::SolvePaperPlaneSwarmMotion(MotionInput, ParticleRandom);
	TestEqual(TEXT("FlightT starts at 0"), Result.FlightT, 0.0f);
	TestEqual(TEXT("WrapT starts at 0"), Result.WrapT, 0.0f);
	TestVectorNear(TEXT("Initial position stays on start"), Result.Position, FVector::ZeroVector);

	MotionInput.FlightAlpha = 1.0f;
	Result = AUOUFlyingSwarmEffectActor::SolvePaperPlaneSwarmMotion(MotionInput, ParticleRandom);
	TestEqual(TEXT("FlightT reaches 1"), Result.FlightT, 1.0f);
	TestEqual(TEXT("Pattern index is preserved"), Result.PatternIndex, static_cast<int32>(EUOUPaperPlaneSwarmFlightPattern::WideGlide));
	TestFalse(TEXT("Wide glide uses a first control point away from start"), Result.ControlPointA.Equals(MotionInput.StartPosition, 0.01f));
	TestFalse(TEXT("Wide glide uses a second control point away from target"), Result.ControlPointB.Equals(MotionInput.TargetPosition, 0.01f));
	TestVectorNear(TEXT("Far point is preserved from particle random data"), Result.FarPoint, FVector(0.0f, -2000.0f, 1000.0f));
	TestVectorNear(TEXT("Flight path approaches spherical PreWrapPosition instead of target center"), Result.PreWrapPosition, FVector(1350.0f, 0.0f, 100.0f));
	TestVectorNear(TEXT("Before wrap, final position follows path position"), Result.Position, FVector(1350.0f, 0.0f, 100.0f));

	MotionInput.FlightAlpha = 0.5f;
	MotionInput.WrapAlpha = 0.0f;
	FUOUPaperPlaneSwarmMotionResult WideGlideMidResult = AUOUFlyingSwarmEffectActor::SolvePaperPlaneSwarmMotion(MotionInput, ParticleRandom);
	TestTrue(TEXT("Wide glide mid-flight moves toward the configured far point"), WideGlideMidResult.Position.Y < -1200.0f);
	ParticleRandom.PatternIndex = static_cast<int32>(EUOUPaperPlaneSwarmFlightPattern::SCurve);
	FUOUPaperPlaneSwarmMotionResult SCurveMidResult = AUOUFlyingSwarmEffectActor::SolvePaperPlaneSwarmMotion(MotionInput, ParticleRandom);
	TestFalse(TEXT("Different glide patterns produce different mid-flight paths"), WideGlideMidResult.Position.Equals(SCurveMidResult.Position, 0.01f));

	MotionInput.FlightAlpha = 1.0f;
	MotionInput.WrapAlpha = 1.0f;
	ParticleRandom.PatternIndex = static_cast<int32>(EUOUPaperPlaneSwarmFlightPattern::WideGlide);
	Result = AUOUFlyingSwarmEffectActor::SolvePaperPlaneSwarmMotion(MotionInput, ParticleRandom);
	TestEqual(TEXT("WrapT reaches 1"), Result.WrapT, 1.0f);
	TestVectorNear(TEXT("After wrap, final position follows spherical orbit position"), Result.Position, FVector(1350.0f, 0.0f, 0.0f));
	TestVectorNear(TEXT("Velocity is derived from previous position and DeltaTime"), Result.Velocity, FVector(2700.0f, 0.0f, 0.0f));
	TestVectorNear(TEXT("Forward direction follows velocity alignment"), Result.ForwardDirection, Result.Velocity.GetSafeNormal());
	TestEqual(TEXT("Default scale comes from particle scale random"), Result.Scale, 1.0f);
	TestEqual(TEXT("Bank is zero when bank sine is zero"), Result.BankRadians, 0.0f);

	MotionInput.Time = UE_PI * 0.125f;
	ParticleRandom.BankAmount = 30.0f;
	ParticleRandom.ScaleRandom = 0.85f;
	Result = AUOUFlyingSwarmEffectActor::SolvePaperPlaneSwarmMotion(MotionInput, ParticleRandom);
	TestTrue(TEXT("Bank uses guide sine formula in radians"), FMath::IsNearlyEqual(Result.BankRadians, FMath::DegreesToRadians(30.0f), 0.001f));
	TestEqual(TEXT("Scale follows ScaleRandom"), Result.Scale, 0.85f);

	MotionInput.FlightAlpha = 0.1f;
	MotionInput.WrapAlpha = 0.1f;
	ParticleRandom.RandomDelay = 0.2f;
	Result = AUOUFlyingSwarmEffectActor::SolvePaperPlaneSwarmMotion(MotionInput, ParticleRandom);
	TestEqual(TEXT("RandomDelay holds FlightT at 0 until alpha catches up"), Result.FlightT, 0.0f);
	TestEqual(TEXT("RandomDelay holds WrapT at 0 until alpha catches up"), Result.WrapT, 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUPaperPlaneSwarmRandomTest,
	"UnderOneUmBrella.World.Environment.PaperPlaneSwarm.Random",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUPaperPlaneSwarmRandomTest::RunTest(const FString& Parameters)
{
	auto TestInRange = [this](const TCHAR* What, float Value, float MinValue, float MaxValue)
	{
		const float RangeMin = FMath::Min(MinValue, MaxValue);
		const float RangeMax = FMath::Max(MinValue, MaxValue);
		TestTrue(What, Value >= RangeMin && Value <= RangeMax);
	};

	FUOUPaperPlaneSwarmRandomRanges Ranges;
	const FUOUPaperPlaneSwarmParticleRandom Particle0A = AUOUFlyingSwarmEffectActor::MakePaperPlaneSwarmParticleRandom(0, 2026, Ranges);
	const FUOUPaperPlaneSwarmParticleRandom Particle0B = AUOUFlyingSwarmEffectActor::MakePaperPlaneSwarmParticleRandom(0, 2026, Ranges);
	const FUOUPaperPlaneSwarmParticleRandom Particle1 = AUOUFlyingSwarmEffectActor::MakePaperPlaneSwarmParticleRandom(1, 2026, Ranges);

	TestEqual(TEXT("Same particle index and seed are deterministic"), Particle0A.RandomPhase, Particle0B.RandomPhase);
	TestNotEqual(TEXT("Different particle index changes RandomPhase"), Particle0A.RandomPhase, Particle1.RandomPhase);

	TestInRange(TEXT("RandomPhase is in guide range"), Particle0A.RandomPhase, Ranges.RandomPhaseMin, Ranges.RandomPhaseMax);
	TestInRange(TEXT("RandomDelay is in guide range"), Particle0A.RandomDelay, Ranges.RandomDelayMin, Ranges.RandomDelayMax);
	TestInRange(TEXT("RandomSpeed is in guide range"), Particle0A.RandomSpeed, Ranges.RandomSpeedMin, Ranges.RandomSpeedMax);
	TestInRange(TEXT("RandomRadius is in guide range"), Particle0A.RandomRadius, Ranges.RandomRadiusMin, Ranges.RandomRadiusMax);
	TestInRange(TEXT("RandomHeight is in guide range"), Particle0A.RandomHeight, Ranges.RandomHeightMin, Ranges.RandomHeightMax);
	TestInRange(TEXT("SideOffset is in guide range"), Particle0A.SideOffset, Ranges.SideOffsetMin, Ranges.SideOffsetMax);
	TestInRange(TEXT("HeightOffset is in guide range"), Particle0A.HeightOffset, Ranges.HeightOffsetMin, Ranges.HeightOffsetMax);
	TestInRange(TEXT("BankAmount is in guide range"), Particle0A.BankAmount, Ranges.BankAmountMin, Ranges.BankAmountMax);
	TestInRange(TEXT("ScaleRandom is in guide range"), Particle0A.ScaleRandom, Ranges.ScaleRandomMin, Ranges.ScaleRandomMax);
	TestTrue(TEXT("PatternIndex is in enabled pattern range"), Particle0A.PatternIndex >= 0 && Particle0A.PatternIndex < Ranges.FlightPatternCount);
	TestInRange(TEXT("PatternPhase is in guide range"), Particle0A.PatternPhase, Ranges.PatternPhaseMin, Ranges.PatternPhaseMax);
	TestInRange(TEXT("SwoopAmount is in guide range"), Particle0A.SwoopAmount, Ranges.SwoopAmountMin, Ranges.SwoopAmountMax);
	TestInRange(TEXT("SwoopHeight is in guide range"), Particle0A.SwoopHeight, Ranges.SwoopHeightMin, Ranges.SwoopHeightMax);
	TestInRange(TEXT("SwoopSideAmount is in guide range"), Particle0A.SwoopSideAmount, Ranges.SwoopSideAmountMin, Ranges.SwoopSideAmountMax);
	TestInRange(TEXT("SwoopSpeed is in guide range"), Particle0A.SwoopSpeed, Ranges.SwoopSpeedMin, Ranges.SwoopSpeedMax);
	TestInRange(TEXT("FarPoint X is in guide range"), Particle0A.FarPoint.X, Ranges.FarPointMin.X, Ranges.FarPointMax.X);
	TestInRange(TEXT("FarPoint Y is in guide range"), Particle0A.FarPoint.Y, Ranges.FarPointMin.Y, Ranges.FarPointMax.Y);
	TestInRange(TEXT("FarPoint Z is in guide range"), Particle0A.FarPoint.Z, Ranges.FarPointMin.Z, Ranges.FarPointMax.Z);

	Ranges.RandomDelayMin = 0.35f;
	Ranges.RandomDelayMax = 0.1f;
	const FUOUPaperPlaneSwarmParticleRandom SwappedRangeParticle = AUOUFlyingSwarmEffectActor::MakePaperPlaneSwarmParticleRandom(2, 2026, Ranges);
	TestInRange(TEXT("Swapped ranges are normalized"), SwappedRangeParticle.RandomDelay, 0.1f, 0.35f);

	return true;
}

#endif
