// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Audio/UOUAudioTypes.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UOUSettingsMenuWidget.generated.h"

class AUOUMenuPlayerController;
class UUOUAudioSubsystem;

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

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ToggleTestSetting();

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

private:
	// 위젯은 상태를 직접 들고 있지 않고 메뉴 컨트롤러로 위임합니다.
	AUOUMenuPlayerController* GetMenuPlayerController() const;
	UUOUAudioSubsystem* GetAudioSubsystem() const;
};
