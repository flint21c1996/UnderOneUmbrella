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
#include "Puzzle/Light/UOULightPaintColorConditionComponent.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"
#include "World/Light/UOULightBeamVisualComponent.h"
#include "World/Light/UOULightBeamMeshVisualActor.h"
#include "World/Light/UOULightExposureReceiverComponent.h"
#include "World/Light/UOULightExposureSourceComponent.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"
#include "World/Light/UOULightCountBulbActor.h"
#include "World/Light/UOULightColorReceiverComponent.h"
#include "World/Light/UOULightReflectionPathTypes.h"
#include "World/Light/UOULightReflectionSpotLightComponent.h"
#include "World/Light/UOULightSourceActor.h"
#include "World/Light/UOULumenDynamicRayVisualActor.h"
#include "World/Light/UOURotatableMirrorActor.h"
#include "World/Light/UOURotatableMirrorComponent.h"

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
	FUOULightCountBulbStateEvaluationTest,
	"UnderOneUmbrella.Light.Bulb.LightCountStateEvaluation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightCountBulbStateEvaluationTest::RunTest(const FString& Parameters)
{
	const AUOULightCountBulbActor* BulbDefaults = GetDefault<AUOULightCountBulbActor>();
	TestNotNull(TEXT("광원 개수 전구 기본 오브젝트가 존재한다"), BulbDefaults);
	if (BulbDefaults == nullptr)
	{
		return false;
	}

	TestNotNull(TEXT("전구에 빛 수신 컴포넌트가 존재한다"), BulbDefaults->LightReceiver.Get());
	TestNotNull(TEXT("전구에 독립된 빛 수신 볼륨이 존재한다"), BulbDefaults->LightReceiverVolume.Get());
	if (BulbDefaults->LightReceiver != nullptr)
	{
		TestTrue(
			TEXT("전구는 빔과 수신 볼륨의 겹침 깊이 판정을 사용한다"),
			BulbDefaults->LightReceiver->bUseBeamVolumeOverlap);
		TestEqual(
			TEXT("전구의 기본 최소 겹침 깊이는 10cm다"),
			BulbDefaults->LightReceiver->MinimumBeamOverlapDepth,
			10.0f);
	}
	TestTrue(
		TEXT("전구 액터는 통합 퍼즐 디버그 Provider로 등록된다"),
		BulbDefaults->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass()));
	TestEqual(
		TEXT("전구 디버그 Provider는 Puzzle 카테고리를 사용한다"),
		IUOUDebugProvider::Execute_GetDebugCategory(
			const_cast<AUOULightCountBulbActor*>(BulbDefaults)),
		EUOUDebugCategory::Puzzle);
	const FString DefaultDebugSummary = IUOUDebugProvider::Execute_GetDebugSummaryText(
		const_cast<AUOULightCountBulbActor*>(BulbDefaults)).ToString();
	TestTrue(
		TEXT("전구 디버그 요약은 현재 빛 개수와 필요 개수를 표시한다"),
		DefaultDebugSummary.Contains(TEXT("Lights: 0 / 1")));
	TestEqual(
		TEXT("필요 광원이 2개일 때 빛이 없으면 꺼짐 상태다"),
		AUOULightCountBulbActor::EvaluateState(0, 2),
		EUOULightCountBulbState::Off);
	TestEqual(
		TEXT("필요 광원이 2개일 때 빛 하나는 부족 상태다"),
		AUOULightCountBulbActor::EvaluateState(1, 2),
		EUOULightCountBulbState::Insufficient);
	TestEqual(
		TEXT("필요 광원이 2개일 때 빛 두 개는 만족 상태다"),
		AUOULightCountBulbActor::EvaluateState(2, 2),
		EUOULightCountBulbState::Satisfied);
	TestEqual(
		TEXT("필요 광원이 2개일 때 빛 세 개는 과열 상태다"),
		AUOULightCountBulbActor::EvaluateState(3, 2),
		EUOULightCountBulbState::Overheated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightCountBulbBeamOverlapTest,
	"UnderOneUmbrella.Light.Bulb.BeamVolumeOverlapDepth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightCountBulbBeamOverlapTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOULightCountBulbBeamOverlapWorld"),
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
	TestNotNull(TEXT("전구 겹침 깊이 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOULightSourceActor* SourceActor = World->SpawnActor<AUOULightSourceActor>();
	AUOULightCountBulbActor* BulbActor = World->SpawnActor<AUOULightCountBulbActor>();
	TestNotNull(TEXT("겹침 깊이 테스트 광원을 생성한다"), SourceActor);
	TestNotNull(TEXT("겹침 깊이 테스트 전구를 생성한다"), BulbActor);
	if (SourceActor == nullptr || SourceActor->ExposureSource == nullptr ||
		SourceActor->SourceSpotLight == nullptr || BulbActor == nullptr ||
		BulbActor->LightReceiver == nullptr)
	{
		return false;
	}

	SourceActor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	SourceActor->SourceSpotLight->SetAttenuationRadius(1000.0f);
	SourceActor->ExposureSource->BeamShape = EUOULightBeamShape::Cylinder;
	SourceActor->ExposureSource->CylinderRadius = 20.0f;
	SourceActor->ExposureSource->BeamLength = 1000.0f;
	SourceActor->ExposureSource->Intensity = 1.0f;
	SourceActor->ExposureSource->SampleInterval = 0.0f;
	SourceActor->ExposureSource->bEnableReflectedLight = false;

	BulbActor->SetActorLocation(FVector(400.0f, 70.0f, 0.0f));
	SourceActor->ExposureSource->EmitLight(0.1f);
	TestFalse(
		TEXT("5cm만 겹치는 가장자리 스침은 전구 수광으로 인정하지 않는다"),
		BulbActor->LightReceiver->IsReceivingLight());
	TestTrue(
		TEXT("거부된 가장자리 스침도 약 5cm의 마지막 평가값으로 기록된다"),
		FMath::IsNearlyEqual(BulbActor->LightReceiver->LastBeamOverlapDepth, 5.0f, 0.5f));
	TestFalse(
		TEXT("최소 깊이 미달 평가 결과는 Rejected로 기록된다"),
		BulbActor->LightReceiver->bLastBeamOverlapAccepted);

	BulbActor->SetActorLocation(FVector(400.0f, 60.0f, 0.0f));
	SourceActor->ExposureSource->EmitLight(0.1f);
	TestTrue(
		TEXT("15cm 겹침은 최소 깊이 10cm를 넘어 전구 수광으로 인정한다"),
		BulbActor->LightReceiver->IsReceivingLight());
	TestTrue(
		TEXT("마지막 겹침 깊이는 약 15cm로 기록된다"),
		FMath::IsNearlyEqual(BulbActor->LightReceiver->LastBeamOverlapDepth, 15.0f, 0.5f));
	TestTrue(
		TEXT("최소 깊이를 넘긴 평가 결과는 Accepted로 기록된다"),
		BulbActor->LightReceiver->bLastBeamOverlapAccepted);

	const TArray<FUOULightPathData> LightPaths = SourceActor->ExposureSource->GetLightPaths();
	TestTrue(TEXT("전구에 도달한 직접광 경로를 생성한다"), !LightPaths.IsEmpty());
	if (!LightPaths.IsEmpty() && !LightPaths[0].Segments.IsEmpty())
	{
		TestTrue(
			TEXT("초록 경로에 사용하는 실제 도달 목록에 전구 Receiver가 포함된다"),
			LightPaths[0].Segments[0].ReachedReceivers.Contains(BulbActor->LightReceiver.Get()));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightColorAdditiveMixTest,
	"UnderOneUmbrella.Light.Color.AdditiveRGBMix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightColorAdditiveMixTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOULightColorAdditiveMixWorld"),
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
	TestNotNull(TEXT("RGB 혼합 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AActor* ReceiverActor = World->SpawnActor<AActor>();
	UUOULightColorReceiverComponent* Receiver = ReceiverActor != nullptr
		? NewObject<UUOULightColorReceiverComponent>(ReceiverActor, TEXT("ColorReceiver"))
		: nullptr;
	TestNotNull(TEXT("색상 수신 액터를 생성한다"), ReceiverActor);
	TestNotNull(TEXT("색상 수신 컴포넌트를 생성한다"), Receiver);
	if (ReceiverActor == nullptr || Receiver == nullptr)
	{
		return false;
	}

	ReceiverActor->AddInstanceComponent(Receiver);
	Receiver->bUseReceiverVolumeSampling = false;
	Receiver->bWeightColorByExposureIntensity = false;
	Receiver->bApplyPaintTint = false;
	Receiver->MaterialTransitionDuration = 0.0f;
	Receiver->MinimumPaintChannel = 0.15f;
	Receiver->RegisterComponent();

	AActor* RedSource = World->SpawnActor<AActor>();
	AActor* GreenSource = World->SpawnActor<AActor>();
	AActor* BlueSource = World->SpawnActor<AActor>();
	TestNotNull(TEXT("빨강 광원 식별자를 생성한다"), RedSource);
	TestNotNull(TEXT("초록 광원 식별자를 생성한다"), GreenSource);
	TestNotNull(TEXT("파랑 광원 식별자를 생성한다"), BlueSource);
	if (RedSource == nullptr || GreenSource == nullptr || BlueSource == nullptr)
	{
		return false;
	}

	const auto MakeExposure = [](UObject* Source, const FLinearColor& Color)
	{
		return FUOULightExposureData(
			Source,
			FVector::ZeroVector,
			FVector::ZeroVector,
			FVector::ForwardVector,
			100.0f,
			0.25f,
			1.0f,
			1.0f,
			0.1f,
			Color);
	};

	Receiver->ReceiveLightExposure_Implementation(MakeExposure(RedSource, FLinearColor::Red));
	TestTrue(
		TEXT("빨강 빛 하나는 빨강으로 표시된다"),
		Receiver->GetMixedLightColor().Equals(FLinearColor::Red, KINDA_SMALL_NUMBER));
	TestEqual(
		TEXT("빨강 빛은 0번 RGB 상태를 사용한다"),
		Receiver->GetCurrentStateMaterialIndex(),
		0);
	TestTrue(
		TEXT("빨강 물감은 G와 B만 최소 채널까지 낮춘다"),
		Receiver->CurrentPaintTint.Equals(
			FLinearColor(1.0f, 0.15f, 0.15f, 1.0f),
			KINDA_SMALL_NUMBER));

	Receiver->ReceiveLightExposure_Implementation(MakeExposure(GreenSource, FLinearColor::Green));
	TestTrue(
		TEXT("빨강과 초록은 노랑으로 가산 혼합된다"),
		Receiver->GetMixedLightColor().Equals(FLinearColor::Yellow, KINDA_SMALL_NUMBER));
	TestEqual(
		TEXT("빨강과 초록은 3번 RGB 상태를 사용한다"),
		Receiver->GetCurrentStateMaterialIndex(),
		3);
	TestTrue(
		TEXT("빨강과 초록 물감은 B만 최소 채널까지 낮춘다"),
		Receiver->CurrentPaintTint.Equals(
			FLinearColor(1.0f, 1.0f, 0.15f, 1.0f),
			KINDA_SMALL_NUMBER));

	Receiver->ReceiveLightExposure_Implementation(MakeExposure(BlueSource, FLinearColor::Blue));
	TestTrue(
		TEXT("빨강, 초록, 파랑은 흰색으로 가산 혼합된다"),
		Receiver->GetMixedLightColor().Equals(FLinearColor::White, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("흰색 상태에서 R 채널을 감지한다"), Receiver->bHasRedLight);
	TestTrue(TEXT("흰색 상태에서 G 채널을 감지한다"), Receiver->bHasGreenLight);
	TestTrue(TEXT("흰색 상태에서 B 채널을 감지한다"), Receiver->bHasBlueLight);
	TestEqual(
		TEXT("RGB 빛은 6번 RGB 상태를 사용한다"),
		Receiver->GetCurrentStateMaterialIndex(),
		6);
	TestTrue(
		TEXT("RGB 흰색 빛은 원래 알베도를 쓰는 흰색 Tint로 복구한다"),
		Receiver->CurrentPaintTint.Equals(FLinearColor::White, KINDA_SMALL_NUMBER));

	Receiver->ClearColorExposures();
	TestFalse(TEXT("노출 기록을 비우면 활성 색상 빛이 없다"), Receiver->HasAnyColorLight());
	TestTrue(
		TEXT("노출 기록을 비우면 혼합 색은 검정이다"),
		Receiver->GetMixedLightColor().Equals(FLinearColor::Black, KINDA_SMALL_NUMBER));
	TestEqual(
		TEXT("노출 기록을 비우면 상태 머티리얼 인덱스가 -1이 된다"),
		Receiver->GetCurrentStateMaterialIndex(),
		INDEX_NONE);
	Receiver->ReceiveLightExposure_Implementation(MakeExposure(BlueSource, FLinearColor::Blue));
	TestEqual(
		TEXT("파랑 빛 하나는 2번 RGB 상태를 사용한다"),
		Receiver->GetCurrentStateMaterialIndex(),
		2);
	const FLinearColor BluePaintTint(0.15f, 0.15f, 1.0f, 1.0f);
	TestTrue(
		TEXT("파랑 빛은 R과 G만 최소 채널까지 낮춘다"),
		Receiver->CurrentPaintTint.Equals(BluePaintTint, KINDA_SMALL_NUMBER));

	Receiver->ClearColorExposures();
	TestTrue(
		TEXT("빛 밖으로 나가도 마지막 파랑 물감색을 유지한다"),
		Receiver->CurrentPaintTint.Equals(BluePaintTint, KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("빛이 사라지면 목표값도 현재 물감색에 고정된다"),
		Receiver->TargetPaintTint.Equals(BluePaintTint, KINDA_SMALL_NUMBER));

	Receiver->ReceiveLightExposure_Implementation(MakeExposure(RedSource, FLinearColor::White));
	TestTrue(
		TEXT("흰색 빛을 다시 받으면 원래 알베도용 흰색 Tint로 복구한다"),
		Receiver->CurrentPaintTint.Equals(FLinearColor::White, KINDA_SMALL_NUMBER));

	AUOULightSourceActor* PresetSource = World->SpawnActor<AUOULightSourceActor>();
	TestNotNull(TEXT("색상 프리셋 광원을 생성한다"), PresetSource);
	if (PresetSource != nullptr && PresetSource->SourceSpotLight != nullptr)
	{
		PresetSource->SetLightColorPreset(EUOULightColorPreset::Blue);
		TestTrue(
			TEXT("파랑 프리셋은 실제 SpotLight를 파랑으로 설정한다"),
			PresetSource->SourceSpotLight->GetLightColor().Equals(
				FLinearColor::Blue,
				KINDA_SMALL_NUMBER));
		TestTrue(
			TEXT("게임플레이 노출도 같은 파랑을 전달한다"),
			PresetSource->ExposureSource->GetGameplayLightColor().Equals(
				FLinearColor::Blue,
				KINDA_SMALL_NUMBER));

		PresetSource->SetLightColorPreset(EUOULightColorPreset::White);
		TestTrue(
			TEXT("흰색 프리셋은 물감 제거용 흰색 노출을 전달한다"),
			PresetSource->ExposureSource->GetGameplayLightColor().Equals(
				FLinearColor::White,
				KINDA_SMALL_NUMBER));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaShadeColorReceiverTest,
	"UnderOneUmbrella.Light.Color.UmbrellaShadeRemovesBlockedChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaShadeColorReceiverTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOUUmbrellaShadeColorReceiverWorld"),
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
	TestNotNull(TEXT("우산 RGB 차단 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	const FVector ReceiverLocation = FVector::ZeroVector;
	AActor* ReceiverActor = World->SpawnActor<AActor>();
	UBoxComponent* ReceiverBox = ReceiverActor != nullptr
		? NewObject<UBoxComponent>(ReceiverActor, TEXT("ColorReceiverBox"))
		: nullptr;
	UUOULightColorReceiverComponent* Receiver = ReceiverActor != nullptr
		? NewObject<UUOULightColorReceiverComponent>(ReceiverActor, TEXT("ColorReceiver"))
		: nullptr;
	TestNotNull(TEXT("색상 수신 상자를 생성한다"), ReceiverBox);
	TestNotNull(TEXT("색상 수신 컴포넌트를 생성한다"), Receiver);
	if (ReceiverActor == nullptr || ReceiverBox == nullptr || Receiver == nullptr)
	{
		return false;
	}

	ReceiverActor->AddInstanceComponent(ReceiverBox);
	ReceiverActor->SetRootComponent(ReceiverBox);
	ReceiverBox->SetBoxExtent(FVector(80.0f));
	ReceiverBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ReceiverBox->SetCollisionObjectType(ECC_WorldDynamic);
	ReceiverBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ReceiverBox->RegisterComponent();
	ReceiverActor->SetActorLocation(ReceiverLocation);
	ReceiverActor->AddInstanceComponent(Receiver);
	Receiver->bUseReceiverVolumeSampling = true;
	Receiver->RequiredReceiverSampleHits = 2;
	Receiver->bApplyPaintTint = false;
	Receiver->MaterialTransitionDuration = 0.0f;
	Receiver->RegisterComponent();

	AActor* ShadeActor = World->SpawnActor<AActor>();
	UUOUUmbrellaLightShadeVolumeComponent* ShadeVolume = ShadeActor != nullptr
		? NewObject<UUOUUmbrellaLightShadeVolumeComponent>(ShadeActor, TEXT("UmbrellaShade"))
		: nullptr;
	TestNotNull(TEXT("빨간 빛을 막을 우산 그늘을 생성한다"), ShadeVolume);
	if (ShadeActor == nullptr || ShadeVolume == nullptr)
	{
		return false;
	}

	ShadeActor->AddInstanceComponent(ShadeVolume);
	ShadeActor->SetRootComponent(ShadeVolume);
	ShadeVolume->SetBoxExtent(FVector(30.0f, 30.0f, 10.0f));
	ShadeVolume->RegisterComponent();
	ShadeActor->SetActorLocation(FVector(0.0f, 0.0f, 250.0f));
	ShadeVolume->SetShadeEnabled(true);

	const auto ConfigureSource = [ReceiverLocation](
		AUOULightSourceActor* SourceActor,
		const FVector& SourceLocation,
		EUOULightColorPreset ColorPreset)
	{
		const FVector SourceDirection = (ReceiverLocation - SourceLocation).GetSafeNormal();
		SourceActor->SetActorLocationAndRotation(SourceLocation, SourceDirection.Rotation());
		SourceActor->SourceSpotLight->SetAttenuationRadius(1000.0f);
		SourceActor->SourceSpotLight->SetOuterConeAngle(30.0f);
		SourceActor->ExposureSource->BeamShape = EUOULightBeamShape::Cone;
		SourceActor->ExposureSource->SampleInterval = 0.0f;
		SourceActor->ExposureSource->bEnableReflectedLight = false;
		SourceActor->SetLightColorPreset(ColorPreset);
	};

	AUOULightSourceActor* RedSource = World->SpawnActor<AUOULightSourceActor>();
	AUOULightSourceActor* BlueSource = World->SpawnActor<AUOULightSourceActor>();
	TestNotNull(TEXT("빨간 광원을 생성한다"), RedSource);
	TestNotNull(TEXT("파란 광원을 생성한다"), BlueSource);
	if (RedSource == nullptr || BlueSource == nullptr ||
		RedSource->ExposureSource == nullptr || RedSource->SourceSpotLight == nullptr ||
		BlueSource->ExposureSource == nullptr || BlueSource->SourceSpotLight == nullptr)
	{
		return false;
	}

	ConfigureSource(
		RedSource,
		FVector(0.0f, 0.0f, 500.0f),
		EUOULightColorPreset::Red);
	ConfigureSource(
		BlueSource,
		FVector(200.0f, 0.0f, 500.0f),
		EUOULightColorPreset::Blue);

	RedSource->ExposureSource->EmitLight(0.1f);
	BlueSource->ExposureSource->EmitLight(0.1f);

	TestFalse(TEXT("우산이 중심 광로를 막은 빨강 채널은 제거된다"), Receiver->bHasRedLight);
	TestTrue(TEXT("우산을 비껴간 파랑 채널은 유지된다"), Receiver->bHasBlueLight);
	TestEqual(
		TEXT("빨강 차단 후 상자는 보라가 아니라 파랑 상태가 된다"),
		Receiver->GetCurrentColorState(),
		EUOULightColorState::Blue);
	TestTrue(
		TEXT("혼합된 실제 빛 색도 파랑만 남는다"),
		Receiver->GetMixedLightColor().Equals(FLinearColor::Blue, KINDA_SMALL_NUMBER));

	// 튜토리얼처럼 별도 Line Of Sight와 볼륨 샘플링을 사용하지 않는 설정에서도
	// 우산 차폐는 독립적인 게임플레이 규칙으로 적용되어야 합니다.
	Receiver->ClearColorExposures();
	Receiver->bUseReceiverVolumeSampling = false;
	RedSource->ExposureSource->bRequireLineOfSight = false;
	BlueSource->SetLightColorPreset(EUOULightColorPreset::Green);

	RedSource->ExposureSource->EmitLight(0.1f);
	BlueSource->ExposureSource->EmitLight(0.1f);

	TestFalse(TEXT("Line Of Sight를 꺼도 우산에 막힌 빨강은 제거된다"), Receiver->bHasRedLight);
	TestTrue(TEXT("우산을 비껴간 초록 채널은 유지된다"), Receiver->bHasGreenLight);
	TestEqual(
		TEXT("빨강 차단 후 상자는 노랑이 아니라 초록 상태가 된다"),
		Receiver->GetCurrentColorState(),
		EUOULightColorState::Green);
	TestTrue(
		TEXT("혼합된 실제 빛 색도 초록만 남는다"),
		Receiver->GetMixedLightColor().Equals(FLinearColor::Green, KINDA_SMALL_NUMBER));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULightPaintColorConditionTest,
	"UnderOneUmbrella.Light.Color.PaintConditionAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULightPaintColorConditionTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOULightPaintColorConditionWorld"),
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
	TestNotNull(TEXT("물감색 조건 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AActor* PaintedActor = World->SpawnActor<AActor>();
	UUOULightColorReceiverComponent* Receiver = PaintedActor != nullptr
		? NewObject<UUOULightColorReceiverComponent>(PaintedActor, TEXT("ColorReceiver"))
		: nullptr;
	UUOULightPaintColorConditionComponent* Condition = PaintedActor != nullptr
		? NewObject<UUOULightPaintColorConditionComponent>(PaintedActor, TEXT("PaintColorCondition"))
		: nullptr;
	TestNotNull(TEXT("물감색 조건 액터를 생성한다"), PaintedActor);
	TestNotNull(TEXT("색상 수신 컴포넌트를 생성한다"), Receiver);
	TestNotNull(TEXT("물감색 조건 컴포넌트를 생성한다"), Condition);
	if (PaintedActor == nullptr || Receiver == nullptr || Condition == nullptr)
	{
		return false;
	}

	PaintedActor->AddInstanceComponent(Receiver);
	Receiver->bUseReceiverVolumeSampling = false;
	Receiver->bWeightColorByExposureIntensity = false;
	Receiver->bApplyPaintTint = false;
	Receiver->MaterialTransitionDuration = 0.0f;
	Receiver->MinimumPaintChannel = 0.15f;
	Receiver->RegisterComponent();

	PaintedActor->AddInstanceComponent(Condition);
	Condition->RequiredColorState = EUOULightColorState::Red;
	Condition->MatchTolerance = 0.05f;
	Condition->ReleaseTolerance = 0.08f;
	Condition->RegisterComponent();
	Condition->RefreshNow();
	TestFalse(TEXT("흰색 초기 상태는 빨강 조건을 만족하지 않는다"), Condition->IsSatisfied());

	AActor* RedSource = World->SpawnActor<AActor>();
	AActor* GreenSource = World->SpawnActor<AActor>();
	TestNotNull(TEXT("빨강 광원 식별자를 생성한다"), RedSource);
	TestNotNull(TEXT("초록 광원 식별자를 생성한다"), GreenSource);
	if (RedSource == nullptr || GreenSource == nullptr)
	{
		return false;
	}

	const auto MakeExposure = [](UObject* Source, const FLinearColor& Color)
	{
		return FUOULightExposureData(
			Source,
			FVector::ZeroVector,
			FVector::ZeroVector,
			FVector::ForwardVector,
			100.0f,
			0.25f,
			1.0f,
			1.0f,
			0.1f,
			Color);
	};

	Receiver->ReceiveLightExposure_Implementation(MakeExposure(RedSource, FLinearColor::Red));
	TestTrue(TEXT("실제 PaintTint가 빨강에 도달하면 조건이 만족된다"), Condition->IsSatisfied());
	TestTrue(
		TEXT("조건이 수신체의 최소 채널 설정을 반영한 빨강 목표색을 사용한다"),
		Condition->RequiredPaintTint.Equals(
			FLinearColor(1.0f, 0.15f, 0.15f, 1.0f),
			KINDA_SMALL_NUMBER));

	Receiver->ClearColorExposures();
	Receiver->ReceiveLightExposure_Implementation(MakeExposure(GreenSource, FLinearColor::Green));
	TestFalse(TEXT("물감색이 초록으로 바뀌면 빨강 조건이 해제된다"), Condition->IsSatisfied());

	Condition->bLatchOnceSatisfied = true;
	Receiver->ClearColorExposures();
	Receiver->ReceiveLightExposure_Implementation(MakeExposure(RedSource, FLinearColor::Red));
	TestTrue(TEXT("래치 조건도 빨강에서 만족된다"), Condition->IsSatisfied());
	Receiver->ClearColorExposures();
	Receiver->ReceiveLightExposure_Implementation(MakeExposure(GreenSource, FLinearColor::Green));
	TestTrue(TEXT("래치를 켜면 다른 색으로 바뀌어도 만족 상태를 유지한다"), Condition->IsSatisfied());

	return true;
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
	TestNotNull(
		TEXT("회전 거울에 조작 컴포넌트가 존재한다"),
		MirrorDefaults != nullptr ? MirrorDefaults->RotatableMirror.Get() : nullptr);
	if (MirrorDefaults == nullptr || MirrorDefaults->LightInteractionSurface == nullptr ||
		MirrorDefaults->RotatableMirror == nullptr)
	{
		return false;
	}

	TestEqual(
		TEXT("회전 거울은 Box 옆면 HitNormal 대신 컴포넌트 앞 방향을 반사 법선으로 사용한다"),
		MirrorDefaults->LightInteractionSurface->ReflectionNormalMode,
		EUOULightReflectionNormalMode::ComponentForward);
	TestEqual(
		TEXT("회전 거울은 기울어진 초기 배치와 조작 회전축을 분리한다"),
		MirrorDefaults->RotatableMirror->RotationAxisMode,
		EUOURotatableMirrorAxisMode::WorldUp);
	TestEqual(
		TEXT("회전 거울은 빛 단면이 20%까지 밖으로 나와도 반사를 시작한다"),
		MirrorDefaults->LightInteractionSurface->BeamFootprintOverflowAllowancePercent,
		20.0f);
	TestEqual(
		TEXT("회전 거울은 반사 중이면 자동으로 10% 더 돌출을 허용한다"),
		MirrorDefaults->LightInteractionSurface->GetRetainedBeamFootprintCoverageRatio(),
		0.7f);
	TestEqual(
		TEXT("회전 거울은 89도까지 새 반사를 시작한다"),
		MirrorDefaults->LightInteractionSurface->MaximumReflectionIncidenceAngle,
		89.0f);
	TestEqual(
		TEXT("회전 거울은 반사 중이면 95도까지 반사를 유지한다"),
		MirrorDefaults->LightInteractionSurface->RetainedMaximumReflectionIncidenceAngle,
		95.0f);
	TestTrue(
		TEXT("회전 거울은 빛 중심축이 빗나가도 단면이 걸치면 반사 후보로 사용한다"),
		MirrorDefaults->LightInteractionSurface->bAllowEdgeOnlyCylinderReflection);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOURotatableMirrorTiltedWorldUpRotationTest,
	"UnderOneUmbrella.Light.Reflection.RotatableMirrorTiltedWorldUpRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOURotatableMirrorTiltedWorldUpRotationTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOURotatableMirrorTiltedWorldUpWorld"),
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
	TestNotNull(TEXT("기울어진 회전 거울 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOURotatableMirrorActor* Mirror = World->SpawnActor<AUOURotatableMirrorActor>();
	TestNotNull(TEXT("기울어진 회전 거울을 생성한다"), Mirror);
	if (Mirror == nullptr || Mirror->MirrorPivot == nullptr || Mirror->RotatableMirror == nullptr)
	{
		return false;
	}
	Mirror->SetActorRotation(FRotator(25.0f, 35.0f, 15.0f));
	if (!Mirror->RotatableMirror->HasBegunPlay())
	{
		Mirror->RotatableMirror->BeginPlay();
	}

	const FQuat InitialRotation = Mirror->MirrorPivot->GetComponentQuat();
	const float AppliedAngle = 40.0f;
	const FQuat ExpectedRotation =
		FQuat(FVector::UpVector, FMath::DegreesToRadians(AppliedAngle)) * InitialRotation;
	Mirror->RotatableMirror->SetMirrorAngle(AppliedAngle);

	TestTrue(
		TEXT("Pitch와 Roll이 있는 거울도 초기 기울기를 유지하며 월드 Z축으로 회전한다"),
		Mirror->MirrorPivot->GetComponentQuat().Equals(ExpectedRotation, 0.001f));

	AActor* Interactor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("회전 입력축을 확인할 상호작용자를 생성한다"), Interactor);
	if (Interactor != nullptr && Mirror->PushHandleRight != nullptr)
	{
		Interactor->SetActorLocation(Mirror->PushHandleRight->GetComponentLocation());
		const FVector InputAxis =
			Mirror->RotatableMirror->GetWorldInputAxisForInteractor(Interactor);
		TestTrue(TEXT("기울어진 거울의 플레이어 입력축은 수평이다"), FMath::IsNearlyZero(InputAxis.Z));
		TestTrue(TEXT("기울어진 거울의 플레이어 입력축은 정규화된다"), InputAxis.IsNormalized());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOURotatableMirrorSurfaceMeshSyncTest,
	"UnderOneUmbrella.Light.Reflection.RotatableMirrorSurfaceMeshSync",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOURotatableMirrorSurfaceMeshSyncTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOURotatableMirrorSurfaceMeshSyncWorld"),
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
	TestNotNull(TEXT("거울 메시 동기화 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOURotatableMirrorActor* Mirror = World->SpawnActor<AUOURotatableMirrorActor>();
	TestNotNull(TEXT("메시 동기화 테스트용 회전 거울을 생성한다"), Mirror);
	if (Mirror == nullptr || Mirror->MirrorMesh == nullptr ||
		Mirror->LightInteractionSurface == nullptr)
	{
		return false;
	}

	Mirror->MirrorMesh->SetRelativeLocation(FVector(3.0f, 7.0f, 11.0f));
	Mirror->MirrorMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f));
	Mirror->MirrorMesh->SetRelativeScale3D(FVector(0.2f, 2.4f, 1.6f));
	Mirror->OnConstruction(Mirror->GetActorTransform());

	const FVector ExpectedExtent(11.0f, 120.0f, 80.0f);
	TestTrue(
		TEXT("반사 판정 박스 크기가 메시 스케일과 두께 여유를 따른다"),
		Mirror->LightInteractionSurface->GetUnscaledBoxExtent().Equals(ExpectedExtent, 0.01f));
	TestTrue(
		TEXT("반사 판정 박스 위치가 메시 위치를 따른다"),
		Mirror->LightInteractionSurface->GetRelativeLocation().Equals(
			Mirror->MirrorMesh->GetRelativeLocation(),
			0.01f));
	TestTrue(
		TEXT("반사 판정 박스 회전이 메시 회전을 따른다"),
		Mirror->LightInteractionSurface->GetRelativeRotation().Equals(
			Mirror->MirrorMesh->GetRelativeRotation(),
			0.01f));
	Mirror->SetReflectionIncidenceAngles(75.0f, 85.0f);
	TestEqual(
		TEXT("거울 액터 BP 함수로 반사 시작 각도를 설정할 수 있다"),
		Mirror->LightInteractionSurface->MaximumReflectionIncidenceAngle,
		75.0f);
	TestEqual(
		TEXT("거울 액터 BP 함수로 반사 유지 각도를 설정할 수 있다"),
		Mirror->LightInteractionSurface->RetainedMaximumReflectionIncidenceAngle,
		85.0f);
	Mirror->SetBeamFootprintOverflowAllowance(40.0f);
	TestEqual(
		TEXT("거울 액터 BP 함수로 빛 단면 돌출 허용량을 설정할 수 있다"),
		Mirror->LightInteractionSurface->BeamFootprintOverflowAllowancePercent,
		40.0f);
	TestEqual(
		TEXT("돌출 허용량 40%는 새 반사에 필요한 포함 비율 60%로 변환된다"),
		Mirror->LightInteractionSurface->GetStartingBeamFootprintCoverageRatio(),
		0.6f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOURotatableMirrorFootprintCoverageTest,
	"UnderOneUmbrella.Light.Reflection.RotatableMirrorFootprintCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOURotatableMirrorFootprintCoverageTest::RunTest(const FString& Parameters)
{
	UUOULightInteractionSurfaceComponent* Surface =
		NewObject<UUOULightInteractionSurfaceComponent>();
	TestNotNull(TEXT("회전 거울 단면 포함 비율 테스트 표면을 생성한다"), Surface);
	if (Surface == nullptr)
	{
		return false;
	}

	Surface->SetBoxExtent(FVector(6.0f, 100.0f, 100.0f));
	Surface->ReflectionFrontNormalMode =
		EUOULightReflectionFrontNormalMode::ComponentForward;
	Surface->bRequireFullBeamFootprint = true;
	const float BeamRadius = 50.0f;
	const FVector CenterImpact = FVector::ZeroVector;
	const FVector ModeratelyObliqueDirection =
		FRotator(0.0f, 68.0f, 0.0f).RotateVector(FVector::ForwardVector);
	const FVector RetainedOnlyDirection =
		FRotator(0.0f, 72.0f, 0.0f).RotateVector(FVector::ForwardVector);
	const FVector StronglyObliqueDirection =
		FRotator(0.0f, 76.0f, 0.0f).RotateVector(FVector::ForwardVector);

	TestTrue(
		TEXT("거울을 충분히 비추는 68도 입사광은 80% 시작 기준을 만족한다"),
		Surface->HasMinimumBeamFootprintCoverage(
			BeamRadius,
			ModeratelyObliqueDirection,
			FVector::ForwardVector,
			CenterImpact,
			0.8f));
	TestTrue(
		TEXT("조금 더 삐져나온 72도 입사광은 70% 유지 기준을 만족한다"),
		Surface->HasMinimumBeamFootprintCoverage(
			BeamRadius,
			RetainedOnlyDirection,
			FVector::ForwardVector,
			CenterImpact,
			0.7f));
	TestFalse(
		TEXT("72도 입사광은 새 반사를 시작하는 80% 기준은 만족하지 않는다"),
		Surface->HasMinimumBeamFootprintCoverage(
			BeamRadius,
			RetainedOnlyDirection,
			FVector::ForwardVector,
			CenterImpact,
			0.8f));
	TestFalse(
		TEXT("더 회전해 거울 밖으로 많이 벗어난 76도 입사광은 유지 기준도 만족하지 않는다"),
		Surface->HasMinimumBeamFootprintCoverage(
			BeamRadius,
			StronglyObliqueDirection,
			FVector::ForwardVector,
			CenterImpact,
			0.7f));
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
	FUOUConeBeamFixedWidthTest,
	"UnderOneUmbrella.Light.Visual.ConeBeamFixedWidth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUConeBeamFixedWidthTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOUConeBeamFixedWidthWorld"),
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
	TestNotNull(TEXT("Create a world for the fixed cone width test"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOULightSourceActor* SourceActor = World->SpawnActor<AUOULightSourceActor>();
	TestNotNull(TEXT("Spawn a light source for the fixed cone width test"), SourceActor);
	if (SourceActor == nullptr || SourceActor->ExposureSource == nullptr ||
		SourceActor->BeamVisual == nullptr)
	{
		return false;
	}

	FUOULightPathSegmentData DirectSegment;
	DirectSegment.Direction = FVector::ForwardVector;
	DirectSegment.Length = 200.0f;
	DirectSegment.End = DirectSegment.Start + DirectSegment.Direction * DirectSegment.Length;
	DirectSegment.StartRadius = 0.0f;
	DirectSegment.EndRadius = 50.0f;
	DirectSegment.ConeAngle = 35.0f;

	FUOULightPathSegmentData ReflectedSegment;
	ReflectedSegment.SegmentIndex = 1;
	ReflectedSegment.bReflected = true;
	ReflectedSegment.Start = DirectSegment.End;
	ReflectedSegment.Direction = FVector::RightVector;
	ReflectedSegment.Length = 400.0f;
	ReflectedSegment.End = ReflectedSegment.Start +
		ReflectedSegment.Direction * ReflectedSegment.Length;
	ReflectedSegment.StartRadius = 50.0f;
	ReflectedSegment.EndRadius = 50.0f;
	ReflectedSegment.ConeAngle = 0.0f;

	FUOULightPathData Path;
	Path.Segments = {DirectSegment, ReflectedSegment};
	SourceActor->ExposureSource->LightPaths = {Path};
	SourceActor->BeamVisual->VFXActorClass = AUOULightBeamMeshVisualActor::StaticClass();
	SourceActor->BeamVisual->EndPadding = 0.0f;
	SourceActor->BeamVisual->RefreshVisuals();

	TArray<FVector> BeamScales;
	for (TActorIterator<AUOULightBeamMeshVisualActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		if (ActorIt->GetOwner() == SourceActor && ActorIt->BeamMeshComponent != nullptr)
		{
			BeamScales.Add(ActorIt->BeamMeshComponent->GetComponentScale());
		}
	}

	TestEqual(TEXT("Create direct cone and reflected cylinder VFX actors"), BeamScales.Num(), 2);
	if (BeamScales.Num() == 2)
	{
		TestTrue(
			TEXT("Direct cone and reflected cylinder VFX share the same X scale"),
			FMath::IsNearlyEqual(BeamScales[0].X, BeamScales[1].X));
		TestTrue(
			TEXT("Direct cone and reflected cylinder VFX share the same Y scale"),
			FMath::IsNearlyEqual(BeamScales[0].Y, BeamScales[1].Y));
		TestFalse(
			TEXT("Direct cone and reflected cylinder VFX keep independent Z length scales"),
			FMath::IsNearlyEqual(BeamScales[0].Z, BeamScales[1].Z));
	}

	DirectSegment.Length = 100.0f;
	DirectSegment.End = DirectSegment.Start + DirectSegment.Direction * DirectSegment.Length;
	Path.Segments = {DirectSegment};
	SourceActor->ExposureSource->LightPaths = {Path};
	SourceActor->BeamVisual->RefreshVisuals();

	const AUOULightBeamMeshVisualActor* ShortenedDirectVFX = nullptr;
	for (TActorIterator<AUOULightBeamMeshVisualActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		if (ActorIt->GetOwner() == SourceActor && !ActorIt->IsHidden())
		{
			ShortenedDirectVFX = *ActorIt;
			break;
		}
	}
	TestNotNull(TEXT("Keep the direct cone VFX active after shortening"), ShortenedDirectVFX);
	if (ShortenedDirectVFX != nullptr && ShortenedDirectVFX->BeamMeshComponent != nullptr &&
		BeamScales.Num() == 2)
	{
		const FVector ShortenedScale =
			ShortenedDirectVFX->BeamMeshComponent->GetComponentScale();
		TestTrue(
			TEXT("Shortening a cone path preserves its X scale"),
			FMath::IsNearlyEqual(ShortenedScale.X, BeamScales[0].X));
		TestTrue(
			TEXT("Shortening a cone path preserves its Y scale"),
			FMath::IsNearlyEqual(ShortenedScale.Y, BeamScales[0].Y));
		TestTrue(
			TEXT("Shortening a cone path changes only its Z scale"),
			!FMath::IsNearlyEqual(ShortenedScale.Z, BeamScales[0].Z) &&
			!FMath::IsNearlyEqual(ShortenedScale.Z, BeamScales[1].Z));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULumenDynamicRaySegmentScaleTest,
	"UnderOneUmbrella.Light.Visual.LumenDynamicRaySegmentScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULumenDynamicRaySegmentScaleTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOULumenDynamicRaySegmentScaleWorld"),
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
	TestNotNull(TEXT("Create a world for the Dynamic Ray segment scale test"), World);
	if (World == nullptr)
	{
		return false;
	}

	auto FindFirstVisibleLayer = [](AUOULumenDynamicRayVisualActor* VisualActor)
		-> UStaticMeshComponent*
	{
		if (VisualActor == nullptr)
		{
			return nullptr;
		}
		TInlineComponentArray<UStaticMeshComponent*> Components(VisualActor);
		for (UStaticMeshComponent* Component : Components)
		{
			if (Component != nullptr && Component->IsVisible())
			{
				return Component;
			}
		}
		return nullptr;
	};

	AUOULumenDynamicRayVisualActor* DirectVisual =
		World->SpawnActor<AUOULumenDynamicRayVisualActor>();
	AUOULumenDynamicRayVisualActor* ReflectionVisual =
		World->SpawnActor<AUOULumenDynamicRayVisualActor>();
	TestNotNull(TEXT("Spawn a direct Dynamic Ray visual"), DirectVisual);
	TestNotNull(TEXT("Spawn a reflected Dynamic Ray visual"), ReflectionVisual);
	if (DirectVisual == nullptr || ReflectionVisual == nullptr)
	{
		return false;
	}

	FUOULightBeamVisualSegmentData DirectSegment;
	DirectSegment.Direction = FVector::ForwardVector;
	DirectSegment.Length = 300.0f;
	DirectSegment.StartRadius = 20.0f;
	DirectSegment.EndRadius = 80.0f;
	DirectSegment.Intensity = 1.0f;
	DirectVisual->ApplyLightBeamSegment_Implementation(DirectSegment);

	UStaticMeshComponent* DirectLayer = FindFirstVisibleLayer(DirectVisual);
	TestNotNull(TEXT("Activate a Dynamic Ray layer for the direct segment"), DirectLayer);
	if (DirectLayer == nullptr)
	{
		return false;
	}
	const FVector InitialDirectScale = DirectLayer->GetRelativeScale3D();

	DirectSegment.Length = 150.0f;
	DirectSegment.EndRadius = 40.0f;
	DirectVisual->ApplyLightBeamSegment_Implementation(DirectSegment);
	const FVector ShortenedDirectScale = DirectLayer->GetRelativeScale3D();
	TestFalse(
		TEXT("Changing a Dynamic Ray radius updates its X scale"),
		FMath::IsNearlyEqual(ShortenedDirectScale.X, InitialDirectScale.X));
	TestFalse(
		TEXT("Changing a Dynamic Ray radius updates its Y scale"),
		FMath::IsNearlyEqual(ShortenedDirectScale.Y, InitialDirectScale.Y));
	TestFalse(
		TEXT("Shortening a Dynamic Ray changes its Z scale"),
		FMath::IsNearlyEqual(ShortenedDirectScale.Z, InitialDirectScale.Z));

	FUOULightBeamVisualSegmentData ReflectionSegment = DirectSegment;
	ReflectionSegment.bReflected = true;
	ReflectionSegment.Length = 450.0f;
	ReflectionSegment.EndRadius = 120.0f;
	ReflectionVisual->ApplyLightBeamSegment_Implementation(ReflectionSegment);
	UStaticMeshComponent* ReflectionLayer = FindFirstVisibleLayer(ReflectionVisual);
	TestNotNull(TEXT("Activate a Dynamic Ray layer for the reflected segment"), ReflectionLayer);
	if (ReflectionLayer != nullptr)
	{
		const FVector ReflectionScale = ReflectionLayer->GetRelativeScale3D();
		TestFalse(
			TEXT("Reflected Dynamic Ray uses its own X scale"),
			FMath::IsNearlyEqual(ReflectionScale.X, ShortenedDirectScale.X));
		TestFalse(
			TEXT("Reflected Dynamic Ray uses its own Y scale"),
			FMath::IsNearlyEqual(ReflectionScale.Y, ShortenedDirectScale.Y));
		TestFalse(
			TEXT("Reflected Dynamic Ray keeps its own Z length scale"),
			FMath::IsNearlyEqual(ReflectionScale.Z, ShortenedDirectScale.Z));
	}

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
	const FVector ParallelIncomingDirection =
		-FRotator(0.0f, 90.0f, 0.0f).RotateVector(FrontNormal);
	const FVector BackfaceToleranceBoundaryDirection =
		-FRotator(0.0f, 95.0f, 0.0f).RotateVector(FrontNormal);
	const FVector OutsideBackfaceToleranceDirection =
		-FRotator(0.0f, 96.0f, 0.0f).RotateVector(FrontNormal);

	TestEqual(
		TEXT("우산 기본 최대 반사 입사각은 95도이다"),
		UmbrellaSettings->MaximumUmbrellaReflectionIncidenceAngle,
		95.0f);
	TestFalse(
		TEXT("우산 반사면과 평행한 90도 입사광은 반사하므로 통과하지 않는다"),
		Surface->ShouldPassThroughIncomingLight(ParallelIncomingDirection, FrontNormal));
	TestFalse(
		TEXT("우산 뒷면 쪽 5도인 입사광은 허용 경계이므로 통과하지 않는다"),
		Surface->ShouldPassThroughIncomingLight(BackfaceToleranceBoundaryDirection, FrontNormal));
	TestTrue(
		TEXT("우산 뒷면 쪽 6도인 입사광은 허용 범위를 벗어나므로 통과한다"),
		Surface->ShouldPassThroughIncomingLight(OutsideBackfaceToleranceDirection, FrontNormal));
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

	ShadeVolume->MaximumBlockingIncidenceAngle = 95.0f;
	const FVector ShadeFrontNormal = FVector::UpVector;
	const FVector ParallelIncomingDirection =
		-FRotator(0.0f, 90.0f, 0.0f).RotateVector(FVector::ForwardVector);
	const FVector BackfaceToleranceBoundaryDirection = -(
		ShadeFrontNormal * FMath::Cos(FMath::DegreesToRadians(95.0f)) +
		FVector::ForwardVector * FMath::Sin(FMath::DegreesToRadians(95.0f)));
	const FVector OutsideBackfaceToleranceDirection = -(
		ShadeFrontNormal * FMath::Cos(FMath::DegreesToRadians(96.0f)) +
		FVector::ForwardVector * FMath::Sin(FMath::DegreesToRadians(96.0f)));
	TestTrue(
		TEXT("반사 상태에서는 우산 차단면과 평행한 빛도 그늘 범위가 차단한다"),
		ShadeVolume->CanShadeIncomingLight(ParallelIncomingDirection));
	TestTrue(
		TEXT("반사 상태에서는 우산 뒷면 쪽 5도인 빛도 허용 경계로 차단한다"),
		ShadeVolume->CanShadeIncomingLight(BackfaceToleranceBoundaryDirection));
	TestFalse(
		TEXT("반사 상태에서도 우산 뒷면 쪽 6도인 빛은 그늘 범위를 통과한다"),
		ShadeVolume->CanShadeIncomingLight(OutsideBackfaceToleranceDirection));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaParallelCylinderEdgeReflectionTest,
	"UnderOneUmbrella.Light.Reflection.UmbrellaParallelCylinderEdgeOverlap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaParallelCylinderEdgeReflectionTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOUUmbrellaParallelCylinderEdgeWorld"),
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
	TestNotNull(TEXT("평행 원기둥 반사 테스트용 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOULightSourceActor* SourceActor = World->SpawnActor<AUOULightSourceActor>();
	TestNotNull(TEXT("원기둥 광원을 생성한다"), SourceActor);
	if (SourceActor == nullptr || SourceActor->ExposureSource == nullptr ||
		SourceActor->SourceSpotLight == nullptr)
	{
		return false;
	}

	SourceActor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	SourceActor->SourceSpotLight->SetAttenuationRadius(1000.0f);
	SourceActor->ExposureSource->BeamShape = EUOULightBeamShape::Cylinder;
	SourceActor->ExposureSource->CylinderRadius = 50.0f;
	SourceActor->ExposureSource->BeamLength = 1000.0f;
	SourceActor->ExposureSource->SampleInterval = 0.0f;
	SourceActor->ExposureSource->ReflectionPathLossGraceTime = 0.0f;

	AActor* UmbrellaActor = World->SpawnActor<AActor>();
	UUOULightInteractionSurfaceComponent* UmbrellaSurface = UmbrellaActor != nullptr
		? NewObject<UUOULightInteractionSurfaceComponent>(UmbrellaActor, TEXT("ParallelUmbrellaSurface"))
		: nullptr;
	TestNotNull(TEXT("광선과 평행한 우산 반사면을 생성한다"), UmbrellaSurface);
	if (UmbrellaActor == nullptr || UmbrellaSurface == nullptr)
	{
		return false;
	}

	UmbrellaActor->AddInstanceComponent(UmbrellaSurface);
	UmbrellaActor->SetRootComponent(UmbrellaSurface);
	UmbrellaSurface->SetBoxExtent(FVector(6.0f, 70.0f, 70.0f));
	UmbrellaSurface->bUseSurfaceAreaSampling = true;
	UmbrellaSurface->bReflectFrontFaceOnly = true;
	UmbrellaSurface->ReflectionFrontNormalMode =
		EUOULightReflectionFrontNormalMode::ComponentForward;
	UmbrellaSurface->ReflectionDirectionMode = EUOULightReflectionDirectionMode::OwnerForward;
	UmbrellaSurface->MaximumReflectionIncidenceAngle = 95.0f;
	UmbrellaSurface->bAllowEdgeOnlyCylinderReflection = true;
	UmbrellaSurface->RegisterComponent();
	UmbrellaActor->SetActorLocationAndRotation(
		FVector(400.0f, 45.0f, 0.0f),
		FRotator(0.0f, 90.0f, 0.0f));
	UmbrellaSurface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);

	UUOUUmbrellaLightShadeVolumeComponent* UmbrellaShade =
		NewObject<UUOUUmbrellaLightShadeVolumeComponent>(UmbrellaActor, TEXT("ParallelUmbrellaShade"));
	TestNotNull(TEXT("가장자리 반사 우산의 직접광 차단 볼륨을 생성한다"), UmbrellaShade);
	if (UmbrellaShade == nullptr)
	{
		return false;
	}
	UmbrellaActor->AddInstanceComponent(UmbrellaShade);
	UmbrellaShade->SetupAttachment(UmbrellaSurface);
	UmbrellaShade->SetBoxExtent(FVector(6.0f, 70.0f, 70.0f));
	UmbrellaShade->MaximumBlockingIncidenceAngle = 180.0f;
	UmbrellaShade->RegisterComponent();
	UmbrellaShade->SetShadeEnabled(true);

	SourceActor->ExposureSource->EmitLight(0.1f);
	const TArray<FUOULightReflectionPathData> ReflectionPaths =
		SourceActor->ExposureSource->GetReflectionPaths();
	TestTrue(
		TEXT("중심축이 빗나가도 원기둥 빛 단면이 평행한 우산에 걸치면 반사한다"),
		!ReflectionPaths.IsEmpty());
	if (!ReflectionPaths.IsEmpty() && !ReflectionPaths[0].Segments.IsEmpty())
	{
		const FUOULightReflectionSegmentData& Segment = ReflectionPaths[0].Segments[0];
		TestEqual(
			TEXT("가장자리 겹침 반사의 반사체는 우산 표면이다"),
			Segment.Reflector.Get(),
			UmbrellaSurface);
		TestTrue(
			TEXT("평행 입사광은 우산 소유 액터의 전방으로 반사된다"),
			Segment.ReflectedDirection.Equals(FVector::RightVector, 0.01f));
	}
	const TArray<FUOULightPathData> EdgeReflectionLightPaths =
		SourceActor->ExposureSource->GetLightPaths();
	TestTrue(TEXT("가장자리 반사의 통합 빛 경로를 생성한다"), !EdgeReflectionLightPaths.IsEmpty());
	if (!EdgeReflectionLightPaths.IsEmpty() && !EdgeReflectionLightPaths[0].Segments.IsEmpty())
	{
		const FUOULightPathSegmentData& DirectSegment = EdgeReflectionLightPaths[0].Segments[0];
		TestTrue(
			TEXT("원기둥 직접광은 가장자리 우산 충돌점 쪽으로 꺾이지 않고 광원 Forward를 유지한다"),
			DirectSegment.Direction.Equals(FVector::ForwardVector, 0.01f));
		TestTrue(
			TEXT("원기둥 직접광 중심선은 광원에서 직선으로 진행한다"),
			FMath::IsNearlyZero(DirectSegment.End.Y, 0.01f));
	}

	AActor* DisconnectedMirrorActor = World->SpawnActor<AActor>();
	UUOULightInteractionSurfaceComponent* DisconnectedMirrorSurface =
		DisconnectedMirrorActor != nullptr
			? NewObject<UUOULightInteractionSurfaceComponent>(
				DisconnectedMirrorActor,
				TEXT("DisconnectedMirrorSurface"))
			: nullptr;
	TestNotNull(TEXT("우산 뒤 연결되지 않은 거울을 생성한다"), DisconnectedMirrorSurface);
	if (DisconnectedMirrorActor == nullptr || DisconnectedMirrorSurface == nullptr)
	{
		return false;
	}
	DisconnectedMirrorActor->AddInstanceComponent(DisconnectedMirrorSurface);
	DisconnectedMirrorActor->SetRootComponent(DisconnectedMirrorSurface);
	DisconnectedMirrorSurface->SetBoxExtent(FVector(5.0f, 10.0f, 70.0f));
	DisconnectedMirrorSurface->bUseSurfaceAreaSampling = false;
	DisconnectedMirrorSurface->bReflectFrontFaceOnly = false;
	DisconnectedMirrorSurface->ReflectionNormalMode =
		EUOULightReflectionNormalMode::ComponentForward;
	DisconnectedMirrorSurface->RegisterComponent();
	DisconnectedMirrorActor->SetActorLocation(FVector(650.0f, 0.0f, 0.0f));
	DisconnectedMirrorSurface->SetLightInteractionMode(EUOULightInteractionMode::Reflecting);

	SourceActor->ExposureSource->EmitLight(0.1f);
	bool bFoundDisconnectedMirrorReflection = false;
	for (const FUOULightReflectionPathData& Path : SourceActor->ExposureSource->GetReflectionPaths())
	{
		for (const FUOULightReflectionSegmentData& Segment : Path.Segments)
		{
			bFoundDisconnectedMirrorReflection |= Segment.Reflector == DisconnectedMirrorSurface;
		}
	}
	TestFalse(
		TEXT("우산에서 거울로 이어지는 구간이 없으면 거울 반사가 독립적으로 시작되지 않는다"),
		bFoundDisconnectedMirrorReflection);

	AActor* ReceiverActor = World->SpawnActor<AActor>();
	UBoxComponent* ReceiverBox = ReceiverActor != nullptr
		? NewObject<UBoxComponent>(ReceiverActor, TEXT("BehindUmbrellaReceiverBox"))
		: nullptr;
	UUOULightExposureReceiverComponent* Receiver = ReceiverActor != nullptr
		? NewObject<UUOULightExposureReceiverComponent>(ReceiverActor, TEXT("BehindUmbrellaReceiver"))
		: nullptr;
	TestNotNull(TEXT("우산 뒤 직접광 수신 액터를 생성한다"), ReceiverActor);
	TestNotNull(TEXT("우산 뒤 직접광 수신 볼륨을 생성한다"), ReceiverBox);
	TestNotNull(TEXT("우산 뒤 직접광 수신 컴포넌트를 생성한다"), Receiver);
	if (ReceiverActor == nullptr || ReceiverBox == nullptr || Receiver == nullptr)
	{
		return false;
	}

	ReceiverActor->AddInstanceComponent(ReceiverBox);
	ReceiverActor->SetRootComponent(ReceiverBox);
	ReceiverBox->SetBoxExtent(FVector(10.0f));
	ReceiverBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ReceiverBox->SetCollisionObjectType(ECC_WorldDynamic);
	ReceiverBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ReceiverBox->RegisterComponent();
	ReceiverActor->SetActorLocation(FVector(700.0f, -40.0f, 0.0f));
	ReceiverActor->AddInstanceComponent(Receiver);
	Receiver->bUseReceiverVolumeSampling = false;
	Receiver->RegisterComponent();

	SourceActor->ExposureSource->EmitLight(0.1f);
	TestFalse(
		TEXT("우산 가장자리에서 반사가 성립하면 우산을 비껴간 직접광 샘플도 뒤쪽 수신체에 전달되지 않는다"),
		Receiver->IsReceivingLight());
	return true;
}

#endif
