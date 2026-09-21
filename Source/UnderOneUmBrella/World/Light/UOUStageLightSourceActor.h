// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "World/Light/UOULightExposureTypes.h"
#include "UOUStageLightSourceActor.generated.h"

class USceneComponent;
class USpotLightComponent;
class UUOULightExposureSourceComponent;
class UUOULightReflectionSpotLightComponent;
class UUOUStageLightBeamVisualComponent;

// 기존 빛 판정과 반사 조명을 재사용하고, 빛기둥 표현은 별도로 연결하는 무대 광원 액터입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Stage Light Source"))
class UNDERONEUMBRELLA_API AUOUStageLightSourceActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUStageLightSourceActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

	// 실제 조명과 게임플레이 빛 판정을 켜거나 끄고, 경로 변경을 통해 반사 조명에 반영합니다.
	UFUNCTION(BlueprintCallable, Category = "Light|Activation")
	void SetLightEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintCallable, Category = "Light|Activation")
	void EnableLight();

	UFUNCTION(BlueprintCallable, Category = "Light|Activation")
	void DisableLight();

	UFUNCTION(BlueprintCallable, Category = "Light|Activation")
	void ToggleLight();

	// Source SpotLight, 반사 조명, 게임플레이 색상 판정에 같은 색을 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Light|Color")
	void SetSourceLightColor(FLinearColor NewLightColor);

	UFUNCTION(BlueprintCallable, Category = "Light|Color")
	void SetLightColorPreset(EUOULightColorPreset NewPreset);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Color", meta = (ToolTip = "빨강·초록·파랑 또는 물감 제거용 흰색 프리셋을 선택합니다. Source SpotLight 색상 유지는 기존 BP의 조명 색을 덮어쓰지 않습니다."))
	EUOULightColorPreset LightColorPreset = EUOULightColorPreset::UseSourceSpotLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light|Color", meta = (EditCondition = "LightColorPreset == EUOULightColorPreset::Custom", EditConditionHides, ToolTip = "사용자 지정 프리셋에서 적용할 색상입니다."))
	FLinearColor CustomLightColor = FLinearColor::White;

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

	// 기존 빛 경로를 무대 조명 전용 표현 컴포넌트로 전달합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Components")
	TObjectPtr<UUOUStageLightBeamVisualComponent> StageBeamVisual;

protected:
	void ApplyConfiguredLightColor();
	FLinearColor ResolveConfiguredLightColor() const;
};
