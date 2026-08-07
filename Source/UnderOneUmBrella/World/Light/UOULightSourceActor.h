// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOULightSourceActor.generated.h"

class USceneComponent;
class USpotLightComponent;
class UUOULightBeamVisualComponent;
class UUOULightExposureSourceComponent;
class UUOULightReflectionSpotLightComponent;

// 실제 SpotLight, 게임플레이 빛 판정, 반사 조명과 VFX 출력을 하나로 묶은 기본 광원 액터입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Light Source"))
class UNDERONEUMBRELLA_API AUOULightSourceActor : public AActor
{
	GENERATED_BODY()

public:
	AUOULightSourceActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Components")
	TObjectPtr<USpotLightComponent> SourceSpotLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Components")
	TObjectPtr<UUOULightExposureSourceComponent> ExposureSource;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Components")
	TObjectPtr<UUOULightReflectionSpotLightComponent> ReflectionSpotLights;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Components")
	TObjectPtr<UUOULightBeamVisualComponent> BeamVisual;
};
