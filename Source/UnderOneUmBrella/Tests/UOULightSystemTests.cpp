// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EditorWorldUtils.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "Player/UOUUmbrellaLightInteractionComponent.h"
#include "Player/UOUUmbrellaLightShadeVolumeComponent.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"
#include "World/Light/UOULightBeamVisualComponent.h"
#include "World/Light/UOULightExposureReceiverComponent.h"
#include "World/Light/UOULightExposureSourceComponent.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"
#include "World/Light/UOULightReflectionPathTypes.h"
#include "World/Light/UOULightReflectionSpotLightComponent.h"
#include "World/Light/UOULightSourceActor.h"
#include "World/Light/UOURotatableMirrorActor.h"

namespace
{
	FNumericProperty* FindNormalizedNumericProperty(const UObject* Object, FString NormalizedName)
	{
		if (Object == nullptr)
		{
			return nullptr;
		}

		NormalizedName.ToLowerInline();
		NormalizedName.ReplaceInline(TEXT(" "), TEXT(""));
		NormalizedName.ReplaceInline(TEXT("_"), TEXT(""));
		NormalizedName.ReplaceInline(TEXT("-"), TEXT(""));

		for (TFieldIterator<FProperty> PropertyIt(Object->GetClass(), EFieldIteratorFlags::IncludeSuper);
			PropertyIt;
			++PropertyIt)
		{
			FString InternalName = PropertyIt->GetName().ToLower();
			InternalName.ReplaceInline(TEXT(" "), TEXT(""));
			InternalName.ReplaceInline(TEXT("_"), TEXT(""));
			InternalName.ReplaceInline(TEXT("-"), TEXT(""));

			FString DisplayName = PropertyIt->GetDisplayNameText().ToString().ToLower();
			DisplayName.ReplaceInline(TEXT(" "), TEXT(""));
			DisplayName.ReplaceInline(TEXT("_"), TEXT(""));
			DisplayName.ReplaceInline(TEXT("-"), TEXT(""));

			if (InternalName.StartsWith(NormalizedName) || DisplayName == NormalizedName)
			{
				return CastField<FNumericProperty>(*PropertyIt);
			}
		}

		return nullptr;
	}

	bool ReadFloatingPointProperty(
		const UObject* Object,
		const TCHAR* NormalizedName,
		double& OutValue)
	{
		FNumericProperty* Property = FindNormalizedNumericProperty(Object, NormalizedName);
		if (Property == nullptr || !Property->IsFloatingPoint())
		{
			return false;
		}

		const void* ValueAddress = Property->ContainerPtrToValuePtr<void>(Object);
		OutValue = Property->GetFloatingPointPropertyValue(ValueAddress);
		return true;
	}

	FBoolProperty* FindNormalizedBoolProperty(const UObject* Object, FString NormalizedName)
	{
		if (Object == nullptr)
		{
			return nullptr;
		}

		NormalizedName.ToLowerInline();
		NormalizedName.ReplaceInline(TEXT(" "), TEXT(""));
		NormalizedName.ReplaceInline(TEXT("_"), TEXT(""));
		NormalizedName.ReplaceInline(TEXT("-"), TEXT(""));

		for (TFieldIterator<FProperty> PropertyIt(Object->GetClass(), EFieldIteratorFlags::IncludeSuper);
			PropertyIt;
			++PropertyIt)
		{
			FString InternalName = PropertyIt->GetName().ToLower();
			InternalName.ReplaceInline(TEXT(" "), TEXT(""));
			InternalName.ReplaceInline(TEXT("_"), TEXT(""));
			InternalName.ReplaceInline(TEXT("-"), TEXT(""));

			FString DisplayName = PropertyIt->GetDisplayNameText().ToString().ToLower();
			DisplayName.ReplaceInline(TEXT(" "), TEXT(""));
			DisplayName.ReplaceInline(TEXT("_"), TEXT(""));
			DisplayName.ReplaceInline(TEXT("-"), TEXT(""));

			if (InternalName.StartsWith(NormalizedName) || DisplayName == NormalizedName)
			{
				return CastField<FBoolProperty>(*PropertyIt);
			}
		}

		return nullptr;
	}

	bool ReadBoolProperty(const UObject* Object, const TCHAR* NormalizedName, bool& bOutValue)
	{
		FBoolProperty* Property = FindNormalizedBoolProperty(Object, NormalizedName);
		if (Property == nullptr)
		{
			return false;
		}

		const void* ValueAddress = Property->ContainerPtrToValuePtr<void>(Object);
		bOutValue = Property->GetPropertyValue(ValueAddress);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightMirrorReflectionTest,
	"UnderOneUmbrella.Light.Reflection.MirrorByHitNormal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightMirrorReflectionTest::RunTest(const FString& Parameters)
{
	UUOULightInteractionSurfaceComponent* Surface =
		NewObject<UUOULightInteractionSurfaceComponent>();
	TestNotNull(TEXT("빛 상호작용 표면을 생성한다"), Surface);
	if (Surface == nullptr)
	{
		return false;
	}

	Surface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);
	Surface->ReflectionNormalMode = EUOULightReflectionNormalMode::HitNormal;
	Surface->ReflectionDirectionMode = EUOULightReflectionDirectionMode::MirrorByNormal;
	Surface->bReflectFrontFaceOnly = false;

	const FVector IncomingDirection = FVector::ForwardVector;
	const FVector SurfaceNormal = FVector(-1.0f, 1.0f, 0.0f).GetSafeNormal();
	const FVector ReflectedDirection =
		Surface->GetReflectionDirection(IncomingDirection, SurfaceNormal);

	TestTrue(
		TEXT("45도 거울은 +X 입사광을 +Y 방향으로 반사한다"),
		ReflectedDirection.Equals(FVector::RightVector, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUReflectionImpactCoverageTest,
	"UnderOneUmbrella.Light.Reflection.ImpactOffsetLimitsCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUReflectionImpactCoverageTest::RunTest(const FString& Parameters)
{
	UUOULightInteractionSurfaceComponent* Surface =
		NewObject<UUOULightInteractionSurfaceComponent>();
	TestNotNull(TEXT("충돌 위치 기반 반사 폭 테스트 표면을 생성한다"), Surface);
	if (Surface == nullptr)
	{
		return false;
	}

	Surface->SetBoxExtent(FVector(70.0f, 70.0f, 6.0f));
	Surface->bLimitReflectionBySurfaceAperture = true;
	Surface->bLimitReflectionByImpactOffset = true;
	Surface->ReflectionImpactEdgeInset = 4.0f;
	Surface->MinimumReflectionCoverageRatio = 0.3f;

	const FVector IncomingDirection = -FVector::UpVector;
	const FVector HitNormal = FVector::UpVector;
	const float IncomingRadius = 50.0f;
	const float CenterRadius = Surface->ClampReflectionBeamRadius(
		IncomingRadius,
		IncomingDirection,
		HitNormal,
		FVector::ZeroVector);
	const FVector MiddleImpact(30.0f, 0.0f, 0.0f);
	const float MiddleRadius = Surface->ClampReflectionBeamRadius(
		IncomingRadius,
		IncomingDirection,
		HitNormal,
		MiddleImpact);
	const FVector EdgeImpact(60.0f, 0.0f, 0.0f);

	TestEqual(TEXT("중앙 충돌은 원래 반사광 굵기를 유지한다"), CenterRadius, IncomingRadius);
	TestEqual(TEXT("가장자리로 이동하면 남은 반사 폭만 사용한다"), MiddleRadius, 36.0f);
	TestTrue(
		TEXT("충분히 걸친 충돌은 반사를 허용한다"),
		Surface->HasSufficientReflectionCoverage(
			IncomingRadius,
			IncomingDirection,
			HitNormal,
			MiddleImpact));
	TestFalse(
		TEXT("가장자리에 조금만 걸친 충돌은 반사를 거부한다"),
		Surface->HasSufficientReflectionCoverage(
			IncomingRadius,
			IncomingDirection,
			HitNormal,
			EdgeImpact));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOURotatableMirrorStableNormalTest,
	"UnderOneUmbrella.Light.Reflection.RotatableMirrorUsesStableComponentNormal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOURotatableMirrorStableNormalTest::RunTest(const FString& Parameters)
{
	const AUOURotatableMirrorActor* MirrorDefaults = GetDefault<AUOURotatableMirrorActor>();
	TestNotNull(TEXT("회전 거울 기본 오브젝트가 존재한다"), MirrorDefaults);
	TestNotNull(
		TEXT("회전 거울에 빛 상호작용 표면이 존재한다"),
		MirrorDefaults != nullptr ? MirrorDefaults->LightInteractionSurface.Get() : nullptr);
	if (MirrorDefaults == nullptr || MirrorDefaults->LightInteractionSurface == nullptr)
	{
		return false;
	}

	TestEqual(
		TEXT("회전 거울은 Box 옆면 HitNormal 대신 컴포넌트 앞 방향을 반사 법선으로 사용한다"),
		MirrorDefaults->LightInteractionSurface->ReflectionNormalMode,
		EUOULightReflectionNormalMode::ComponentForward);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUCylinderMultiReflectionDirectionTest,
	"UnderOneUmbrella.Light.Reflection.CylinderMultiBouncePreservesParallelDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUCylinderMultiReflectionDirectionTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOUCylinderMultiReflectionDirectionWorld"),
		nullptr,
		false,
		ERHIFeatureLevel::Num,
		nullptr,
		true);
	FScopedEditorWorld ScopedWorld(
		UninitializedWorld,
		UWorld::InitializationValues()
			.RequiresHitProxies(false)
			.ShouldSimulatePhysics(false)
			.EnableTraceCollision(true)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.AllowAudioPlayback(false)
			.CreatePhysicsScene(true));
	UWorld* World = ScopedWorld.GetWorld();
	TestNotNull(TEXT("실린더 다중 반사 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOULightSourceActor* SourceActor = World->SpawnActor<AUOULightSourceActor>();
	TestNotNull(TEXT("실린더 광원을 생성한다"), SourceActor);
	if (SourceActor == nullptr || SourceActor->ExposureSource == nullptr || SourceActor->SourceSpotLight == nullptr)
	{
		return false;
	}

	SourceActor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator(0.0f, 90.0f, 0.0f));
	SourceActor->SourceSpotLight->SetAttenuationRadius(1500.0f);
	SourceActor->ExposureSource->BeamShape = EUOULightBeamShape::Cylinder;
	SourceActor->ExposureSource->CylinderRadius = 50.0f;
	SourceActor->ExposureSource->BeamLength = 1500.0f;
	SourceActor->ExposureSource->Intensity = 1.0f;
	SourceActor->ExposureSource->SampleInterval = 0.0f;
	SourceActor->ExposureSource->bEnableReflectedLight = true;
	SourceActor->ExposureSource->MaxReflectionBouncesPerPath = 4;
	SourceActor->ExposureSource->ReflectionPathLossGraceTime = 0.0f;

	auto SpawnMirror = [World](const FVector& Location)
	{
		AUOURotatableMirrorActor* Mirror = World->SpawnActor<AUOURotatableMirrorActor>();
		if (Mirror == nullptr || Mirror->LightInteractionSurface == nullptr)
		{
			return Mirror;
		}

		Mirror->SetActorLocationAndRotation(Location, FRotator(0.0f, 135.0f, 0.0f));
		Mirror->LightInteractionSurface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);
		Mirror->LightInteractionSurface->bUseSurfaceAreaSampling = true;
		Mirror->LightInteractionSurface->bReflectFrontFaceOnly = false;
		Mirror->LightInteractionSurface->ReflectionDirectionMode =
			EUOULightReflectionDirectionMode::MirrorByNormal;
		return Mirror;
	};

	AUOURotatableMirrorActor* FirstMirror = SpawnMirror(FVector(0.0f, 600.0f, 10.0f));
	AUOURotatableMirrorActor* SecondMirror = SpawnMirror(FVector(600.0f, 600.0f, 10.0f));
	TestNotNull(TEXT("첫 번째 거울을 생성한다"), FirstMirror);
	TestNotNull(TEXT("두 번째 거울을 생성한다"), SecondMirror);
	if (FirstMirror == nullptr || SecondMirror == nullptr)
	{
		return false;
	}

	SourceActor->ExposureSource->EmitLight(0.1f);
	const TArray<FUOULightPathData> LightPaths = SourceActor->ExposureSource->GetLightPaths();
	const FUOULightPathData* DoubleReflectionPath = LightPaths.FindByPredicate(
		[](const FUOULightPathData& Path)
		{
			return Path.Segments.Num() >= 3;
		});
	TestNotNull(TEXT("두 번 반사되는 실린더 경로를 계산한다"), DoubleReflectionPath);
	if (DoubleReflectionPath == nullptr)
	{
		return false;
	}

	const FUOULightPathSegmentData& FirstReflection = DoubleReflectionPath->Segments[1];
	const FUOULightPathSegmentData& SecondReflection = DoubleReflectionPath->Segments[2];
	TestTrue(
		TEXT("면 샘플을 맞힌 뒤에도 두 번째 입사 방향은 첫 반사의 평행 진행 방향을 유지한다"),
		SecondReflection.IncomingDirection.Equals(FirstReflection.Direction, 0.001f));
	TestTrue(
		TEXT("두 번째 반사는 수직·수평 오차 없이 +Y 방향으로 진행한다"),
		SecondReflection.Direction.Equals(FVector::RightVector, 0.001f));
	TestTrue(
		TEXT("첫 번째 반사 구간의 종점은 방향과 길이로 재구성한 종점과 일치한다"),
		FirstReflection.End.Equals(
			FirstReflection.Start + FirstReflection.Direction * FirstReflection.Length,
			0.01f));
	TestTrue(
		TEXT("두 번째 반사 구간의 종점은 방향과 길이로 재구성한 종점과 일치한다"),
		SecondReflection.End.Equals(
			SecondReflection.Start + SecondReflection.Direction * SecondReflection.Length,
			0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightInteractionModeTest,
	"UnderOneUmbrella.Light.Reflection.InteractionModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightInteractionModeTest::RunTest(const FString& Parameters)
{
	UUOULightInteractionSurfaceComponent* Surface =
		NewObject<UUOULightInteractionSurfaceComponent>();
	TestNotNull(TEXT("빛 상호작용 표면을 생성한다"), Surface);
	if (Surface == nullptr)
	{
		return false;
	}

	Surface->SetLightInteractionMode(EUOULightInteractionMode::Disabled);
	TestFalse(TEXT("Disabled 표면은 빛을 막지 않는다"), Surface->CanBlockLight());
	TestFalse(TEXT("Disabled 표면은 빛을 반사하지 않는다"), Surface->CanReflectLight());

	Surface->SetLightInteractionMode(EUOULightInteractionMode::Blocking);
	TestTrue(TEXT("Blocking 표면은 빛을 막는다"), Surface->CanBlockLight());
	TestFalse(TEXT("Blocking 표면은 빛을 반사하지 않는다"), Surface->CanReflectLight());

	Surface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);
	TestTrue(TEXT("Reflecting 표면은 입사광을 막는다"), Surface->CanBlockLight());
	TestTrue(TEXT("Reflecting 표면은 반사광을 만든다"), Surface->CanReflectLight());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightPathDataTest,
	"UnderOneUmbrella.Light.Path.DirectAndReflectedSegments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightPathDataTest::RunTest(const FString& Parameters)
{
	FUOULightPathData Path;

	FUOULightPathSegmentData DirectSegment;
	DirectSegment.SegmentIndex = 0;
	DirectSegment.bReflected = false;
	DirectSegment.HitType = EUOULightPathHitType::ReflectingSurface;
	Path.Segments.Add(DirectSegment);

	FUOULightPathSegmentData ReflectedSegment;
	ReflectedSegment.SegmentIndex = 1;
	ReflectedSegment.bReflected = true;
	ReflectedSegment.HitType = EUOULightPathHitType::Receiver;
	ReflectedSegment.EndReason = EUOULightReflectionPathEndReason::Blocked;
	Path.Segments.Add(ReflectedSegment);

	TestEqual(TEXT("통합 경로는 직접광과 반사광 구간을 함께 가진다"), Path.Segments.Num(), 2);
	TestFalse(TEXT("첫 구간은 직접광이다"), Path.Segments[0].bReflected);
	TestTrue(TEXT("두 번째 구간은 반사광이다"), Path.Segments[1].bReflected);
	TestEqual(
		TEXT("반사광은 수신 대상에서 종료될 수 있다"),
		Path.Segments[1].HitType,
		EUOULightPathHitType::Receiver);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightSourceCompositionTest,
	"UnderOneUmbrella.Light.Source.Composition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightSourceCompositionTest::RunTest(const FString& Parameters)
{
	const AUOULightSourceActor* SourceActor = GetDefault<AUOULightSourceActor>();
	TestNotNull(TEXT("통합 광원 액터 CDO가 존재한다"), SourceActor);
	if (SourceActor == nullptr)
	{
		return false;
	}

	TestNotNull(
		TEXT("통합 광원은 게임플레이 판정 컴포넌트를 가진다"),
		SourceActor->FindComponentByClass<UUOULightExposureSourceComponent>());
	TestNotNull(
		TEXT("통합 광원은 경로 기반 VFX 컴포넌트를 가진다"),
		SourceActor->FindComponentByClass<UUOULightBeamVisualComponent>());
	TestNotNull(
		TEXT("통합 광원은 반사 보조 조명 컴포넌트를 가진다"),
		SourceActor->FindComponentByClass<UUOULightReflectionSpotLightComponent>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightVisualEventDrivenTest,
	"UnderOneUmbrella.Light.Visual.EventDriven",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightVisualEventDrivenTest::RunTest(const FString& Parameters)
{
	const UUOULightBeamVisualComponent* VisualComponent =
		GetDefault<UUOULightBeamVisualComponent>();
	TestNotNull(TEXT("빛줄기 VFX 컴포넌트 CDO가 존재한다"), VisualComponent);
	if (VisualComponent == nullptr)
	{
		return false;
	}

	TestFalse(
		TEXT("빛줄기 VFX는 자체 Tick 추적을 사용하지 않는다"),
		VisualComponent->PrimaryComponentTick.bCanEverTick);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULazyGodrayFrustumProfileTest,
	"UnderOneUmbrella.Light.Visual.LazyGodrayFrustumProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULazyGodrayFrustumProfileTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOULazyGodrayProfileWorld"),
		nullptr,
		false,
		ERHIFeatureLevel::Num,
		nullptr,
		true);
	FScopedEditorWorld ScopedWorld(
		UninitializedWorld,
		UWorld::InitializationValues()
			.RequiresHitProxies(false)
			.ShouldSimulatePhysics(false)
			.EnableTraceCollision(false)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.AllowAudioPlayback(false)
			.CreatePhysicsScene(false));
	UWorld* World = ScopedWorld.GetWorld();
	TestNotNull(TEXT("LazyGodray 프로필 테스트용 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	UClass* LazyGodrayClass = LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Fab/LazyGodray/Blueprints/BP_LazyGodray_V2.BP_LazyGodray_V2_C"));
	TestNotNull(TEXT("LazyGodray V2 Blueprint 클래스를 불러온다"), LazyGodrayClass);
	if (LazyGodrayClass == nullptr)
	{
		return false;
	}

	AUOULightSourceActor* SourceActor = World->SpawnActor<AUOULightSourceActor>();
	TestNotNull(TEXT("통합 광원 액터를 생성한다"), SourceActor);
	if (SourceActor == nullptr || SourceActor->ExposureSource == nullptr || SourceActor->BeamVisual == nullptr)
	{
		return false;
	}

	FUOULightPathSegmentData Segment;
	Segment.SegmentIndex = 1;
	Segment.bReflected = true;
	Segment.Start = FVector(100.0f, 200.0f, 300.0f);
	Segment.Direction = FVector::ForwardVector;
	Segment.IncomingDirection = FVector::RightVector;
	Segment.Length = 300.0f;
	Segment.End = Segment.Start + Segment.Direction * Segment.Length;
	Segment.StartRadius = 20.0f;
	Segment.EndRadius = 50.0f;
	Segment.Intensity = 0.75f;

	FUOULightPathData Path;
	Path.Segments.Add(Segment);
	SourceActor->ExposureSource->LightPaths = {Path};
	SourceActor->BeamVisual->VFXActorClass = LazyGodrayClass;
	SourceActor->BeamVisual->EndPadding = 0.0f;
	SourceActor->BeamVisual->RefreshVisuals();

	AActor* SpawnedGodray = nullptr;
	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		if (ActorIt->GetOwner() == SourceActor && ActorIt->IsA(LazyGodrayClass))
		{
			SpawnedGodray = *ActorIt;
			break;
		}
	}

	TestNotNull(TEXT("경로 구간에 대응하는 LazyGodray VFX를 생성한다"), SpawnedGodray);
	if (SpawnedGodray == nullptr)
	{
		return false;
	}

	double LengthMultiplier = 0.0;
	double DiameterMultiplier = 0.0;
	double ConvergentRate = 0.0;
	TestTrue(
		TEXT("LazyGodray 길이 배율 프로퍼티를 찾는다"),
		ReadFloatingPointProperty(SpawnedGodray, TEXT("overalllengthmultiplier"), LengthMultiplier));
	TestTrue(
		TEXT("LazyGodray 지름 배율 프로퍼티를 찾는다"),
		ReadFloatingPointProperty(SpawnedGodray, TEXT("overalldiametermultiplier"), DiameterMultiplier));
	TestTrue(
		TEXT("LazyGodray 수렴률 프로퍼티를 찾는다"),
		ReadFloatingPointProperty(
			SpawnedGodray,
			TEXT("overalllightrayconvergentrate"),
			ConvergentRate));

	TestTrue(TEXT("접합부를 잘라낸 구간 길이를 LazyGodray 배율로 전달한다"), FMath::IsNearlyEqual(LengthMultiplier, 25.0 / 9.0, 0.001));
	TestTrue(TEXT("끝 지름을 LazyGodray 폭 배율로 전달한다"), FMath::IsNearlyEqual(DiameterMultiplier, 1.0));
	AddInfo(FString::Printf(TEXT("LazyGodray convergence runtime value: %.6f"), ConvergentRate));
	TestTrue(TEXT("잘라낸 시작/끝 단면 비율을 LazyGodray 수렴률로 전달한다"), FMath::IsNearlyEqual(ConvergentRate, 5.0 / 9.0, 0.001));
	TestTrue(
		TEXT("90도 반사 VFX 시작점을 반사면 앞쪽으로 이동해 뒤쪽 돌출을 제거한다"),
		SpawnedGodray->GetActorLocation().Equals(
			Segment.Start + Segment.Direction * (200.0 / 9.0),
			0.1));

	TInlineComponentArray<UBoxComponent*> BoxComponents(SpawnedGodray);
	bool bValidatedGodrayBounds = false;
	for (const UBoxComponent* BoxComponent : BoxComponents)
	{
		if (BoxComponent != nullptr && BoxComponent->GetName().Contains(TEXT("godray_card_bounds")))
		{
			AddInfo(FString::Printf(
				TEXT("LazyGodray box %s: location=%s scale=%s extent=%s"),
				*BoxComponent->GetName(),
				*BoxComponent->GetRelativeLocation().ToCompactString(),
				*BoxComponent->GetRelativeScale3D().ToCompactString(),
				*BoxComponent->GetUnscaledBoxExtent().ToCompactString()));
			TestTrue(
				TEXT("Godray Bounds가 빛 구간 길이와 끝 반경을 감싼다"),
				BoxComponent->GetUnscaledBoxExtent().Equals(FVector(50.0f, 50.0f, 1250.0f / 9.0f), 1.0f));
			TestTrue(
				TEXT("Godray Bounds 중심이 V2 로컬 -Z 진행축 중앙에 놓인다"),
				BoxComponent->GetRelativeLocation().Equals(FVector(0.0f, 0.0f, -1250.0f / 9.0f), 1.0f));
			bValidatedGodrayBounds = true;
		}
	}
	TestTrue(TEXT("LazyGodray V2 Bounds를 찾고 갱신한다"), bValidatedGodrayBounds);

	UStaticMeshComponent* CrossGodrayCard = FindObjectFast<UStaticMeshComponent>(
		SpawnedGodray,
		TEXT("UOUCrossGodrayCard"));
	TestNotNull(TEXT("LazyGodray에 시점 보완용 교차 카드를 생성한다"), CrossGodrayCard);

	TInlineComponentArray<UNiagaraComponent*> NiagaraComponents(SpawnedGodray);
	TestTrue(TEXT("LazyGodray V2의 Dust Niagara가 존재한다"), !NiagaraComponents.IsEmpty());
	if (!NiagaraComponents.IsEmpty() && NiagaraComponents[0] != nullptr)
	{
		bool bHasCylinderHeight = false;
		const float CylinderHeight = NiagaraComponents[0]->GetVariableFloat(
			TEXT("User.CylinderHeight"),
			bHasCylinderHeight);
		bool bHasCylinderRadius = false;
		const float CylinderRadius = NiagaraComponents[0]->GetVariableFloat(
			TEXT("User.CylinderRadius"),
			bHasCylinderRadius);
		TestTrue(TEXT("Dust 실린더 높이 파라미터를 갱신한다"), bHasCylinderHeight);
		TestTrue(TEXT("Dust 실린더 반경 파라미터를 갱신한다"), bHasCylinderRadius);
		AddInfo(FString::Printf(
			TEXT("LazyGodray dust cylinder radius: %.3f (valid=%d)"),
			CylinderRadius,
			bHasCylinderRadius));
		if (bHasCylinderHeight)
		{
			AddInfo(FString::Printf(TEXT("LazyGodray dust cylinder height: %.3f"), CylinderHeight));
			TestTrue(TEXT("Dust 범위가 접합부를 잘라낸 길이를 따른다"), FMath::IsNearlyEqual(CylinderHeight, 2500.0f / 9.0f, 1.0f));
		}
		if (bHasCylinderRadius)
		{
			TestTrue(TEXT("Dust 반경이 구간 최대 반경을 따른다"), FMath::IsNearlyEqual(CylinderRadius, Segment.EndRadius, 1.0f));
		}
	}

	if (!NiagaraComponents.IsEmpty() && NiagaraComponents[0] != nullptr)
	{
		const FVector ExpectedDustMidpoint = SpawnedGodray->GetActorLocation() +
			Segment.Direction * (1250.0f / 9.0f);
		TestTrue(
			TEXT("Dust 중심이 잘린 빛 구간의 중앙에 배치된다"),
			NiagaraComponents[0]->GetComponentLocation().Equals(ExpectedDustMidpoint, 1.0f));
	}

	SourceActor->ExposureSource->LightPaths.Reset();
	SourceActor->BeamVisual->RefreshVisuals();
	TestTrue(TEXT("반사 경로가 사라지면 기존 VFX 액터를 즉시 숨긴다"), SpawnedGodray->IsHidden());
	for (const UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (NiagaraComponent != nullptr)
		{
			TestFalse(TEXT("숨긴 반사 VFX의 먼지 Niagara를 즉시 정지한다"), NiagaraComponent->IsActive());
			TestFalse(TEXT("숨긴 반사 VFX의 먼지 렌더링을 끈다"), NiagaraComponent->IsVisible());
		}
	}

	UClass* LazyGodrayV1Class = LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Fab/LazyGodray/Blueprints/BP_LazyGodray.BP_LazyGodray_C"));
	TestNotNull(TEXT("LazyGodray V1.2 Blueprint 클래스를 불러온다"), LazyGodrayV1Class);
	if (LazyGodrayV1Class != nullptr)
	{
		AUOULightSourceActor* V1SourceActor = World->SpawnActor<AUOULightSourceActor>();
		TestNotNull(TEXT("LazyGodray V1.2 테스트용 광원을 생성한다"), V1SourceActor);
		if (V1SourceActor != nullptr &&
			V1SourceActor->ExposureSource != nullptr &&
			V1SourceActor->BeamVisual != nullptr)
		{
			V1SourceActor->ExposureSource->LightPaths = {Path};
			V1SourceActor->BeamVisual->VFXActorClass = LazyGodrayV1Class;
			V1SourceActor->BeamVisual->EndPadding = 0.0f;
			V1SourceActor->BeamVisual->RefreshVisuals();

			AActor* SpawnedGodrayV1 = nullptr;
			for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
			{
				if (ActorIt->GetOwner() == V1SourceActor && ActorIt->IsA(LazyGodrayV1Class))
				{
					SpawnedGodrayV1 = *ActorIt;
					break;
				}
			}

			TestNotNull(TEXT("반사 구간용 LazyGodray V1.2 VFX를 생성한다"), SpawnedGodrayV1);
			if (SpawnedGodrayV1 != nullptr)
			{
				bool bUseConvergent = false;
				double V1ConvergentRate = 0.0;
				TestTrue(
					TEXT("LazyGodray V1.2 수렴 토글을 찾는다"),
					ReadBoolProperty(
						SpawnedGodrayV1,
						TEXT("uselightrayconvergent"),
						bUseConvergent));
				TestTrue(
					TEXT("LazyGodray V1.2 수렴률을 찾는다"),
					ReadFloatingPointProperty(
						SpawnedGodrayV1,
						TEXT("volumetriclightrayconvergentrate"),
						V1ConvergentRate));
				TestTrue(TEXT("LazyGodray V1.2의 수렴 표현을 활성화한다"), bUseConvergent);
				TestTrue(
					TEXT("LazyGodray V1.2에 반사 구간 시작/끝 반경 비율을 전달한다"),
					FMath::IsNearlyEqual(V1ConvergentRate, 5.0 / 9.0, 0.001));
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaShadePathOcclusionTest,
	"UnderOneUmbrella.Light.Path.UmbrellaShadeOccludesMirror",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaShadePathOcclusionTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOUUmbrellaShadePathWorld"),
		nullptr,
		false,
		ERHIFeatureLevel::Num,
		nullptr,
		true);
	FScopedEditorWorld ScopedWorld(
		UninitializedWorld,
		UWorld::InitializationValues()
			.RequiresHitProxies(false)
			.ShouldSimulatePhysics(false)
			.EnableTraceCollision(true)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.AllowAudioPlayback(false)
			.CreatePhysicsScene(true));
	UWorld* World = ScopedWorld.GetWorld();
	TestNotNull(TEXT("우산 광선 차단 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOULightSourceActor* SourceActor = World->SpawnActor<AUOULightSourceActor>();
	if (SourceActor == nullptr || SourceActor->ExposureSource == nullptr || SourceActor->SourceSpotLight == nullptr)
	{
		AddError(TEXT("통합 광원 액터를 생성하지 못했다"));
		return false;
	}

	SourceActor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	SourceActor->SourceSpotLight->SetAttenuationRadius(1000.0f);
	SourceActor->SourceSpotLight->SetOuterConeAngle(10.0f);
	SourceActor->ExposureSource->SampleInterval = 0.0f;
	SourceActor->ExposureSource->bEnableReflectedLight = true;
	SourceActor->ExposureSource->ReflectionPathLossGraceTime = 1.0f;

	AActor* ShadeActor = World->SpawnActor<AActor>();
	UUOUUmbrellaLightShadeVolumeComponent* ShadeVolume = ShadeActor != nullptr
		? NewObject<UUOUUmbrellaLightShadeVolumeComponent>(ShadeActor, TEXT("TestUmbrellaShade"))
		: nullptr;
	TestNotNull(TEXT("우산 그늘 볼륨을 생성한다"), ShadeVolume);
	if (ShadeActor == nullptr || ShadeVolume == nullptr)
	{
		return false;
	}
	ShadeActor->AddInstanceComponent(ShadeVolume);
	ShadeActor->SetRootComponent(ShadeVolume);
	ShadeVolume->SetBoxExtent(FVector(30.0f, 30.0f, 100.0f));
	ShadeVolume->RegisterComponent();
	ShadeActor->SetActorLocation(FVector(200.0f, 0.0f, 0.0f));
	ShadeVolume->SetShadeEnabled(true);

	AActor* MirrorActor = World->SpawnActor<AActor>();
	UUOULightInteractionSurfaceComponent* MirrorSurface = MirrorActor != nullptr
		? NewObject<UUOULightInteractionSurfaceComponent>(MirrorActor, TEXT("TestMirrorSurface"))
		: nullptr;
	TestNotNull(TEXT("우산 뒤쪽 거울 표면을 생성한다"), MirrorSurface);
	if (MirrorActor == nullptr || MirrorSurface == nullptr)
	{
		return false;
	}
	MirrorActor->AddInstanceComponent(MirrorSurface);
	MirrorActor->SetRootComponent(MirrorSurface);
	MirrorSurface->SetBoxExtent(FVector(5.0f, 200.0f, 100.0f));
	MirrorSurface->bUseSurfaceAreaSampling = true;
	MirrorSurface->bReflectFrontFaceOnly = false;
	MirrorSurface->ReflectionFrontNormalMode =
		EUOULightReflectionFrontNormalMode::OwnerForward;
	MirrorSurface->ReflectionNormalMode = EUOULightReflectionNormalMode::HitNormal;
	MirrorSurface->ReflectionDirectionMode = EUOULightReflectionDirectionMode::MirrorByNormal;
	MirrorSurface->RegisterComponent();
	// 중심은 원뿔 밖에 있지만 넓은 면이 광원 중심축을 가로지르는 배치입니다.
	// 우산이 중심축을 막을 때 가장자리 샘플만으로 반사되지 않아야 합니다.
	MirrorActor->SetActorLocation(FVector(400.0f, 100.0f, 0.0f));
	MirrorSurface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);

	SourceActor->ExposureSource->EmitLight(0.1f);
	const TArray<FUOULightReflectionPathData> BlockedReflectionPaths =
		SourceActor->ExposureSource->GetReflectionPaths();
	const TArray<FUOULightPathData> BlockedLightPaths = SourceActor->ExposureSource->GetLightPaths();
	TestTrue(TEXT("우산 뒤쪽 거울에는 반사 경로가 생성되지 않는다"), BlockedReflectionPaths.IsEmpty());
	TestTrue(TEXT("광선은 우산 그늘까지의 직접광 경로를 남긴다"), !BlockedLightPaths.IsEmpty());
	if (!BlockedLightPaths.IsEmpty() && !BlockedLightPaths[0].Segments.IsEmpty())
	{
		const FUOULightPathSegmentData& BlockedSegment = BlockedLightPaths[0].Segments[0];
		TestEqual(
			TEXT("직접광은 우산 그늘 볼륨에서 종료된다"),
			BlockedSegment.HitComponent.Get(),
			static_cast<UPrimitiveComponent*>(ShadeVolume));
		TestTrue(TEXT("직접광은 뒤쪽 거울보다 앞에서 종료된다"), BlockedSegment.End.X < MirrorActor->GetActorLocation().X);
	}

	ShadeVolume->SetShadeEnabled(false);
	SourceActor->ExposureSource->EmitLight(0.1f);
	const TArray<FUOULightPathData> UnblockedLightPaths = SourceActor->ExposureSource->GetLightPaths();
	const TArray<FUOULightReflectionPathData> UnblockedReflectionPaths =
		SourceActor->ExposureSource->GetReflectionPaths();
	TestTrue(TEXT("우산 차단을 끄면 뒤쪽 거울의 반사 경로가 생성된다"), !UnblockedReflectionPaths.IsEmpty());
	TestTrue(TEXT("우산 차단을 끄면 직접광 경로가 다시 계산된다"), !UnblockedLightPaths.IsEmpty());
	if (!UnblockedLightPaths.IsEmpty() && !UnblockedLightPaths[0].Segments.IsEmpty())
	{
		const FUOULightPathSegmentData& UnblockedSegment = UnblockedLightPaths[0].Segments[0];
		TestEqual(
			TEXT("우산 차단을 끄면 동일한 배치의 거울에 광선이 도달한다"),
			UnblockedSegment.HitComponent.Get(),
			static_cast<UPrimitiveComponent*>(MirrorSurface));
	}

	UUOULightInteractionSurfaceComponent* UmbrellaSurface =
		NewObject<UUOULightInteractionSurfaceComponent>(ShadeActor, TEXT("TestUmbrellaReflector"));
	TestNotNull(TEXT("차단과 반사를 함께 수행할 우산 표면을 생성한다"), UmbrellaSurface);
	if (UmbrellaSurface != nullptr)
	{
		ShadeActor->AddInstanceComponent(UmbrellaSurface);
		UmbrellaSurface->SetupAttachment(ShadeVolume);
		UmbrellaSurface->SetBoxExtent(FVector(5.0f, 30.0f, 100.0f));
		UmbrellaSurface->bUseSurfaceAreaSampling = false;
		UmbrellaSurface->bReflectFrontFaceOnly = true;
		UmbrellaSurface->MaximumReflectionIncidenceAngle = 30.0f;
		UmbrellaSurface->bPassThroughWhenReflectionRejected = true;
		UmbrellaSurface->ReflectionFrontNormalMode =
			EUOULightReflectionFrontNormalMode::OwnerForward;
		UmbrellaSurface->ReflectionDirectionMode = EUOULightReflectionDirectionMode::OwnerForward;
		UmbrellaSurface->RegisterComponent();
		UmbrellaSurface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);
		ShadeVolume->SetShadeEnabled(true);
		SourceActor->ExposureSource->ReflectionPathLossGraceTime = 0.0f;

		SourceActor->ExposureSource->EmitLight(0.1f);
		const TArray<FUOULightReflectionPathData> RejectedUmbrellaReflectionPaths =
			SourceActor->ExposureSource->GetReflectionPaths();
		bool bFoundMirrorBehindRejectedUmbrella = false;
		bool bFoundRejectedUmbrellaReflector = false;
		for (const FUOULightReflectionPathData& ReflectionPath : RejectedUmbrellaReflectionPaths)
		{
			if (ReflectionPath.Segments.IsEmpty())
			{
				continue;
			}

			bFoundMirrorBehindRejectedUmbrella |=
				ReflectionPath.Segments[0].Reflector == MirrorSurface;
			bFoundRejectedUmbrellaReflector |=
				ReflectionPath.Segments[0].Reflector == UmbrellaSurface;
		}
		TestTrue(
			TEXT("반사 허용 각도 밖의 우산과 그늘 뒤에 있는 거울까지 광선이 도달한다"),
			bFoundMirrorBehindRejectedUmbrella);
		TestFalse(
			TEXT("반사 허용 각도 밖의 우산에서는 반사 경로를 만들지 않는다"),
			bFoundRejectedUmbrellaReflector);

		UmbrellaSurface->bReflectFrontFaceOnly = false;

		SourceActor->ExposureSource->EmitLight(0.1f);
		const TArray<FUOULightReflectionPathData> UmbrellaReflectionPaths =
			SourceActor->ExposureSource->GetReflectionPaths();
		TestTrue(TEXT("우산 반사 경로가 생성된다"), !UmbrellaReflectionPaths.IsEmpty());

		bool bFoundUmbrellaFirstReflector = false;
		bool bFoundBlockedMirrorFirstReflector = false;
		for (const FUOULightReflectionPathData& ReflectionPath : UmbrellaReflectionPaths)
		{
			if (ReflectionPath.Segments.IsEmpty())
			{
				continue;
			}

			bFoundUmbrellaFirstReflector |=
				ReflectionPath.Segments[0].Reflector == UmbrellaSurface;
			bFoundBlockedMirrorFirstReflector |=
				ReflectionPath.Segments[0].Reflector == MirrorSurface;
		}

		TestTrue(TEXT("직접광은 우산에서 먼저 반사된다"), bFoundUmbrellaFirstReflector);
		TestFalse(
			TEXT("우산이 중심광을 막으면 가장자리 샘플만으로 뒤쪽 거울 반사를 만들지 않는다"),
			bFoundBlockedMirrorFirstReflector);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaReflectionAngleBoundaryTest,
	"UnderOneUmbrella.Light.Reflection.UmbrellaAngleBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaReflectionAngleBoundaryTest::RunTest(const FString& Parameters)
{
	const UUOUUmbrellaLightInteractionComponent* UmbrellaSettings =
		GetDefault<UUOUUmbrellaLightInteractionComponent>();
	TestNotNull(TEXT("우산 빛 상호작용 기본 설정이 존재한다"), UmbrellaSettings);
	if (UmbrellaSettings == nullptr)
	{
		return false;
	}

	UUOULightInteractionSurfaceComponent* Surface =
		NewObject<UUOULightInteractionSurfaceComponent>();
	TestNotNull(TEXT("우산 반사 각도 테스트 표면을 생성한다"), Surface);
	if (Surface == nullptr)
	{
		return false;
	}

	Surface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);
	Surface->ReflectionFrontNormalMode = EUOULightReflectionFrontNormalMode::ComponentForward;
	Surface->bReflectFrontFaceOnly = true;
	Surface->bPassThroughWhenReflectionRejected = true;
	Surface->MaximumReflectionIncidenceAngle =
		UmbrellaSettings->MaximumUmbrellaReflectionIncidenceAngle;

	const FVector FrontNormal = FVector::ForwardVector;
	const FVector ValidIncomingDirection =
		-FRotator(0.0f, 45.0f, 0.0f).RotateVector(FrontNormal);
	const FVector GrazingIncomingDirection =
		-FRotator(0.0f, 80.0f, 0.0f).RotateVector(FrontNormal);

	TestFalse(
		TEXT("우산 정면에서 45도인 빛은 반사하므로 통과하지 않는다"),
		Surface->ShouldPassThroughIncomingLight(ValidIncomingDirection, FrontNormal));
	TestTrue(
		TEXT("우산 정면에서 80도인 비스듬한 빛은 반사하지 않고 통과한다"),
		Surface->ShouldPassThroughIncomingLight(GrazingIncomingDirection, FrontNormal));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaShadeDirectionTest,
	"UnderOneUmbrella.Light.Shade.UmbrellaDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaShadeDirectionTest::RunTest(const FString& Parameters)
{
	UUOUUmbrellaLightShadeVolumeComponent* ShadeVolume =
		NewObject<UUOUUmbrellaLightShadeVolumeComponent>();
	TestNotNull(TEXT("우산 빛 차단 방향 테스트 컴포넌트를 생성한다"), ShadeVolume);
	if (ShadeVolume == nullptr)
	{
		return false;
	}

	ShadeVolume->SetShadeEnabled(true);
	ShadeVolume->MaximumBlockingIncidenceAngle = 75.0f;
	ShadeVolume->bBlockFrontFaceOnly = true;

	TestTrue(
		TEXT("위에서 내려오는 빛은 펼친 우산이 차단한다"),
		ShadeVolume->CanShadeIncomingLight(FVector::DownVector));
	TestTrue(
		TEXT("대각선 상단에서 들어오는 빛은 펼친 우산이 차단한다"),
		ShadeVolume->CanShadeIncomingLight(FVector(1.0f, 0.0f, -1.0f)));
	TestFalse(
		TEXT("수평 측면에서 들어오는 빛은 펼친 우산을 통과한다"),
		ShadeVolume->CanShadeIncomingLight(FVector::ForwardVector));
	TestFalse(
		TEXT("우산 뒷면에서 들어오는 빛은 차단하지 않는다"),
		ShadeVolume->CanShadeIncomingLight(FVector::UpVector));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightWorldPathIntegrationTest,
	"UnderOneUmbrella.Light.Path.WorldCollisionIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightWorldPathIntegrationTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOULightPathIntegrationWorld"),
		nullptr,
		false,
		ERHIFeatureLevel::Num,
		nullptr,
		true);
	FScopedEditorWorld ScopedWorld(
		UninitializedWorld,
		UWorld::InitializationValues()
			.RequiresHitProxies(false)
			.ShouldSimulatePhysics(false)
			.EnableTraceCollision(true)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.AllowAudioPlayback(false)
			.CreatePhysicsScene(true));
	UWorld* World = ScopedWorld.GetWorld();
	TestNotNull(TEXT("빛 경로 통합 테스트용 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOULightSourceActor* SourceActor = World->SpawnActor<AUOULightSourceActor>();
	TestNotNull(TEXT("통합 광원 액터를 월드에 생성한다"), SourceActor);
	TestNotNull(
		TEXT("통합 광원 액터에 게임플레이 판정 컴포넌트가 있다"),
		SourceActor != nullptr ? SourceActor->ExposureSource.Get() : nullptr);
	TestNotNull(
		TEXT("통합 광원 액터에 원본 SpotLight 컴포넌트가 있다"),
		SourceActor != nullptr ? SourceActor->SourceSpotLight.Get() : nullptr);
	if (SourceActor == nullptr || SourceActor->ExposureSource == nullptr || SourceActor->SourceSpotLight == nullptr)
	{
		return false;
	}

	SourceActor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	SourceActor->SourceSpotLight->SetAttenuationRadius(1000.0f);
	SourceActor->SourceSpotLight->SetOuterConeAngle(15.0f);
	SourceActor->ExposureSource->bEmitLight = true;
	SourceActor->ExposureSource->Intensity = 1.0f;
	SourceActor->ExposureSource->SampleInterval = 0.0f;
	SourceActor->ExposureSource->bEnableReflectedLight = true;
	SourceActor->ExposureSource->MaxReflectionBouncesPerPath = 4;
	SourceActor->ExposureSource->ReflectionPathLossGraceTime = 0.0f;

	AActor* SurfaceActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("우산과 거울을 대표하는 상호작용 표면 액터를 생성한다"), SurfaceActor);
	if (SurfaceActor == nullptr)
	{
		return false;
	}

	UUOULightInteractionSurfaceComponent* Surface =
		NewObject<UUOULightInteractionSurfaceComponent>(SurfaceActor, TEXT("TestLightSurface"));
	TestNotNull(TEXT("빛 상호작용 표면 컴포넌트를 생성한다"), Surface);
	if (Surface == nullptr)
	{
		return false;
	}
	SurfaceActor->AddInstanceComponent(Surface);
	SurfaceActor->SetRootComponent(Surface);
	Surface->SetBoxExtent(FVector(5.0f, 100.0f, 100.0f));
	Surface->bUseSurfaceAreaSampling = false;
	Surface->bReflectFrontFaceOnly = false;
	Surface->ReflectionFrontNormalMode = EUOULightReflectionFrontNormalMode::ComponentForward;
	Surface->ReflectionNormalMode = EUOULightReflectionNormalMode::HitNormal;
	Surface->ReflectionDirectionMode = EUOULightReflectionDirectionMode::MirrorByNormal;
	Surface->RegisterComponent();
	SurfaceActor->SetActorLocation(FVector(400.0f, 0.0f, 0.0f));

	Surface->SetLightInteractionMode(EUOULightInteractionMode::Blocking);
	SourceActor->ExposureSource->EmitLight(0.1f);

	TArray<FUOULightPathData> LightPaths = SourceActor->ExposureSource->GetLightPaths();
	TestTrue(TEXT("차단 표면까지 직접광 경로가 하나 이상 계산된다"), !LightPaths.IsEmpty());
	if (LightPaths.IsEmpty() || LightPaths[0].Segments.IsEmpty())
	{
		return false;
	}

	const FUOULightPathSegmentData& BlockedSegment = LightPaths[0].Segments[0];
	TestEqual(
		TEXT("Blocking 상태의 표면은 빛을 종료한다"),
		BlockedSegment.HitType,
		EUOULightPathHitType::BlockingSurface);
	TestFalse(TEXT("Blocking 상태에서는 반사 구간을 만들지 않는다"), BlockedSegment.bReflected);
	TestTrue(TEXT("직접광은 표면 뒤까지 통과하지 않는다"), BlockedSegment.End.X < 410.0f);

	AActor* ReceiverActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("반사광 수신 액터를 생성한다"), ReceiverActor);
	if (ReceiverActor == nullptr)
	{
		return false;
	}

	UBoxComponent* ReceiverBox = NewObject<UBoxComponent>(ReceiverActor, TEXT("ReceiverBox"));
	TestNotNull(TEXT("반사광 수신 볼륨을 생성한다"), ReceiverBox);
	if (ReceiverBox == nullptr)
	{
		return false;
	}
	ReceiverActor->AddInstanceComponent(ReceiverBox);
	ReceiverActor->SetRootComponent(ReceiverBox);
	ReceiverBox->SetBoxExtent(FVector(10.0f, 100.0f, 100.0f));
	ReceiverBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ReceiverBox->SetCollisionObjectType(ECC_WorldDynamic);
	ReceiverBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ReceiverBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ReceiverBox->RegisterComponent();
	ReceiverActor->SetActorLocation(FVector(-200.0f, 0.0f, 0.0f));

	UUOULightExposureReceiverComponent* Receiver =
		NewObject<UUOULightExposureReceiverComponent>(ReceiverActor, TEXT("LightReceiver"));
	TestNotNull(TEXT("반사광 수신 컴포넌트를 생성한다"), Receiver);
	if (Receiver == nullptr)
	{
		return false;
	}
	ReceiverActor->AddInstanceComponent(Receiver);
	Receiver->bUseReceiverVolumeSampling = false;
	Receiver->RegisterComponent();

	Surface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);
	SourceActor->ExposureSource->EmitLight(0.1f);
	LightPaths = SourceActor->ExposureSource->GetLightPaths();

	TestTrue(TEXT("Reflecting 상태에서는 직접광과 반사광 경로가 계산된다"), !LightPaths.IsEmpty());
	if (LightPaths.IsEmpty() || LightPaths[0].Segments.Num() < 2)
	{
		return false;
	}

	const FUOULightPathSegmentData& IncomingSegment = LightPaths[0].Segments[0];
	const FUOULightPathSegmentData& ReflectedSegment = LightPaths[0].Segments[1];
	TestEqual(
		TEXT("직접광은 반사 표면에서 종료된다"),
		IncomingSegment.HitType,
		EUOULightPathHitType::ReflectingSurface);
	TestTrue(TEXT("두 번째 구간은 반사광이다"), ReflectedSegment.bReflected);
	TestTrue(
		TEXT("수직 거울에 들어간 +X 빛은 -X 방향으로 반사된다"),
		ReflectedSegment.Direction.Equals(-FVector::ForwardVector, 0.01f));
	TestEqual(
		TEXT("반사광은 수신 대상에서 종료된다"),
		ReflectedSegment.HitType,
		EUOULightPathHitType::Receiver);
	TestTrue(TEXT("반사광이 수신 컴포넌트에 전달된다"), Receiver->IsReceivingLight());
	TestTrue(TEXT("반사광 경로는 수신 대상 뒤까지 통과하지 않는다"), ReflectedSegment.End.X > -220.0f);

	SourceActor->ExposureSource->BeamShape = EUOULightBeamShape::Cylinder;
	SourceActor->ExposureSource->CylinderRadius = 50.0f;
	SourceActor->ExposureSource->BeamLength = 1000.0f;
	SourceActor->ExposureSource->EmitLight(0.1f);
	LightPaths = SourceActor->ExposureSource->GetLightPaths();

	TestTrue(TEXT("원기둥 빛도 같은 반사 경로를 계산한다"), !LightPaths.IsEmpty());
	if (LightPaths.IsEmpty() || LightPaths[0].Segments.Num() < 2)
	{
		return false;
	}

	const FUOULightPathSegmentData& CylinderIncomingSegment = LightPaths[0].Segments[0];
	const FUOULightPathSegmentData& CylinderReflectedSegment = LightPaths[0].Segments[1];
	TestEqual(TEXT("원기둥 직접광의 시작 반지름을 유지한다"), CylinderIncomingSegment.StartRadius, 50.0f);
	TestEqual(TEXT("원기둥 직접광의 끝 반지름을 유지한다"), CylinderIncomingSegment.EndRadius, 50.0f);
	TestEqual(TEXT("원기둥 반사광은 확산각이 없다"), CylinderReflectedSegment.ConeAngle, 0.0f);
	TestTrue(
		TEXT("원기둥 반사광도 수신 대상에서 종료된다"),
		CylinderReflectedSegment.HitType == EUOULightPathHitType::Receiver);

	SourceActor->ExposureSource->BeamShape = EUOULightBeamShape::Cone;
	SourceActor->SourceSpotLight->SetOuterConeAngle(60.0f);
	Surface->ReflectionIntensityMultiplier = 0.1f;
	ReceiverBox->SetBoxExtent(FVector(10.0f, 10.0f, 10.0f));
	ReceiverActor->SetActorLocation(FVector(200.0f, 50.0f, 0.0f));
	SourceActor->ExposureSource->EmitLight(0.1f);

	const TArray<FUOULightReflectionPathData> MultiPathReflectionPaths =
		SourceActor->ExposureSource->GetReflectionPaths();
	bool bReflectedPathReachedReceiver = false;
	for (const FUOULightReflectionPathData& ReflectionPath : MultiPathReflectionPaths)
	{
		for (const FUOULightReflectionSegmentData& Segment : ReflectionPath.Segments)
		{
			bReflectedPathReachedReceiver |= Segment.ReachedReceivers.Contains(Receiver);
		}
	}
	TestTrue(
		TEXT("같은 대상에 직접광과 반사광 후보가 모두 도달한다"),
		bReflectedPathReachedReceiver);
	TestEqual(
		TEXT("한 광원의 다중 경로는 대상에 한 번만 적용한다"),
		SourceActor->ExposureSource->LastLitCount,
		1);
	TestEqual(
		TEXT("약한 반사광보다 강한 직접광을 선택한다"),
		SourceActor->ExposureSource->LastReflectedCount,
		0);

	AActor* FallbackSourceActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Fallback 사거리 테스트용 광원 액터를 생성한다"), FallbackSourceActor);
	if (FallbackSourceActor == nullptr)
	{
		return false;
	}

	USceneComponent* FallbackRoot =
		NewObject<USceneComponent>(FallbackSourceActor, TEXT("FallbackSourceRoot"));
	FallbackSourceActor->AddInstanceComponent(FallbackRoot);
	FallbackSourceActor->SetRootComponent(FallbackRoot);
	FallbackRoot->RegisterComponent();
	FallbackSourceActor->SetActorLocationAndRotation(
		FVector(0.0f, 2000.0f, 0.0f),
		FRotator(0.0f, 90.0f, 0.0f));

	UUOULightExposureSourceComponent* FallbackSource =
		NewObject<UUOULightExposureSourceComponent>(FallbackSourceActor, TEXT("FallbackExposureSource"));
	FallbackSourceActor->AddInstanceComponent(FallbackSource);
	FallbackSource->bAutoFindSourceSpotLight = false;
	FallbackSource->BeamShape = EUOULightBeamShape::Cone;
	FallbackSource->BeamLength = 321.0f;
	FallbackSource->bEnableReflectedLight = false;
	FallbackSource->SampleInterval = 0.0f;
	FallbackSource->RegisterComponent();
	FallbackSource->EmitLight(0.1f);

	const TArray<FUOULightPathData> FallbackLightPaths = FallbackSource->GetLightPaths();
	TestTrue(TEXT("LocalLight가 없어도 Cone 경로를 생성한다"), !FallbackLightPaths.IsEmpty());
	if (!FallbackLightPaths.IsEmpty() && !FallbackLightPaths[0].Segments.IsEmpty())
	{
		TestEqual(
			TEXT("LocalLight가 없어도 빛 총 길이를 사용한다"),
			FallbackLightPaths[0].Segments[0].Length,
			321.0f);
	}

	return true;
}

#endif
