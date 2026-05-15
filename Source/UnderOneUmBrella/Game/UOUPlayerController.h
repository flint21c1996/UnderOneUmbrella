// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUMenuPlayerController.h"
#include "UOUPlayerController.generated.h"

class UUserWidget;

// 실제 플레이 맵에서 사용하는 PlayerController입니다.
// 공통 설정 메뉴 기능을 상속받고, 인게임 HUD를 생성합니다.
UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUPlayerController : public AUOUMenuPlayerController
{
	GENERATED_BODY()

public:
	AUOUPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditDefaultsOnly, Config, Category = "HUD")
	TSoftClassPtr<UUserWidget> InGameHUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InGameHUDWidget;
};
