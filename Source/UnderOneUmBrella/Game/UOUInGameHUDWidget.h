// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UOUInGameHUDWidget.generated.h"

class AUOUMenuPlayerController;

// 인게임 HUD BP에서 설정 버튼 클릭을 C++ 메뉴 시스템으로 넘기는 연결 클래스입니다.
UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUInGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OpenSettingsMenu();

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsSettingsMenuOpen() const;

private:
	// HUD도 메뉴 상태를 직접 들고 있지 않고 PlayerController에 위임합니다.
	AUOUMenuPlayerController* GetMenuPlayerController() const;
};
