// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOULightSourceActor.generated.h"

class USceneComponent;
class USpotLightComponent;
class UUOULightBeamVisualComponent;
class UUOULightExposureSourceComponent;
class UUOULightReflectionSpotLightComponent;

// 실제 SpotLight, 게임플레이 빛 판정, 반사 조명과 VFX 출력을 하나로 묶은 기본 광원 액터입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Light Source"))
class UNDERONEUMBRELLA_API AUOULightSourceActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOULightSourceActor();
	virtual void BeginPlay() override;
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	// 실제 조명, 게임플레이 빛 판정, 빛줄기 연출을 한 번에 켜거나 끕니다.
	UFUNCTION(BlueprintCallable, Category = "Light|Activation")
	void SetLightEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintCallable, Category = "Light|Activation")
	void EnableLight();

	UFUNCTION(BlueprintCallable, Category = "Light|Activation")
	void DisableLight();

	UFUNCTION(BlueprintCallable, Category = "Light|Activation")
	void ToggleLight();

	// 게임 시작 시 광원이 켜져 있을지 정합니다. 버튼으로 켜는 광원은 끄고 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Activation", meta = (DisplayName = "게임 시작 시 활성화"))
	bool bStartEnabled = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Activation|Runtime", meta = (DisplayName = "현재 활성화"))
	bool bLightEnabled = true;

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
