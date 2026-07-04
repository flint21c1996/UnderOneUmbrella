// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UI/UOUTransitionMessageTypes.h"
#include "UOULevelTransitionSettingsActor.generated.h"

class USceneComponent;
class UWorld;

UCLASS(meta = (DisplayName = "UOU Level Transition Settings Actor"))
class UNDERONEUMBRELLA_API AUOULevelTransitionSettingsActor : public AActor
{
	GENERATED_BODY()

public:
	AUOULevelTransitionSettingsActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Transition")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Level Links", meta = (AllowedClasses = "/Script/Engine.World", DisplayName = "다음 레벨", ToolTip = "이 맵에서 Next Level 전환을 요청했을 때 열 레벨입니다. 비워두면 타이틀 레벨로 돌아갑니다."))
	TSoftObjectPtr<UWorld> TargetLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Level Links", meta = (AllowedClasses = "/Script/Engine.World", DisplayName = "이전 레벨", ToolTip = "이 맵에서 Previous Level 전환을 요청했을 때 열 레벨입니다. 비워두면 타이틀 레벨로 돌아갑니다."))
	TSoftObjectPtr<UWorld> PreviousLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Enter", meta = (ClampMin = "0.0", DisplayName = "페이드 인 시간", ToolTip = "다른 맵에서 이 맵으로 들어왔을 때 검은 화면에서 플레이 화면으로 밝아지는 시간입니다."))
	float EnterFadeInDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Enter", meta = (DisplayName = "페이드 인 문구", ToolTip = "다른 맵에서 이 맵으로 들어왔을 때 검은 화면 위에 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FUOUTransitionMessageSettings EnterMessageSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Exit", meta = (ClampMin = "0.0", DisplayName = "페이드 아웃 시간", ToolTip = "이 맵에서 다른 맵으로 떠날 때 플레이 화면이 검은 화면으로 가려지는 시간입니다."))
	float ExitFadeOutDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Exit", meta = (ClampMin = "0.0", DisplayName = "검은 화면 유지 시간", ToolTip = "이 맵을 떠날 때 페이드 아웃 문구가 사라진 뒤 다음 레벨을 열기 전까지 검은 화면만 유지하는 시간입니다."))
	float ExitBlackHoldDuration = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Exit", meta = (DisplayName = "페이드 아웃 문구", ToolTip = "이 맵에서 다른 맵으로 떠날 때 화면이 완전히 검게 가려진 뒤 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FUOUTransitionMessageSettings ExitMessageSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Shared", meta = (DisplayName = "페이드 색상", ToolTip = "이 맵을 떠나거나 이 맵에 들어올 때 사용할 화면 가림 색상입니다."))
	FLinearColor FadeColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Shared", meta = (DisplayName = "오디오 페이드", ToolTip = "켜져 있으면 카메라 페이드 기능으로 오디오도 함께 페이드합니다."))
	bool bFadeAudio = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Shared", meta = (DisplayName = "전환 중 입력 잠금", ToolTip = "켜져 있으면 전환이 끝날 때까지 플레이어 이동/시점 입력을 막습니다."))
	bool bLockPlayerInputDuringTransition = true;
};
