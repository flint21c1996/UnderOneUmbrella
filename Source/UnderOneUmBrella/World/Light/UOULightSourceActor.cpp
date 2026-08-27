// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightSourceActor.h"

#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "World/Light/UOULightBeamMeshVisualActor.h"
#include "World/Light/UOULightBeamVisualComponent.h"
#include "World/Light/UOULightExposureSourceComponent.h"
#include "World/Light/UOULightReflectionSpotLightComponent.h"

AUOULightSourceActor::AUOULightSourceActor()
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
	ReflectionSpotLights = CreateDefaultSubobject<UUOULightReflectionSpotLightComponent>(TEXT("ReflectionSpotLights"));
	BeamVisual = CreateDefaultSubobject<UUOULightBeamVisualComponent>(TEXT("BeamVisual"));
	BeamVisual->VFXActorClass = AUOULightBeamMeshVisualActor::StaticClass();
}

void AUOULightSourceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyConfiguredLightColor();
}

void AUOULightSourceActor::BeginPlay()
{
	Super::BeginPlay();
	SetLightEnabled(bStartEnabled);
}

void AUOULightSourceActor::ApplyPuzzleResult_Implementation(const EOUUPuzzleResultAction Action)
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

void AUOULightSourceActor::SetLightEnabled(const bool bNewEnabled)
{
	bLightEnabled = bNewEnabled;

	if (SourceSpotLight != nullptr)
	{
		SourceSpotLight->SetVisibility(bLightEnabled);
	}

	if (ExposureSource != nullptr)
	{
		// ExposureSource는 꺼진 다음 Tick에서 경로를 비우고 빔과 반사광에 변경을 전파합니다.
		ExposureSource->bEmitLight = bLightEnabled;
	}
}

void AUOULightSourceActor::EnableLight()
{
	SetLightEnabled(true);
}

void AUOULightSourceActor::DisableLight()
{
	SetLightEnabled(false);
}

void AUOULightSourceActor::ToggleLight()
{
	SetLightEnabled(!bLightEnabled);
}

void AUOULightSourceActor::SetSourceLightColor(FLinearColor NewLightColor)
{
	NewLightColor.A = 1.0f;
	if (SourceSpotLight != nullptr)
	{
		SourceSpotLight->SetLightColor(NewLightColor);
	}

	// Construction Script 중에는 컴포넌트 속성만 갱신합니다. 런타임 VFX 재생성은
	// 실제 플레이가 시작된 뒤 색상이 변경될 때만 필요합니다.
	if (!HasActorBegunPlay())
	{
		return;
	}

	if (BeamVisual != nullptr)
	{
		BeamVisual->RefreshVisuals();
	}
	if (ReflectionSpotLights != nullptr)
	{
		ReflectionSpotLights->RefreshSpotLights();
	}
}

void AUOULightSourceActor::SetLightColorPreset(const EUOULightColorPreset NewPreset)
{
	LightColorPreset = NewPreset;
	ApplyConfiguredLightColor();
}

void AUOULightSourceActor::ApplyConfiguredLightColor()
{
	if (LightColorPreset == EUOULightColorPreset::UseSourceSpotLight)
	{
		return;
	}

	SetSourceLightColor(ResolveConfiguredLightColor());
}

FLinearColor AUOULightSourceActor::ResolveConfiguredLightColor() const
{
	switch (LightColorPreset)
	{
	case EUOULightColorPreset::Red:
		return FLinearColor::Red;
	case EUOULightColorPreset::Green:
		return FLinearColor::Green;
	case EUOULightColorPreset::Blue:
		return FLinearColor::Blue;
	case EUOULightColorPreset::Custom:
		return CustomLightColor;
	case EUOULightColorPreset::UseSourceSpotLight:
	default:
		return SourceSpotLight != nullptr
			? SourceSpotLight->GetLightColor()
			: FLinearColor::White;
	}
}
