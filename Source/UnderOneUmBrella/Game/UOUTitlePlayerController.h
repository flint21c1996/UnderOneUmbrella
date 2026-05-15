// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUMenuPlayerController.h"
#include "UOUTitlePlayerController.generated.h"

class UWorld;
class UUserWidget;

// 타이틀 화면의 메뉴 생성, 시작/종료 입력, 설정창 토글을 담당합니다.
UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUTitlePlayerController : public AUOUMenuPlayerController
{
	GENERATED_BODY()

public:
	AUOUTitlePlayerController();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void QuitGame();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void RestoreInputModeAfterSettingsMenu() override;

private:
	void ApplyTitleMenuInputMode();

	// Start 버튼을 눌렀을 때 넘어갈 임시 플레이 맵입니다.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Title")
	TSoftObjectPtr<UWorld> TestLevel;

	// 타이틀 화면에 띄울 WBP_TitleMenu 클래스입니다.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Title")
	TSoftClassPtr<UUserWidget> TitleMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> TitleMenuWidget;

	// OpenLevel 요청이 여러 번 들어가는 것을 막기 위한 간단한 가드입니다.
	bool bIsOpeningLevel = false;
};
