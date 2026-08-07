// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUMenuPlayerController.h"
#include "UOUPlayerController.generated.h"

class UUserWidget;
class UWorld;

// 실제 플레이 맵에서 사용하는 PlayerController입니다.
// 공통 설정 메뉴 기능을 상속받고, 인게임 HUD를 생성합니다.
UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUPlayerController : public AUOUMenuPlayerController
{
	GENERATED_BODY()

public:
	AUOUPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings", meta = (DisplayName = "스테이지 선택 화면으로"))
	void OpenStageSelect();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void RestoreInputModeAfterSettingsMenu() override;

private:
	void ApplyInGameInputMode();

	UPROPERTY(EditDefaultsOnly, Config, Category = "HUD")
	TSoftClassPtr<UUserWidget> InGameHUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InGameHUDWidget;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Menu|Settings", meta = (AllowedClasses = "/Script/Engine.World", ToolTip = "인게임 설정 메뉴에서 스테이지 선택 버튼을 눌렀을 때 이동할 레벨입니다."))
	TSoftObjectPtr<UWorld> StageSelectLevel;

	bool bIsOpeningStageSelect = false;
};
