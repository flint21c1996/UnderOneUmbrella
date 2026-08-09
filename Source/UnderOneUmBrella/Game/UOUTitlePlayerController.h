// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "UOUMenuPlayerController.h"
#include "UOUTitlePlayerController.generated.h"

class UWorld;
class UUserWidget;
class AUOULevelTransitionSettingsActor;
class AUOUTitleLevelTransitionActor;

// 타이틀 화면의 메뉴 생성, 시작/종료 입력, 설정창 토글을 담당합니다.
UCLASS(Config=Game)
class UNDERONEUMBRELLA_API AUOUTitlePlayerController : public AUOUMenuPlayerController
{
	GENERATED_BODY()

public:
	AUOUTitlePlayerController();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void StartGame();

	// 이전 WBP_TitleMenu의 이벤트 참조를 유지하기 위한 호환 함수입니다.
	// 타이틀에서는 더 이상 스테이지 선택 화면을 직접 열지 않고 일반 시작 동작을 수행합니다.
	UFUNCTION(BlueprintCallable, Category = "Title", meta = (DisplayName = "플레이 시작"))
	void OpenMapSelect();

	UFUNCTION(BlueprintCallable, Category = "Title")
	void QuitGame();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void RestoreInputModeAfterSettingsMenu() override;

private:
	void ApplyTitleMenuInputMode();
	const AUOULevelTransitionSettingsActor* FindLevelTransitionSettingsActor() const;
	AUOUTitleLevelTransitionActor* FindTitleLevelTransitionActor() const;

	// 시작 버튼을 눌렀을 때 이동할 기본 플레이 레벨입니다.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Title")
	TSoftObjectPtr<UWorld> nextLevel;

	// 타이틀 화면에 띄울 WBP_TitleMenu 클래스입니다.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Title")
	TSoftClassPtr<UUserWidget> TitleMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> TitleMenuWidget;

	// OpenLevel 요청이 여러 번 들어가는 것을 막기 위한 간단한 가드입니다.
	bool bIsOpeningLevel = false;
};
