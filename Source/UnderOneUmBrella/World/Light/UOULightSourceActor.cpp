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
