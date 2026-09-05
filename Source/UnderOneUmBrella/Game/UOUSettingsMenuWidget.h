// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Audio/UOUAudioTypes.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UOUSettingsMenuWidget.generated.h"

class AUOUMenuPlayerController;
class UUOUAudioSubsystem;
class USlider;

// 설정창 BP가 호출할 얇은 연결용 위젯 클래스입니다.
// 실제 메뉴 상태와 레벨 이동은 Owning Player의 PlayerController가 담당합니다.
UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUSettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void CloseSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ReturnToTitle();

	UFUNCTION(BlueprintCallable, Category = "Settings", meta = (DisplayName = "스테이지 선택 화면으로"))
	void OpenStageSelect();

	UFUNCTION(BlueprintCallable, Category = "Settings", meta = (DisplayName = "현재 스테이지 재시작", ToolTip = "설정창에서 현재 스테이지를 다시 시작합니다."))
	void RestartCurrentStage();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void GoToNextLevel();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void GoToPreviousLevel();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ToggleTestSetting();

	UFUNCTION(BlueprintPure, Category = "Settings", meta = (DisplayName = "현재 스테이지 재시작 가능"))
	bool CanRestartCurrentStage() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	bool IsTestSettingEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Settings")
	bool CanReturnToTitle() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio", meta = (DisplayName = "오디오 볼륨 설정", ClampMin = "0.0", ClampMax = "1.0"))
	void SetAudioVolume(EUOUAudioCategory Category, float Volume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio", meta = (DisplayName = "오디오 볼륨 가져오기"))
	float GetAudioVolume(EUOUAudioCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio", meta = (DisplayName = "오디오 최종 볼륨 가져오기"))
	float GetEffectiveAudioVolume(EUOUAudioCategory Category) const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio", meta = (DisplayName = "오디오 설정 저장"))
	void SaveAudioSettings();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 위젯은 상태를 직접 들고 있지 않고 메뉴 컨트롤러로 위임합니다.
	AUOUMenuPlayerController* GetMenuPlayerController() const;
	UUOUAudioSubsystem* GetAudioSubsystem() const;

	void BindAudioSliders();
	void InitializeAudioSliderValues();
	void ConfigureAudioSlider(USlider* Slider) const;
	void SetAudioSliderValue(USlider* Slider, EUOUAudioCategory Category);
	void HandleAudioSliderChanged(EUOUAudioCategory Category, float Value);

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleBGMVolumeChanged(float Value);

	UFUNCTION()
	void HandleSFXVolumeChanged(float Value);

	UFUNCTION()
	void HandleUIVolumeChanged(float Value);

	UFUNCTION()
	void HandleAmbienceVolumeChanged(float Value);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MasterVolumeSlider = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> BGMVolumeSlider = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> SFXVolumeSlider = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> UIVolumeSlider = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> AmbienceVolumeSlider = nullptr;

	bool bUpdatingAudioSliderValues = false;
	bool bAudioVolumeDirty = false;
};
