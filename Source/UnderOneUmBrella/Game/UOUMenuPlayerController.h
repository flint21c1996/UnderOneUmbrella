// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UOUMenuPlayerController.generated.h"

class UUserWidget;
class UWorld;

// 타이틀과 인게임에서 공통으로 쓰는 메뉴 계층 컨트롤러입니다.
// 설정창 생성, 입력 모드 전환, 타이틀 복귀 같은 UI 흐름을 여기서 관리합니다.
UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUOUMenuPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void OpenSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void CloseSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void ToggleSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void ReturnToTitle();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings", meta = (DisplayName = "현재 스테이지 재시작", ToolTip = "현재 플레이 중인 스테이지를 다시 시작합니다."))
	void RestartCurrentStage();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void GoToNextLevel();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void GoToPreviousLevel();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void ToggleTestSetting();

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	bool IsSettingsMenuOpen() const { return bSettingsMenuOpen; }

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	bool IsTestSettingEnabled() const { return CanRestartCurrentStage(); }

	UFUNCTION(BlueprintPure, Category = "Menu|Settings", meta = (DisplayName = "현재 스테이지 재시작 가능"))
	bool CanRestartCurrentStage() const { return bCanRestartCurrentStage; }

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	bool CanReturnToTitle() const { return bCanReturnToTitle; }

protected:
	void SetCanReturnToTitle(bool bInCanReturnToTitle) { bCanReturnToTitle = bInCanReturnToTitle; }
	void SetCanRestartCurrentStage(bool bInCanRestartCurrentStage) { bCanRestartCurrentStage = bInCanRestartCurrentStage; }

	virtual void ApplySettingsMenuInputMode(UUserWidget* InSettingsMenuWidget);

	// 설정창을 닫은 뒤 돌아갈 입력 상태는 화면마다 다르므로 필요하면 하위 클래스에서 재정의합니다.
	virtual void RestoreInputModeAfterSettingsMenu();

private:
	// 설정창 BP는 여러 화면에서 재사용하므로 config에서 경로를 바꿀 수 있게 둡니다.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Settings")
	TSoftClassPtr<UUserWidget> SettingsMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Settings")
	TSoftObjectPtr<UWorld> TitleLevel;

	UPROPERTY()
	TObjectPtr<UUserWidget> SettingsMenuWidget;

	bool bSettingsMenuOpen = false;
	bool bCanRestartCurrentStage = false;
	bool bCanReturnToTitle = false;
};
