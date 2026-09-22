// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOUStageLightSourceActor.h"

#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "World/Light/UOULightExposureSourceComponent.h"
#include "World/Light/UOUStageLightBeamVisualComponent.h"

AUOUStageLightSourceActor::AUOUStageLightSourceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	SourceSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SourceSpotLight"));
	SourceSpotLight->SetupAttachment(RootScene);
	SourceSpotLight->SetMobility(EComponentMobility::Movable);
	SourceSpotLight->SetAttenuationRadius(1000.0f);
	SourceSpotLight->SetInnerConeAngle(20.0f);
	SourceSpotLight->SetOuterConeAngle(35.0f);
	SourceSpotLight->SetIntensity(5000.0f);

	ExposureSource = CreateDefaultSubobject<UUOULightExposureSourceComponent>(TEXT("ExposureSource"));
	ExposureSource->bEnableReflectedLight = false;
	StageBeamVisual = CreateDefaultSubobject<UUOUStageLightBeamVisualComponent>(TEXT("StageBeamVisual"));
}

void AUOUStageLightSourceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyConfiguredLightColor();
}

void AUOUStageLightSourceActor::BeginPlay()
{
	Super::BeginPlay();
	SetLightEnabled(bStartEnabled);
}

void AUOUStageLightSourceActor::ApplyPuzzleResult_Implementation(const EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		EnableLight();
		break;
	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		DisableLight();
		break;
	case EOUUPuzzleResultAction::Toggle:
		ToggleLight();
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

void AUOUStageLightSourceActor::SetLightEnabled(const bool bNewEnabled)
{
	bLightEnabled = bNewEnabled;

	if (SourceSpotLight != nullptr)
	{
		SourceSpotLight->SetVisibility(bLightEnabled);
	}

	if (ExposureSource != nullptr)
	{
		// ExposureSource는 꺼진 다음 Tick에서 경로를 비우고 전용 빔 컴포넌트에 변경을 전파합니다.
		ExposureSource->bEmitLight = bLightEnabled;
	}
}

void AUOUStageLightSourceActor::EnableLight()
{
	SetLightEnabled(true);
}

void AUOUStageLightSourceActor::DisableLight()
{
	SetLightEnabled(false);
}

void AUOUStageLightSourceActor::ToggleLight()
{
	SetLightEnabled(!bLightEnabled);
}

void AUOUStageLightSourceActor::SetSourceLightColor(FLinearColor NewLightColor)
{
	NewLightColor.A = 1.0f;
	if (SourceSpotLight != nullptr)
	{
		SourceSpotLight->SetLightColor(NewLightColor);
	}

	// Construction Script 중에는 컴포넌트 속성만 갱신합니다. 빔 데이터 갱신은
	// 실제 플레이가 시작된 뒤 색상이 변경될 때만 필요합니다.
	if (!HasActorBegunPlay())
	{
		return;
	}

	// 색상 변경 시 경로 변화가 없어도 전용 표현 데이터를 다시 계산합니다.
	if (StageBeamVisual != nullptr)
	{
		StageBeamVisual->RefreshVisuals();
	}
}

void AUOUStageLightSourceActor::SetLightColorPreset(const EUOULightColorPreset NewPreset)
{
	LightColorPreset = NewPreset;
	ApplyConfiguredLightColor();
}

void AUOUStageLightSourceActor::ApplyConfiguredLightColor()
{
	if (LightColorPreset == EUOULightColorPreset::UseSourceSpotLight)
	{
		return;
	}

	SetSourceLightColor(ResolveConfiguredLightColor());
}

FLinearColor AUOUStageLightSourceActor::ResolveConfiguredLightColor() const
{
	switch (LightColorPreset)
	{
	case EUOULightColorPreset::Red:
		return FLinearColor::Red;
	case EUOULightColorPreset::Green:
		return FLinearColor::Green;
	case EUOULightColorPreset::Blue:
		return FLinearColor::Blue;
	case EUOULightColorPreset::White:
		return FLinearColor::White;
	case EUOULightColorPreset::Custom:
		return CustomLightColor;
	case EUOULightColorPreset::UseSourceSpotLight:
	default:
		return SourceSpotLight != nullptr
			? SourceSpotLight->GetLightColor()
			: FLinearColor::White;
	}
}
